package main

import (
	"bytes"
	"compress/zlib"
	"crypto/cipher"
	"crypto/des"
	"encoding/binary"
	"io"
	"os"
	"path/filepath"
	"testing"
)

func TestPresenceRoundTrip(t *testing.T) {
	l := new(lobby)
	body := binary.LittleEndian.AppendUint64([]byte{10}, 0)
	body = binary.LittleEndian.AppendUint32(append(body, 19, 8), 3)
	body = append(body, 2, 6, 7)

	if reply := l.handleTask(0x44, 1, body); reply.status != bdNoError {
		t.Fatalf("presence update status %d", reply.status)
	}
	query := binary.LittleEndian.AppendUint64([]byte{10}, localUserID)
	query = binary.LittleEndian.AppendUint64(append(query, 10), 123)
	reply := l.handleTask(0x44, 2, query)
	if reply.status != bdNoError || len(reply.results) != 2 ||
		!bytes.Equal(reply.results[0], []byte{1, 1, 19, 8, 3, 0, 0, 0, 2, 6, 7}) ||
		!bytes.Equal(reply.results[1], []byte{1, 0, 19, 8, 0, 0, 0, 0}) {
		t.Fatalf("presence query result %+v", reply)
	}

	if reply := l.handleTask(0x44, 1, body[:len(body)-1]); reply.status != bdHandleTaskFailed {
		t.Fatalf("accepted incomplete presence blob: status %d", reply.status)
	}
}

func TestEncryptedTaskFailure(t *testing.T) {
	key := lobbyKey{42, 61, 78}
	seed := [4]byte{15, 77, 91, 131}
	reply := encryptTaskReply(key, seed, 123, 7, taskReply{status: bdNoFile})

	if binary.LittleEndian.Uint32(reply[:4]) != uint32(len(reply)-4) ||
		reply[4] != 1 || !bytes.Equal(reply[5:9], seed[:]) {
		t.Fatalf("invalid header %x", reply[:9])
	}
	block, _ := des.NewTripleDESCipher(key[:])
	plain := reply[9:]
	cipher.NewCBCDecrypter(block, lobbyIV(seed[:])).CryptBlocks(plain, plain)
	if !bytes.Equal(plain[:21], []byte{239, 190, 173, 222, 1, 10, 123, 0, 0, 0, 0, 0, 0, 0, 8, 232, 3, 0, 0, 3, 7}) {
		t.Fatalf("invalid native task reply %x", plain)
	}
}

func TestBandwidthTestGetsAnOrderedFailure(t *testing.T) {
	l := new(lobby)
	request := append([]byte{0x12, 1}, make([]byte, 16)...)
	service, task, payload, ok := decodeTaskRequest(request)
	if !ok || service != 0x12 || task != 1 || len(payload) != 16 {
		t.Fatalf("bandwidth-test request decoded as %x/%d with %d bytes, valid %t", service, task, len(payload), ok)
	}
	if reply := l.handleTask(service, task, payload); reply.status != bdServiceNotAvailable {
		t.Fatalf("bandwidth-test reply status %d", reply.status)
	}
	service, task, _, ok = decodeTaskRequest([]byte{0x0c, 3, 6})
	if !ok || service != 0x0c || task != 6 {
		t.Fatalf("time request decoded as %x/%d, valid %t", service, task, ok)
	}
}

func TestPublisherFileReply(t *testing.T) {
	dir := t.TempDir()
	if err := os.WriteFile(filepath.Join(dir, "startup.wad"), []byte{42, 53}, 0600); err != nil {
		t.Fatal(err)
	}
	files, err := os.OpenRoot(dir)
	if err != nil {
		t.Fatal(err)
	}
	defer files.Close()
	l := &lobby{publisherFiles: files}
	reply := l.handleTask(0x0a, 7, []byte("\x10startup.wad\x00"))
	if reply.status != bdNoError || len(reply.results) != 1 ||
		!bytes.Equal(reply.results[0], []byte{19, 8, 2, 0, 0, 0, 42, 53}) {
		t.Fatalf("publisher file result %+v", reply)
	}
	for _, name := range []string{"missing.wad", "../startup.wad", "..\\startup.wad"} {
		if reply := l.handleTask(0x0a, 7, []byte("\x10"+name+"\x00")); reply.status != bdNoFile {
			t.Errorf("publisher file %q returned status %d", name, reply.status)
		}
	}
}

func TestLocalPublisherBundleReply(t *testing.T) {
	l := &lobby{localPublisherBundles: buildLocalPublisherBundles()}
	for _, mode := range []string{"mp", "zm"} {
		t.Run(mode, func(t *testing.T) {
			checkLocalPublisherBundleReply(t, l, mode)
		})
	}
}

func checkLocalPublisherBundleReply(t *testing.T, l *lobby, mode string) {
	t.Helper()
	reply := l.handleTask(0x0a, 7, []byte("\x10online_tu17_"+mode+"_english.wad\x00"))
	if reply.status != bdNoError || len(reply.results) != 1 {
		t.Fatalf("local publisher file result %+v", reply)
	}
	file := reply.results[0][6:]
	if binary.LittleEndian.Uint32(file[:4]) != 0x543377ab ||
		binary.LittleEndian.Uint32(file[8:12]) != 2 || binary.LittleEndian.Uint32(file[12:16]) != 0xffffffff {
		t.Fatalf("invalid publisher header %x", file[:16])
	}
	for i, name := range []string{"playlists_" + mode + ".info", "leagues.info"} {
		record := file[16+44*i : 16+44*(i+1)]
		if string(bytes.TrimRight(record[:32], "\x00")) != name {
			t.Fatalf("unexpected publisher entry %q", record[:32])
		}
		offset := binary.BigEndian.Uint32(record[40:44])
		size := binary.BigEndian.Uint32(record[32:36])
		reader, err := zlib.NewReader(bytes.NewReader(file[offset : offset+size]))
		if err != nil {
			t.Fatal(err)
		}
		data, err := io.ReadAll(reader)
		reader.Close()
		if err != nil || len(data) != int(binary.BigEndian.Uint32(record[36:40])) || len(data) == 0 {
			t.Fatalf("invalid publisher entry %q: %q, %v", name, data, err)
		}
	}
}

func TestLocalProfileStorageRoundTrip(t *testing.T) {
	l := new(lobby)
	name := []byte("\x10mpdata\x00")
	read := binary.LittleEndian.AppendUint64(append(bytes.Clone(name), 10), localUserID)
	if reply := l.handleTask(0x0a, 3, read); reply.status != bdNoFile {
		t.Fatalf("new profile file status %d", reply.status)
	}

	write := append(bytes.Clone(name), 1, 1, 19, 8, 3, 0, 0, 0, 7, 11, 13)
	foreignWrite := binary.LittleEndian.AppendUint64(append(bytes.Clone(write), 10), localUserID+1)
	if reply := l.handleTask(0x0a, 1, foreignWrite); reply.status != bdHandleTaskFailed {
		t.Fatalf("foreign owner upload status %d", reply.status)
	}
	if reply := l.handleTask(0x0a, 1, append(write, 0, 0)); reply.status != bdNoError {
		t.Fatalf("profile upload status %d", reply.status)
	}
	write[len(write)-1] = 0
	reply := l.handleTask(0x0a, 3, read)
	if reply.status != bdNoError || len(reply.results) != 1 ||
		!bytes.Equal(reply.results[0], []byte{19, 8, 3, 0, 0, 0, 7, 11, 13}) {
		t.Fatalf("profile file result %+v", reply)
	}
}
