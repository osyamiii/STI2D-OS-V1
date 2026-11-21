#include "gfx.h"

#include <math.h>
#include <string.h>

uint16_t g_strip[SW * STRIP_H];
int g_sy;
uint16_t g_pal[128];

// Police 5x7 (colonnes, bit 0 en haut). ASCII 32..126 puis lettres accentuées.
static const uint8_t FONT[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x5F, 0x00, 0x00}, {0x00, 0x07, 0x00, 0x07, 0x00},
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, {0x24, 0x2A, 0x7F, 0x2A, 0x12}, {0x23, 0x13, 0x08, 0x64, 0x62},
    {0x36, 0x49, 0x55, 0x22, 0x50}, {0x00, 0x05, 0x03, 0x00, 0x00}, {0x00, 0x1C, 0x22, 0x41, 0x00},
    {0x00, 0x41, 0x22, 0x1C, 0x00}, {0x08, 0x2A, 0x1C, 0x2A, 0x08}, {0x08, 0x08, 0x3E, 0x08, 0x08},
    {0x00, 0x50, 0x30, 0x00, 0x00}, {0x08, 0x08, 0x08, 0x08, 0x08}, {0x00, 0x60, 0x60, 0x00, 0x00},
    {0x20, 0x10, 0x08, 0x04, 0x02}, {0x3E, 0x51, 0x49, 0x45, 0x3E}, {0x00, 0x42, 0x7F, 0x40, 0x00},
    {0x42, 0x61, 0x51, 0x49, 0x46}, {0x21, 0x41, 0x45, 0x4B, 0x31}, {0x18, 0x14, 0x12, 0x7F, 0x10},
    {0x27, 0x45, 0x45, 0x45, 0x39}, {0x3C, 0x4A, 0x49, 0x49, 0x30}, {0x01, 0x71, 0x09, 0x05, 0x03},
    {0x36, 0x49, 0x49, 0x49, 0x36}, {0x06, 0x49, 0x49, 0x29, 0x1E}, {0x00, 0x36, 0x36, 0x00, 0x00},
    {0x00, 0x56, 0x36, 0x00, 0x00}, {0x08, 0x14, 0x22, 0x41, 0x00}, {0x14, 0x14, 0x14, 0x14, 0x14},
    {0x00, 0x41, 0x22, 0x14, 0x08}, {0x02, 0x01, 0x51, 0x09, 0x06}, {0x32, 0x49, 0x79, 0x41, 0x3E},
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, {0x7F, 0x49, 0x49, 0x49, 0x36}, {0x3E, 0x41, 0x41, 0x41, 0x22},
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, {0x7F, 0x49, 0x49, 0x49, 0x41}, {0x7F, 0x09, 0x09, 0x01, 0x01},
    {0x3E, 0x41, 0x41, 0x51, 0x32}, {0x7F, 0x08, 0x08, 0x08, 0x7F}, {0x00, 0x41, 0x7F, 0x41, 0x00},
    {0x20, 0x40, 0x41, 0x3F, 0x01}, {0x7F, 0x08, 0x14, 0x22, 0x41}, {0x7F, 0x40, 0x40, 0x40, 0x40},
    {0x7F, 0x02, 0x04, 0x02, 0x7F}, {0x7F, 0x04, 0x08, 0x10, 0x7F}, {0x3E, 0x41, 0x41, 0x41, 0x3E},
    {0x7F, 0x09, 0x09, 0x09, 0x06}, {0x3E, 0x41, 0x51, 0x21, 0x5E}, {0x7F, 0x09, 0x19, 0x29, 0x46},
    {0x46, 0x49, 0x49, 0x49, 0x31}, {0x01, 0x01, 0x7F, 0x01, 0x01}, {0x3F, 0x40, 0x40, 0x40, 0x3F},
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, {0x7F, 0x20, 0x18, 0x20, 0x7F}, {0x63, 0x14, 0x08, 0x14, 0x63},
    {0x03, 0x04, 0x78, 0x04, 0x03}, {0x61, 0x51, 0x49, 0x45, 0x43}, {0x00, 0x00, 0x7F, 0x41, 0x41},
    {0x02, 0x04, 0x08, 0x10, 0x20}, {0x41, 0x41, 0x7F, 0x00, 0x00}, {0x04, 0x02, 0x01, 0x02, 0x04},
    {0x40, 0x40, 0x40, 0x40, 0x40}, {0x00, 0x01, 0x02, 0x04, 0x00}, {0x20, 0x54, 0x54, 0x54, 0x78},
    {0x7F, 0x48, 0x44, 0x44, 0x38}, {0x38, 0x44, 0x44, 0x44, 0x20}, {0x38, 0x44, 0x44, 0x48, 0x7F},
    {0x38, 0x54, 0x54, 0x54, 0x18}, {0x08, 0x7E, 0x09, 0x01, 0x02}, {0x18, 0xA4, 0xA4, 0xA4, 0x7C},
    {0x7F, 0x08, 0x04, 0x04, 0x78}, {0x00, 0x44, 0x7D, 0x40, 0x00}, {0x40, 0x80, 0x84, 0x7D, 0x00},
    {0x00, 0x7F, 0x10, 0x28, 0x44}, {0x00, 0x41, 0x7F, 0x40, 0x00}, {0x7C, 0x04, 0x18, 0x04, 0x78},
    {0x7C, 0x08, 0x04, 0x04, 0x78}, {0x38, 0x44, 0x44, 0x44, 0x38}, {0xFC, 0x24, 0x24, 0x24, 0x18},
    {0x18, 0x24, 0x24, 0x18, 0xFC}, {0x7C, 0x08, 0x04, 0x04, 0x08}, {0x48, 0x54, 0x54, 0x54, 0x20},
    {0x04, 0x3F, 0x44, 0x40, 0x20}, {0x3C, 0x40, 0x40, 0x20, 0x7C}, {0x1C, 0x20, 0x40, 0x20, 0x1C},
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, {0x44, 0x28, 0x10, 0x28, 0x44}, {0x1C, 0xA0, 0xA0, 0xA0, 0x7C},
    {0x44, 0x64, 0x54, 0x4C, 0x44}, {0x00, 0x08, 0x36, 0x41, 0x00}, {0x00, 0x00, 0x7F, 0x00, 0x00},
    {0x00, 0x41, 0x36, 0x08, 0x00}, {0x08, 0x04, 0x08, 0x10, 0x08},
    // 95.. : é è ê à ç ô î û ù â ë ï
    {0x38, 0x54, 0x56, 0x55, 0x18}, {0x38, 0x55, 0x56, 0x54, 0x18}, {0x38, 0x56, 0x55, 0x56, 0x18},
    {0x20, 0x55, 0x56, 0x54, 0x78}, {0x38, 0x44, 0xC4, 0x44, 0x20}, {0x38, 0x46, 0x45, 0x46, 0x38},
    {0x00, 0x46, 0x7D, 0x42, 0x00}, {0x3C, 0x42, 0x41, 0x22, 0x7C}, {0x3C, 0x41, 0x42, 0x20, 0x7C},
    {0x20, 0x56, 0x55, 0x56, 0x78}, {0x38, 0x55, 0x54, 0x55, 0x18}, {0x00, 0x45, 0x7C, 0x41, 0x00},
};

