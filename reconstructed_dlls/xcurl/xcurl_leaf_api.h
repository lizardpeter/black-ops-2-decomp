#pragma once
// Only ABI-complete leaf exports. Other recovered operations currently
// require explicit research state and callbacks from their own headers.
struct curl_slist {
    char* data;
    curl_slist* next;
};
static_assert(sizeof(curl_slist)==16);
extern "C" curl_slist* curl_slist_append(curl_slist*,const char*);
extern "C" void curl_slist_free_all(curl_slist*);
extern "C" const char* curl_easy_strerror(int);
