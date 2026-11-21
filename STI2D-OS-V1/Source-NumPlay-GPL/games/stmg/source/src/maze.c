#include "maze.h"

#include <string.h>

uint8_t g_map[MAP_MAX];
uint8_t g_cell[CH][CW];
uint16_t g_cdist[CH][CW];
int g_maxdist, g_pathlen;

static uint32_t rng;
static uint32_t rnd(void) {
  rng ^= rng << 13;
  rng ^= rng >> 17;
  rng ^= rng << 5;
  return rng;
}

static const int DX[4] = {1, 0, -1, 0}, DY[4] = {0, 1, 0, -1};

bool cell_in_room(int cx, int cy) { return cx >= ROOM_X0 && cy >= ROOM_Y0; }

static bool inside(int cx, int cy) { return cx >= 0 && cy >= 0 && cx < CW && cy < CH; }

bool cell_link(int cx, int cy, int dir) {
  switch (dir) {
    case D_E: return cx + 1 < CW && (g_cell[cy][cx] & 1);
    case D_S: return cy + 1 < CH && (g_cell[cy][cx] & 2);
    case D_W: return cx > 0 && (g_cell[cy][cx - 1] & 1);
    default: return cy > 0 && (g_cell[cy - 1][cx] & 2);
  }
}

static void link_set(int cx, int cy, int dir) {
  switch (dir) {
    case D_E: g_cell[cy][cx] |= 1; break;
    case D_S: g_cell[cy][cx] |= 2; break;
    case D_W: g_cell[cy][cx - 1] |= 1; break;
    default: g_cell[cy - 1][cx] |= 2; break;
  }
}

int cell_degree(int cx, int cy) {
  int n = 0;
  for (int d = 0; d < 4; d++) n += cell_link(cx, cy, d);
  return n;
}

static void floor_at(int tx, int ty, uint8_t t) { g_map[ty * MW + tx] = t; }

void maze_build(uint32_t seed) {
  static uint8_t vis[CH][CW];
  static uint16_t stack[CW * CH];
  rng = seed ? seed : 1;
  memset(g_cell, 0, sizeof g_cell);
  memset(vis, 0, sizeof vis);
  for (int cy = ROOM_Y0; cy < CH; cy++)
    for (int cx = ROOM_X0; cx < CW; cx++) vis[cy][cx] = 1;

  // Labyrinthe parfait (backtracker itératif).
  int sp = 0;
  stack[sp++] = 0;
  vis[0][0] = 1;
  while (sp) {
    int c = stack[sp - 1], cx = c % CW, cy = c / CW;
    int dirs[4], n = 0;
    for (int d = 0; d < 4; d++) {
      int nx = cx + DX[d], ny = cy + DY[d];
      if (inside(nx, ny) && !vis[ny][nx]) dirs[n++] = d;
    }
    if (!n) { sp--; continue; }
    int d = dirs[rnd() % n], nx = cx + DX[d], ny = cy + DY[d];
    link_set(cx, cy, d);
    vis[ny][nx] = 1;
    stack[sp++] = (uint16_t)(ny * CW + nx);
  }

  // Petites salles ouvertes 2x2 : créent des boucles et des zones de combat.
  for (int i = 0; i < 9; i++) {
    int rx = 2 + (int)(rnd() % (CW - 7)), ry = 2 + (int)(rnd() % (CH - 7));
    link_set(rx, ry, D_E); link_set(rx, ry, D_S);
    link_set(rx + 1, ry, D_S); link_set(rx, ry + 1, D_E);
  }
  // Tresse : quelques culs-de-sac sont percés pour multiplier les chemins.
  for (int cy = 0; cy < CH; cy++)
    for (int cx = 0; cx < CW; cx++) {
      if (cell_in_room(cx, cy) || cell_degree(cx, cy) != 1 || rnd() % 5) continue;
      int d = (int)(rnd() % 4);
      for (int k = 0; k < 4; k++, d = (d + 1) % 4) {
        int nx = cx + DX[d], ny = cy + DY[d];
        if (inside(nx, ny) && !cell_in_room(nx, ny) && !cell_link(cx, cy, d)) { link_set(cx, cy, d); break; }
      }
    }

  // Salle finale : entièrement ouverte, avec une seule entrée.
  for (int cy = ROOM_Y0; cy < CH; cy++)
    for (int cx = ROOM_X0; cx < CW; cx++) {
      if (cx + 1 < CW) link_set(cx, cy, D_E);
      if (cy + 1 < CH) link_set(cx, cy, D_S);
    }
  if (rnd() & 1) link_set(ROOM_X0, ROOM_Y0 + (int)(rnd() % 2), D_W);
  else link_set(ROOM_X0 + (int)(rnd() % 2), ROOM_Y0, D_N);

  // Conversion en tuiles.
  memset(g_map, T_WALL, sizeof g_map);
  for (int cy = 0; cy < CH; cy++)
    for (int cx = 0; cx < CW; cx++) {
      int tx = cx * 3 + 1, ty = cy * 3 + 1;
      floor_at(tx, ty, T_FLOOR); floor_at(tx + 1, ty, T_FLOOR);
      floor_at(tx, ty + 1, T_FLOOR); floor_at(tx + 1, ty + 1, T_FLOOR);
      bool e = cell_link(cx, cy, D_E), s = cell_link(cx, cy, D_S);
      if (e) { floor_at(tx + 2, ty, T_FLOOR); floor_at(tx + 2, ty + 1, T_FLOOR); }
      if (s) { floor_at(tx, ty + 2, T_FLOOR); floor_at(tx + 1, ty + 2, T_FLOOR); }
      if (e && s && cell_link(cx + 1, cy, D_S) && cell_link(cx, cy + 1, D_E)) floor_at(tx + 2, ty + 2, T_FLOOR);
    }
  {
    int tx = (CW - 1) * 3 + 1, ty = (CH - 1) * 3 + 1;
    floor_at(tx, ty, T_STAIRUP); floor_at(tx + 1, ty, T_STAIRUP);
    floor_at(tx, ty + 1, T_STAIRUP); floor_at(tx + 1, ty + 1, T_STAIRUP);
  }

  // Distances depuis le départ (BFS).
  memset(g_cdist, 0xFF, sizeof g_cdist);
  int head = 0, tail = 0;
  stack[tail++] = 0;
  g_cdist[0][0] = 0;
  g_maxdist = 0;
  g_pathlen = 0;
  while (head < tail) {
    int c = stack[head++], cx = c % CW, cy = c / CW, dd = g_cdist[cy][cx];
    if (dd > g_maxdist && !cell_in_room(cx, cy)) g_maxdist = dd;
    if (cell_in_room(cx, cy) && !g_pathlen) g_pathlen = dd;
    for (int d = 0; d < 4; d++) {
      int nx = cx + DX[d], ny = cy + DY[d];
      if (cell_link(cx, cy, d) && g_cdist[ny][nx] == 0xFFFF) {
        g_cdist[ny][nx] = (uint16_t)(dd + 1);
        stack[tail++] = (uint16_t)(ny * CW + nx);
      }
    }
  }
}
