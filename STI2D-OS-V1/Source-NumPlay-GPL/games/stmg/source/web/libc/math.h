// Fonctions mathématiques : instructions WebAssembly natives ou Math.* du navigateur.
#ifndef WEB_MATH_H
#define WEB_MATH_H
#define sqrtf __builtin_sqrtf
#define fabsf __builtin_fabsf
#define floorf __builtin_floorf
float sinf(float x) __attribute__((import_module("env"), import_name("sinf")));
float cosf(float x) __attribute__((import_module("env"), import_name("cosf")));
float atan2f(float y, float x) __attribute__((import_module("env"), import_name("atan2f")));
#endif
