#pragma once

// Incremental checksums over caller-owned byte buffers.
void LiveAntiCheat_GenerateCRC32Checksum(const unsigned char *data, int length, unsigned int *key);
unsigned long long LiveAntiCheat_GenerateCRC32ChecksumSplit(const unsigned char *data, int length, unsigned long long key);
