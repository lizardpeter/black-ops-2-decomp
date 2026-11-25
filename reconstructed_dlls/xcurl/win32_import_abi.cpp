// Optional static link ABI for authored/recompiled Windows x64 callers
// that still declare the original functions __declspec(dllimport).
// This is not a PE import table patch, nor a replacement DLL.
#include "game_api.h"
#if !defined(_WIN32) || !defined(_WIN64)
#error This static import ABI is Windows x64 only.
#endif
extern "C" {
decltype(&curl_global_init) __imp_curl_global_init=&curl_global_init;
decltype(&curl_global_cleanup) __imp_curl_global_cleanup=&curl_global_cleanup;
decltype(&curl_easy_init) __imp_curl_easy_init=&curl_easy_init;
decltype(&curl_easy_cleanup) __imp_curl_easy_cleanup=&curl_easy_cleanup;
decltype(&curl_easy_setopt) __imp_curl_easy_setopt=&curl_easy_setopt;
decltype(&curl_easy_getinfo) __imp_curl_easy_getinfo=&curl_easy_getinfo;
decltype(&curl_easy_strerror) __imp_curl_easy_strerror=&curl_easy_strerror;
decltype(&curl_slist_append) __imp_curl_slist_append=&curl_slist_append;
decltype(&curl_slist_free_all) __imp_curl_slist_free_all=&curl_slist_free_all;
decltype(&curl_multi_init) __imp_curl_multi_init=&curl_multi_init;
decltype(&curl_multi_cleanup) __imp_curl_multi_cleanup=&curl_multi_cleanup;
decltype(&curl_multi_add_handle) __imp_curl_multi_add_handle=&curl_multi_add_handle;
decltype(&curl_multi_remove_handle) __imp_curl_multi_remove_handle=&curl_multi_remove_handle;
decltype(&curl_multi_perform) __imp_curl_multi_perform=&curl_multi_perform;
decltype(&curl_multi_poll) __imp_curl_multi_poll=&curl_multi_poll;
decltype(&curl_multi_info_read) __imp_curl_multi_info_read=&curl_multi_info_read;
}
