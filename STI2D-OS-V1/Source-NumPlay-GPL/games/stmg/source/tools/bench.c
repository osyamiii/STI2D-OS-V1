#include <stdio.h>
#include <time.h>
#include "../src/game.h"
#include "../src/gfx.h"
#include "../src/platform.h"
enum { ST_TITLE, ST_INTRO, ST_MAZE, ST_ENDCHEST, ST_STAIRS, ST_HEAVEN };
static double bench(int n) {
  struct timespec a, b; clock_gettime(CLOCK_MONOTONIC, &a);
  for (int i = 0; i < n; i++) { game_update(i % 3 ? 1ull << K_OK : 1ull << K_LEFT);
    for (int y = 0; y < SH; y += STRIP_H) { g_sy = y; game_draw(); } }
  clock_gettime(CLOCK_MONOTONIC, &b);
  return ((b.tv_sec - a.tv_sec) * 1e3 + (b.tv_nsec - a.tv_nsec) / 1e6) / n;
}
int main(void) {
  game_init(1);
  game_debug(DBG_GOD_MODE, 1, 0);
  game_debug(DBG_STATE, ST_MAZE, 0);
  printf("maze  : %.3f ms/frame\n", bench(600));
  game_debug(DBG_STATE, ST_HEAVEN, 0);
  game_debug(DBG_TELEPORT, 160, 200);
  for (int i = 0; i < 100; i++) game_update(1ull << K_UP);
  game_debug(DBG_BOSS_HP, 140, 0);
  game_debug(DBG_TELEPORT, 160, 200);
  printf("boss  : %.3f ms/frame\n", bench(600));
}
