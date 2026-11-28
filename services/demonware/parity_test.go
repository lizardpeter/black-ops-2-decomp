package main

// Vectors for the in-process service (enhanced/demonware/local_service.cpp): with T6_PARITY_OUT set, this writes
// what this service does on a fixed sequence of lobby traffic, which tests/demonware/test_local_service.py replays
// against the C++ build. Regenerate with:
//
//	T6_PARITY_OUT=../../tests/demonware/parity_vectors.json go test -run TestWriteParityVectors .

import (
	"bytes"
	"crypto/cipher"
	"crypto/des"
	"crypto/hmac"
	"crypto/sha1"
	"encoding/binary"
	"encoding/hex"
	"encoding/json"
	"os"
	"testing"
)

type authVector struct {
	Payload string `json:"payload"`
	Title   uint32 `json:"title"`
	Ticket  string `json:"ticket"` // "" when the frame is rejected
}

type frameVector struct {
	Name  string `json:"name"`
	Frame string `json:"frame"`
	Seed  string `json:"seed"`
	Reply string `json:"reply"` // "close", "" for no reply, or the reply frame
	Kind  string `json:"kind"`  // "bytes"; "time" (the clock differs); "bundle" (compressed differently)
}

type bundleVector struct {
	Name    string            `json:"name"`
	Entries []bundleEntryJSON `json:"entries"`
}

type bundleEntryJSON struct {
	Name string `json:"name"`
	Text string `json:"text"`
}

type parityVectors struct {
	Key     string         `json:"key"`
	Auth    []authVector   `json:"auth"`
	Frames  []frameVector  `json:"frames"`
	Bundles []bundleVector `json:"bundles"`
}

// A lobby task frame as the client sends it: the task padded to the cipher block, a truncated HMAC-SHA1 of all
// but the task's first byte, Triple DES CBC with the seed's IV.
func clientFrame(key lobbyKey, seed [4]byte, task []byte) []byte {
	plain := append(make([]byte, 4), task...)
	plain = append(plain, make([]byte, (des.BlockSize-len(plain)%des.BlockSize)%des.BlockSize)...)
	mac := hmac.New(sha1.New, key[:])
	mac.Write(plain[5:])
	copy(plain[:4], mac.Sum(nil)[:4])
	block, _ := des.NewTripleDESCipher(key[:])
	cipher.NewCBCEncrypter(block, lobbyIV(seed[:])).CryptBlocks(plain, plain)
	return append(append([]byte{1}, seed[:]...), plain...)
}

func typedTask(service, task byte, payload []byte) []byte {
	return append([]byte{service, 3, task}, payload...)
}

