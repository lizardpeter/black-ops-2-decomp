// enhanced/qcommon/frame_timing.cpp: one frame on the CPU at a time (P0009) and the frame's stage timing.
//
// The original lets the main thread run a frame ahead of the render thread: it reads input and builds frame N+1 while
// the render thread submits frame N, and waits for the render thread only when it hands N+1 over (R_ToggleSmpFrameCmd).
// That's two frames on the CPU, and input read up to a frame earlier than it needs to be when the render thread is
// the slower of the two. With r_renderOverlap 0 (the default) the frame waits for the render thread before it reads
// input (enhanced/qcommon/common.cpp), so a frame's input, simulation and submission run back to back and only the
// GPU overlaps them, as in Forge (Rust-test). r_renderOverlap 1 is the original's pipelining: a higher frame-rate
// ceiling when the CPU is the limit, up to a frame more latency.
//
// com_frameTiming 1 times the stages on the performance counter and writes, once a second, each stage's average and
// maximum in milliseconds to t6_frame_timing.log next to t6_enhanced.dll (the Store build has no console printing:
// Com_Printf is compiled out). Off, the timing costs a branch per stage. Store build only.
#ifdef T6_STORE
#include "enhanced/frame_timing.h"

#include "universal/dvar.h"

#include <windows.h>

static const dvar_t *r_renderOverlap;
static const dvar_t *com_frameTiming;

bool R_RenderOverlap()
{
	if (!r_renderOverlap)
		r_renderOverlap = _Dvar_RegisterBool("r_renderOverlap", false, 1,
		                                     "Let the main thread build the next frame while the render thread "
		                                     "submits the last: a higher frame-rate ceiling when the CPU is the "
		                                     "limit, up to a frame more input latency");
	return Dvar_GetBool(r_renderOverlap);
}

// Each stage's total and count (one thread adds to each), and its maximum since the last line (the reporting main
// thread takes it with an exchange)
struct FrameTimingTotals
{
	volatile long long ticks;
	volatile long long count;
	volatile long long max;
};

static volatile bool s_timingOn;  // com_frameTiming, as the main thread last read it
static FrameTimingTotals s_totals[FRAME_TIMING_COUNT];
static long long s_reported[FRAME_TIMING_COUNT][2];  // ticks and count at the last line
static long long s_frequency;
static long long s_lastLine;      // the counter at the last line (0: none yet)
static long long s_firstLine;
static HANDLE s_log = INVALID_HANDLE_VALUE;
static bool s_logFailed;

static long long FrameTiming_Counter()
{
	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);
	return now.QuadPart;
}

long long FrameTiming_Now()
{
	return s_timingOn ? FrameTiming_Counter() : 0;
}

void FrameTiming_Add(FrameTimingStage stage, long long start)
{
	if (!start)
		return;
	long long ticks = FrameTiming_Counter() - start;
	FrameTimingTotals &t = s_totals[stage];
	t.ticks = t.ticks + ticks;
	t.count = t.count + 1;
	for (long long max = t.max; ticks > max;)
	{
		long long seen = InterlockedCompareExchange64(&t.max, ticks, max);
		if (seen == max)
			break;
		max = seen;
	}
}

// ---- the log line ---------------------------------------------------------------------------------------------------
struct FrameTimingLine
{
	char text[512];
	int length;

	void Add(const char *s)
	{
		while (*s && length < (int)sizeof(text) - 1)
			text[length++] = *s++;
	}
	void AddNumber(long long hundredths)  // a value in hundredths, written with two decimals
	{
		if (hundredths < 0)
			hundredths = 0;
		char digits[24];
		int n = 0;
		long long whole = hundredths / 100;
		do
			digits[n++] = (char)('0' + whole % 10);
		while ((whole /= 10) && n < 20);
		while (n)
		{
			char c[2] = {digits[--n], 0};
			Add(c);
		}
		char fraction[4] = {'.', (char)('0' + hundredths / 10 % 10), (char)('0' + hundredths % 10), 0};
		Add(fraction);
	}
};

