#include "game.h"
#include "gfx.h"
#include "platform.h"

#if PLATFORM_DEVICE
#include <eadk.h>
const char eadk_app_name[] __attribute__((section(".rodata.eadk_app_name"))) = "STI2D vs STMG";
const uint32_t eadk_api_level __attribute__((section(".rodata.eadk_api_level"))) = 0;
#endif

// Boucle principale : logique à 30 Hz fixes, rendu par bandes après chaque mise à jour.
int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  plat_init();
  game_init(plat_rand());
  uint64_t last = plat_millis();
  uint32_t acc = 0;  // en tiers de milliseconde (1 tick = 100)
  for (;;) {
    uint64_t now = plat_millis();
    acc += (uint32_t)(now - last) * 3;
    last = now;
    if (acc > 400) acc = 400;
    if (acc < 100) {
      plat_sleep(2);
      continue;
    }
    while (acc >= 100) {
      acc -= 100;
      if (!game_update(plat_keys())) return 0;
    }
    for (int y = 0; y < SH; y += STRIP_H) {
      g_sy = y;
      game_draw();
      plat_push(y, STRIP_H, g_strip);
    }
    plat_frame_done();
    if (plat_quit()) return 0;
  }
}
