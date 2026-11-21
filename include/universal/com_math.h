#pragma once

union vec3_t;

signed char ClampChar(int i);
short ClampShort(int i);

// com_math_anglevectors.cpp
void AngleVectors(const vec3_t &angles, vec3_t *forward, vec3_t *right, vec3_t *up);
