// Couche d'abstraction : NumWorks (EADK), PC (SDL2) ou tests sans écran.
#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdbool.h>
#include <stdint.h>

// Numéros de touches identiques à ceux d'EADK.
enum {
  K_LEFT = 0, K_UP = 1, K_DOWN = 2, K_RIGHT = 3, K_OK = 4, K_BACK = 5, K_HOME = 6,
  K_SHIFT = 12, K_ALPHA = 13, K_EXE = 52,
};

void plat_init(void);
uint64_t plat_keys(void);
uint64_t plat_millis(void);
void plat_push(int y, int h, const uint16_t *pixels);
void plat_frame_done(void);
bool plat_quit(void);
void plat_sleep(int ms);
uint32_t plat_rand(void);

#endif
