# XCurl multi scheduling recovery

Original Store XCurl.dll SHA-256: 65cd27b336abecf821896611c91e8ec74d0328bd65ec11aa5ef4e2affbdd4551.

Recovered add/remove/perform/poll/info scheduling and intrusive queues at RVAs 0x19744/0x17F28, 0x19D20/0x18B44, 0x187E8, 0x18920, and 0x198E4/0x18A28. Local original-machine differential: 800/800 cases, checking entire object memory after pointer normalization, return values, callback order, lock/wait/event interactions, cancellation progression, counts and message ownership. This is the disabled-ETW path. OS primitives and unported pending/active engine steps, cancellation engine, and global adjustment are explicit boundaries, not a finished network transport or drop-in ABI. No libcurl used.
