#include <stdio.h>
#include <string.h>
#include "../src/gfx.h"
#include "../src/sprites.h"
int main(void) {
  static uint16_t fb[320 * 240];
  gfx_init();
  const Sprite *L[] = {&SPR_PLAYER[0], &SPR_PLAYER[1], &SPR_EE[0], &SPR_EE[1], &SPR_SIN[0], &SPR_SIN[1], &SPR_ITEC[0], &SPR_ITEC[1], &SPR_RHC[0], &SPR_RHC[1], &SPR_GOD, &SPR_CHEST, &SPR_GOLD_CHEST, &SPR_ICON_FIRE, &SPR_ICON_BOOTS, &SPR_ICON_TORCH, &SPR_ICON_SHIELD, &SPR_ICON_HEAL};
  for (int y = 0; y < 240; y += STRIP_H) {
    g_sy = y;
    gfx_clear(RGB(52, 44, 58));
    int x = 4, yy = 4, rowh = 0;
    for (unsigned i = 0; i < sizeof L / sizeof L[0]; i++) {
      int sc = L[i] == &SPR_GOD ? 3 : 2;
      if (x + L[i]->w * sc > 316) { x = 4; yy += rowh + 6; rowh = 0; }
      gfx_sprite(L[i], x, yy, sc, 0);
      x += L[i]->w * sc + 6;
      if (L[i]->h * sc > rowh) rowh = L[i]->h * sc;
    }
    memcpy(fb + y * 320, g_strip, sizeof g_strip);
  }
  FILE *f = fopen("shots/sprites.ppm", "wb");
  fprintf(f, "P6 320 240 255\n");
  for (int i = 0; i < 320 * 240; i++) { uint16_t c = fb[i]; fputc((c >> 11) << 3, f); fputc(((c >> 5) & 63) << 2, f); fputc((c & 31) << 3, f); }
  fclose(f);
}
