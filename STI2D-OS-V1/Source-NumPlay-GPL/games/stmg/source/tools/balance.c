// Simulation d'équilibrage : un pilote suit le plus court chemin en attaquant, sans invincibilité.
#include <stdio.h>
#include <string.h>
#include "../src/game.h"
#include "../src/maze.h"
#include "../src/platform.h"
#ifndef GIVE
#define GIVE 0
#endif
enum { ST_TITLE, ST_INTRO, ST_MAZE };
static uint16_t rd[CH][CW];
static void room_bfs(void) {
  static uint16_t q[CW * CH]; const int DX[4] = {1, 0, -1, 0}, DY[4] = {0, 1, 0, -1};
  memset(rd, 0xFF, sizeof rd); int h = 0, t = 0; rd[CH - 2][CW - 2] = 0; q[t++] = (CH - 2) * CW + CW - 2;
  while (h < t) { int c = q[h++], x = c % CW, y = c / CW;
    for (int k = 0; k < 4; k++) if (cell_link(x, y, k)) { int nx = x + DX[k], ny = y + DY[k];
      if (rd[ny][nx] == 0xFFFF) { rd[ny][nx] = rd[y][x] + 1; q[t++] = ny * CW + nx; } } }
}
int main(void) {
  int wins = 0;
  for (int run = 0; run < 10; run++) {
    game_init(1000 + run);
    game_update(1ull << K_OK); game_update(0);
    for (int i = 0; i < 12; i++) game_update(0);
    game_update(0);
    game_update(1ull << K_OK); game_update(0);
    room_bfs();
    if (GIVE) { game_debug(DBG_GIVE, 2, 0); game_debug(DBG_GIVE, 1, 0); }
    int t = 0;
    while (game_debug(DBG_INFO, 5, 0) == ST_MAZE && t < 30 * 60 * 10) {
      int px = game_debug(DBG_INFO, 9, 0), py = game_debug(DBG_INFO, 10, 0);
      const int DX[4] = {1, 0, -1, 0}, DY[4] = {0, 1, 0, -1};
      int cx = (px - 16) / 48, cy = (py - 16) / 48; if (cx >= CW) cx = CW - 1; if (cy >= CH) cy = CH - 1;
      int tx = cell_px(cx), ty = cell_px(cy);
      for (int k = 0; k < 4; k++) if (cell_link(cx, cy, k)) { int nx = cx + DX[k], ny = cy + DY[k];
        if (rd[ny][nx] < rd[cy][cx]) { tx = cell_px(nx); ty = cell_px(ny); break; } }
      uint64_t k = 0; int ccx = cell_px(cx), ccy = cell_px(cy);
      if (ty == ccy && tx != ccx) { if (py < ty - 3) k |= 1ull << K_DOWN; else if (py > ty + 3) k |= 1ull << K_UP;
        if (tx > px + 1) k |= 1ull << K_RIGHT; else if (tx < px - 1) k |= 1ull << K_LEFT; }
      else { if (px < tx - 3) k |= 1ull << K_RIGHT; else if (px > tx + 3) k |= 1ull << K_LEFT;
        if (ty > py + 1) k |= 1ull << K_DOWN; else if (ty < py - 1) k |= 1ull << K_UP; }
      int ex = game_debug(DBG_INFO, 13, 0), ey = game_debug(DBG_INFO, 14, 0);
      bool aligned = (ex * ex + ey * ey < 110 * 110) && (ey < 10 && ey > -10 || ex < 10 && ex > -10);
      if (GIVE && aligned && ex * ex + ey * ey > 30 * 30 && (t % 90) < 45) {  // tir à distance : un tap pour viser puis feu
        k = 0;
        if (ey < 10 && ey > -10) k |= ex > 0 ? 1ull << K_RIGHT : 1ull << K_LEFT;
        else k |= ey > 0 ? 1ull << K_DOWN : 1ull << K_UP;
        game_update(k); t++;
        k = (t & 1) ? 1ull << K_OK : 0;
        if (ex * ex + ey * ey < 60 * 60) k |= (ey < 10 && ey > -10) ? (ex > 0 ? 1ull << K_LEFT : 1ull << K_RIGHT) : (ey > 0 ? 1ull << K_UP : 1ull << K_DOWN);
      } else if (ex * ex + ey * ey < 45 * 45) {  // se tourne vers l'ennemi et frappe
        k = 0;
        if (ex > 8) k |= 1ull << K_RIGHT; else if (ex < -8) k |= 1ull << K_LEFT;
        if (ey > 8) k |= 1ull << K_DOWN; else if (ey < -8) k |= 1ull << K_UP;
        if ((t / 2) % 2) k |= 1ull << K_OK;
      }
      int hp0 = game_debug(DBG_INFO, 4, 0);
      game_update(k); t++;
      if (0 && game_debug(DBG_INFO, 4, 0) < hp0) printf("  t=%.1fs hp %d->%d nearest type=%d d=(%d,%d) pos=(%d,%d)\n", t/30.0, hp0, game_debug(DBG_INFO, 4, 0), game_debug(DBG_INFO, 15, 0), ex, ey, px, py);
    }
    int st = game_debug(DBG_INFO, 5, 0);
    { int px = game_debug(DBG_INFO, 9, 0), py = game_debug(DBG_INFO, 10, 0); int cx = (px - 16) / 48, cy = (py - 16) / 48;
      printf("run %d: state=%d after %.0fs, hp=%d, remaining %d/%d cells, killer type %d\n", run, st, t / 30.0, game_debug(DBG_INFO, 4, 0), rd[cy][cx], rd[0][0], game_debug(DBG_INFO, 15, 0)); }
    wins += st == 3;
  }
  printf("wins %d/10\n", wins);
}
