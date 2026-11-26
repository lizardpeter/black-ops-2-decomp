package main

import (
	"crypto/cipher"
	"crypto/des"
	"crypto/hmac"
	"crypto/sha1"
	"encoding/binary"
	"errors"

	"github.com/cxmcc/tiger"
)

type lobbyKey [24]byte

// The Store client hashes the little-endian seed with Tiger192 and uses
// the first eight bytes as the Triple DES CBC initialization vector.
func lobbyIV(seed []byte) []byte {
	hash := tiger.New()
	hash.Write(seed)
	return hash.Sum(nil)[:des.BlockSize]
}

func decryptLobbyTask(key lobbyKey, frame []byte) ([]byte, error) {
	if len(frame) < 5+des.BlockSize || frame[0] != 1 || (len(frame)-5)%des.BlockSize != 0 {
		return nil, errors.New("invalid encrypted lobby frame")
	}
	block, err := des.NewTripleDESCipher(key[:])
	if err != nil {
		return nil, err
	}
	plain := make([]byte, len(frame)-5)
	cipher.NewCBCDecrypter(block, lobbyIV(frame[1:5])).CryptBlocks(plain, frame[5:])
	mac := hmac.New(sha1.New, key[:])
	mac.Write(plain[5:])
	if !hmac.Equal(plain[:4], mac.Sum(nil)[:4]) {
		return nil, errors.New("invalid lobby task authentication code")
	}
	return plain[4:], nil
}

// The client reads the magic word before dispatching server replies. The
// lobby task reply starts with a typed transaction ID and status.
func encryptTaskReply(key lobbyKey, seed [4]byte, transaction uint64, task byte, reply taskReply) []byte {
	plain := binary.LittleEndian.AppendUint32(nil, 0xdeadbeef)
	plain = append(plain, 1, 10)
	plain = binary.LittleEndian.AppendUint64(plain, transaction)
	plain = binary.LittleEndian.AppendUint32(append(plain, 8), reply.status)
	plain = append(plain, 3, task)
	if reply.status == 0 {
		count := uint32(len(reply.results))
		plain = binary.LittleEndian.AppendUint32(append(plain, 8), count)
		if count != 0 {
			plain = binary.LittleEndian.AppendUint32(append(plain, 8), count)
			for _, result := range reply.results {
				plain = append(plain, result...)
			}
		}
	}
	plain = append(plain, make([]byte, (des.BlockSize-len(plain)%des.BlockSize)%des.BlockSize)...)

	out := binary.LittleEndian.AppendUint32(nil, uint32(5+len(plain)))
	out = append(out, 1)
	out = append(out, seed[:]...)
	block, _ := des.NewTripleDESCipher(key[:]) // lobbyKey is always 24 bytes.
	cipher.NewCBCEncrypter(block, lobbyIV(seed[:])).CryptBlocks(plain, plain)
	return append(out, plain...)
}
