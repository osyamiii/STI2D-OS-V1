// Génération du labyrinthe du Purgatoire.
// Le labyrinthe est une grille de CW x CH cellules ; chaque cellule fait 2x2
// tuiles de sol, séparées par des murs d'une tuile (couloirs de 32 px).
#ifndef MAZE_H
#define MAZE_H

#include <stdbool.h>
#include <stdint.h>

#define TS 16
#define CW 30
#define CH 30
#define MW (CW * 3 + 1)
#define MH (CH * 3 + 1)
#define MAP_MAX (MW * MH)

// Salle finale (coffre + escalier) : les 3x3 dernières cellules.
#define ROOM_X0 (CW - 3)
#define ROOM_Y0 (CH - 3)

enum {
  T_FLOOR, T_WALL, T_STAIRUP,          // Purgatoire
  T_STEP, T_SWALL,                     // escaliers
  T_CLOUD, T_SKY, T_PILLAR, T_GATE, T_BLOCK,  // Paradis
};

enum { D_E, D_S, D_W, D_N };

extern uint8_t g_map[MAP_MAX];
extern uint8_t g_cell[CH][CW];      // bit0 : ouvert à l'est, bit1 : ouvert au sud
extern uint16_t g_cdist[CH][CW];    // distance (en cellules) depuis le départ
extern int g_maxdist, g_pathlen;

void maze_build(uint32_t seed);
bool cell_link(int cx, int cy, int dir);
bool cell_in_room(int cx, int cy);
int cell_degree(int cx, int cy);
static inline int cell_px(int c) { return c * 48 + 32; }  // centre d'une cellule en pixels

#endif