static int next_glyph(const char **ps) {
  const uint8_t *p = (const uint8_t *)*ps;
  uint8_t c = *p++;
  int g = 0;
  if (c >= 32 && c < 127) {
    g = c - 32;
  } else if (c == 0xC3) {
    uint8_t d = *p ? *p++ : 0;
    switch (d) {
      case 0xA9: g = 95; break;   // é
      case 0xA8: g = 96; break;   // è
      case 0xAA: g = 97; break;   // ê
      case 0xA0: g = 98; break;   // à
      case 0xA7: g = 99; break;   // ç
      case 0xB4: g = 100; break;  // ô
      case 0xAE: g = 101; break;  // î
      case 0xBB: g = 102; break;  // û
      case 0xB9: g = 103; break;  // ù
      case 0xA2: g = 104; break;  // â
      case 0xAB: g = 105; break;  // ë
      case 0xAF: g = 106; break;  // ï
      case 0x89: case 0x88: case 0x8A: g = 'E' - 32; break;
      case 0x80: g = 'A' - 32; break;
      default: g = '?' - 32;
    }
  } else if (c >= 0x80) {
    while ((*p & 0xC0) == 0x80) p++;
    g = '?' - 32;
  }
  *ps = (const char *)p;
  return g;
}

void gfx_init(void) {
  memset(g_pal, 0, sizeof g_pal);
  g_pal['k'] = RGB(0, 0, 0);
  g_pal['G'] = RGB(55, 52, 66);
  g_pal['g'] = RGB(128, 128, 142);
  g_pal['W'] = RGB(200, 204, 216);
  g_pal['w'] = RGB(255, 255, 255);
  g_pal['r'] = RGB(225, 32, 44);
  g_pal['R'] = RGB(140, 12, 28);
  g_pal['m'] = RGB(72, 6, 18);
  g_pal['o'] = RGB(255, 140, 24);
  g_pal['y'] = RGB(255, 222, 64);
  g_pal['Y'] = RGB(196, 132, 24);
  g_pal['h'] = RGB(255, 250, 200);
  g_pal['p'] = RGB(255, 208, 164);
  g_pal['P'] = RGB(220, 160, 120);
  g_pal['b'] = RGB(64, 128, 255);
  g_pal['B'] = RGB(24, 44, 150);
  g_pal['c'] = RGB(150, 224, 255);
  g_pal['n'] = RGB(134, 82, 42);
  g_pal['N'] = RGB(70, 40, 20);
  g_pal['l'] = RGB(112, 204, 64);
  g_pal['L'] = RGB(40, 112, 32);
  g_pal['v'] = RGB(140, 64, 178);
  g_pal['V'] = RGB(62, 22, 84);
  g_pal['e'] = RGB(255, 60, 30);
}

