#pragma once

// The Store client uses TCP port 3074 for its Demonware lobby service.
constexpr unsigned short demonwareLobbyPort = 3074;
// 127.0.0.1 in network byte order, as an integer on the x64 Store build.
constexpr unsigned demonwareLoopback = 0x0100007f;
constexpr unsigned short demonwareAuthPort = 3080;
constexpr char demonwareAuthUrl[] = "http://127.0.0.1:3080/auth/";
bool LocalDemonware_SendAuthRequest(void *request);

void LocalDemonware_Enable();
bool LocalDemonware_IsEnabled();
void LocalDemonware_Trace(const char *event);
void IdentityAudit_Enable();
bool IdentityAudit_IsEnabled();
void IdentityAudit_Trace(const char *event);
