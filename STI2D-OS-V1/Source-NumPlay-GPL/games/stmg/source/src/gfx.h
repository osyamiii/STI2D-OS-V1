// Rendu par bandes horizontales : l'écran 320x240 est dessiné en STRIP_H lignes
// à la fois dans un petit tampon, puis envoyé à l'écran. Toutes les primitives
// découpent automatiquement à la bande courante (g_sy .. g_sy+STRIP_H).
#ifndef GFX_H
#define GFX_H

#include <stdbool.h>
#include <stdint.h>

#define SW 320
#define SH 240
#define STRIP_H 24

#define RGB(r, g, b) ((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)))

extern uint16_t g_strip[SW * STRIP_H];
extern int g_sy;
extern uint16_t g_pal[128];

typedef struct {
  uint8_t w, h;
  const char *data;  // w*h caractères, '.' = transparent, sinon index de palette
} Sprite;

enum {
  SPR_FLIP = 1,   // miroir horizontal
  SPR_WHITE = 2,  // silhouette blanche (flash de dégât)
  SPR_BLACK = 4,  // silhouette noire
  SPR_GHOST = 8,  // un pixel sur deux (clignotement / transparence)
};

void gfx_init(void);
void gfx_clear(uint16_t c);
void gfx_fill(int x, int y, int w, int h, uint16_t c);
void gfx_darken(int x, int y, int w, int h, int shift);
void gfx_blend_rect(int x, int y, int w, int h, uint16_t c, int t32);
void gfx_fade(uint16_t c, int t32);
void gfx_rect(int x, int y, int w, int h, uint16_t c);
void gfx_disc(int cx, int cy, int r, uint16_t c);
void gfx_ring(int cx, int cy, int ro, int ri, uint16_t c);
void gfx_line(int x0, int y0, int x1, int y1, int thick, uint16_t c);
void gfx_sprite(const Sprite *s, int x, int y, int sc, int flags);
void gfx_sprite_ex(const Sprite *s, int x, int y, int sc, int flags, int maxcol);
void gfx_blit16(const uint16_t *tex, int x, int y);
void gfx_vignette(int cx, int cy, int r);

int gfx_text_w(const char *s, int sc);
void gfx_text(const char *s, int x, int y, int sc, uint16_t c);
void gfx_text_c(const char *s, int cx, int y, int sc, uint16_t c);
void gfx_text_sh(const char *s, int cx, int y, int sc, uint16_t c);  // centré + ombre

uint16_t blend565(uint16_t a, uint16_t b, int t32);
// Racine carrée matérielle : évite de tirer le wrapper newlib (errno/stdio) sur la calculatrice.
#if PLATFORM_DEVICE
static inline float fsqrt(float x) {
  float r;
  __asm__("vsqrt.f32 %0, %1" : "=t"(r) : "t"(x));
  return r;
}
#else
#include <math.h>
#define fsqrt sqrtf
#endif

static inline uint16_t half565(uint16_t c) { return (c >> 1) & 0x7BEF; }
static inline uint16_t quarter565(uint16_t c) { return (c >> 2) & 0x39E7; }
static inline bool gfx_rows_visible(int y, int h) { return y < g_sy + STRIP_H && y + h > g_sy; }

#endif
