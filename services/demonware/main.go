package main

import (
	"crypto/rand"
	"encoding/binary"
	"encoding/json"
	"flag"
	"io"
	"log"
	"net"
	"net/http"
	"os"
	"time"
)

const (
	authAddress        = "127.0.0.1:3080"
	localAuthorization = "T6-Local"
	localUserID        = 0x2100000000000001
)

type authRequest struct {
	Task    uint32 `json:"auth_task"`
	IVSeed  uint32 `json:"iv_seed"`
	TitleID uint32 `json:"title_id"`
}

type authResponse struct {
	Task         uint32 `json:"auth_task"`
	Code         uint32 `json:"code"`
	IVSeed       uint32 `json:"iv_seed"`
	ClientTicket []byte `json:"client_ticket"`
	ServerTicket []byte `json:"server_ticket"`
	ExtraData    string `json:"extra_data"`
}

func issueTickets(title uint32) (client, server []byte, err error) {
	// The Store parser consumes a 128-byte packed client ticket. The server
	// ticket is opaque to the client and is carried into the lobby handshake.
	client, server = make([]byte, 128), make([]byte, 128)
	copy(client[:4], []byte{0xde, 0xad, 0xbd, 0xef})
	binary.LittleEndian.PutUint32(client[5:9], title)
	binary.LittleEndian.PutUint64(client[25:33], localUserID)
	copy(client[33:97], "Local Player")
	if _, err = rand.Read(client[97:121]); err != nil {
		return nil, nil, err
	}
	if _, err = rand.Read(server); err != nil {
		return nil, nil, err
	}
	return client, server, nil
}

func (s *ticketStore) authenticate(w http.ResponseWriter, r *http.Request) {
	if r.Header.Get("Authorization") != localAuthorization {
		http.Error(w, "local profile required", http.StatusUnauthorized)
		return
	}

	defer r.Body.Close()
	decoder := json.NewDecoder(http.MaxBytesReader(w, r.Body, 1024))
	var request authRequest
	if err := decoder.Decode(&request); err != nil || request.Task != 44 || request.TitleID == 0 {
		http.Error(w, "invalid Store auth request", http.StatusBadRequest)
		return
	}
	if err := decoder.Decode(new(any)); err != io.EOF {
		http.Error(w, "invalid Store auth request", http.StatusBadRequest)
		return
	}

	client, server, err := issueTickets(request.TitleID)
	if err != nil {
		log.Printf("could not issue local tickets: %v", err)
		http.Error(w, "ticket generation failed", http.StatusInternalServerError)
		return
	}

	if !s.add(ticket(server), request.TitleID, lobbyKey(client[97:121]), time.Now()) {
		http.Error(w, "too many pending sign-ins", http.StatusServiceUnavailable)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	response := authResponse{45, 700, request.IVSeed, client, server, `{"sbx":"RETAIL","dpi":"LOCAL"}`}
	if err := json.NewEncoder(w).Encode(response); err != nil {
		log.Printf("could not send local auth response: %v", err)
		return
	}
	log.Printf("issued local Store auth response for title %d", request.TitleID)
}

func main() {
	publisherDir := flag.String("publisher-dir", "", "directory containing local publisher files")
	flag.Parse()
	var publisherFiles *os.Root
	if *publisherDir != "" {
		var err error
		publisherFiles, err = os.OpenRoot(*publisherDir)
		if err != nil {
			log.Fatalf("open publisher directory: %v", err)
		}
		defer publisherFiles.Close()
	}
	tickets := &ticketStore{pending: make(map[ticket]pendingTicket)}
	lobbyListener, err := net.Listen("tcp4", lobbyAddress)
	if err != nil {
		log.Fatal(err)
	}
	log.Printf("Demonware lobby listening on %s", lobbyListener.Addr())
	localLobby := &lobby{tickets: tickets, publisherFiles: publisherFiles}
	if publisherFiles == nil {
		localLobby.localPublisherBundles = buildLocalPublisherBundles()
	}
	go func() {
		for {
			conn, err := lobbyListener.Accept()
			if err != nil {
				log.Fatal(err)
			}
			go localLobby.serve(conn)
		}
	}()

	mux := http.NewServeMux()
	mux.HandleFunc("POST /auth/{$}", tickets.authenticate)
	server := &http.Server{
		Addr:              authAddress,
		Handler:           mux,
		ReadHeaderTimeout: 3 * time.Second,
		ReadTimeout:       3 * time.Second,
		WriteTimeout:      3 * time.Second,
		IdleTimeout:       3 * time.Second,
		MaxHeaderBytes:    4096,
	}
	log.Printf("Demonware auth listening on %s", server.Addr)
	log.Fatal(server.ListenAndServe())
}
