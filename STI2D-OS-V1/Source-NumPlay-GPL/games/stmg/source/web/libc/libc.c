#include <string.h>

// Compilé avec -mbulk-memory : ces deux appels deviennent memory.fill / memory.copy.
void *memset(void *d, int c, size_t n) { return __builtin_memset(d, c, n); }
void *memcpy(void *d, const void *s, size_t n) { return __builtin_memcpy(d, s, n); }
size_t strlen(const char *s) {
  size_t n = 0;
  while (s[n]) n++;
  return n;
}
char *strcpy(char *d, const char *s) {
  char *r = d;
  while ((*d++ = *s++)) {}
  return r;
}
char *strncpy(char *d, const char *s, size_t n) {
  size_t i = 0;
  for (; i < n && s[i]; i++) d[i] = s[i];
  for (; i < n; i++) d[i] = 0;
  return d;
}
char *strcat(char *d, const char *s) {
  strcpy(d + strlen(d), s);
  return d;
}
