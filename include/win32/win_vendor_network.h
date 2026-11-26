#pragma once

extern "C" bool Sys_ProbeTimeServer(const char *address);
extern "C" int BdConnectStream(int socket, const unsigned *ipv4, unsigned short port);
int BdConnectStreamSocket(int socket, const unsigned *ipv4, unsigned short port);
extern "C" void *WinInetOpenConnection(const char *url);
extern "C" void BdBeginWinHttpRequest(void *worker, void *request);
extern "C" int XCurlAddRequest(void *multi, void *request);
extern "C" int WinHttpInitializeRequest(void *connection);
