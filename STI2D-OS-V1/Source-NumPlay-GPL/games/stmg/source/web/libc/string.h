// libc minimale pour la version WebAssembly (pas de sysroot WASI nécessaire).
#ifndef WEB_STRING_H
#define WEB_STRING_H
#include <stddef.h>
void *memset(void *d, int c, size_t n);
void *memcpy(void *d, const void *s, size_t n);
size_t strlen(const char *s);
char *strcpy(char *d, const char *s);
char *strncpy(char *d, const char *s, size_t n);
char *strcat(char *d, const char *s);
#endif