uint16_t blend565(uint16_t a, uint16_t b, int t) {
  if (t <= 0) return a;
  if (t >= 32) return b;
  int ra = a >> 11, ga = (a >> 5) & 63, ba = a & 31;
  int rb = b >> 11, gb = (b >> 5) & 63, bb = b & 31;
  int r = ra + (((rb - ra) * t) >> 5);
  int g = ga + (((gb - ga) * t) >> 5);
  int bl = ba + (((bb - ba) * t) >> 5);
  return (uint16_t)((r << 11) | (g << 5) | bl);
}

void gfx_clear(uint16_t c) {
  for (int i = 0; i < SW * STRIP_H; i++) g_strip[i] = c;
}

// Calcule l'intersection d'un rectangle avec la bande courante.
static bool clip(int *x, int *y, int *w, int *h) {
  int x0 = *x < 0 ? 0 : *x, x1 = *x + *w > SW ? SW : *x + *w;
  int y0 = *y < g_sy ? g_sy : *y, y1 = *y + *h > g_sy + STRIP_H ? g_sy + STRIP_H : *y + *h;
  if (x0 >= x1 || y0 >= y1) return false;
  *x = x0; *y = y0; *w = x1 - x0; *h = y1 - y0;
  return true;
}

void gfx_fill(int x, int y, int w, int h, uint16_t c) {
  if (!clip(&x, &y, &w, &h)) return;
  for (int yy = 0; yy < h; yy++) {
    uint16_t *p = g_strip + (y - g_sy + yy) * SW + x;
    for (int n = 0; n < w; n++) p[n] = c;
  }
}

