package main

import (
	"bytes"
	"encoding/binary"
	"encoding/json"
	"net/http"
	"net/http/httptest"
	"strings"
	"testing"
)

func TestStoreAuthTickets(t *testing.T) {
	const titleID = 2410
	request := httptest.NewRequest("POST", "/auth/", strings.NewReader(
		`{"auth_task":44,"iv_seed":21,"title_id":2410}`))
	request.Header.Set("Authorization", localAuthorization)
	result := httptest.NewRecorder()
	(&ticketStore{pending: make(map[ticket]pendingTicket)}).authenticate(result, request)
	if result.Code != http.StatusOK {
		t.Fatalf("auth status %d, body %s", result.Code, result.Body.String())
	}
	var response authResponse
	if err := json.Unmarshal(result.Body.Bytes(), &response); err != nil {
		t.Fatal(err)
	}
	if response.Task != 45 || response.Code != 700 || response.IVSeed != 21 {
		t.Fatalf("unexpected auth response fields: %+v", response)
	}
	if len(response.ClientTicket) != 128 || len(response.ServerTicket) != 128 {
		t.Fatalf("invalid ticket lengths %d, %d", len(response.ClientTicket), len(response.ServerTicket))
	}
	ticket := response.ClientTicket
	if !bytes.Equal(ticket[:4], []byte{0xde, 0xad, 0xbd, 0xef}) ||
		binary.LittleEndian.Uint32(ticket[5:9]) != titleID ||
		binary.LittleEndian.Uint64(ticket[25:33]) != localUserID ||
		bytes.Equal(ticket[97:121], make([]byte, 24)) {
		t.Fatal("invalid packed Store client ticket")
	}
	var extra map[string]string
	if err := json.Unmarshal([]byte(response.ExtraData), &extra); err != nil || extra["sbx"] != "RETAIL" || extra["dpi"] != "LOCAL" {
		t.Fatalf("invalid extra_data: %q", response.ExtraData)
	}
}

func TestAuthRejectsInvalidRequests(t *testing.T) {
	const valid = `{"auth_task":44,"iv_seed":21,"title_id":2410}`
	for _, tc := range []struct {
		name, authorization, body string
		status                    int
	}{
		{"wrong credentials", "remote", valid, 401},
		{"unexpected task", localAuthorization, `{"auth_task":45,"title_id":2410}`, 400},
		{"trailing payload", localAuthorization, valid + `{}`, 400},
		{"oversized", localAuthorization, valid + strings.Repeat(" ", 2048), 400},
	} {
		t.Run(tc.name, func(t *testing.T) {
			request := httptest.NewRequest("POST", "/auth/", strings.NewReader(tc.body))
			request.Header.Set("Authorization", tc.authorization)
			result := httptest.NewRecorder()
			(&ticketStore{pending: make(map[ticket]pendingTicket)}).authenticate(result, request)
			if result.Code != tc.status {
				t.Fatalf("status %d, want %d", result.Code, tc.status)
			}
		})
	}
}
