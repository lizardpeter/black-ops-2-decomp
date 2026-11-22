// enhanced/qcommon/frame_timing.cpp: one frame on the CPU at a time (P0009) and the frame's stage timing.
#pragma once

// Whether the main thread may build the next frame while the render thread submits the last (r_renderOverlap, the
// original's pipelining). Off by default: the main thread waits for the render thread before reading input.
// Main thread only (registers the dvar on first use).
bool R_RenderOverlap();

// The stages timed (com_frameTiming 1: averages and maxima once a second to t6_frame_timing.log next to
// t6_enhanced.dll). Main-thread stages are averaged per main frame, render-thread stages per frame it draws.
enum FrameTimingStage
{
	FRAME_TIMING_MAIN_FRAME,     // main: from the end of the frame cap's wait to the end of the frame
	FRAME_TIMING_WAIT_RENDERER,  // main: waiting for the render thread to finish the last frame
	FRAME_TIMING_WAIT_DISPLAY,   // main: waiting for the swap chain to have room (P0007)
	FRAME_TIMING_WAIT_JOBS,      // main: the hand-off's wait for the front end's jobs (B0028)
	FRAME_TIMING_RENDER_FRAME,   // render: uploading, the back end and the present
	FRAME_TIMING_RENDER_IDLE,    // render: waiting for work (P0008)
	FRAME_TIMING_COUNT,
};

// The performance counter now, or 0 while timing is off (then FrameTiming_Add does nothing): either thread.
long long FrameTiming_Now();
// Adds the time since start (from FrameTiming_Now) to a stage: each stage is added to by one thread only.
void FrameTiming_Add(FrameTimingStage stage, long long start);
// The main thread, once at the end of each frame: follows com_frameTiming and writes a line each second.
void FrameTiming_EndFrame();
