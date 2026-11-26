package main

import (
	"bytes"
	"compress/zlib"
	"crypto/des"
	"encoding/binary"
	"errors"
	"io"
	"os"
	"path/filepath"
	"strings"
)

// The client advertises a 65535-byte frame limit. The encrypted reply has
// five bytes before the cipher blocks and 37 bytes before the blob data.
const maxStorageFile = (lobbyFrameLimit-5)/des.BlockSize*des.BlockSize - 37

func readStorageFilename(payload []byte) (string, []byte, bool) {
	if len(payload) < 3 || payload[0] != 16 {
		return "", nil, false
	}
	end := bytes.IndexByte(payload[1:], 0)
	if end <= 0 || end >= 128 {
		return "", nil, false
	}
	return string(payload[1 : 1+end]), payload[2+end:], true
}

func storageFileReply(data []byte) taskReply {
	blob := binary.LittleEndian.AppendUint32([]byte{19, 8}, uint32(len(data)))
	return taskReply{results: [][]byte{append(blob, data...)}}
}

func (l *lobby) getPublisherFile(payload []byte) taskReply {
	name, _, ok := readStorageFilename(payload)
	if !ok {
		return taskReply{status: bdHandleTaskFailed}
	}
	if bundle, ok := l.localPublisherBundles[name]; ok {
		return storageFileReply(bundle)
	}
	files := l.publisherFiles
	if !filepath.IsLocal(name) || strings.ContainsAny(name, `/\:`) || files == nil {
		return taskReply{status: bdNoFile}
	}
	info, err := files.Stat(name)
	if errors.Is(err, os.ErrNotExist) {
		return taskReply{status: bdNoFile}
	}
	if err != nil || !info.Mode().IsRegular() || info.Size() > maxStorageFile {
		return taskReply{status: bdServiceNotAvailable}
	}
	file, err := files.Open(name)
	if err != nil {
		return taskReply{status: bdServiceNotAvailable}
	}
	defer file.Close()
	data, err := io.ReadAll(io.LimitReader(file, maxStorageFile+1))
	if err != nil || len(data) > maxStorageFile {
		return taskReply{status: bdServiceNotAvailable}
	}
	return storageFileReply(data)
}

func buildLocalPublisherBundles() map[string][]byte {
	return map[string][]byte{
		"online_tu17_mp_english.wad": buildLocalPublisherBundle("playlists_mp.info"),
		"online_tu17_zm_english.wad": buildLocalPublisherBundle("playlists_zm.info"),
	}
}

func buildLocalPublisherBundle(playlist string) []byte {
	type entry struct {
		name string
		text string
	}
	entries := [...]entry{
		{playlist, "version 1\n"},
		{"leagues.info", `{"version":1,"leagues":[{"name":"Local"}],"divisions":[{"name":"Local"}]}`},
	}
	const headerSize = 16
	const entrySize = 44
	manifest := make([]byte, headerSize+len(entries)*entrySize)
	binary.LittleEndian.PutUint32(manifest[0:4], 0x543377ab)
	binary.LittleEndian.PutUint32(manifest[8:12], uint32(len(entries)))
	// The signed publisher version matches the native no-update fallback.
	binary.LittleEndian.PutUint32(manifest[12:16], 0xffffffff)
	for i, item := range entries {
		var compressed bytes.Buffer
		writer := zlib.NewWriter(&compressed)
		// bytes.Buffer writes cannot fail.
		writer.Write([]byte(item.text))
		writer.Close()
		record := manifest[headerSize+i*entrySize : headerSize+(i+1)*entrySize]
		copy(record[:32], item.name)
		binary.BigEndian.PutUint32(record[32:36], uint32(compressed.Len()))
		binary.BigEndian.PutUint32(record[36:40], uint32(len(item.text)))
		binary.BigEndian.PutUint32(record[40:44], uint32(len(manifest)))
		manifest = append(manifest, compressed.Bytes()...)
	}
	return manifest
}