void gfx_darken(int x, int y, int w, int h, int shift) {
  if (!clip(&x, &y, &w, &h)) return;
  for (int yy = 0; yy < h; yy++) {
    uint16_t *p = g_strip + (y - g_sy + yy) * SW + x;
    for (int n = 0; n < w; n++) p[n] = shift >= 2 ? quarter565(p[n]) : half565(p[n]);
  }
}

void gfx_blend_rect(int x, int y, int w, int h, uint16_t c, int t) {
  if (!clip(&x, &y, &w, &h)) return;
  for (int yy = 0; yy < h; yy++) {
    uint16_t *p = g_strip + (y - g_sy + yy) * SW + x;
    for (int n = 0; n < w; n++) p[n] = blend565(p[n], c, t);
  }
}

void gfx_fade(uint16_t c, int t) {
  if (t <= 0) return;
  if (t >= 32) { gfx_clear(c); return; }
  for (int i = 0; i < SW * STRIP_H; i++) g_strip[i] = blend565(g_strip[i], c, t);
}

void gfx_rect(int x, int y, int w, int h, uint16_t c) {
  gfx_fill(x, y, w, 1, c);
  gfx_fill(x, y + h - 1, w, 1, c);
  gfx_fill(x, y, 1, h, c);
  gfx_fill(x + w - 1, y, 1, h, c);
}

static inline int isqrt(int v) { return v <= 0 ? 0 : (int)fsqrt((float)v); }

void gfx_disc(int cx, int cy, int r, uint16_t c) {
  if (!gfx_rows_visible(cy - r, 2 * r + 1)) return;
  for (int y = cy - r; y <= cy + r; y++) {
    if (y < g_sy || y >= g_sy + STRIP_H) continue;
    int w = isqrt(r * r - (y - cy) * (y - cy));
    gfx_fill(cx - w, y, 2 * w + 1, 1, c);
  }
}

void gfx_ring(int cx, int cy, int ro, int ri, uint16_t c) {
  if (!gfx_rows_visible(cy - ro, 2 * ro + 1)) return;
  for (int y = cy - ro; y <= cy + ro; y++) {
    if (y < g_sy || y >= g_sy + STRIP_H) continue;
    int dy2 = (y - cy) * (y - cy);
    int wo = isqrt(ro * ro - dy2);
    if (dy2 >= ri * ri) {
      gfx_fill(cx - wo, y, 2 * wo + 1, 1, c);
    } else {
      int wi = isqrt(ri * ri - dy2);
      gfx_fill(cx - wo, y, wo - wi, 1, c);
      gfx_fill(cx + wi + 1, y, wo - wi, 1, c);
    }
  }
}