func TestWriteParityVectors(t *testing.T) {
	out := os.Getenv("T6_PARITY_OUT")
	if out == "" {
		t.Skip("T6_PARITY_OUT not set")
	}
	var v parityVectors

	// the lobby auth frame decoder
	var good ticket
	for i := range good {
		good[i] = byte(i*7 ^ 0x5a)
	}
	valid := lobbyHello(2410, good)[12:]
	auth := [][]byte{valid, valid[:139], append(bytes.Clone(valid), 0)}
	for _, mutate := range []func([]byte){
		func(p []byte) { p[0] = 1 }, func(p []byte) { p[1] = 6 }, func(p []byte) { p[2] ^= 1 },
		func(p []byte) { p[2] ^= 2 }, func(p []byte) { p[6] ^= 0x40 }, func(p []byte) { p[3] ^= 0x10 },
	} {
		p := bytes.Clone(valid)
		mutate(p)
		auth = append(auth, p)
	}
	auth = append(auth, lobbyHello(2405, good)[12:], lobbyHello(0, ticket{})[12:])
	for _, p := range auth {
		title, value, err := decodeLobbyAuth(p)
		vector := authVector{Payload: hex.EncodeToString(p)}
		if err == nil {
			vector.Title, vector.Ticket = title, hex.EncodeToString(value[:])
		}
		v.Auth = append(v.Auth, vector)
	}

	// lobby traffic on one connection
	key := lobbyKey{}
	for i := range key {
		key[i] = byte(0x31 + i*13)
	}
	v.Key = hex.EncodeToString(key[:])
	l := &lobby{localPublisherBundles: buildLocalPublisherBundles()}
	var transaction uint64
	seedOf := func(i int) [4]byte { return [4]byte{byte(i), byte(i >> 8), 0x5c, 0xa3} }
	add := func(name, kind string, frame []byte, seed [4]byte) {
		vector := frameVector{Name: name, Frame: hex.EncodeToString(frame), Seed: hex.EncodeToString(seed[:]), Kind: kind}
		switch task, err := decryptLobbyTask(key, frame); {
		case len(frame) == 0:
			vector.Reply = "00000000"
		case err != nil:
			vector.Reply = "close"
		default:
			if service, taskID, payload, ok := decodeTaskRequest(task); ok {
				transaction++
				vector.Reply = hex.EncodeToString(encryptTaskReply(key, seed, transaction, taskID,
					l.handleTask(service, taskID, payload)))
			}
		}
		v.Frames = append(v.Frames, vector)
	}
	n := 0
	task := func(name string, data []byte) {
		n++
		add(name, "bytes", clientFrame(key, seedOf(n), data), seedOf(n))
	}

	u64 := func(prefix []byte, value uint64) []byte { return binary.LittleEndian.AppendUint64(prefix, value) }
	blob := func(data ...byte) []byte {
		return append(binary.LittleEndian.AppendUint32([]byte{19, 8}, uint32(len(data))), data...)
	}
	add("keepalive", "bytes", nil, [4]byte{})
	task("console report", typedTask(0x26, 4, []byte{1, 2, 3}))
	task("presence query before any", typedTask(0x44, 2, u64([]byte{10}, localUserID)))
	task("presence set", typedTask(0x44, 1, append(u64([]byte{10}, 0), blob(2, 6, 7)...)))
	task("presence set too large", typedTask(0x44, 1, append(u64([]byte{10}, 0), blob(make([]byte, 33)...)...)))
	task("presence set short", typedTask(0x44, 1, append(u64([]byte{10}, 0), 19, 8, 3, 0, 0, 0, 2)))
	task("presence set wrong user", typedTask(0x44, 1, append(u64([]byte{10}, 5), blob(1)...)))
	task("presence query", typedTask(0x44, 2, u64(u64([]byte{10}, localUserID), 0)[:9]))
	task("presence query two", typedTask(0x44, 2, append(u64([]byte{10}, localUserID), u64([]byte{10}, 123)...)))
	task("presence query trailing", typedTask(0x44, 2, append(u64([]byte{10}, localUserID), 1, 2, 3)))
	task("presence query bad tag", typedTask(0x44, 2, append(u64([]byte{10}, localUserID), u64([]byte{9}, 1)...)))
	task("presence query eight over", typedTask(0x44, 2, append(u64([]byte{10}, localUserID), make([]byte, 8)...)))
	task("presence query empty", typedTask(0x44, 2, nil))
	task("presence query 65 users", typedTask(0x44, 2, bytes.Repeat(u64([]byte{10}, 7), 65)))
	for _, id := range []byte{2, 6, 8, 3} {
		task("league", typedTask(0x51, id, []byte{1}))
	}
	file := func(name string) []byte { return append(append([]byte{16}, name...), 0) }
	owner := u64([]byte{10}, localUserID)
	task("profile get missing", typedTask(0x0a, 3, append(file("mpdata"), owner...)))
	task("profile upload", typedTask(0x0a, 1, append(file("mpdata"), append([]byte{1, 1}, blob(7, 11, 13)...)...)))
	task("profile get", typedTask(0x0a, 3, append(file("mpdata"), owner...)))
	task("profile get foreign", typedTask(0x0a, 3, append(file("mpdata"), u64([]byte{10}, 99)...)))
	task("profile get no owner", typedTask(0x0a, 3, file("mpdata")))
	task("profile get bad name", typedTask(0x0a, 3, append([]byte{15, 'a', 0}, owner...)))
	task("profile get unterminated", typedTask(0x0a, 3, []byte{16, 'a', 'b', 'c'}))
	task("profile upload with owner", typedTask(0x0a, 1, append(append(file("cmn"), append([]byte{1, 0}, blob(1, 2)...)...), owner...)))
	task("profile upload foreign owner", typedTask(0x0a, 1, append(append(file("cmn"), append([]byte{1, 0}, blob(1, 2)...)...), u64([]byte{10}, 3)...)))
	task("profile upload bad visibility", typedTask(0x0a, 1, append(file("cmn"), append([]byte{1, 2}, blob(1)...)...)))
	task("profile upload short", typedTask(0x0a, 1, append(file("cmn"), 1, 1, 19, 8, 9, 0, 0, 0, 1)))
	task("profile upload empty", typedTask(0x0a, 1, append(file("empty"), append([]byte{1, 1}, blob()...)...)))
	task("profile get empty", typedTask(0x0a, 3, append(file("empty"), owner...)))
	task("profile overwrite", typedTask(0x0a, 1, append(file("mpdata"), append([]byte{1, 1}, blob(9)...)...)))
	task("profile get overwritten", typedTask(0x0a, 3, append(file("mpdata"), owner...)))
	task("profile name with symbols", typedTask(0x0a, 1, append(file("a b/../c:d"), append([]byte{1, 1}, blob(4)...)...)))
	task("profile get symbols", typedTask(0x0a, 3, append(file("a b/../c:d"), owner...)))
	for i := 0; i < 126; i++ {
		task("profile fill", typedTask(0x0a, 1, append(file("f"+string(rune('A'+i/26))+string(rune('a'+i%26))), append([]byte{1, 1}, blob(byte(i))...)...)))
	}
	task("profile upload one more", typedTask(0x0a, 1, append(file("toomany"), append([]byte{1, 1}, blob(1)...)...)))
	task("profile overwrite when full", typedTask(0x0a, 1, append(file("mpdata"), append([]byte{1, 1}, blob(5)...)...)))
	for _, name := range []string{"ffotd_tu17_mp_147.ff.00", "largeheatmap.raw", "online_tu17_mp_french.wad", "../x"} {
		task("publisher "+name, typedTask(0x0a, 7, file(name)))
	}
	task("publisher bad name", typedTask(0x0a, 7, []byte{16, 0}))
	task("region", typedTask(0x1b, 3, nil))
	task("unknown service", typedTask(0x33, 1, []byte{1}))
	task("unknown task", typedTask(0x0a, 9, nil))
	task("bandwidth test", append([]byte{0x12, 1}, make([]byte, 16)...))
	task("untyped", []byte{0x0a, 2, 3})
	task("too short", []byte{0x0a})
	n++
	add("time", "time", clientFrame(key, seedOf(n), typedTask(0x0c, 6, nil)), seedOf(n))
	for _, mode := range []string{"mp", "zm"} {
		n++
		add("bundle "+mode, "bundle", clientFrame(key, seedOf(n), typedTask(0x0a, 7, file("online_tu17_"+mode+"_english.wad"))), seedOf(n))
	}
	// frames the service drops the connection for (each would end it: they're checked on their own)
	bad := clientFrame(key, seedOf(1), typedTask(0x0c, 6, nil))
	bad[len(bad)-1] ^= 1
	for _, c := range []struct {
		name  string
		frame []byte
	}{{"bad mac", bad}, {"short", bad[:12]}, {"not encrypted", append([]byte{0}, bad[1:]...)}, {"unaligned", bad[:len(bad)-1]}} {
		add("close: "+c.name, "bytes", c.frame, seedOf(1))
	}

	for _, mode := range []string{"mp", "zm"} {
		v.Bundles = append(v.Bundles, bundleVector{"online_tu17_" + mode + "_english.wad", []bundleEntryJSON{
			{"playlists_" + mode + ".info", "version 1\n"},
			{"leagues.info", `{"version":1,"leagues":[{"name":"Local"}],"divisions":[{"name":"Local"}]}`},
		}})
	}
	data, err := json.MarshalIndent(v, "", " ")
	if err != nil {
		t.Fatal(err)
	}
	if err := os.WriteFile(out, append(data, '\n'), 0o644); err != nil {
		t.Fatal(err)
	}
}
