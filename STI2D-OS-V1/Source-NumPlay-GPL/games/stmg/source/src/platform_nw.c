// Plateforme NumWorks (EADK).
#include <eadk.h>

#include "platform.h"

void plat_init(void) {}
uint64_t plat_keys(void) { return eadk_keyboard_scan(); }
uint64_t plat_millis(void) { return eadk_timing_millis(); }
void plat_push(int y, int h, const uint16_t *px) {
  eadk_rect_t r = {0, (uint16_t)y, 320, (uint16_t)h};
  eadk_display_push_rect(r, px);
}
void plat_frame_done(void) {}
bool plat_quit(void) { return false; }
void plat_sleep(int ms) { eadk_timing_msleep((uint32_t)ms); }
uint32_t plat_rand(void) { return eadk_random(); }