void gfx_line(int x0, int y0, int x1, int y1, int thick, uint16_t c) {
  int miny = (y0 < y1 ? y0 : y1) - thick, maxy = (y0 > y1 ? y0 : y1) + thick;
  if (!gfx_rows_visible(miny, maxy - miny + 1)) return;
  int dx = x1 > x0 ? x1 - x0 : x0 - x1, sx = x0 < x1 ? 1 : -1;
  int dy = y1 > y0 ? y0 - y1 : y1 - y0, sy = y0 < y1 ? 1 : -1;
  int err = dx + dy, o = thick / 2;
  for (;;) {
    gfx_fill(x0 - o, y0 - o, thick, thick, c);
    if (x0 == x1 && y0 == y1) break;
    int e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}

void gfx_sprite_ex(const Sprite *s, int x, int y, int sc, int flags, int maxcol) {
  int top = g_sy, bot = g_sy + STRIP_H;
  if (y >= bot || y + s->h * sc <= top || x >= SW || x + s->w * sc <= 0) return;
  int r0 = y < top ? (top - y) / sc : 0;
  int r1 = s->h;
  if (y + r1 * sc > bot) r1 = (bot - y + sc - 1) / sc;
  int cols = maxcol < s->w ? maxcol : s->w;
  for (int r = r0; r < r1; r++) {
    int py = y + r * sc;
    int ya = py < top ? top : py, yb = py + sc > bot ? bot : py + sc;
    const char *row = s->data + r * s->w;
    for (int c = 0; c < cols; c++) {
      char ch = row[(flags & SPR_FLIP) ? s->w - 1 - c : c];
      if (ch == '.') continue;
      if ((flags & SPR_GHOST) && ((r + c) & 1)) continue;
      uint16_t col = (flags & SPR_WHITE) ? 0xFFFF : (flags & SPR_BLACK) ? 0 : g_pal[(uint8_t)ch & 127];
      int px = x + c * sc, pw = sc;
      if (px < 0) { pw += px; px = 0; }
      if (px + pw > SW) pw = SW - px;
      if (pw <= 0) continue;
      for (int yy = ya; yy < yb; yy++) {
        uint16_t *p = g_strip + (yy - top) * SW + px;
        for (int n = 0; n < pw; n++) p[n] = col;
      }
    }
  }
}

void gfx_sprite(const Sprite *s, int x, int y, int sc, int flags) { gfx_sprite_ex(s, x, y, sc, flags, 255); }

void gfx_blit16(const uint16_t *tex, int x, int y) {
  int top = g_sy, bot = g_sy + STRIP_H;
  if (y >= bot || y + 16 <= top || x >= SW || x + 16 <= 0) return;
  int c0 = x < 0 ? -x : 0, c1 = x + 16 > SW ? SW - x : 16;
  int r0 = y < top ? top - y : 0, r1 = y + 16 > bot ? bot - y : 16;
  for (int r = r0; r < r1; r++)
    memcpy(g_strip + (y + r - top) * SW + x + c0, tex + r * 16 + c0, (size_t)(c1 - c0) * 2);
}

// Obscurité du Purgatoire : seul un disque autour de (cx,cy) reste visible,
// avec deux anneaux tramés en pixel art avant le noir total.
void gfx_vignette(int cx, int cy, int r) {
  int r1 = r * 5 / 8, r2 = r * 13 / 16;
  for (int yy = 0; yy < STRIP_H; yy++) {
    int y = g_sy + yy, dy2 = (y - cy) * (y - cy);
    uint16_t *row = g_strip + yy * SW;
    if (dy2 >= r * r) { memset(row, 0, SW * 2); continue; }
    int w3 = isqrt(r * r - dy2);
    int w2 = dy2 < r2 * r2 ? isqrt(r2 * r2 - dy2) : -1;
    int w1 = dy2 < r1 * r1 ? isqrt(r1 * r1 - dy2) : -1;
    for (int x = 0; x < SW; x++) {
      int ax = x - cx;
      if (ax < 0) ax = -ax;
      if (ax > w3) row[x] = 0;
      else if (ax > w2) row[x] = ((x + y) & 1) ? 0 : quarter565(row[x]);
      else if (ax > w1) row[x] = half565(row[x]);
    }
  }
}

int gfx_text_w(const char *s, int sc) {
  int n = 0;
  while (*s) { next_glyph(&s); n++; }
  return n ? n * 6 * sc - sc : 0;
}

void gfx_text(const char *s, int x, int y, int sc, uint16_t c) {
  if (!gfx_rows_visible(y, 8 * sc)) return;
  while (*s) {
    const uint8_t *g = FONT[next_glyph(&s)];
    for (int col = 0; col < 5; col++)
      for (int row = 0; row < 8; row++)
        if (g[col] & (1 << row)) gfx_fill(x + col * sc, y + row * sc, sc, sc, c);
    x += 6 * sc;
  }
}

void gfx_text_c(const char *s, int cx, int y, int sc, uint16_t c) { gfx_text(s, cx - gfx_text_w(s, sc) / 2, y, sc, c); }

void gfx_text_sh(const char *s, int cx, int y, int sc, uint16_t c) {
  int x = cx - gfx_text_w(s, sc) / 2;
  gfx_text(s, x + sc, y + sc, sc, 0);
  gfx_text(s, x, y, sc, c);
}
