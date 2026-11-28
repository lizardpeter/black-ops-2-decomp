#pragma once

// The local Demonware service (enhanced/demonware/local_service*.cpp), always on in the enhanced Store build: the
// Store auth is answered in process, the lobby listens on a loopback TCP port inside the game.
// The lobby port the game connects to (the Store client's), redirected to the service's own.
constexpr unsigned short demonwareLobbyPort = 3074;
// 127.0.0.1 in network byte order, as an integer on the x64 Store build.
constexpr unsigned demonwareLoopback = 0x0100007f;
// The URL the native auth response is formed with (the request never leaves the process).
constexpr char demonwareAuthUrl[] = "http://127.0.0.1/auth/";

// Starts the service (once; the first auth or lobby lookup does).
void LocalDemonware_Start();
// The service's lobby port in network order (0 when it couldn't listen).
unsigned short LocalDemonware_LobbyPort();
// Answers the auth request formed for the native response (the HTTP request object): true when it holds a reply.
bool LocalDemonware_AnswerAuth(void *request, unsigned titleId);
void LocalDemonware_Trace(const char *event);
void IdentityAudit_Enable();
bool IdentityAudit_IsEnabled();
void IdentityAudit_Trace(const char *event);
