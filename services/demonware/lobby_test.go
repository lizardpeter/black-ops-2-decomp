package main

import (
	"bytes"
	"encoding/binary"
	"io"
	"net"
	"testing"
	"time"
)

func lobbyHello(title uint32, value ticket) []byte {
	frame := make([]byte, 8+4+140)
	copy(frame, []byte{200, 0, 0, 0, 255, 255, 0, 0, 140, 0, 0, 0, 0, 7})
	bits := frame[14:]
	write := func(start, count int, value uint32) {
		for bit := 0; bit < count; bit++ {
			if value&(1<<bit) != 0 {
				bits[(start+bit)/8] |= 1 << ((start + bit) % 8)
			}
		}
	}
	write(0, 1, 1)
	write(1, 5, 8)
	write(6, 32, title)
	write(38, 5, 8)
	for i, b := range value {
		write(75+8*i, 8, uint32(b))
	}
	return frame
}

func TestLobbyConsumesAuthTicket(t *testing.T) {
	const title = 2405
	var value ticket
	for i := range value {
		value[i] = byte(i ^ 0x5a)
	}
	tickets := &ticketStore{pending: make(map[ticket]pendingTicket)}
	l := &lobby{tickets: tickets}
	if !tickets.add(value, title, lobbyKey{}, time.Now()) {
		t.Fatal("could not store ticket")
	}
	if _, _, err := l.handshake(bytes.NewBuffer(lobbyHello(title+1, value))); err == nil {
		t.Fatal("accepted a ticket for another title")
	}

	conn := bytes.NewBuffer(lobbyHello(title, value))
	id, _, err := l.handshake(conn)
	if err != nil || id == 0 {
		t.Fatalf("handshake failed: %v", err)
	}
	ack := conn.Bytes()
	if len(ack) != 15 || !bytes.Equal(ack[:7], []byte{11, 0, 0, 0, 0, 4, 10}) ||
		binary.LittleEndian.Uint64(ack[7:]) != id {
		t.Fatalf("invalid lobby ack: %x", ack)
	}
	if _, _, err := l.handshake(bytes.NewBuffer(lobbyHello(title, value))); err == nil {
		t.Fatal("accepted a redeemed ticket")
	}
	if !tickets.add(value, title, lobbyKey{}, time.Now().Add(-2*ticketTTL)) {
		t.Fatal("could not store expired ticket")
	}
	if _, _, err := l.handshake(bytes.NewBuffer(lobbyHello(title, value))); err == nil {
		t.Fatal("accepted an expired ticket")
	}
}

type deadlineRecordingConn struct {
	net.Conn
	readDeadline chan time.Time
}

func (c deadlineRecordingConn) SetReadDeadline(deadline time.Time) error {
	c.readDeadline <- deadline
	return c.Conn.SetReadDeadline(deadline)
}

func TestLobbyClearsHandshakeReadDeadline(t *testing.T) {
	const title = 2405
	value := ticket{2, 5, 7}
	tickets := &ticketStore{pending: make(map[ticket]pendingTicket)}
	if !tickets.add(value, title, lobbyKey{}, time.Now()) {
		t.Fatal("could not store ticket")
	}
	l := &lobby{tickets: tickets}
	server, client := net.Pipe()
	defer client.Close()
	client.SetDeadline(time.Now().Add(3 * time.Second))
	readDeadline := make(chan time.Time, 1)
	go l.serve(deadlineRecordingConn{server, readDeadline})
	if _, err := client.Write(lobbyHello(title, value)); err != nil {
		t.Fatal(err)
	}
	if _, err := io.ReadFull(client, make([]byte, 15)); err != nil {
		t.Fatal(err)
	}
	select {
	case deadline := <-readDeadline:
		if !deadline.IsZero() {
			t.Fatalf("authenticated lobby read deadline %v", deadline)
		}
	case <-time.After(time.Second):
		t.Fatal("authenticated lobby kept the handshake read deadline")
	}
	if _, err := client.Write(make([]byte, 4)); err != nil {
		t.Fatal(err)
	}
	if frame, err := readLobbyFrame(client); err != nil || len(frame) != 0 {
		t.Fatalf("invalid keepalive reply %x: %v", frame, err)
	}
}
