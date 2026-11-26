package main

import (
	"encoding/binary"
	"sync"
	"time"
)

const (
	bdNoError             = 0
	bdHandleTaskFailed    = 4
	bdServiceNotAvailable = 108
	bdNoFile              = 1000
	bandwidthTestService  = 0x12
	bandwidthTestTask     = 1
)

type localPresence struct {
	mu   sync.Mutex
	blob []byte
}

func (p *localPresence) update(payload []byte) uint32 {
	// setInfo writes a typed uint64 placeholder, then a typed blob.
	if len(payload) < 15 || payload[0] != 10 || binary.LittleEndian.Uint64(payload[1:9]) != 0 ||
		payload[9] != 19 || payload[10] != 8 {
		return bdHandleTaskFailed
	}
	size := binary.LittleEndian.Uint32(payload[11:15])
	if size > 32 || int(size) > len(payload)-15 {
		return bdHandleTaskFailed
	}
	p.mu.Lock()
	p.blob = append(p.blob[:0], payload[15:15+int(size)]...)
	p.mu.Unlock()
	return bdNoError
}

func (p *localPresence) query(payload []byte) taskReply {
	const maxUsers = 64
	if len(payload) < 9 || len(payload)/9 > maxUsers || len(payload)%9 == 8 {
		return taskReply{status: bdHandleTaskFailed}
	}
	p.mu.Lock()
	defer p.mu.Unlock()
	results := make([][]byte, 0, len(payload)/9)
	for len(payload) >= 9 {
		if payload[0] != 10 {
			return taskReply{status: bdHandleTaskFailed}
		}
		// bdRichPresenceInfo's wire layout is a typed boolean and blob.
		result := []byte{1, 0, 19, 8}
		var data []byte
		if binary.LittleEndian.Uint64(payload[1:9]) == localUserID {
			result[1] = 1
			data = p.blob
		}
		result = binary.LittleEndian.AppendUint32(result, uint32(len(data)))
		results = append(results, append(result, data...))
		payload = payload[9:]
	}
	return taskReply{results: results}
}

type taskReply struct {
	status  uint32
	results [][]byte
}

func decodeTaskRequest(data []byte) (service, task byte, payload []byte, ok bool) {
	// The bandwidth test writes both IDs as raw bytes, then queues a task
	// in the same FIFO as typed requests. It must receive a reply in order.
	if len(data) >= 2 && data[0] == bandwidthTestService && data[1] == bandwidthTestTask {
		return data[0], data[1], data[2:], true
	}
	if len(data) >= 3 && data[1] == 3 {
		return data[0], data[2], data[3:], true
	}
	return 0, 0, nil, false
}

func (l *lobby) handleTask(service, task byte, payload []byte) taskReply {
	switch {
	case service == 0x26 && task == 4:
		return taskReply{} // Acknowledge the local console report without retaining it.
	case service == 0x44 && task == 1:
		return taskReply{status: l.presence.update(payload)}
	case service == 0x44 && task == 2:
		return l.presence.query(payload)
	case service == 0x51 && (task == 2 || task == 6 || task == 8):
		// The synthetic profile has no league teams or team members.
		return taskReply{}
	case service == 0x0a && task == 1:
		return l.profileFiles.upload(payload)
	case service == 0x0a && task == 3:
		return l.profileFiles.get(payload)
	case service == 0x0a && task == 7:
		return l.getPublisherFile(payload)
	case service == 0x0c && task == 6:
		return taskReply{results: [][]byte{binary.LittleEndian.AppendUint32([]byte{8}, uint32(time.Now().Unix()))}}
	case service == 0x1b && task == 3:
		// bdDMLHierarchicalInfo reads four location strings, two float32
		// coordinates and five uint32 hierarchy fields. Use synthetic data.
		location := []byte("\x10ZZ\x00\x10Local\x00\x10Local\x00\x10Local\x00")
		for _, tag := range []byte{13, 13, 8, 8, 8, 8, 8} {
			location = binary.LittleEndian.AppendUint32(append(location, tag), 0)
		}
		return taskReply{results: [][]byte{location}}
	default:
		return taskReply{status: bdServiceNotAvailable}
	}
}
