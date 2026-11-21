// Banc d'essai sans écran : joue des scénarios et enregistre des captures PPM.
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/game.h"
#include "../src/gfx.h"
#include "../src/maze.h"
#include "../src/platform.h"
#include "../src/sprites.h"

enum { ST_TITLE, ST_INTRO, ST_MAZE, ST_ENDCHEST, ST_STAIRS, ST_HEAVEN, ST_DYING, ST_GAMEOVER, ST_VICTORY, ST_CREDITS,
       ST_DIFFICULTY, ST_STATS };

static uint16_t fb[320 * 240];
static const char *outdir = "shots";

static void render(void) {
  for (int y = 0; y < SH; y += STRIP_H) {
    g_sy = y;
    game_draw();
    memcpy(fb + y * 320, g_strip, sizeof g_strip);
  }
}

static void shot(const char *name) {
  render();
  char path[256];
  snprintf(path, sizeof path, "%s/%s.ppm", outdir, name);
  FILE *f = fopen(path, "wb");
  fprintf(f, "P6 320 240 255\n");
  for (int i = 0; i < 320 * 240; i++) {
    uint16_t c = fb[i];
    unsigned char rgb[3] = {(unsigned char)((c >> 11) << 3), (unsigned char)(((c >> 5) & 63) << 2), (unsigned char)((c & 31) << 3)};
    fwrite(rgb, 1, 3, f);
  }
  fclose(f);
}

static void tick(uint64_t k, int n) {
  for (int i = 0; i < n; i++) { game_update(k); if (i % 7 == 0) render(); }
}
static void press(int key) { tick(1ull << key, 1); tick(0, 1); }

// Pilote automatique : suit le plus court chemin vers la salle finale.
static uint16_t rd[CH][CW];
static void room_bfs(void) {
  static uint16_t q[CW * CH];
  const int DX[4] = {1, 0, -1, 0}, DY[4] = {0, 1, 0, -1};
  memset(rd, 0xFF, sizeof rd);
  int h = 0, t = 0;
  rd[CH - 2][CW - 2] = 0; q[t++] = (CH - 2) * CW + CW - 2;
  while (h < t) {
    int c = q[h++], x = c % CW, y = c / CW;
    for (int k = 0; k < 4; k++) if (cell_link(x, y, k)) {
      int nx = x + DX[k], ny = y + DY[k];
      if (rd[ny][nx] == 0xFFFF) { rd[ny][nx] = rd[y][x] + 1; q[t++] = ny * CW + nx; }
    }
  }
}
static bool stairs_mode;
static uint64_t autopilot(int px, int py, bool fire) {
  const int DX[4] = {1, 0, -1, 0}, DY[4] = {0, 1, 0, -1};
  int cx = (px - 16) / 48, cy = (py - 16) / 48;
  if (cx >= CW) cx = CW - 1;
  if (cy >= CH) cy = CH - 1;
  int tx = cell_px(cx), ty = cell_px(cy);
  for (int k = 0; k < 4; k++) if (cell_link(cx, cy, k)) {
    int nx = cx + DX[k], ny = cy + DY[k];
    if (rd[ny][nx] < rd[cy][cx]) { tx = cell_px(nx); ty = cell_px(ny); break; }
  }
  if (cell_in_room(cx, cy) && game_debug(DBG_INFO, 5, 0) == ST_MAZE && game_debug(DBG_INFO, 11, 0) >= 0 && rd[cy][cx] <= 2 && stairs_mode) { tx = cell_px(CW - 1); ty = cell_px(CH - 1); }
  uint64_t k = 0;
  // Se recentre d'abord sur l'axe du couloir pour ne pas accrocher les coins.
  int ccx = cell_px(cx), ccy = cell_px(cy);
  bool horiz = ty == ccy && tx != ccx;
  if (horiz) {
    if (py < ty - 3) k |= 1ull << K_DOWN; else if (py > ty + 3) k |= 1ull << K_UP;
    if (tx > px + 1) k |= 1ull << K_RIGHT; else if (tx < px - 1) k |= 1ull << K_LEFT;
  } else {
    if (px < tx - 3) k |= 1ull << K_RIGHT; else if (px > tx + 3) k |= 1ull << K_LEFT;
    if (ty > py + 1) k |= 1ull << K_DOWN; else if (ty < py - 1) k |= 1ull << K_UP;
  }
  if (fire) k |= 1ull << K_OK;
  return k;
}

