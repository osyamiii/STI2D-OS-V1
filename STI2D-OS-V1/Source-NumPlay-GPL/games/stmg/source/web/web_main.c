// Point d'entrée WebAssembly : le JavaScript de la page pilote la boucle à 30 Hz,
// envoie l'état des touches et récupère l'image 320x240 (RGB565).
#include <string.h>

#include "../src/game.h"
#include "../src/gfx.h"

#define EXPORT(name) __attribute__((export_name(name)))

static uint16_t fb[SW * SH];
static uint8_t save_buf[256];

EXPORT("init") void web_init(uint32_t seed) { game_init(seed); }

// false : le jeu demande à quitter (Back sur le titre) ; la page revient alors au titre.
EXPORT("update") int web_update(uint32_t keys) { return game_update(keys); }

EXPORT("frame") uint16_t *web_frame(void) {
  for (int y = 0; y < SH; y += STRIP_H) {
    g_sy = y;
    game_draw();
    memcpy(fb + y * SW, g_strip, sizeof g_strip);
  }
  return fb;
}

EXPORT("save_buf") uint8_t *web_save_buf(void) { return save_buf; }
EXPORT("save") int web_save(void) { return game_save(save_buf, sizeof save_buf); }
EXPORT("load") void web_load(int n) { game_load(save_buf, n); }
