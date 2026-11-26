package main

import (
	"encoding/binary"
	"sync"
)

type profileStorage struct {
	mu    sync.RWMutex
	files map[string][]byte
}

func (s *profileStorage) get(payload []byte) taskReply {
	name, owner, ok := readStorageFilename(payload)
	if !ok || len(owner) < 9 || len(owner) > 16 || owner[0] != 10 {
		return taskReply{status: bdHandleTaskFailed}
	}
	if binary.LittleEndian.Uint64(owner[1:9]) != localUserID {
		return taskReply{status: bdNoFile}
	}
	s.mu.RLock()
	defer s.mu.RUnlock()
	data, exists := s.files[name]
	if !exists {
		return taskReply{status: bdNoFile}
	}
	return storageFileReply(data)
}

func (s *profileStorage) upload(payload []byte) taskReply {
	name, body, ok := readStorageFilename(payload)
	// uploadFile writes a typed visibility flag followed by a typed blob.
	if !ok || len(body) < 8 || body[0] != 1 || body[1] > 1 || body[2] != 19 || body[3] != 8 {
		return taskReply{status: bdHandleTaskFailed}
	}
	size := binary.LittleEndian.Uint32(body[4:8])
	if size > maxStorageFile || size > uint32(len(body)-8) {
		return taskReply{status: bdHandleTaskFailed}
	}
	data := body[8 : 8+size]
	owner := body[8+size:]
	// Up to seven trailing bytes are cipher padding. An explicit owner,
	// when supplied, must be the same synthetic user as the connection.
	if len(owner) > 7 && (len(owner) < 9 || len(owner) > 16 || owner[0] != 10 ||
		binary.LittleEndian.Uint64(owner[1:9]) != localUserID) {
		return taskReply{status: bdHandleTaskFailed}
	}
	s.mu.Lock()
	defer s.mu.Unlock()
	const maxFiles = 128
	if _, exists := s.files[name]; !exists && len(s.files) >= maxFiles {
		return taskReply{status: bdServiceNotAvailable}
	}
	if s.files == nil {
		s.files = make(map[string][]byte)
	}
	s.files[name] = append([]byte(nil), data...)
	return taskReply{}
}