int main(int argc, char **argv) {
  if (argc > 1) outdir = argv[1];
  if (sprites_check() != -1) { printf("SPRITE INVALIDE #%d\n", sprites_check()); return 1; }
  game_init(12345);
  shot("01_title");
  // Menu : Difficulté -> Hardcore -> retour, puis remet Normal pour la course de test.
  press(K_DOWN);
  tick(0, 8);
  shot("01b_title_menu");
  press(K_OK);
  press(K_DOWN);
  press(K_DOWN);
  tick(0, 2);
  shot("01c_difficulty");
  press(K_OK);
  tick(0, 8);
  shot("01d_title_hard");
  press(K_OK);
  press(K_UP);
  press(K_UP);
  press(K_OK);
  press(K_DOWN);
  press(K_OK);
  tick(0, 70);
  shot("01e_credits_menu");
  press(K_OK);
  press(K_UP);
  press(K_UP);
  tick(0, 2);
  press(K_OK);
  tick(0, 15);
  shot("02_intro");
  press(K_OK);
  tick(0, 5);
  printf("pathlen=%d maxdist=%d enemies=%d chests=%d rankS=%d ticks (%.1fs)\n", game_debug(DBG_INFO, 0, 0),
         game_debug(DBG_INFO, 1, 0), game_debug(DBG_INFO, 2, 0), game_debug(DBG_INFO, 3, 0),
         game_debug(DBG_INFO, 8, 0), game_debug(DBG_INFO, 8, 0) / 30.0);
  shot("03_maze_start");
  tick(1ull << K_RIGHT, 20);
  tick((1ull << K_RIGHT) | (1ull << K_OK), 3);
  shot("04_maze_attack");

  // Vue de tout le labyrinthe (sans vignette) pour vérification : dump de la carte.
  {
    FILE *f = fopen("shots/maze_map.pgm", "wb");
    fprintf(f, "P5 %d %d 255\n", MW, MH);
    for (int i = 0; i < MW * MH; i++) fputc(g_map[i] == T_WALL ? 0 : g_map[i] == T_STAIRUP ? 128 : 255, f);
    fclose(f);
  }

  // Course complète en pilote automatique, invincible, pour tester l'enchaînement.
  game_debug(DBG_GOD_MODE, 1, 0);
  room_bfs();
  int t = 0, shots = 0;
  bool seen_enemy = false;
  while (game_debug(DBG_INFO, 5, 0) == ST_MAZE && t < 30 * 60 * 15) {
    int px = game_debug(DBG_INFO, 9, 0), py = game_debug(DBG_INFO, 10, 0);
    game_update(autopilot(px, py, (t / 3) % 2));
    t++;
    if (t % 1500 == 0 && shots < 3) { render(); char n[32]; sprintf(n, "05_maze_run%d", shots++); shot(n); }
    (void)seen_enemy;
  }
  printf("autopilot: %d ticks (%.1fs), state=%d hp=%d enemies alive=%d\n", t, t / 30.0, game_debug(DBG_INFO, 5, 0),
         game_debug(DBG_INFO, 4, 0), game_debug(DBG_INFO, 2, 0));
  tick(0, 30);
  shot("06_endchest");
  printf("rank=%d\n", game_debug(DBG_INFO, 11, 0));
  press(K_OK);
  tick(0, 2);
  shot("07_final_room");
  stairs_mode = true;
  for (int i = 0; i < 400 && game_debug(DBG_INFO, 5, 0) == ST_MAZE; i++) {
    int px = game_debug(DBG_INFO, 9, 0), py = game_debug(DBG_INFO, 10, 0);
    game_update(autopilot(px, py, false));
  }
  printf("after stairs walk state=%d\n", game_debug(DBG_INFO, 5, 0));
  tick(0, 5);
  shot("08_stairs_bottom");
  tick(1ull << K_UP, 150);
  shot("09_stairs_mid");
  tick(1ull << K_UP, 160);
  printf("state after stairs=%d\n", game_debug(DBG_INFO, 5, 0));
  tick(0, 40);
  shot("10_heaven_arrival");
  tick(1ull << K_UP, 150);
  shot("11_heaven_path");
  tick(1ull << K_UP, 140);
  printf("fight=%d\n", game_debug(DBG_INFO, 7, 0));
  tick(0, 80);
  shot("12_boss_intro");
  for (int i = 0; i < 300; i++) game_update(((i / 40) % 2 ? 1ull << K_LEFT : 1ull << K_RIGHT) | ((i % 4) ? 0 : 1ull << K_OK));
  shot("13_boss_fight");
  game_debug(DBG_BOSS_HP, 151, 0);
  game_debug(DBG_TELEPORT, 160, 200);
  tick(1ull << K_UP, 1);
  for (int i = 0; i < 200 && game_debug(DBG_INFO, 6, 0) > 150; i++) { game_update((i % 2) ? 1ull << K_OK : 1ull << K_UP); if (i % 2) game_debug(DBG_TELEPORT, 160, 200); }
  tick(0, 60);
  printf("boss hp=%d angels=%d\n", game_debug(DBG_INFO, 6, 0), game_debug(DBG_INFO, 12, 0));
  shot("14_boss_phase2");
  game_debug(DBG_BOSS_HP, 1, 0);
  game_debug(DBG_TELEPORT, 160, 200);
  for (int i = 0; i < 400 && game_debug(DBG_INFO, 5, 0) == ST_HEAVEN; i++) { game_update((i % 2) ? 1ull << K_OK : 1ull << K_UP); if (i % 2) game_debug(DBG_TELEPORT, 160, 200); }
  printf("state after kill=%d\n", game_debug(DBG_INFO, 5, 0));
  tick(0, 40);
  shot("15_victory_explosion");
  tick(0, 200);
  shot("16_throne");
  tick(0, 100);
  press(K_OK);
  tick(0, 40);
  shot("16b_stats");
  press(K_OK);
  tick(0, 100);
  shot("17_credits");
  press(K_OK);
  tick(0, 5);
  shot("18_title_record");

  // Mort et game over
  game_debug(DBG_STATE, ST_MAZE, 0);
  game_debug(DBG_GOD_MODE, 0, 0);
  game_debug(DBG_GIVE, 1, 0);  // bouclier
  game_debug(DBG_GIVE, 2, 0);  // feu
  game_debug(DBG_GIVE, 4, 0);  // torche
  tick(0, 1);
  int hp0 = game_debug(DBG_INFO, 4, 0);
  game_debug(DBG_TELEPORT, cell_px(CW / 2), cell_px(CH / 2));
  for (int i = 0; i < 9000 && game_debug(DBG_INFO, 5, 0) == ST_MAZE; i++) {
    game_update(((i / 25) % 4 == 0 ? 1ull << K_LEFT : (i / 25) % 4 == 1 ? 1ull << K_UP : (i / 25) % 4 == 2 ? 1ull << K_RIGHT : 1ull << K_DOWN) | (i % 3 ? 0 : 1ull << K_OK));
    if (i == 60) shot("19_maze_torch_fire");
  }
  for (int want = 0; want <= 2; want++) {
    game_debug(DBG_STATE, ST_MAZE, 0);
    game_debug(DBG_GOD_MODE, 1, 0);
    game_debug(DBG_GIVE, 4, 0);
    bool found = false;
    for (int cy = 3; cy < CH && !found; cy++)
      for (int cx = 3; cx < CW && !found; cx++) {
        game_debug(DBG_TELEPORT, cell_px(cx), cell_px(cy));
        int ex = game_debug(DBG_INFO, 13, 0), ey = game_debug(DBG_INFO, 14, 0);
        if (game_debug(DBG_INFO, 15, 0) == want && ex * ex + ey * ey < 70 * 70 && ex * ex + ey * ey > 30 * 30) found = true;
      }
    tick(0, 20);
    char n[32]; sprintf(n, "23_enemy_type%d", want); shot(n);
  }
  printf("hp before=%d, state now=%d (6=dying)\n", hp0, game_debug(DBG_INFO, 5, 0));
  tick(0, 80);
  shot("20_gameover");
  press(K_OK);
  tick(0, 3);
  shot("21_restart");
  press(K_BACK);
  tick(0, 2);
  shot("22_pause");
  return 0;
}