// t6_frame_timing.log next to t6_enhanced.dll, created on the first line (once: a failure isn't retried)
static bool FrameTiming_OpenLog()
{
	if (s_log != INVALID_HANDLE_VALUE)
		return true;
	if (s_logFailed)
		return false;
	s_logFailed = true;
	wchar_t path[600];
	DWORD n = GetModuleFileNameW(GetModuleHandleW(L"t6_enhanced.dll"), path, 520);
	if (!n || n >= 520)
		return false;
	while (n && path[n - 1] != L'\\' && path[n - 1] != L'/')
		n--;
	const wchar_t *name = L"t6_frame_timing.log";
	for (int i = 0; name[i]; i++)
		path[n++] = name[i];
	path[n] = 0;
	s_log = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (s_log == INVALID_HANDLE_VALUE)
		return false;
	s_logFailed = false;
	FrameTimingLine head = {};
	head.Add("t6_enhanced frame timing (ms, average and maximum per frame; com_frameTiming 0 stops it)\r\n");
	DWORD written;
	WriteFile(s_log, head.text, (DWORD)head.length, &written, 0);
	return true;
}

static const char *const s_stageNames[FRAME_TIMING_COUNT] = {"main", "wait renderer", "wait display", "wait jobs",
                                                             "render frame", "render idle"};

static void FrameTiming_WriteLine(long long now)
{
	long long counts[FRAME_TIMING_COUNT], ticks[FRAME_TIMING_COUNT], max[FRAME_TIMING_COUNT];
	for (int i = 0; i < FRAME_TIMING_COUNT; i++)
	{
		long long total = s_totals[i].ticks, count = s_totals[i].count;
		ticks[i] = total - s_reported[i][0];
		counts[i] = count - s_reported[i][1];
		s_reported[i][0] = total;
		s_reported[i][1] = count;
		max[i] = InterlockedExchange64(&s_totals[i].max, 0);
	}
	long long mainFrames = counts[FRAME_TIMING_MAIN_FRAME], renderFrames = counts[FRAME_TIMING_RENDER_FRAME];
	long long toHundredthsMs = 100000;  // ticks * 100000 / frequency = hundredths of a millisecond
	FrameTimingLine line = {};
	line.AddNumber((now - s_firstLine) * 100 / s_frequency);
	line.Add(" s: ");
	line.AddNumber(mainFrames * 100 * s_frequency / (now - s_lastLine));
	line.Add(" fps");
	for (int i = 0; i < FRAME_TIMING_COUNT; i++)
	{
		long long frames = i < FRAME_TIMING_RENDER_FRAME ? mainFrames : renderFrames;
		long long sum = ticks[i];
		if (i == FRAME_TIMING_MAIN_FRAME)  // the main thread's own work: its frame less its waits
			sum -= ticks[FRAME_TIMING_WAIT_RENDERER] + ticks[FRAME_TIMING_WAIT_DISPLAY] + ticks[FRAME_TIMING_WAIT_JOBS];
		line.Add(" | ");
		line.Add(i == FRAME_TIMING_MAIN_FRAME ? "main work" : s_stageNames[i]);
		line.Add(" ");
		line.AddNumber(frames ? sum * toHundredthsMs / s_frequency / frames : 0);
		line.Add(i == FRAME_TIMING_MAIN_FRAME ? " (frame max " : " (max ");
		line.AddNumber(max[i] * toHundredthsMs / s_frequency);
		line.Add(")");
	}
	line.Add("\r\n");
	DWORD written;
	WriteFile(s_log, line.text, (DWORD)line.length, &written, 0);
}

void FrameTiming_EndFrame()
{
	if (!com_frameTiming)
		com_frameTiming = _Dvar_RegisterBool("com_frameTiming", false, 0,
		                                     "Write the frame's stage times to t6_frame_timing.log once a second");
	bool on = Dvar_GetBool(com_frameTiming);
	if (!on)
	{
		s_timingOn = false;
		s_lastLine = 0;
		return;
	}
	if (!s_frequency)
	{
		LARGE_INTEGER f;
		QueryPerformanceFrequency(&f);
		s_frequency = f.QuadPart;
	}
	long long now = FrameTiming_Counter();
	if (!s_timingOn || !s_lastLine)
	{
		// starting: the totals so far don't count
		s_timingOn = true;
		for (int i = 0; i < FRAME_TIMING_COUNT; i++)
		{
			s_reported[i][0] = s_totals[i].ticks;
			s_reported[i][1] = s_totals[i].count;
			InterlockedExchange64(&s_totals[i].max, 0);
		}
		s_lastLine = now;
		if (!s_firstLine)
			s_firstLine = now;
		return;
	}
	if (now - s_lastLine < s_frequency)
		return;
	if (FrameTiming_OpenLog())
		FrameTiming_WriteLine(now);
	s_lastLine = now;
}
#endif
