#pragma once

// Minimal RenderWare value types/math needed to compile the engine's real
// Quaternion.cpp without a graphics device or the game asset dependencies.
#include <cmath>
struct RwV3d { float x, y, z; };
struct RwMatrix { RwV3d right, up, at, pos; };
static const float PI = 3.14159265358979323846f;
static const float TWOPI = 2.0f * PI;
inline float Sqrt(float value) { return std::sqrt(value); }
inline float RecipSqrt(float value) { return 1.0f / std::sqrt(value); }
inline float Sin(float value) { return std::sin(value); }
inline float Cos(float value) { return std::cos(value); }
inline float Acos(float value) { return std::acos(value); }
inline float Atan2(float y, float x) { return std::atan2(y, x); }
