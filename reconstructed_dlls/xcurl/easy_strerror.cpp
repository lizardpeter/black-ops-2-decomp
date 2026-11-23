// Exact error-code text mapping from original XCurl.dll's machine code,
// exported curl_easy_strerror at RVA 0x17910. Original SHA-256:
// 65cd27b336abecf821896611c91e8ec74d0328bd65ec11aa5ef4e2affbdd4551
//
// Every return string below was extracted from the pinned binary's Ghidra
// decompilation, NOT from libcurl source. No external runtime dependencies.
// Original's invalid-code fallback is "Unknown error".
extern "C" const char* curl_easy_strerror(int error) {
  switch (error) {
    case  0: return "No error";
    case  1: return "Unsupported protocol";
    case  2: return "Failed initialization";
    case  3: return "URL using bad/illegal format or missing URL";
    case  4: return "A requested feature, protocol or option was not found built-in in this libcurl due to a build-time decision.";
    case  5: return "Couldn't resolve proxy name";
    case  6: return "Couldn't resolve host name";
    case  7: return "Couldn't connect to server";
    case  8: return "Weird server reply";
    case  9: return "Access denied to remote resource";
    case 10: return "FTP: The server failed to connect to data port";
    case 11: return "FTP: unknown PASS reply";
    case 12: return "FTP: Accepting server connect has timed out";
    case 13: return "FTP: unknown PASV reply";
    case 14: return "FTP: unknown 227 response format";
    case 15: return "FTP: can't figure out the host in the PASV response";
    case 16: return "Error in the HTTP2 framing layer";
    case 17: return "FTP: couldn't set file type";
    case 18: return "Transferred a partial file";
    case 19: return "FTP: couldn't retrieve (RETR failed) the specified file";
    case 21: return "Quote command returned error";
    case 22: return "HTTP response code said error";
    case 23: return "Failed writing received data to disk/application";
    case 25: return "Upload failed (at start/before it took off)";
    case 26: return "Failed to open/read local data from file/application";
    case 27: return "Out of memory";
    case 28: return "Timeout was reached";
    case 30: return "FTP: command PORT failed";
    case 31: return "FTP: command REST failed";
    case 33: return "Requested range was not delivered by the server";
    case 34: return "Internal problem setting up the POST";
    case 35: return "SSL connect error";
    case 36: return "Couldn't resume download";
    case 37: return "Couldn't read a file:// file";
    case 38: return "LDAP: cannot bind";
    case 39: return "LDAP: search failed";
    case 41: return "A required function in the library was not found";
    case 42: return "Operation was aborted by an application callback";
    case 43: return "A libcurl function was given a bad argument";
    case 45: return "Failed binding local connection end";
    case 47: return "Number of redirects hit maximum amount";
    case 48: return "An unknown option was passed in to libcurl";
    case 49: return "Malformed telnet option";
    case 52: return "Server returned nothing (no headers, no data)";
    case 53: return "SSL crypto engine not found";
    case 54: return "Can not set SSL crypto engine as default";
    case 55: return "Failed sending data to the peer";
    case 56: return "Failure when receiving data from the peer";
    case 58: return "Problem with the local SSL certificate";
    case 59: return "Couldn't use specified SSL cipher";
    case 60: return "SSL peer certificate or SSH remote key was not OK";
    case 61: return "Unrecognized or bad HTTP Content or Transfer-Encoding";
    case 62: return "Invalid LDAP URL";
    case 63: return "Maximum file size exceeded";
    case 64: return "Requested SSL level failed";
    case 65: return "Send failed since rewinding of the data stream failed";
    case 66: return "Failed to initialise SSL crypto engine";
    case 67: return "Login denied";
    case 68: return "TFTP: File Not Found";
    case 69: return "TFTP: Access Violation";
    case 70: return "Disk full or allocation exceeded";
    case 71: return "TFTP: Illegal operation";
    case 72: return "TFTP: Unknown transfer ID";
    case 73: return "Remote file already exists";
    case 74: return "TFTP: No such user";
    case 75: return "Conversion failed";
    case 76: return "Caller must register CURLOPT_CONV_ callback options";
    case 77: return "Problem with the SSL CA cert (path? access rights?)";
    case 78: return "Remote file not found";
    case 79: return "Error in the SSH layer";
    case 80: return "Failed to shut down the SSL connection";
    case 81: return "Socket not ready for send/recv";
    case 82: return "Failed to load CRL file (path? access rights?, format?)";
    case 83: return "Issuer check against peer certificate failed";
    case 84: return "FTP: The server did not accept the PRET command.";
    case 85: return "RTSP CSeq mismatch or invalid CSeq";
    case 86: return "RTSP session error";
    case 87: return "Unable to parse FTP file list";
    case 88: return "Chunk callback failed";
    case 89: return "The max connection limit is reached";
    case 90: return "SSL public key does not match pinned public key";
    case 91: return "SSL server certificate status verification FAILED";
    case 92: return "Stream error in the HTTP/2 framing layer";
    case 93: return "API function called from within callback";
    case 94: return "An authentication function returned an error";
    case 95: return "HTTP/3 error";
    case 96: return "QUIC connection error";
    default: return "Unknown error";
  }
}
