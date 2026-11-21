// Assert macros as they expand in the original build.
//
// Build environment, not game source. The original's asserts call Assert_MyHandler(__FILE__, __LINE__,
// type, fmt, ...) and break into the debugger when it returns false. The reconstruction's line numbers
// differ from the original's, so each call site gives the ORIGINAL line explicitly, and each .cpp defines
// T6_ASSERT_FILE as the original __FILE__ string. A header's asserts name the header as the including file saw it
// ("c:\t6\code\src\universal\../qcommon/bitarray.h"), so each .cpp also defines T6_ASSERT_DIR, its own
// directory, before its includes. The original binary shows several argument shapes; each macro below reproduces
// one of them exactly (format string and argument order). When Assert_MyHandler returns true the program carries
// on past the failed assert, so the code after an assert matters.
#pragma once

// Only builds whose target.toml entry says `asserts = true` (CoDMPServer_PC) compiled their asserts in; for every
// other build tools/build_recon.py defines T6_NO_ASSERTS and the macros vanish, expression and all, as they did there.

extern "C" bool Assert_MyHandler(const char *filename, int line, int type, const char *fmt, ...);  // universal/assertive.cpp

// Assert_MyHandler(file, line, 0, "%s", "expr")
#define iassert_at(line, expr) \
	do { if (!(expr) && !Assert_MyHandler(T6_ASSERT_FILE, line, 0, "%s", #expr)) __debugbreak(); } while (0)

// Assert_MyHandler(file, line, 0, "%s\n\t%s", "expr", msg)
#define iassertmsg_at(line, expr, msg) \
	do { if (!(expr) && !Assert_MyHandler(T6_ASSERT_FILE, line, 0, "%s\n\t%s", #expr, msg)) __debugbreak(); } while (0)

// Assert_MyHandler(file, line, 0, "expr\n\t<fmt>", args...)
#define vassert_at(line, expr, fmt, ...) \
	do { if (!(expr) && !Assert_MyHandler(T6_ASSERT_FILE, line, 0, #expr "\n\t" fmt, __VA_ARGS__)) __debugbreak(); } while (0)

// Assert_MyHandler(file, line, 0, "%s\n\t<fmt>", "(expr)", args...)
#define vassertp_at(line, expr, fmt, ...) \
	do { if (!(expr) && !Assert_MyHandler(T6_ASSERT_FILE, line, 0, "%s\n\t" fmt, "(" #expr ")", __VA_ARGS__)) \
		__debugbreak(); } while (0)

// Assert_MyHandler(file, line, type, "%s\n\t<fmt>", "(expr)", args...): vassertp_at with an explicit assert
// type (the original passes 1 for some range checks, e.g. cg_camera.cpp's viewFov check)
#define vassertpt_at(line, type, expr, fmt, ...) \
	do { if (!(expr) && !Assert_MyHandler(T6_ASSERT_FILE, line, type, "%s\n\t" fmt, "(" #expr ")", __VA_ARGS__)) \
		__debugbreak(); } while (0)

// Assert_MyHandler(file, line, 0, "(expr)", ""): q_shared.cpp's asserts (the empty string is a separate argument)
#define assert_at(line, expr) \
	do { if (!(expr) && !Assert_MyHandler(T6_ASSERT_FILE, line, 0, "(" #expr ")", "")) __debugbreak(); } while (0)

// Assert_MyHandler(file, line, 0, "(expr)", fmt, args...): q_shared.cpp's asserts with a message
#define assertf_at(line, expr, fmt, ...) \
	do { if (!(expr) && !Assert_MyHandler(T6_ASSERT_FILE, line, 0, "(" #expr ")", fmt, __VA_ARGS__)) \
		__debugbreak(); } while (0)

// Assert_MyHandler(file, line, 0, "", msg): a failure with only a message, where the code has already decided it
// failed (q_shared.cpp: the caller tests, asserts, then prints and returns)
#define assertfail_at(line, msg) \
	do { if (!Assert_MyHandler(T6_ASSERT_FILE, line, 0, "", msg)) __debugbreak(); } while (0)

// Assert_MyHandler(file, line, 0, "(expr)", "") and Assert_MyHandler(file, line, 0, "(expr)", fmt, args...), and
// assert_at with an assert type, naming the file explicitly or not
#define assert_in(file, line, expr) \
	do { if (!(expr) && !Assert_MyHandler(file, line, 0, "(" #expr ")", "")) __debugbreak(); } while (0)
#define assertf_in(file, line, expr, fmt, ...) \
	do { if (!(expr) && !Assert_MyHandler(file, line, 0, "(" #expr ")", fmt, __VA_ARGS__)) __debugbreak(); } while (0)
#define assertt_at(line, type, expr) \
	do { if (!(expr) && !Assert_MyHandler(T6_ASSERT_FILE, line, type, "(" #expr ")", "")) __debugbreak(); } while (0)

// Assert_MyHandler(file, line, 0, "(expr)", "%s", "expr"), naming the file explicitly: headers (qcommon/bitarray.h)
#define assertexpr_in(file, line, expr) \
	do { if (!(expr) && !Assert_MyHandler(file, line, 0, "(" #expr ")", "%s", #expr)) __debugbreak(); } while (0)

// Assert_MyHandler(file, line, 0, "(unsigned)(index) < (unsigned)(count)", "index doesn't index COUNT\n\t%i not in [0, %i)",
// index, count): the bounds check. The expression text has the bound expanded ("(unsigned)(1)"), the message names it
// ("MAX_LOCAL_CLIENTS"), so the message is made one macro level up, before the bound expands.
#define bcassert_at(line, index, count) bcassert_at_(line, index, count, #index " doesn't index " #count)
#define bcassert_at_(line, index, count, msg) \
	do { if (!((unsigned)(index) < (unsigned)(count)) && !Assert_MyHandler(T6_ASSERT_FILE, line, 0, \
		"(unsigned)(" #index ") < (unsigned)(" #count ")", msg "\n\t%i not in [0, %i)", index, count)) __debugbreak(); } \
	while (0)

#ifdef T6_NO_ASSERTS
#undef iassert_at
#undef iassertmsg_at
#undef vassert_at
#undef vassertp_at
#undef vassertpt_at
#undef assert_at
#undef assertf_at
#undef assertfail_at
#undef assertexpr_in
#undef assert_in
#undef assertf_in
#undef assertt_at
#undef bcassert_at
#define iassert_at(...) ((void)0)
#define iassertmsg_at(...) ((void)0)
#define vassert_at(...) ((void)0)
#define vassertp_at(...) ((void)0)
#define vassertpt_at(...) ((void)0)
#define assert_at(...) ((void)0)
#define assertf_at(...) ((void)0)
#define assertfail_at(...) ((void)0)
#define assertexpr_in(...) ((void)0)
#define assert_in(...) ((void)0)
#define assertf_in(...) ((void)0)
#define assertt_at(...) ((void)0)
#define bcassert_at(...) ((void)0)
#endif
