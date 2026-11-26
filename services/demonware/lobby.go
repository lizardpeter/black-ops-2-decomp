package main

import (
	"bytes"
	"crypto/rand"
	"encoding/binary"
	"errors"
	"io"
	"log"
	"net"
	"os"
	"sync"
	"sync/atomic"
	"time"
)

const (
	lobbyAddress    = "127.0.0.1:3074"
	lobbyFrameLimit = 65535
	ticketSize      = 128
	ticketTTL       = time.Minute
)

type ticket [ticketSize]byte

type pendingTicket struct {
	title   uint32
	key     lobbyKey
	expires time.Time
}

type ticketStore struct {
	mu      sync.Mutex
	pending map[ticket]pendingTicket
}

func (s *ticketStore) add(value ticket, title uint32, key lobbyKey, now time.Time) bool {
	s.mu.Lock()
	defer s.mu.Unlock()
	for key, pending := range s.pending {
		if !now.Before(pending.expires) {
			delete(s.pending, key)
		}
	}
	if len(s.pending) >= 128 {
		return false
	}
	s.pending[value] = pendingTicket{title, key, now.Add(ticketTTL)}
	return true
}

func (s *ticketStore) consume(value ticket, title uint32, now time.Time) (lobbyKey, bool) {
	s.mu.Lock()
	defer s.mu.Unlock()
	pending, ok := s.pending[value]
	if !ok || pending.title != title || !now.Before(pending.expires) {
		return lobbyKey{}, false
	}
	delete(s.pending, value)
	return pending.key, true
}

type lobby struct {
	tickets               *ticketStore
	nextID                atomic.Uint64
	nextTransaction       atomic.Uint64
	presence              localPresence
	profileFiles          profileStorage
	publisherFiles        *os.Root
	localPublisherBundles map[string][]byte
}

// Lobby auth is a length-prefixed plaintext frame. Its bdBitBuffer starts
// after the message byte. It packs a marker, two tagged uint32s and the ticket
// least significant bit first.
func decodeLobbyAuth(payload []byte) (uint32, ticket, error) {
	var value ticket
	if len(payload) != 140 || payload[0] != 0 || payload[1] != 7 {
		return 0, value, errors.New("invalid lobby auth frame")
	}
	bits := payload[2:]
	read := func(start, count int) uint32 {
		var result uint32
		for i := 0; i < count; i++ {
			bit := (bits[(start+i)/8] >> ((start + i) % 8)) & 1
			result |= uint32(bit) << i
		}
		return result
	}
	if read(0, 1) != 1 || read(1, 5) != 8 || read(38, 5) != 8 {
		return 0, value, errors.New("invalid lobby auth header")
	}
	for i := range value {
		value[i] = byte(read(75+i*8, 8))
	}
	return read(6, 32), value, nil
}

func readLobbyFrame(r io.Reader) ([]byte, error) {
	var length [4]byte
	if _, err := io.ReadFull(r, length[:]); err != nil {
		return nil, err
	}
	size := binary.LittleEndian.Uint32(length[:])
	if size > lobbyFrameLimit {
		return nil, errors.New("lobby frame exceeds advertised limit")
	}
	frame := make([]byte, size)
	_, err := io.ReadFull(r, frame)
	return frame, err
}

func writeLobbyFrame(conn net.Conn, frame []byte) error {
	conn.SetWriteDeadline(time.Now().Add(5 * time.Second))
	_, err := conn.Write(frame)
	return err
}

func (l *lobby) handshake(conn io.ReadWriter) (uint64, lobbyKey, error) {
	var hello [8]byte
	if _, err := io.ReadFull(conn, hello[:]); err != nil {
		return 0, lobbyKey{}, err
	}
	if !bytes.Equal(hello[:], []byte{200, 0, 0, 0, 255, 255, 0, 0}) {
		return 0, lobbyKey{}, errors.New("unsupported lobby hello")
	}
	frame, err := readLobbyFrame(conn)
	if err != nil {
		return 0, lobbyKey{}, err
	}
	title, value, err := decodeLobbyAuth(frame)
	if err != nil {
		return 0, lobbyKey{}, err
	}
	key, ok := l.tickets.consume(value, title, time.Now())
	if !ok {
		return 0, lobbyKey{}, errors.New("unknown or expired lobby ticket")
	}
	id := l.nextID.Add(1)
	ack := [15]byte{11, 0, 0, 0, 0, 4, 10}
	binary.LittleEndian.PutUint64(ack[7:], id)
	_, err = conn.Write(ack[:])
	return id, key, err
}

func (l *lobby) serve(conn net.Conn) {
	defer conn.Close()
	conn.SetDeadline(time.Now().Add(5 * time.Second))
	id, key, err := l.handshake(conn)
	if err != nil {
		log.Printf("lobby handshake failed: %v", err)
		return
	}
	// The client can pause lobby traffic while loading a map. Keep its
	// authenticated connection open and time out only stalled writes.
	conn.SetReadDeadline(time.Time{})
	log.Printf("authenticated local lobby connection %d", id)
	for {
		frame, err := readLobbyFrame(conn)
		if err != nil {
			return
		}
		if len(frame) == 0 {
			var keepAlive [4]byte
			if err := writeLobbyFrame(conn, keepAlive[:]); err != nil {
				return
			}
			continue
		}
		task, err := decryptLobbyTask(key, frame)
		if err != nil {
			log.Printf("lobby connection %d rejected a frame: %v", id, err)
			return
		}
		service, taskID, payload, ok := decodeTaskRequest(task)
		if !ok {
			continue
		}
		result := l.handleTask(service, taskID, payload)
		var seed [4]byte
		if _, err := rand.Read(seed[:]); err != nil {
			return
		}
		reply := encryptTaskReply(key, seed, l.nextTransaction.Add(1), taskID, result)
		if err := writeLobbyFrame(conn, reply); err != nil {
			return
		}
		log.Printf("lobby connection %d service %02x task %d status %d", id, service, taskID, result.status)
	}
}
