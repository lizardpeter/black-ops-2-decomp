#pragma once

union vec3_t;

// 0041D690  universal/com_math.h:544: the larger, branch-free style (a if a - b >= 0, else b: b when either is NaN)
inline float I_fmax(float a, float b)
{
	return a - b >= 0.0f ? a : b;
}

// 0041D6D0  universal/com_math.h:550: the smaller (a if b - a >= 0, else b: b when either is NaN)
inline float I_fmin(float a, float b)
{
	return b - a >= 0.0f ? a : b;
}

signed char ClampChar(int i);
short ClampShort(int i);

// com_math_anglevectors.cpp
void AngleVectors(const vec3_t &angles, vec3_t *forward, vec3_t *right, vec3_t *up);
