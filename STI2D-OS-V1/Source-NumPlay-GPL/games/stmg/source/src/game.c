// STI2D vs STMG — logique du jeu, états, IA et rendu des scènes.
#include "game.h"

#include <math.h>
#include <string.h>

#include "gfx.h"
#include "maze.h"
#include "platform.h"
#include "sprites.h"

// Graine fixe : tout le monde court dans le même labyrinthe, les chronos sont comparables.
#define MAZE_SEED 0x5712D2u

enum { ST_TITLE, ST_INTRO, ST_MAZE, ST_ENDCHEST, ST_STAIRS, ST_HEAVEN, ST_DYING, ST_GAMEOVER, ST_VICTORY, ST_CREDITS,
       ST_DIFFICULTY, ST_STATS };
enum { W_MAZE, W_STAIRS, W_HEAVEN };
enum { E_EE, E_SIN, E_ITEC, E_RHC };
enum { P_FIRE, P_ORB, P_BEAM, P_HEART };
enum { IT_HEAL, IT_SHIELD, IT_FIRE, IT_BOOTS, IT_TORCH };
enum { A_IDLE, A_BURST, A_AIM, A_SPIRAL, A_BEAMS, A_DASH, A_COUNT };

#define MAX_EN 200
#define MAX_PR 140
#define MAX_PA 96
#define MAX_CH 16
#define PHS 6            // demi-taille de la hitbox du joueur
#define BEAM_ACTIVE 14
#define GOD_HP 300
#define WHITE 0xFFFF
#define BLACK 0x0000
#define YELLOW RGB(255, 222, 64)
#define GOLD RGB(196, 132, 24)
#define RED RGB(225, 32, 44)

typedef struct {
  float x, y, kx, ky, fx, fy;
  int hp, maxhp, shields, fire;
  bool boots, torch, moving, left;
  int inv, atk, cd, anim;
} Player;

typedef struct {
  float x, y, kx, ky, wx, wy;
  int16_t hp, flash, cd, t;
  uint8_t type, alive, face;
} Enemy;

typedef struct { float x, y, vx, vy; int16_t life; uint8_t type, alive; } Proj;
typedef struct { float x, y, vx, vy; int16_t life; uint16_t col; } Part;
typedef struct { int16_t x, y; uint8_t item, open; } Chest;

typedef struct {
  float x, y, vx, vy;
  int hp, flash, t, atk, at, dur, last;
  bool alive, phase2;
} Boss;

static const struct {
  int16_t hp;
  uint8_t hs, dmg;
  float spd;
  const Sprite *spr;
} EST[4] = {
    {2, 5, 1, 1.30f, SPR_EE},     // EE : 0,5 cœur puis disparaît
    {6, 9, 2, 0.95f, SPR_SIN},    // SIN : 1 cœur, 3 boules de feu
    {20, 12, 3, 0.70f, SPR_ITEC}, // ITEC : 1,5 cœur, 10 boules de feu
    {1, 6, 1, 1.45f, SPR_RHC},    // RHC : fragiles
};

// Difficulté : dégâts subis (en demi-unités : 2 = x1), vitesse et nombre des monstres,
// chance de lâcher un cœur, vie de Dieu et multiplicateur de score.
enum { D_EASY, D_NORMAL, D_HARD, D_HARDCORE, D_COUNT };
static const struct {
  const char *name, *desc;
  uint16_t col;
  uint8_t hp, dmg, spd, dens, drop;  // spd, dens, drop en %
  uint8_t light;                     // rayon de lumière du Purgatoire (sans lampe)
  int16_t god_hp, score;             // score en %
} DIFF[D_COUNT] = {
    {"Facile", "4 coeurs, flèche-guide, plus de lumière", RGB(120, 230, 120), 8, 1, 85, 75, 150, 70, 220, 50},
    {"Normal", "L'aventure prévue : 3 coeurs", RGB(255, 222, 64), 6, 2, 100, 100, 100, 50, GOD_HP, 100},
    {"Difficile", "Dégâts x1,5, monstres rapides et nombreux", RGB(255, 150, 60), 6, 3, 115, 125, 60, 50, 380, 160},
    {"Hardcore", "Dégâts x2, aucun coeur lâché, Dieu enragé", RGB(255, 60, 60), 6, 4, 125, 145, 0, 50, 450, 250},
};

static const char *const ITEM_NAME[5] = {"Soin", "Bouclier", "Pouvoir", "Bottes", "Lampe torche"};

// ---------------------------------------------------------------- état global
static uint32_t rs = 1;
static int state, st_t;
static int world, mw, mh, cam_x, cam_y, shake, fade;
static uint64_t keys, prevk;
static bool paused, timer_on, maze_done, god_mode;
static uint32_t maze_ticks, best_ticks, rank_th[3];
static int rank, light_r;
static Player P;
static Enemy en[MAX_EN];
static int n_en;
static Proj pr[MAX_PR];
static Part pa[MAX_PA];
static Chest ch[MAX_CH], endchest;
static int n_ch;
static uint8_t pdist[CH][CW];
static int pcx = -1, pcy = -1;
static char msg1[48], msg2[64];
static int msg_t;
static char rw[5][40];
static int n_rw;
static Boss B;
static int fight, waves, wave_t;
static bool seated;
static int seat_t;
static int star_x[5], star_y[5];
static int diff = D_NORMAL, menu_sel, diff_sel;
static bool aim_assist = true;      // visée assistée : les tirs vont vers la cible la plus proche
static bool tgt_on;                 // cible actuelle de la visée assistée
static float tgt_x, tgt_y;
static uint8_t seen[CH][CW];        // cellules du labyrinthe déjà vues (carte de la pause)
static uint8_t xdist[CH][CW];       // distance (en cellules) jusqu'à la salle finale
// Statistiques de la partie et classement (gardé tant que l'application est ouverte).
#define N_BOARD 5
typedef struct { uint32_t score, ticks; uint16_t kills; uint8_t diff; } Entry;
static uint32_t run_ticks, score;
static int kills[4], n_kills, place, n_runs;
static Entry board[N_BOARD];
static int n_board;

#define KD(k) ((keys >> (k)) & 1)
#define KP(k) (((keys & ~prevk) >> (k)) & 1)

static uint32_t rnd(void) {
  rs ^= rs << 13;
  rs ^= rs >> 17;
  rs ^= rs << 5;
  return rs;
}
static float rf(void) { return (float)(rnd() >> 8) * (1.0f / 16777216.0f); }
static float clampf(float v, float a, float b) { return v < a ? a : v > b ? b : v; }
static int clampi(int v, int a, int b) { return v < a ? a : v > b ? b : v; }

static void fmt_time(uint32_t ticks, char *b) {
  uint32_t ms = ticks * 1000 / FPS, m = ms / 60000, s = (ms / 1000) % 60, d = (ms / 100) % 10;
  if (m > 99) m = 99;
  b[0] = (char)('0' + m / 10); b[1] = (char)('0' + m % 10); b[2] = ':';
  b[3] = (char)('0' + s / 10); b[4] = (char)('0' + s % 10); b[5] = '.';
  b[6] = (char)('0' + d); b[7] = 0;
}

static void fmt_int(int v, char *b) {
  char t[12];
  int n = 0;
  do t[n++] = (char)('0' + v % 10); while ((v /= 10) > 0);
  while (n) *b++ = t[--n];
  *b = 0;
}

static void say(const char *a, const char *b) {
  strncpy(msg1, a, sizeof msg1 - 1);
  strncpy(msg2, b ? b : "", sizeof msg2 - 1);
  msg_t = 90;
}

// ---------------------------------------------------------------- textures
enum { TX_FLOOR0, TX_FLOOR1, TX_FLOOR2, TX_WALL_TOP, TX_WALL_FACE, TX_STAIRUP, TX_CLOUD0, TX_CLOUD1,
       TX_SKY, TX_SKY_EDGE, TX_PILLAR, TX_GATE, TX_COUNT };
static uint16_t tex[TX_COUNT][256];

static uint32_t hash2(int x, int y, int s) {
  uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u + (uint32_t)s * 2246822519u;
  h = (h ^ (h >> 13)) * 1274126177u;
  return h ^ (h >> 16);
}

static void gen_textures(void) {
  for (int y = 0; y < 16; y++)
    for (int x = 0; x < 16; x++) {
      int i = y * 16 + x;
      uint32_t h = hash2(x, y, 7);
      // Sol de pierre du Purgatoire
      uint16_t f = (x == 0 || y == 0) ? RGB(34, 28, 40) : RGB(52, 44, 58);
      if (h % 9 == 0) f = RGB(66, 56, 72);
      else if (h % 13 == 0) f = RGB(38, 30, 44);
      tex[TX_FLOOR0][i] = f;
      tex[TX_FLOOR1][i] = (x == y || x == y + 1 || (x == 12 - y / 2 && y > 6)) ? RGB(28, 22, 32) : f;
      tex[TX_FLOOR2][i] = ((x - 7) * (x - 7) + (y - 9) * (y - 9) < 7 || (x == 11 && y == 4) || (x == 3 && y == 12))
                              ? RGB(110, 14, 24) : f;
      // Mur : façade de briques (quand du sol est dessous) ou dessus sombre
      int off = ((y / 4) & 1) * 4;
      bool mortar = (y % 4 == 3) || ((x + off) % 8 == 7);
      uint16_t brick = (h & 3) ? RGB(122, 44, 58) : RGB(104, 36, 50);
      tex[TX_WALL_FACE][i] = mortar ? RGB(46, 18, 28) : (y < 2 ? RGB(150, 62, 74) : brick);
      tex[TX_WALL_TOP][i] = (x == 0 || x == 15 || y == 0 || y == 15) ? RGB(44, 28, 40)
                            : (h % 7 == 0) ? RGB(36, 22, 34) : RGB(26, 16, 26);
      // Marches lumineuses vers le Paradis
      int sy = y % 8;
      tex[TX_STAIRUP][i] = sy == 0 ? RGB(255, 255, 240) : sy < 3 ? RGB(230, 226, 210) : RGB(170 + y * 4, 168 + y * 4, 176 + y * 3);
      // Nuages
      uint16_t c0 = RGB(232, 238, 255);
      bool puff = (x - 5) * (x - 5) + (y - 6) * (y - 6) < 10 || (x - 12) * (x - 12) + (y - 12) * (y - 12) < 6;
      tex[TX_CLOUD0][i] = puff ? RGB(250, 252, 255) : (h % 17 == 0 ? RGB(212, 222, 252) : c0);
      bool puff2 = (x - 10) * (x - 10) + (y - 4) * (y - 4) < 8 || (x - 3) * (x - 3) + (y - 12) * (y - 12) < 9;
      tex[TX_CLOUD1][i] = puff2 ? RGB(250, 252, 255) : (h % 19 == 0 ? RGB(212, 222, 252) : c0);
      // Ciel
      uint16_t sky = RGB(112, 166, 250);
      if ((y == 5 && x > 2 && x < 9) || (y == 12 && x > 8 && x < 15)) sky = RGB(150, 196, 255);
      tex[TX_SKY][i] = sky;
      int bump = (x % 8 - 4) * (x % 8 - 4) / 4;
      tex[TX_SKY_EDGE][i] = y < 4 - bump ? RGB(200, 214, 250) : (y < 6 - bump ? RGB(170, 194, 252) : sky);
      // Colonne dorée
      uint16_t gold = x < 5 ? RGB(255, 246, 180) : x > 10 ? RGB(196, 132, 24) : RGB(255, 214, 70);
      bool cap = y < 2 || y > 13;
      tex[TX_PILLAR][i] = (cap || (x >= 3 && x <= 12)) ? (cap ? RGB(255, 230, 120) : gold) : c0;
      tex[TX_GATE][i] = (x % 4 < 2 || y == 2 || y == 13) ? RGB(255, 214, 70) : c0;
    }
}

// ---------------------------------------------------------------- monde & collisions
static int tile_at(int tx, int ty) {
  if (tx < 0 || ty < 0 || tx >= mw || ty >= mh) return world == W_HEAVEN ? T_SKY : T_WALL;
  return g_map[ty * mw + tx];
}
static bool solid(int t) { return t == T_WALL || t == T_SWALL || t == T_SKY || t == T_PILLAR || t == T_GATE || t == T_BLOCK; }

static bool box_solid(float x, float y, int hs) {
  int x0 = (int)floorf(x - (float)hs) >> 4, x1 = (int)floorf(x + (float)hs - 0.01f) >> 4;
  int y0 = (int)floorf(y - (float)hs) >> 4, y1 = (int)floorf(y + (float)hs - 0.01f) >> 4;
  for (int ty = y0; ty <= y1; ty++)
    for (int tx = x0; tx <= x1; tx++)
      if (solid(tile_at(tx, ty))) return true;
  return false;
}

// Déplacement avec glissement contre les murs. `assist` aide le joueur à
// passer les coins (on le recentre doucement dans le couloir).
static void move_axis(float *x, float *y, float d, int hs, bool horiz, bool assist) {
  if (d == 0) return;
  for (float step = d; fabsf(step) > 0.2f; step *= 0.5f) {
    float nx = horiz ? *x + step : *x, ny = horiz ? *y : *y + step;
    if (!box_solid(nx, ny, hs)) { *x = nx; *y = ny; return; }
  }
  if (!assist) return;
  for (int o = 1; o <= 10; o++)
    for (int s = -1; s <= 1; s += 2) {
      float ox = horiz ? *x : *x + (float)(o * s), oy = horiz ? *y + (float)(o * s) : *y;
      float tx = horiz ? ox + d : ox, ty = horiz ? oy : oy + d;
      if (!box_solid(ox, oy, hs) && !box_solid(tx, ty, hs)) {
        if (horiz) *y += (float)s; else *x += (float)s;
        return;
      }
    }
}

static void move_box(float *x, float *y, float dx, float dy, int hs, bool assist) {
  move_axis(x, y, dx, hs, true, assist && dy == 0);
  move_axis(x, y, dy, hs, false, assist && dx == 0);
}

static int cell_of(float p) { return clampi(((int)p - 16) / 48, 0, CW - 1); }

// ---------------------------------------------------------------- particules & projectiles
static void burst(float x, float y, uint16_t col, int n, float spd) {
  for (int i = 0; i < MAX_PA && n > 0; i++) {
    if (pa[i].life > 0) continue;
    float a = rf() * 6.2832f, s = (0.3f + rf()) * spd;
    pa[i] = (Part){x, y, cosf(a) * s, sinf(a) * s, (int16_t)(10 + rnd() % 16), col};
    n--;
  }
}

static Proj *spawn_proj(int type, float x, float y, float vx, float vy, int life) {
  for (int i = 0; i < MAX_PR; i++)
    if (!pr[i].alive) {
      pr[i] = (Proj){x, y, vx, vy, (int16_t)life, (uint8_t)type, 1};
      return &pr[i];
    }
  return 0;
}

static Enemy *spawn_enemy(int type, float x, float y) {
  Enemy *e = 0;
  for (int i = 0; i < n_en; i++)
    if (!en[i].alive) { e = &en[i]; break; }
  if (!e) {
    if (n_en >= MAX_EN) return 0;
    e = &en[n_en++];
  }
  memset(e, 0, sizeof *e);
  e->type = (uint8_t)type;
  e->alive = 1;
  e->hp = EST[type].hp;
  e->x = e->wx = x;
  e->y = e->wy = y;
  e->t = (int16_t)(rnd() % 50);
  return e;
}

static void clear_entities(void) {
  memset(en, 0, sizeof en);
  memset(pr, 0, sizeof pr);
  memset(pa, 0, sizeof pa);
  n_en = 0;
}

// ---------------------------------------------------------------- joueur
static void player_reset(void) {
  memset(&P, 0, sizeof P);
  P.hp = P.maxhp = DIFF[diff].hp;
  P.fx = 1;
}

static void heal(int halves) { P.hp = P.hp + halves > P.maxhp ? P.maxhp : P.hp + halves; }

static void give_item(int it, bool announce) {
  const char *sub = "";
  if (it < IT_HEAL || it > IT_TORCH) return;
  switch (it) {
    case IT_HEAL: heal(2); sub = "+1 coeur"; break;
    case IT_SHIELD: P.shields++; P.maxhp += 4; P.hp += 4; sub = "+2 coeurs permanents"; break;
    case IT_FIRE:
      if (P.fire < 3) P.fire++;
      sub = P.fire == 1 ? "Boules de feu ! (OK pour tirer)" : P.fire == 2 ? "Boules de feu niv. 2 : tir rapide" : "Boules de feu niv. 3 : tir triple";
      break;
    case IT_BOOTS: if (P.boots) { heal(2); sub = "+1 coeur"; } else { P.boots = true; sub = "Vitesse de déplacement augmentée"; } break;
    case IT_TORCH: if (P.torch) { heal(2); sub = "+1 coeur"; } else { P.torch = true; sub = "Le cercle de lumière s'agrandit"; } break;
  }
  if (announce) say(ITEM_NAME[it], sub);
  burst(P.x, P.y - 10, YELLOW, 14, 1.8f);
}

static bool hurt_player(int halves, float sx, float sy) {
  if (P.inv || P.hp <= 0 || (state != ST_MAZE && state != ST_HEAVEN && state != ST_STAIRS)) return false;
  P.inv = 40;
  shake = 6;
  float dx = P.x - sx, dy = P.y - sy, l = fsqrt(dx * dx + dy * dy);
  if (l < 0.01f) { dx = 0; dy = 1; l = 1; }
  P.kx = dx / l * 4.0f;
  P.ky = dy / l * 4.0f;
  burst(P.x, P.y, RED, 12, 2.0f);
  if (god_mode) return true;
  halves = (halves * DIFF[diff].dmg + 1) / 2;
  P.hp -= halves;
  if (P.hp <= 0) {
    P.hp = 0;
    state = ST_DYING;
    st_t = 0;
  }
  return true;
}

static void kill_enemy(Enemy *e, bool drop) {
  static const uint16_t COL[4] = {RGB(112, 204, 64), RGB(130, 130, 140), RGB(140, 64, 178), RGB(255, 255, 255)};
  e->alive = 0;
  burst(e->x, e->y, COL[e->type], 8 + e->type * 4, 2.2f);
  static const uint8_t DROP[4] = {12, 30, 60, 11};  // chance (%) de lâcher un demi-cœur
  if (drop && (int)(rnd() % 100) < DROP[e->type] * DIFF[diff].drop / 100) spawn_proj(P_HEART, e->x, e->y, 0, 0, 330);
}

static void hurt_enemy(Enemy *e, int dmg, float kx, float ky) {
  static const float KB[4] = {1.0f, 0.6f, 0.25f, 1.0f};
  static const int16_t STUN[4] = {14, 14, 8, 0};
  e->hp = (int16_t)(e->hp - dmg);
  e->flash = 5;
  if (e->cd < STUN[e->type]) e->cd = STUN[e->type];  // un coup interrompt l'attaque
  e->kx = kx * KB[e->type];
  e->ky = ky * KB[e->type];
  if (e->hp <= 0) {
    kills[e->type]++;
    n_kills++;
    kill_enemy(e, true);
  }
}

// Fin de partie : score (monstres, Dieu, rapidité, difficulté) et place dans le classement.
static void finish_run(void) {
  static const uint16_t PTS[4] = {10, 30, 100, 15};
  uint32_t pts = 2000, sec = run_ticks / FPS;
  for (int t = 0; t < 4; t++) pts += (uint32_t)kills[t] * PTS[t];
  if (sec < 600) pts += (600 - sec) * 5;
  score = pts * (uint32_t)DIFF[diff].score / 100;
  n_runs++;
  place = 0;
  int i = n_board < N_BOARD ? n_board++ : N_BOARD;
  while (i > 0 && board[i - 1].score < score) {
    if (i < N_BOARD) board[i] = board[i - 1];
    i--;
  }
  if (i < N_BOARD) {
    board[i] = (Entry){score, run_ticks, (uint16_t)n_kills, (uint8_t)diff};
    place = i + 1;
  }
}

static void boss_hurt(int dmg) {
  if (fight != 2 || B.hp <= 0) return;
  B.hp -= dmg;
  B.flash = 3;
  if (B.hp <= 0) {
    B.hp = 0;
    finish_run();
    state = ST_VICTORY;
    st_t = 0;
    seated = false;
  }
}

// Ligne de vue dans le labyrinthe : un tir vers une cible cachée derrière un mur est inutile.
static bool clear_line(float x0, float y0, float x1, float y1) {
  float dx = x1 - x0, dy = y1 - y0;
  int n = (int)(fsqrt(dx * dx + dy * dy) / 6.0f);
  for (int k = 1; k < n; k++) {
    float t = (float)k / (float)n;
    if (solid(tile_at((int)(x0 + dx * t) >> 4, (int)(y0 + dy * t) >> 4))) return false;
  }
  return true;
}

// Visée assistée : ennemi visible le plus proche (de préférence devant), ou Dieu au Paradis.
static void update_target(void) {
  tgt_on = false;
  if (!aim_assist) return;
  // Dans le Purgatoire, seulement ce qui est éclairé : la visée ne révèle pas les monstres dans le noir.
  float range = !P.fire ? 46.0f : world == W_HEAVEN ? 400.0f : world == W_MAZE ? (float)light_r + 8 : 170.0f, best = 1e9f;
  for (int i = 0; i < n_en; i++) {
    Enemy *e = &en[i];
    if (!e->alive) continue;
    float dx = e->x - P.x, dy = e->y - P.y;
    if (fabsf(dx) > range || fabsf(dy) > range) continue;
    float d = fsqrt(dx * dx + dy * dy);
    if (d > range || d < 0.5f) continue;
    float score = d * (1.5f - 0.5f * (dx * P.fx + dy * P.fy) / d);
    if (score >= best || (world == W_MAZE && !clear_line(P.x, P.y, e->x, e->y))) continue;
    best = score;
    tgt_x = e->x;
    tgt_y = e->y;
    tgt_on = true;
  }
  static float bx0, by0;  // position de Dieu au tick précédent (pour anticiper)
  if (world == W_HEAVEN && fight == 2 && B.alive && B.hp > 0) {
    float dx = B.x - P.x, dy = B.y - P.y, d = fsqrt(dx * dx + dy * dy);
    if (d < range + 20 && d * 0.6f < best) {
      // Vise là où il sera quand la boule de feu arrivera (4,6 px par tick).
      float vx = clampf(B.x - bx0, -6, 6), vy = clampf(B.y - by0, -6, 6), t = P.fire ? d / 4.6f : 0;
      tgt_x = B.x + vx * t;
      tgt_y = B.y + vy * t;
      tgt_on = true;
    }
  }
  bx0 = B.x;
  by0 = B.y;
}

static void player_attack(void) {
  if (tgt_on) {  // vise la cible ; le diable se tourne vers elle
    float dx = tgt_x - P.x, dy = tgt_y - P.y, l = fsqrt(dx * dx + dy * dy) + 0.01f;
    P.fx = dx / l;
    P.fy = dy / l;
    if (fabsf(dx) > 2) P.left = dx < 0;
  }
  if (P.fire > 0) {
    float a = atan2f(P.fy, P.fx);
    int n = P.fire >= 3 ? 3 : 1;
    for (int k = 0; k < n; k++) {
      float aa = a + (n == 3 ? (float)(k - 1) * 0.22f : 0.0f);
      spawn_proj(P_FIRE, P.x + P.fx * 8, P.y + P.fy * 8, cosf(aa) * 4.6f, sinf(aa) * 4.6f, 42);
    }
    P.cd = P.fire >= 2 ? 8 : 13;
    return;
  }
  // Trident (attaque de base, courte portée)
  P.atk = 8;
  P.cd = 12;
  float hx = P.x + P.fx * 15, hy = P.y + P.fy * 15;
  for (int i = 0; i < n_en; i++) {
    Enemy *e = &en[i];
    int r = 11 + EST[e->type].hs;
    if (e->alive && fabsf(e->x - hx) < (float)r && fabsf(e->y - hy) < (float)r) hurt_enemy(e, 1, P.fx * 6, P.fy * 6);
  }
  if (world == W_HEAVEN && B.alive && fabsf(B.x - hx) < 29 && fabsf(B.y - hy) < 37) boss_hurt(1);
}

static void player_update(bool input) {
  if (P.inv) P.inv--;
  if (P.cd) P.cd--;
  if (P.atk) P.atk--;
  float ix = 0, iy = 0;
  if (input) {
    ix = (float)((int)KD(K_RIGHT) - (int)KD(K_LEFT));
    iy = (float)((int)KD(K_DOWN) - (int)KD(K_UP));
  }
  P.moving = ix != 0 || iy != 0;
  bool aim_lock = input && (KD(K_OK) || KD(K_EXE));  // OK maintenu : la visée reste bloquée
  if (P.moving) {
    if (ix != 0 && iy != 0) { ix *= 0.7071f; iy *= 0.7071f; }
    if (!aim_lock) {
      P.fx = ix;
      P.fy = iy;
      if (ix != 0) P.left = ix < 0;
    }
    P.anim++;
  }
  float spd = P.boots ? 2.6f : 1.8f;
  move_box(&P.x, &P.y, ix * spd, iy * spd, PHS, true);
  if (P.kx != 0 || P.ky != 0) {
    move_box(&P.x, &P.y, P.kx, P.ky, PHS, false);
    P.kx *= 0.72f;
    P.ky *= 0.72f;
    if (fabsf(P.kx) + fabsf(P.ky) < 0.15f) P.kx = P.ky = 0;
  }
  if (input) update_target();
  if (input && (KD(K_OK) || KD(K_EXE)) && P.cd == 0) player_attack();
}

// ---------------------------------------------------------------- construction des mondes
static void build_maze_world(void) {
  world = W_MAZE;
  mw = MW;
  mh = MH;
  maze_build(MAZE_SEED);
  clear_entities();

  // Coffres dans les culs-de-sac, répartis selon la progression.
  static const uint8_t ITEMS[] = {IT_TORCH, IT_HEAL, IT_FIRE, IT_HEAL, IT_BOOTS, IT_SHIELD, IT_HEAL, IT_HEAL, IT_FIRE, IT_SHIELD, IT_HEAL, IT_HEAL};
  static const uint8_t FRAC[] = {10, 16, 22, 34, 44, 52, 60, 66, 72, 80, 86, 94};
  static uint8_t used[CH][CW];
  memset(used, 0, sizeof used);
  n_ch = 0;
  for (unsigned k = 0; k < sizeof ITEMS && n_ch < MAX_CH; k++) {
    int target = FRAC[k] * g_maxdist / 100, best = 1 << 30, bx = -1, by = -1;
    for (int cy = 0; cy < CH; cy++)
      for (int cx = 0; cx < CW; cx++) {
        if (cell_in_room(cx, cy) || used[cy][cx] || g_cdist[cy][cx] < 3 || cell_degree(cx, cy) != 1) continue;
        int d = g_cdist[cy][cx] - target;
        if (d < 0) d = -d;
        if (d < best) { best = d; bx = cx; by = cy; }
      }
    if (bx < 0) break;
    used[by][bx] = 1;
    ch[n_ch++] = (Chest){(int16_t)cell_px(bx), (int16_t)cell_px(by), ITEMS[k], 0};
  }

  // Monstres : plus nombreux et plus gros en s'enfonçant dans le labyrinthe.
  uint32_t save = rs;
  rs = MAZE_SEED * 7u + 1u;
  for (int cy = 0; cy < CH; cy++)
    for (int cx = 0; cx < CW; cx++) {
      if (cell_in_room(cx, cy) || g_cdist[cy][cx] < 4) continue;
      float f = (float)g_cdist[cy][cx] / (float)g_maxdist;
      int p = (6 + (int)(14.0f * f)) * DIFF[diff].dens / 100;
      if ((int)(rnd() % 100) >= p) continue;
      int roll = (int)(rnd() % 100), type = E_EE;
      if (f > 0.45f && roll < 13) type = E_ITEC;
      else if (f > 0.26f && roll < 38) type = E_SIN;
      spawn_enemy(type, (float)cell_px(cx) + (rf() - 0.5f) * 10, (float)cell_px(cy) + (rf() - 0.5f) * 10);
    }
  rs = save ^ 0x9E3779B9u;

  // Distances jusqu'à la salle finale (flèche-guide du mode Facile)
  {
    static uint16_t q[CW * CH];
    static const int DX[4] = {1, 0, -1, 0}, DY[4] = {0, 1, 0, -1};
    int h = 0, t = 0;
    memset(xdist, 255, sizeof xdist);
    memset(seen, 0, sizeof seen);
    for (int cy = 0; cy < CH; cy++)
      for (int cx = 0; cx < CW; cx++)
        if (cell_in_room(cx, cy)) { xdist[cy][cx] = 0; q[t++] = (uint16_t)(cy * CW + cx); }
    while (h < t) {
      int c = q[h++], x = c % CW, y = c / CW;
      for (int k = 0; k < 4; k++) {
        int nx = x + DX[k], ny = y + DY[k];
        if (!cell_link(x, y, k) || xdist[ny][nx] != 255) continue;
        xdist[ny][nx] = (uint8_t)(xdist[y][x] + 1 > 254 ? 254 : xdist[y][x] + 1);
        q[t++] = (uint16_t)(ny * CW + nx);
      }
    }
  }

  endchest = (Chest){(int16_t)cell_px(CW - 2), (int16_t)cell_px(CH - 2), 0, 0};
  P.x = (float)cell_px(0);
  P.y = (float)cell_px(0);
  P.fx = 1; P.fy = 0;
  pcx = pcy = -1;

  // Seuils de rang calculés depuis la longueur du plus court chemin (arrondis à 5 s).
  uint32_t base = (uint32_t)g_pathlen * 48u * FPS / 54u;  // à 1,8 px/tick
  rank_th[0] = (base * 13 / 10 / 150 + 1) * 150;
  rank_th[1] = (base * 2 / 150 + 1) * 150;
  rank_th[2] = (base * 3 / 150 + 1) * 150;
}

static void build_stairs(void) {
  world = W_STAIRS;
  mw = 20;
  mh = 36;
  for (int ty = 0; ty < mh; ty++)
    for (int tx = 0; tx < mw; tx++) g_map[ty * mw + tx] = (tx >= 6 && tx <= 13 && ty < mh - 1) ? T_STEP : T_SWALL;
  clear_entities();
  P.x = 160;
  P.y = (float)((mh - 2) * 16 + 4);
  P.fx = 0; P.fy = -1;
  fade = 0;
  say("Un escalier monte vers la lumière...", "L'obscurité se dissipe à chaque marche.");
}

static void set_rect(int x0, int y0, int x1, int y1, uint8_t t) {
  for (int ty = y0; ty <= y1; ty++)
    for (int tx = x0; tx <= x1; tx++) g_map[ty * mw + tx] = t;
}

static void build_heaven(void) {
  world = W_HEAVEN;
  mw = 20;
  mh = 40;
  memset(g_map, T_SKY, (size_t)(mw * mh));
  set_rect(1, 1, 18, 13, T_CLOUD);   // arène du trône
  set_rect(6, 14, 13, 38, T_CLOUD);  // chemin de nuages
  set_rect(3, 19, 16, 22, T_CLOUD);
  set_rect(2, 28, 17, 31, T_CLOUD);
  set_rect(8, 0, 11, 3, T_BLOCK);    // trône
  static const uint8_t PIL[][2] = {{6, 16}, {13, 16}, {6, 25}, {13, 25}, {6, 34}, {13, 34}, {3, 3}, {16, 3}, {3, 11}, {16, 11}, {3, 20}, {16, 20}, {2, 29}, {17, 29}};
  for (unsigned i = 0; i < sizeof PIL / sizeof PIL[0]; i++) g_map[PIL[i][1] * mw + PIL[i][0]] = T_PILLAR;
  clear_entities();
  P.x = 160;
  P.y = 37 * 16 + 8;
  P.fx = 0; P.fy = -1;
  memset(&B, 0, sizeof B);
  B.x = 160;
  B.y = 34;
  B.hp = DIFF[diff].god_hp;
  B.alive = true;
  fight = 0;
  waves = 0;
  fade = 32;
  say("LE PARADIS", "Plus aucune ombre. STMG t'attend sur son trône.");
}

static void new_run(void) {
  player_reset();
  build_maze_world();
  maze_ticks = 0;
  run_ticks = 0;
  n_kills = 0;
  memset(kills, 0, sizeof kills);
  timer_on = false;
  maze_done = false;
  paused = false;
  fight = 0;
  seated = false;
  fade = 0;
  msg_t = 0;
  shake = 0;
}

// ---------------------------------------------------------------- IA du Purgatoire
static void update_pdist(void) {
  static uint16_t q[CW * CH];
  static const int DX[4] = {1, 0, -1, 0}, DY[4] = {0, 1, 0, -1};
  int cx = cell_of(P.x), cy = cell_of(P.y);
  if (cx == pcx && cy == pcy) return;
  pcx = cx;
  pcy = cy;
  memset(pdist, 255, sizeof pdist);
  if (cell_in_room(cx, cy)) return;  // la salle finale est un sanctuaire
  int h = 0, t = 0;
  q[t++] = (uint16_t)(cy * CW + cx);
  pdist[cy][cx] = 0;
  while (h < t) {
    int c = q[h++], x = c % CW, y = c / CW, d = pdist[y][x];
    if (d >= 8) continue;
    for (int k = 0; k < 4; k++) {
      int nx = x + DX[k], ny = y + DY[k];
      if (!cell_link(x, y, k) || cell_in_room(nx, ny) || pdist[ny][nx] != 255) continue;
      pdist[ny][nx] = (uint8_t)(d + 1);
      q[t++] = (uint16_t)(ny * CW + nx);
    }
  }
}

static void enemy_contact(Enemy *e) {
  int r = EST[e->type].hs + PHS;
  if (e->cd || fabsf(P.x - e->x) >= (float)r || fabsf(P.y - e->y) >= (float)r) return;
  if (!hurt_player(EST[e->type].dmg, e->x, e->y)) return;
  if (e->type == E_EE || e->type == E_RHC) kill_enemy(e, false);  // ils disparaissent après avoir frappé
  else e->cd = 45;
}

static void enemy_update_maze(Enemy *e) {
  float dx = P.x - e->x, dy = P.y - e->y;
  if (fabsf(dx) > 360 || fabsf(dy) > 280) return;  // endormi hors de portée
  int hs = EST[e->type].hs;
  e->t++;
  if (e->flash) e->flash--;
  if (e->cd) e->cd--;
  if (e->kx != 0 || e->ky != 0) {
    move_box(&e->x, &e->y, e->kx, e->ky, hs, false);
    e->kx *= 0.7f;
    e->ky *= 0.7f;
    if (fabsf(e->kx) + fabsf(e->ky) < 0.2f) e->kx = e->ky = 0;
    return;
  }
  int cx = cell_of(e->x), cy = cell_of(e->y), d = pdist[cy][cx];
  float spd = EST[e->type].spd * (float)DIFF[diff].spd * 0.01f, tx = e->x, ty = e->y;
  if (e->cd > 22) {  // recule après avoir mordu
    tx = e->x - dx;
    ty = e->y - dy;
    spd *= 0.6f;
  } else if (e->cd) {
    return;
  } else if (d <= 4) {
    if (d == 0) {
      tx = P.x;
      ty = P.y;
    } else {
      static const int DX[4] = {1, 0, -1, 0}, DY[4] = {0, 1, 0, -1};
      for (int k = 0; k < 4; k++) {
        int nx = cx + DX[k], ny = cy + DY[k];
        if (cell_link(cx, cy, k) && pdist[ny][nx] == d - 1) { tx = (float)cell_px(nx); ty = (float)cell_px(ny); break; }
      }
    }
  } else {
    if (e->t % 50 == 0) {
      e->wx = (float)cell_px(cx) + (rf() - 0.5f) * 20;
      e->wy = (float)cell_px(cy) + (rf() - 0.5f) * 20;
    }
    tx = e->wx;
    ty = e->wy;
    spd *= 0.35f;
  }
  float vx = tx - e->x, vy = ty - e->y, l = fsqrt(vx * vx + vy * vy);
  if (l > 0.5f) {
    if (l < spd) spd = l;
    float ox = e->x, oy = e->y;
    move_box(&e->x, &e->y, vx / l * spd, vy / l * spd, hs, false);
    if (fabsf(e->x - ox) + fabsf(e->y - oy) < 0.1f) {  // coincé : se recentre dans sa cellule
      float cxp = (float)cell_px(cx) - e->x, cyp = (float)cell_px(cy) - e->y;
      move_box(&e->x, &e->y, clampf(cxp, -spd, spd), clampf(cyp, -spd, spd), hs, false);
    }
    if (fabsf(vx) > 0.3f) e->face = vx < 0;
  }
  enemy_contact(e);
}

// ---------------------------------------------------------------- Paradis : anges & Dieu
static void spawn_wave(void) {
  waves++;
  int n = 6 + waves * 2;
  if (n > 14) n = 14;
  n = n * DIFF[diff].dens / 100;
  for (int k = 0; k < n; k++) {
    float x, y;
    switch (k % 3) {
      case 0: x = 16 + rf() * 288; y = -12 - (float)k * 4; break;
      case 1: x = -12; y = 30 + rf() * 140; break;
      default: x = 332; y = 30 + rf() * 140; break;
    }
    spawn_enemy(E_RHC, x, y);
  }
}

static void angel_update(Enemy *e) {
  e->t++;
  if (e->flash) e->flash--;
  float dx = P.x - e->x, dy = P.y - e->y, l = fsqrt(dx * dx + dy * dy) + 0.01f;
  float w = sinf((float)e->t * 0.15f) * 0.9f, sp = EST[E_RHC].spd * (float)DIFF[diff].spd * 0.01f;
  float tvx = dx / l * sp - dy / l * w, tvy = dy / l * sp + dx / l * w;
  e->kx += (tvx - e->kx) * 0.08f;
  e->ky += (tvy - e->ky) * 0.08f;
  e->x += e->kx;
  e->y += e->ky;
  e->face = e->kx < 0;
  enemy_contact(e);
}

static int count_angels(void) {
  int n = 0;
  for (int i = 0; i < n_en; i++) n += en[i].alive && en[i].type == E_RHC;
  return n;
}

static void orb(float a, float spd) { spawn_proj(P_ORB, B.x, B.y + 6, cosf(a) * spd, sinf(a) * spd, 260); }

static void boss_start_attack(int a) {
  static const int DUR[A_COUNT] = {40, 75, 70, 100, 60, 80};
  B.atk = a;
  B.at = 0;
  B.dur = DUR[a];
  if (a == A_IDLE && (B.phase2 || diff == D_HARDCORE)) B.dur = 24;
}

static void boss_update(void) {
  B.t++;
  if (B.flash) B.flash--;
  if (fight == 1) {  // Dieu se lève de son trône
    B.y += (78 - B.y) * 0.05f;
    if (B.t >= 75) {
      fight = 2;
      B.t = 0;
      boss_start_attack(A_IDLE);
    }
    return;
  }
  if (!B.phase2 && B.hp * 2 <= DIFF[diff].god_hp) {
    B.phase2 = true;
    shake = 12;
    say("STMG invoque les RHC !", "Les anges sont fragiles mais nombreux.");
    spawn_wave();
    wave_t = 0;
  }
  if (B.phase2) {
    wave_t++;
    if ((count_angels() <= 2 && wave_t > 210) || wave_t > 560) { spawn_wave(); wave_t = 0; }
  }

  // Déplacement
  if (B.atk == A_DASH && B.at >= 20 && B.at < 50) {
    B.x = clampf(B.x + B.vx, 30, 290);
    B.y = clampf(B.y + B.vy, 40, 200);
  } else {
    float tx = 160 + sinf((float)B.t * 0.021f) * 100, ty = 76 + sinf((float)B.t * 0.037f) * 16;
    if (B.atk == A_DASH && B.at < 20) tx = B.x + (rf() - 0.5f) * 6;
    B.x += (tx - B.x) * 0.06f;
    B.y += (ty - B.y) * 0.06f;
  }

  // Attaques
  float aim = atan2f(P.y - B.y, P.x - B.x);
  int at = B.at;
  switch (B.atk) {
    case A_BURST:
      if (at % 25 == 0 && at < 75) {
        int n = B.phase2 ? 16 : 12;
        float off = rf() * 6.2832f;
        for (int k = 0; k < n; k++) orb(off + (float)k * 6.2832f / (float)n, 1.7f);
      }
      break;
    case A_AIM:
      if (at % 12 == 0 && at < 60)
        for (int k = -1; k <= 1; k++) orb(aim + (float)k * 0.2f, 2.5f);
      break;
    case A_SPIRAL:
      if (at % 5 == 0) {
        int arms = B.phase2 ? 3 : 2;
        for (int k = 0; k < arms; k++) orb((float)at * 0.09f + (float)k * 6.2832f / (float)arms, 1.8f);
      }
      break;
    case A_BEAMS:
      if (at == 0) {
        int n = B.phase2 ? 6 : 4;
        spawn_proj(P_BEAM, P.x, 0, 0, 0, 36 + BEAM_ACTIVE);
        for (int k = 1; k < n; k++) spawn_proj(P_BEAM, 24 + rf() * 272, 0, 0, 0, 36 + BEAM_ACTIVE);
      }
      break;
    case A_DASH:
      if (at == 20) {
        float l = fsqrt((P.x - B.x) * (P.x - B.x) + (P.y - B.y) * (P.y - B.y)) + 0.01f;
        B.vx = (P.x - B.x) / l * 5.5f;
        B.vy = (P.y - B.y) / l * 5.5f;
      }
      break;
  }
  if (++B.at >= B.dur) {
    if (B.atk != A_IDLE) {
      boss_start_attack(A_IDLE);
    } else {
      int a;
      do a = 1 + (int)(rnd() % (A_COUNT - 1)); while (a == B.last);
      B.last = a;
      boss_start_attack(a);
    }
  }
  if (fabsf(P.x - B.x) < 18 + PHS && fabsf(P.y - B.y) < 26 + PHS) hurt_player(3, B.x, B.y);
}

// ---------------------------------------------------------------- mises à jour communes
static void update_projectiles(void) {
  for (int i = 0; i < MAX_PR; i++) {
    Proj *p = &pr[i];
    if (!p->alive) continue;
    if (--p->life <= 0) { p->alive = 0; continue; }
    switch (p->type) {
      case P_FIRE: {
        p->x += p->vx;
        p->y += p->vy;
        if ((p->life & 1) && i % 2 == 0) burst(p->x, p->y, RGB(255, 140, 24), 1, 0.3f);
        if (box_solid(p->x, p->y, 2)) { p->alive = 0; burst(p->x, p->y, RGB(255, 140, 24), 6, 1.5f); break; }
        for (int k = 0; k < n_en; k++) {
          Enemy *e = &en[k];
          int r = EST[e->type].hs + 3;
          if (e->alive && fabsf(e->x - p->x) < (float)r && fabsf(e->y - p->y) < (float)r) {
            hurt_enemy(e, 2, p->vx, p->vy);
            p->alive = 0;
            burst(p->x, p->y, RGB(255, 222, 64), 5, 1.5f);
            break;
          }
        }
        if (p->alive && world == W_HEAVEN && B.alive && fight == 2 && fabsf(B.x - p->x) < 21 && fabsf(B.y - p->y) < 29) {
          boss_hurt(2);
          p->alive = 0;
          burst(p->x, p->y, RGB(255, 222, 64), 5, 1.5f);
        }
        break;
      }
      case P_ORB:
        p->x += p->vx;
        p->y += p->vy;
        if (p->x < -10 || p->x > 330 || p->y < -10 || p->y > 250) { p->alive = 0; break; }
        if (fabsf(p->x - P.x) < PHS && fabsf(p->y - P.y) < PHS && hurt_player(1, p->x, p->y)) p->alive = 0;
        break;
      case P_BEAM:
        if (p->life <= BEAM_ACTIVE && fabsf(p->x - P.x) < 7 + PHS) hurt_player(2, p->x, P.y - 1);
        break;
      case P_HEART:
        if (fabsf(p->x - P.x) < 12 && fabsf(p->y - P.y) < 12 && P.hp < P.maxhp) {
          heal(1);
          p->alive = 0;
          burst(p->x, p->y, RED, 8, 1.5f);
        }
        break;
    }
  }
}

static void update_particles(void) {
  for (int i = 0; i < MAX_PA; i++) {
    Part *q = &pa[i];
    if (q->life <= 0) continue;
    q->life--;
    q->x += q->vx;
    q->y += q->vy;
    q->vx *= 0.9f;
    q->vy *= 0.9f;
  }
}

static void update_camera(void) {
  int maxx = mw * 16 - SW, maxy = mh * 16 - SH;
  cam_x = clampi((int)P.x - SW / 2, 0, maxx < 0 ? 0 : maxx);
  cam_y = clampi((int)P.y - SH / 2, 0, maxy < 0 ? 0 : maxy);
  if (world == W_HEAVEN && fight) cam_y = 0;
  if (shake > 0) {
    shake--;
    cam_x += (int)(rnd() % 5) - 2;
    cam_y += (int)(rnd() % 5) - 2;
  }
}

static void compute_rank(void) {
  rank = maze_ticks <= rank_th[0] ? 0 : maze_ticks <= rank_th[1] ? 1 : maze_ticks <= rank_th[2] ? 2 : 3;
  n_rw = 0;
  switch (rank) {
    case 0:  // S
      P.shields += 2; P.maxhp += 8; P.hp = P.maxhp; P.fire = 3;
      strcpy(rw[n_rw++], "2 Boucliers : +4 coeurs permanents");
      strcpy(rw[n_rw++], "Soin complet");
      strcpy(rw[n_rw++], "Boules de feu niv. 3 (tir triple)");
      break;
    case 1:  // A
      P.shields += 1; P.maxhp += 4; P.hp = P.maxhp; P.fire = P.fire + 1 < 2 ? 2 : (P.fire + 1 > 3 ? 3 : P.fire + 1);
      strcpy(rw[n_rw++], "1 Bouclier : +2 coeurs permanents");
      strcpy(rw[n_rw++], "Soin complet");
      strcpy(rw[n_rw++], P.fire == 3 ? "Boules de feu niv. 3 (tir triple)" : "Boules de feu niv. 2 (tir rapide)");
      break;
    case 2:  // B
      heal(4);
      if (P.fire < 1) P.fire = 1;
      strcpy(rw[n_rw++], "Soin : +2 coeurs");
      strcpy(rw[n_rw++], "Boules de feu (au moins niv. 1)");
      break;
    default:  // C
      heal(2);
      if (P.fire < 1) P.fire = 1;
      strcpy(rw[n_rw++], "Soin : +1 coeur");
      strcpy(rw[n_rw++], "Boules de feu (au moins niv. 1)");
      break;
  }
}

static bool handle_pause(void) {
  if (paused) {
    if (KP(K_OK) || KP(K_EXE)) paused = false;
    else if (KP(K_BACK)) { paused = false; state = ST_TITLE; st_t = 0; }
    return true;
  }
  if (KP(K_BACK)) { paused = true; return true; }
  return false;
}

static void update_maze(void) {
  if (handle_pause()) return;
  if (timer_on) maze_ticks++;
  run_ticks++;
  player_update(true);
  update_pdist();
  for (int i = 0; i < n_en && state == ST_MAZE; i++)
    if (en[i].alive) enemy_update_maze(&en[i]);
  update_projectiles();
  update_particles();

  for (int i = 0; i < n_ch; i++) {
    Chest *c = &ch[i];
    if (!c->open && fabsf(P.x - c->x) < 18 && fabsf(P.y - c->y) < 16) {
      c->open = 1;
      give_item(c->item, true);
    }
  }
  int cx = cell_of(P.x), cy = cell_of(P.y);
  if (!maze_done && cell_in_room(cx, cy) && P.x > (float)(ROOM_X0 * 48 + 16) && P.y > (float)(ROOM_Y0 * 48 + 16)) {
    maze_done = true;
    timer_on = false;
    if (!best_ticks || maze_ticks < best_ticks) best_ticks = maze_ticks;
    say("FIN DU LABYRINTHE !", "Chrono arrêté. Ouvre le grand coffre doré.");
  }
  if (maze_done && !endchest.open && fabsf(P.x - endchest.x) < 22 && fabsf(P.y - endchest.y) < 20) {
    endchest.open = 1;
    compute_rank();
    burst(endchest.x, endchest.y, YELLOW, 30, 3.0f);
    state = ST_ENDCHEST;
    st_t = 0;
    msg_t = 0;
  }
  if (tile_at((int)P.x >> 4, (int)P.y >> 4) == T_STAIRUP) {
    if (endchest.open) {
      build_stairs();
      state = ST_STAIRS;
      st_t = 0;
    } else if (msg_t < 30) {
      say("Ouvre d'abord le grand coffre !", "Son butin dépend de ton chrono.");
    }
  }
  float fl = (sinf((float)st_t * 0.3f) + sinf((float)st_t * 0.71f)) * 1.2f;
  light_r = (P.torch ? 86 : DIFF[diff].light) + (int)fl;
  for (int y = cy - 1; y <= cy + 1; y++)
    for (int x = cx - 1; x <= cx + 1; x++)
      if (x >= 0 && y >= 0 && x < CW && y < CH) seen[y][x] = 1;
}

static void update_stairs(void) {
  if (handle_pause()) return;
  run_ticks++;
  player_update(fade == 0);
  update_particles();
  float base = P.torch ? 86.0f : (float)DIFF[diff].light;
  float p = clampf(((float)((mh - 2) * 16) - P.y) / (float)((mh - 3) * 16), 0, 1);
  light_r = (int)(base + (480.0f - base) * p * p);
  if (st_t % 4 == 0) burst(96 + rf() * 128, (float)cam_y + rf() * 240, RGB(255, 250, 200), 1, 0.4f);
  if (P.y < 40 || fade) {
    fade += 2;
    if (fade >= 32) {
      build_heaven();
      state = ST_HEAVEN;
      st_t = 0;
    }
  }
}

static void update_heaven(void) {
  if (handle_pause()) return;
  run_ticks++;
  if (fade > 0) fade--;
  player_update(true);
  if (fight == 0 && P.y < 14 * 16 - 2) {
    fight = 1;
    B.t = 0;
    set_rect(6, 14, 13, 14, T_GATE);
    if (P.y > 13 * 16 - 7) P.y = 13 * 16 - 7;
    shake = 10;
    say("STMG : \"Un diable au Paradis ?\"", "\"Retourne en Enfer, STi2D !\"");
  }
  if (fight) boss_update();
  for (int i = 0; i < n_en && state == ST_HEAVEN; i++)
    if (en[i].alive) angel_update(&en[i]);
  update_projectiles();
  update_particles();
}

static void update_victory(void) {
  update_particles();
  if (st_t < 90) {
    if (st_t % 3 == 0) burst(B.x + (rf() - 0.5f) * 40, B.y + (rf() - 0.5f) * 50, st_t % 2 ? WHITE : YELLOW, 6, 3.0f);
    shake = 3;
    for (int i = 0; i < MAX_PR; i++) pr[i].alive = 0;
    for (int i = 0; i < n_en; i++)
      if (en[i].alive) kill_enemy(&en[i], false);
  } else if (st_t == 90) {
    B.alive = false;
    burst(B.x, B.y, WHITE, 40, 4.0f);
    say("STMG est vaincu !", "Le trône de Dieu est libre...");
  } else if (!seated && st_t > 110) {
    float tx = 160, ty = 44, dx = tx - P.x, dy = ty - P.y, l = fsqrt(dx * dx + dy * dy);
    P.moving = true;
    P.anim++;
    if (fabsf(dx) > 0.5f) P.left = dx < 0;
    if (l < 1.6f) {
      P.x = tx;
      P.y = ty;
      seated = true;
      seat_t = 0;
      burst(160, 30, YELLOW, 30, 3.0f);
    } else {
      P.x += dx / l * 1.5f;
      P.y += dy / l * 1.5f;
    }
  }
  if (seated) {
    seat_t++;
    if (seat_t % 5 == 0) burst(120 + rf() * 80, 10 + rf() * 60, rnd() & 1 ? YELLOW : WHITE, 1, 0.6f);
    if (seat_t > 60 && (KP(K_OK) || KP(K_EXE))) { state = ST_STATS; st_t = 0; }
  }
}

bool game_update(uint64_t k) {
  prevk = keys;
  keys = k;
  if (KP(K_HOME)) return false;
  st_t++;
  if (msg_t > 0 && !paused) msg_t--;
  switch (state) {
    case ST_TITLE:
      if (KP(K_UP)) menu_sel = (menu_sel + 2) % 3;
      if (KP(K_DOWN)) menu_sel = (menu_sel + 1) % 3;
      if (KP(K_OK) || KP(K_EXE)) {
        st_t = 0;
        if (menu_sel == 0) { new_run(); state = ST_INTRO; }
        else if (menu_sel == 1) { diff_sel = diff; state = ST_DIFFICULTY; }
        else state = ST_CREDITS;
      } else if (KP(K_BACK)) return false;
      break;
    case ST_DIFFICULTY:  // 4 niveaux puis l'option de visée assistée
      if (KP(K_UP)) diff_sel = (diff_sel + D_COUNT) % (D_COUNT + 1);
      if (KP(K_DOWN)) diff_sel = (diff_sel + 1) % (D_COUNT + 1);
      if (diff_sel == D_COUNT) {
        if (KP(K_OK) || KP(K_EXE) || KP(K_LEFT) || KP(K_RIGHT)) aim_assist = !aim_assist;
        if (KP(K_BACK)) { state = ST_TITLE; st_t = 0; }
        break;
      }
      if (KP(K_OK) || KP(K_EXE)) diff = diff_sel;
      if (KP(K_OK) || KP(K_EXE) || KP(K_BACK)) { state = ST_TITLE; st_t = 0; }
      break;
    case ST_STATS:
      if (st_t > 30 && (KP(K_OK) || KP(K_EXE))) { state = ST_CREDITS; st_t = 0; }
      break;
    case ST_INTRO:
      if (st_t > 10 && (KP(K_OK) || KP(K_EXE))) {
        state = ST_MAZE;
        st_t = 0;
        timer_on = true;
        say("LE PURGATOIRE", "Le chrono est lancé ! Trouve l'escalier.");
      }
      break;
    case ST_MAZE: update_maze(); break;
    case ST_ENDCHEST:
      update_particles();
      if (st_t > 25 && (KP(K_OK) || KP(K_EXE))) {
        state = ST_MAZE;
        say("L'escalier est ouvert", "Il t'attend au fond de la salle.");
      }
      break;
    case ST_STAIRS: update_stairs(); break;
    case ST_HEAVEN: update_heaven(); break;
    case ST_DYING:
      update_particles();
      if (st_t % 6 == 0) burst(P.x, P.y, RED, 4, 2.0f);
      if (st_t > 50) { state = ST_GAMEOVER; st_t = 0; }
      break;
    case ST_GAMEOVER:
      if (st_t > 20 && (KP(K_OK) || KP(K_EXE))) {
        new_run();
        state = ST_MAZE;
        st_t = 0;
        timer_on = true;
        say("Nouvelle tentative", "Labyrinthe et chrono réinitialisés.");
      } else if (st_t > 20 && KP(K_BACK)) {
        state = ST_TITLE;
        st_t = 0;
      }
      break;
    case ST_VICTORY: update_victory(); break;
    case ST_CREDITS:
      if (st_t > 60 && (KP(K_OK) || KP(K_EXE) || KP(K_BACK))) { state = ST_TITLE; st_t = 0; }
      break;
  }
  if (state != ST_TITLE && state != ST_INTRO && state != ST_GAMEOVER && state != ST_CREDITS && state != ST_DIFFICULTY &&
      state != ST_STATS)
    update_camera();
  return true;
}

void game_init(uint32_t seed) {
  rs = seed ? seed : 0x1234567u;
  gfx_init();
  gen_textures();
  for (int k = 0; k < 5; k++) {
    float a = -1.5708f + (float)k * 1.25664f;
    star_x[k] = (int)(cosf(a) * 1000.0f);
    star_y[k] = (int)(sinf(a) * 1000.0f);
  }
  player_reset();
  state = ST_TITLE;
}

// ---------------------------------------------------------------- rendu
static const uint16_t *tile_tex(int t, int tx, int ty) {
  uint32_t h = hash2(tx, ty, 3);
  switch (t) {
    case T_FLOOR: return tex[(h % 23 == 0) ? TX_FLOOR2 : (h % 5 == 0) ? TX_FLOOR1 : TX_FLOOR0];
    case T_WALL: return tex[tile_at(tx, ty + 1) == T_WALL ? TX_WALL_TOP : TX_WALL_FACE];
    case T_STAIRUP: return tex[TX_STAIRUP];
    case T_SKY: {
      int up = tile_at(tx, ty - 1);
      return tex[(up == T_CLOUD || up == T_PILLAR) ? TX_SKY_EDGE : TX_SKY];
    }
    case T_PILLAR: return tex[TX_PILLAR];
    case T_GATE: return tex[TX_GATE];
    default: return tex[(h & 1) ? TX_CLOUD1 : TX_CLOUD0];
  }
}

static void draw_stairs_tile(int t, int tx, int ty, int x, int y) {
  int p = 32 - ty * 32 / mh;
  if (t == T_STEP) {
    uint16_t c = blend565(RGB(58, 50, 64), RGB(255, 252, 236), p);
    gfx_fill(x, y, 16, 16, c);
    gfx_fill(x, y, 16, 3, blend565(c, WHITE, 12));
    gfx_fill(x, y + 15, 16, 1, half565(c));
  } else {
    uint16_t c = blend565(RGB(104, 36, 50), RGB(252, 236, 180), p);
    gfx_fill(x, y, 16, 16, c);
    int off = (ty & 1) * 8;
    gfx_fill(x, y + 7, 16, 1, half565(c));
    gfx_fill(x + ((off + tx * 0) % 16), y, 1, 7, half565(c));
    gfx_fill(x + ((off + 8) % 16), y + 8, 1, 8, half565(c));
  }
}

static void draw_tiles(void) {
  int y0 = cam_y + g_sy, y1 = y0 + STRIP_H - 1;
  for (int ty = y0 >> 4; ty <= y1 >> 4; ty++)
    for (int tx = cam_x >> 4; tx <= (cam_x + SW - 1) >> 4; tx++) {
      int t = tile_at(tx, ty), x = tx * 16 - cam_x, y = ty * 16 - cam_y;
      if (world == W_STAIRS) draw_stairs_tile(t, tx, ty, x, y);
      else gfx_blit16(tile_tex(t, tx, ty), x, y);
    }
}

static void draw_centered(const Sprite *s, float x, float y, int sc, int flags) {
  gfx_sprite(s, (int)x - cam_x - s->w * sc / 2, (int)y - cam_y - s->h * sc / 2, sc, flags);
}

static void draw_trident(void) {
  int cx = (int)P.x - cam_x, cy = (int)P.y - cam_y + 2;
  float fx = P.fx, fy = P.fy;
  int bx = cx + (int)(fx * 4), by = cy + (int)(fy * 4);
  int tx = cx + (int)(fx * 22), ty = cy + (int)(fy * 22);
  gfx_line(bx, by, tx, ty, 2, GOLD);
  int px = (int)(-fy * 5), py = (int)(fx * 5);
  gfx_line(tx - px, ty - py, tx + px, ty + py, 2, YELLOW);
  for (int k = -1; k <= 1; k++) {
    int sx = tx + px * k, sy = ty + py * k;
    gfx_line(sx, sy, sx + (int)(fx * 5), sy + (int)(fy * 5), 2, YELLOW);
  }
}

static void draw_player(void) {
  if (seated) {
    gfx_sprite(&SPR_PLAYER_SIT, 150 - cam_x, 22 - cam_y, 2, 0);
    gfx_sprite(&SPR_CROWN, 152 - cam_x, 12 - cam_y + ((seat_t / 15) & 1), 2, 0);
    return;
  }
  bool blink = P.inv && ((P.inv / 3) & 1);
  if (state == ST_DYING) blink = (st_t / 3) & 1;
  int frame = P.moving ? (P.anim / 6) & 1 : 0;
  int flags = (P.left ? SPR_FLIP : 0) | (blink ? SPR_WHITE : 0);
  gfx_sprite(&SPR_PLAYER[frame], (int)P.x - cam_x - 10, (int)P.y - cam_y - 14, 2, flags);
  if (P.atk) draw_trident();
}

static void draw_enemies(void) {
  for (int i = 0; i < n_en; i++) {
    Enemy *e = &en[i];
    if (!e->alive) continue;
    const Sprite *s = &EST[e->type].spr[(e->t / 8) & 1];
    int sx = (int)e->x - cam_x - s->w, sy = (int)e->y - cam_y - s->h;
    if (sy > g_sy + STRIP_H || sy + s->h * 2 < g_sy || sx > SW || sx + s->w * 2 < 0) continue;
    gfx_sprite(s, sx, sy, 2, (e->face ? SPR_FLIP : 0) | (e->flash ? SPR_WHITE : 0));
  }
}

static void draw_projectiles(void) {
  for (int i = 0; i < MAX_PR; i++) {
    Proj *p = &pr[i];
    if (!p->alive) continue;
    int x = (int)p->x - cam_x, y = (int)p->y - cam_y;
    switch (p->type) {
      case P_FIRE:
        gfx_disc(x, y, 4, RGB(255, 120, 20));
        gfx_disc(x, y, 2, RGB(255, 240, 160));
        break;
      case P_ORB:
        gfx_disc(x, y, 5, RGB(255, 210, 60));
        gfx_disc(x, y, 3, WHITE);
        break;
      case P_BEAM:
        if (p->life > BEAM_ACTIVE) {
          if ((p->life / 3) & 1) gfx_fill(x - 1, 16, 2, SH - 16, RGB(255, 200, 40));
        } else {
          gfx_fill(x - 8, 0, 16, SH, RGB(255, 214, 70));
          gfx_fill(x - 5, 0, 10, SH, WHITE);
        }
        break;
      case P_HEART:
        if (p->life > 60 || ((p->life / 4) & 1)) gfx_sprite(&SPR_ICON_HEAL, x - 8, y - 8 + ((p->life / 10) & 1), 2, 0);
        break;
    }
  }
}

static void draw_particles(void) {
  for (int i = 0; i < MAX_PA; i++)
    if (pa[i].life > 0) gfx_fill((int)pa[i].x - cam_x - 1, (int)pa[i].y - cam_y - 1, 2 + (pa[i].life > 14), 2 + (pa[i].life > 14), pa[i].col);
}

// Viseur de la visée assistée et flèche-guide du mode Facile.
static void draw_aids(void) {
  if (tgt_on && !paused) {
    int x = (int)tgt_x - cam_x, y = (int)tgt_y - cam_y, r = 13 + ((st_t / 4) & 1);
    uint16_t c = RGB(255, 222, 64);
    for (int sx = -1; sx <= 1; sx += 2)
      for (int sy = -1; sy <= 1; sy += 2) {
        gfx_fill(x + sx * r - (sx > 0 ? 4 : 0), y + sy * r - (sy > 0 ? 1 : 0), 5, 2, c);
        gfx_fill(x + sx * r - (sx > 0 ? 1 : 0), y + sy * r - (sy > 0 ? 4 : 0), 2, 5, c);
      }
  }
  if (diff != D_EASY || world != W_MAZE || state != ST_MAZE || paused) return;
  static const int DX[4] = {1, 0, -1, 0}, DY[4] = {0, 1, 0, -1};
  int cx = cell_of(P.x), cy = cell_of(P.y);
  float tx, ty;
  if (cell_in_room(cx, cy)) {  // dans la salle : vers le coffre, puis vers l'escalier
    tx = endchest.open ? (float)cell_px(CW - 1) : (float)endchest.x;
    ty = endchest.open ? (float)cell_px(CH - 1) : (float)endchest.y;
  } else {
    tx = (float)cell_px(cx);
    ty = (float)cell_px(cy);
    for (int k = 0; k < 4; k++) {
      int nx = cx + DX[k], ny = cy + DY[k];
      if (cell_link(cx, cy, k) && xdist[ny][nx] < xdist[cy][cx]) { tx = (float)cell_px(nx); ty = (float)cell_px(ny); break; }
    }
  }
  float dx = tx - P.x, dy = ty - P.y, l = fsqrt(dx * dx + dy * dy);
  if (l < 6) return;
  dx /= l;
  dy /= l;
  int bob = (st_t / 6) & 1, d = 22 + bob * 2;
  int ax = (int)P.x - cam_x + (int)(dx * (float)d), ay = (int)P.y - cam_y + (int)(dy * (float)d);
  int bx = ax - (int)(dx * 8), by = ay - (int)(dy * 8), px = (int)(-dy * 5), py = (int)(dx * 5);
  uint16_t c = RGB(255, 200, 60);
  gfx_line(ax, ay, bx + px, by + py, 2, c);
  gfx_line(ax, ay, bx - px, by - py, 2, c);
}

static void draw_box(int x, int y, int w, int h, uint16_t border) {
  if (!gfx_rows_visible(y, h)) return;
  gfx_blend_rect(x, y, w, h, BLACK, 24);
  gfx_rect(x, y, w, h, border);
}

static void draw_hud(void) {
  char buf[16];
  if (gfx_rows_visible(0, 44) && state != ST_VICTORY) {
    int n = P.maxhp / 2, step = n <= 10 ? 19 : 17;
    for (int i = 0; i < n; i++) {
      int v = P.hp - 2 * i, x = 4 + i * step;
      const Sprite *full = i < 3 ? &SPR_HEART : &SPR_HEART_BLUE;
      if (v >= 2) {
        gfx_sprite(full, x, 4, 2, 0);
      } else {
        gfx_sprite(&SPR_HEART_EMPTY, x, 4, 2, 0);
        if (v == 1) gfx_sprite_ex(full, x, 4, 2, 0, 5);
      }
    }
    fmt_time(maze_ticks, buf);
    int tw = gfx_text_w(buf, 2);
    gfx_fill(SW - tw - 8, 3, tw + 6, 19, BLACK);
    gfx_text(buf, SW - tw - 5, 6, 2, timer_on ? WHITE : YELLOW);
    int x = 4;
    if (P.fire) {
      gfx_sprite(&SPR_ICON_FIRE, x, 24, 2, 0);
      buf[0] = (char)('0' + P.fire); buf[1] = 0;
      gfx_text(buf, x + 16, 32, 1, YELLOW);
      x += 26;
    }
    if (P.boots) { gfx_sprite(&SPR_ICON_BOOTS, x, 24, 2, 0); x += 20; }
    if (P.torch && world == W_MAZE) gfx_sprite(&SPR_ICON_TORCH, x, 24, 2, 0);
  }
  if (world == W_HEAVEN && fight && B.alive && state != ST_VICTORY && gfx_rows_visible(224, 16)) {
    gfx_fill(0, 225, SW, 15, BLACK);
    gfx_text("STMG", 6, 229, 1, YELLOW);
    int w = 274 * B.hp / DIFF[diff].god_hp;
    gfx_fill(38, 228, 276, 9, RGB(60, 50, 20));
    gfx_fill(39, 229, w, 7, B.phase2 ? RGB(255, 120, 60) : RGB(255, 214, 70));
    gfx_fill(39, 229, w, 2, WHITE);
  }
  if (msg_t > 0 && !paused) {
    int y = world == W_HEAVEN && fight ? 182 : 194;
    draw_box(16, y, SW - 32, 30, msg_t > 80 ? WHITE : GOLD);
    gfx_text_c(msg1, SW / 2, y + 5, 1, YELLOW);
    gfx_text_c(msg2, SW / 2, y + 17, 1, WHITE);
  }
}

static void draw_endchest_panel(void) {
  static const char *const RN[4] = {"S", "A", "B", "C"};
  static const uint16_t RC[4] = {RGB(255, 222, 64), RGB(120, 230, 120), RGB(120, 180, 255), RGB(200, 200, 200)};
  char buf[16], line[48];
  draw_box(20, 24, 280, 192, YELLOW);
  gfx_text_c("LE GRAND COFFRE", SW / 2, 32, 2, YELLOW);
  fmt_time(maze_ticks, buf);
  strcpy(line, "Chrono : ");
  strcat(line, buf);
  gfx_text_c(line, SW / 2, 54, 1, WHITE);
  gfx_text("Rang", 112, 76, 2, WHITE);
  gfx_text(RN[rank], 172, 70, 4, RC[rank]);
  for (int i = 0; i < n_rw; i++) {
    gfx_fill(38, 110 + i * 16, 4, 4, YELLOW);
    gfx_text(rw[i], 48, 108 + i * 16, 1, i == 0 ? YELLOW : WHITE);
  }
  for (int i = 0; i < 3; i++) {
    fmt_time(rank_th[i], buf);
    buf[5] = 0;
    line[0] = RN[i][0];
    line[1] = ' ';
    line[2] = '<';
    line[3] = ' ';
    strcpy(line + 4, buf);
    gfx_text(line, 36 + i * 86, 172, 1, RC[i]);
  }
  if ((st_t / 10) & 1) gfx_text_c("OK : continuer", SW / 2, 196, 1, YELLOW);
}

static void draw_pause(void) {
  draw_box(60, 70, 200, 100, WHITE);
  gfx_text_c("PAUSE", SW / 2, 84, 3, WHITE);
  gfx_text_c("OK : reprendre", SW / 2, 124, 1, YELLOW);
  gfx_text_c("Back : retour au titre", SW / 2, 142, 1, RGB(200, 200, 200));
}

// Pause dans le Purgatoire : carte des couloirs déjà vus.
static void draw_maze_map(void) {
  char buf[16];
  gfx_clear(BLACK);
  gfx_text_c("PAUSE", SW / 2, 6, 2, WHITE);
  const int S = 6, X0 = SW / 2 - CW * S / 2, Y0 = 26;
  gfx_rect(X0 - 3, Y0 - 3, CW * S + 5, CH * S + 5, RGB(96, 24, 60));
  for (int cy = 0; cy < CH; cy++) {
    int y = Y0 + cy * S;
    if (!gfx_rows_visible(y, S)) continue;
    for (int cx = 0; cx < CW; cx++) {
      if (!seen[cy][cx]) continue;
      int x = X0 + cx * S;
      uint16_t c = cell_in_room(cx, cy) ? GOLD : RGB(120, 100, 130);
      gfx_fill(x, y, S - 1, S - 1, c);
      if (cell_link(cx, cy, D_E)) gfx_fill(x + S - 1, y, 1, S - 1, c);
      if (cell_link(cx, cy, D_S)) gfx_fill(x, y + S - 1, S - 1, 1, c);
    }
  }
  for (int i = 0; i < n_ch; i++) {
    int cx = cell_of(ch[i].x), cy = cell_of(ch[i].y);
    if (seen[cy][cx] && !ch[i].open) gfx_fill(X0 + cx * S + 1, Y0 + cy * S + 1, 3, 3, YELLOW);
  }
  if ((st_t / 8) & 1) gfx_fill(X0 + cell_of(P.x) * S, Y0 + cell_of(P.y) * S, S - 1, S - 1, RED);
  fmt_time(maze_ticks, buf);
  gfx_text(buf, 8, 6, 1, YELLOW);
  gfx_text_c("OK : reprendre   Back : retour au titre", SW / 2, 214, 1, RGB(200, 200, 200));
  gfx_text_c("Rouge : toi   Jaune : coffre   Or : salle finale", SW / 2, 226, 1, RGB(150, 130, 150));
}

static void draw_world(void) {
  if (paused && world == W_MAZE) {
    draw_maze_map();
    return;
  }
  draw_tiles();
  if (world == W_MAZE) {
    for (int i = 0; i < n_ch; i++) draw_centered(ch[i].open ? &SPR_CHEST_OPEN : &SPR_CHEST, ch[i].x, ch[i].y, 2, 0);
    draw_centered(endchest.open ? &SPR_GOLD_CHEST_OPEN : &SPR_GOLD_CHEST, endchest.x, endchest.y, 3, 0);
  }
  if (world == W_HEAVEN) {
    gfx_sprite(&SPR_THRONE, 139 - cam_x, 6 - cam_y, 3, 0);
    if (B.alive) {
      int fl = B.flash ? SPR_WHITE : 0;
      if (state == ST_VICTORY && ((st_t / 2) & 1)) fl = SPR_WHITE;
      if (B.atk == A_DASH && B.at < 20 && fight == 2 && ((B.at / 2) & 1)) fl = SPR_WHITE;
      gfx_ring((int)B.x - cam_x, (int)B.y - cam_y - 26, 14, 12, RGB(255, 240, 150));
      draw_centered(&SPR_GOD, B.x, B.y, 3, fl);
    }
  }
  draw_enemies();
  if (state != ST_GAMEOVER) draw_player();
  draw_projectiles();
  draw_particles();
  int lx = (int)P.x - cam_x, ly = (int)P.y - cam_y;
  if (world == W_MAZE) gfx_vignette(lx, ly, light_r);
  else if (world == W_STAIRS && light_r < 460) gfx_vignette(lx, ly, light_r);
  if (state == ST_MAZE || state == ST_HEAVEN) draw_aids();
  if (fade > 0) gfx_fade(WHITE, fade);
  draw_hud();
  if (state == ST_ENDCHEST) draw_endchest_panel();
  if (paused) draw_pause();
  if (state == ST_VICTORY && seated) {
    char buf[16], line[48];
    draw_box(10, 150, 300, 80, YELLOW);
    gfx_text_c("STi2D s'assoit sur le trône de Dieu !", SW / 2, 158, 1, YELLOW);
    gfx_text_c("Le Paradis appartient aux STI2D.", SW / 2, 174, 1, WHITE);
    fmt_time(maze_ticks, buf);
    strcpy(line, "Chrono du labyrinthe : ");
    strcat(line, buf);
    gfx_text_c(line, SW / 2, 192, 1, WHITE);
    if (seat_t > 60 && ((seat_t / 10) & 1)) gfx_text_c("OK : statistiques", SW / 2, 212, 1, YELLOW);
  }
}

static void draw_red_background(int top_r) {
  for (int y = g_sy; y < g_sy + STRIP_H; y++) {
    int t = y * 32 / SH;
    gfx_fill(0, y, SW, 1, blend565(RGB(top_r, 14, 44), RGB(96, 10, 40), t));
  }
  gfx_fill(0, 0, SW, 6, RGB(96, 24, 60));
  gfx_fill(0, SH - 6, SW, 6, RGB(96, 24, 60));
  gfx_fill(0, 0, 6, SH, RGB(96, 24, 60));
  gfx_fill(SW - 6, 0, 6, SH, RGB(96, 24, 60));
}

static void draw_pentagram(int cx, int cy, int r, uint16_t c) {
  gfx_ring(cx, cy, r, r - 5, c);
  gfx_ring(cx, cy, r - 9, r - 11, c);
  int rr = r - 10;
  for (int k = 0; k < 5; k++) {
    int a = k, b = (k + 2) % 5;
    gfx_line(cx + star_x[a] * rr / 1000, cy + star_y[a] * rr / 1000, cx + star_x[b] * rr / 1000,
             cy + star_y[b] * rr / 1000, 3, c);
  }
}

static void draw_title(void) {
  static const char *const ITEMS[3] = {"Jouer", "Difficulté", "Crédits"};
  char buf[16], line[48];
  draw_red_background(176);
  draw_pentagram(160, 64, 52, BLACK);
  gfx_text_sh("STI2D vs STMG", SW / 2, 122, 3, WHITE);
  for (int i = 0; i < 3; i++) {
    int y = 152 + i * 20;
    if (i == menu_sel) {
      int w = gfx_text_w(ITEMS[i], 2), x = SW / 2 - w / 2;
      gfx_blend_rect(x - 24, y - 3, w + 48, 20, BLACK, 16);
      if ((st_t / 8) & 1) {
        gfx_text(">", x - 18, y, 2, YELLOW);
        gfx_text("<", x + w + 6, y, 2, YELLOW);
      }
    }
    gfx_text_sh(ITEMS[i], SW / 2, y, 2, i == menu_sel ? YELLOW : RGB(255, 200, 200));
  }
  strcpy(line, "Difficulté : ");
  strcat(line, DIFF[diff].name);
  if (best_ticks) {
    fmt_time(best_ticks, buf);
    strcat(line, "   Record : ");
    strcat(line, buf);
  }
  gfx_text_c(line, SW / 2, 216, 1, DIFF[diff].col);
}

static void draw_difficulty(void) {
  draw_red_background(150);
  gfx_text_sh("Difficulté", SW / 2, 8, 3, WHITE);
  for (int i = 0; i < D_COUNT; i++) {
    int y = 34 + i * 39;
    bool sel = i == diff_sel;
    draw_box(24, y, SW - 48, 36, sel ? DIFF[i].col : RGB(96, 24, 60));
    if (sel) gfx_rect(25, y + 1, SW - 50, 34, DIFF[i].col);
    gfx_text(DIFF[i].name, 36, y + 5, 2, sel ? DIFF[i].col : RGB(200, 170, 170));
    if (i == diff) gfx_text("actuelle", SW - 36 - gfx_text_w("actuelle", 1), y + 9, 1, RGB(200, 200, 200));
    gfx_text(DIFF[i].desc, 36, y + 24, 1, sel ? WHITE : RGB(170, 140, 140));
  }
  bool sel = diff_sel == D_COUNT;
  draw_box(24, 192, SW - 48, 22, sel ? YELLOW : RGB(96, 24, 60));
  if (sel) gfx_rect(25, 193, SW - 50, 20, YELLOW);
  gfx_text("Visée assistée", 36, 199, 1, sel ? YELLOW : RGB(200, 170, 170));
  gfx_text(aim_assist ? "< OUI >" : "< NON >", SW - 36 - gfx_text_w("< OUI >", 1), 199, 1,
           aim_assist ? RGB(120, 230, 120) : RGB(255, 120, 120));
  gfx_text_c(sel ? "OK : activer/désactiver  Back : retour" : "Haut/Bas : choisir  OK : valider  Back : retour",
             SW / 2, 224, 1, RGB(255, 200, 200));
}

static void draw_stats(void) {
  static const Sprite *const ICON[4] = {&SPR_EE[0], &SPR_SIN[0], &SPR_ITEC[0], &SPR_RHC[0]};
  static const char *const RN[4] = {"S", "A", "B", "C"};
  char buf[16], line[48];
  gfx_clear(RGB(20, 12, 36));
  gfx_text_sh("VICTOIRE !", SW / 2, 8, 3, YELLOW);
  strcpy(line, "Difficulté : ");
  strcat(line, DIFF[diff].name);
  gfx_text_c(line, SW / 2, 34, 1, DIFF[diff].col);

  // Monstres éliminés, détail par type
  draw_box(12, 46, SW - 24, 56, GOLD);
  gfx_text("Monstres éliminés", 22, 54, 1, WHITE);
  fmt_int(n_kills, buf);
  gfx_text(buf, SW - 22 - gfx_text_w(buf, 2), 51, 2, YELLOW);
  for (int t = 0; t < 4; t++) {
    int x = 26 + t * 72;
    const Sprite *s = ICON[t];
    gfx_sprite(s, x + 14 - s->w, 82 - s->h, 2, 0);
    fmt_int(kills[t], buf);
    strcpy(line, "x");
    strcat(line, buf);
    gfx_text(line, x + 32, 79, 1, WHITE);
  }

  // Temps
  draw_box(12, 106, SW - 24, 36, GOLD);
  gfx_text("Temps total", 22, 113, 1, WHITE);
  fmt_time(run_ticks, buf);
  gfx_text(buf, SW - 22 - gfx_text_w(buf, 1), 113, 1, YELLOW);
  gfx_text("Chrono du labyrinthe", 22, 127, 1, WHITE);
  fmt_time(maze_ticks, buf);
  strcpy(line, buf);
  strcat(line, "  (rang ");
  strcat(line, RN[rank]);
  strcat(line, ")");
  gfx_text(line, SW - 22 - gfx_text_w(line, 1), 127, 1, YELLOW);

  // Classement
  draw_box(12, 146, SW - 24, 76, GOLD);
  fmt_int((int)score, buf);
  strcpy(line, "Score ");
  strcat(line, buf);
  gfx_text(line, 22, 152, 1, YELLOW);
  if (place) {
    fmt_int(place, buf);
    strcpy(line, "Classement : ");
    strcat(line, buf);
    strcat(line, place == 1 ? "er" : "e");
  } else {
    strcpy(line, "Hors du top 5");
  }
  gfx_text(line, SW - 22 - gfx_text_w(line, 1), 152, 1, place == 1 ? YELLOW : WHITE);
  for (int i = 0; i < n_board; i++) {
    int y = 166 + i * 11;
    uint16_t c = i + 1 == place ? YELLOW : RGB(200, 190, 210);
    if (i + 1 == place) gfx_blend_rect(16, y - 2, SW - 32, 11, YELLOW, 6);
    buf[0] = (char)('1' + i); buf[1] = '.'; buf[2] = 0;
    gfx_text(buf, 22, y, 1, c);
    fmt_int((int)board[i].score, buf);
    gfx_text(buf, 98 - gfx_text_w(buf, 1), y, 1, c);
    gfx_text(DIFF[board[i].diff].name, 112, y, 1, DIFF[board[i].diff].col);
    fmt_int(board[i].kills, buf);
    strcpy(line, buf);
    strcat(line, " monstres");
    gfx_text(line, 176, y, 1, c);
    fmt_time(board[i].ticks, buf);
    gfx_text(buf, SW - 22 - gfx_text_w(buf, 1), y, 1, c);
  }
  if (st_t > 30 && ((st_t / 12) & 1)) gfx_text_c("OK : crédits", SW / 2, 228, 1, RGB(255, 200, 200));
}

static void draw_intro(void) {
  gfx_clear(BLACK);
  gfx_text_sh("LE PURGATOIRE", SW / 2, 30, 3, RED);
  static const char *const L[] = {
      "Petit diable STi2D, tu t'es échappé de l'Enfer.",
      "Traverse le labyrinthe dans l'obscurité :",
      "seul un petit cercle de lumière t'entoure.",
      "",
      "Le chrono démarre maintenant. Plus tu es rapide,",
      "meilleur sera le butin du grand coffre final.",
      "",
      "Les coffres cachent soins, boucliers, bottes,",
      "lampe torche et boules de feu.",
      "Ensuite, monte au Paradis et renverse STMG.",
      "Astuce : maintiens OK pour garder ta visée.",
  };
  for (int i = 0; i < 11; i++) gfx_text_c(L[i], SW / 2, 66 + i * 12, 1, i < 3 ? WHITE : i == 10 ? YELLOW : RGB(220, 200, 200));
  gfx_sprite(&SPR_EE[0], 24, 206, 2, 0);
  gfx_sprite(&SPR_SIN[0], 48, 202, 2, 0);
  gfx_sprite(&SPR_ITEC[0], 80, 196, 2, 0);
  if ((st_t / 12) & 1) gfx_text_c("OK : entrer", 220, 214, 2, YELLOW);
}

static void draw_gameover(void) {
  draw_red_background(80);
  draw_pentagram(160, 120, 100, RGB(60, 4, 20));
  gfx_text_sh("GAME OVER", SW / 2, 60, 4, RED);
  gfx_text_c("Tes coeurs sont tombés à 0.", SW / 2, 120, 1, WHITE);
  gfx_text_c("Le labyrinthe et le chrono sont réinitialisés.", SW / 2, 136, 1, WHITE);
  char buf[16], line[64];
  fmt_int(n_kills, buf);
  strcpy(line, "Monstres éliminés : ");
  strcat(line, buf);
  fmt_time(run_ticks, buf);
  strcat(line, "   Temps : ");
  strcat(line, buf);
  gfx_text_c(line, SW / 2, 154, 1, YELLOW);
  if ((st_t / 12) & 1) gfx_text_c("OK : recommencer", SW / 2, 180, 2, YELLOW);
  gfx_text_c("Back : titre", SW / 2, 206, 1, RGB(220, 180, 180));
}

static void draw_credits(void) {
  draw_red_background(150);
  draw_pentagram(160, 120, 104, RGB(70, 6, 26));
  int off = st_t < 60 ? (60 - st_t) * 4 : 0;
  gfx_text_sh("STI2D vs STMG", SW / 2, 56 + off, 3, WHITE);
  gfx_text_sh("discord.gg/sti2d", SW / 2, 112 + off, 2, YELLOW);
  gfx_text_sh("discord : cjnw (yamii)", SW / 2, 144 + off, 2, YELLOW);
  if (st_t > 60 && ((st_t / 12) & 1)) gfx_text_c("OK : retour au titre", SW / 2, 214, 1, RGB(255, 200, 200));
}

void game_draw(void) {
  switch (state) {
    case ST_TITLE: draw_title(); break;
    case ST_INTRO: draw_intro(); break;
    case ST_GAMEOVER: draw_gameover(); break;
    case ST_CREDITS: draw_credits(); break;
    case ST_DIFFICULTY: draw_difficulty(); break;
    case ST_STATS: draw_stats(); break;
    default: draw_world(); break;
  }
}

// ---------------------------------------------------------------- sauvegarde (version web)
#ifdef PLATFORM_WEB
typedef struct {
  uint32_t magic, best;
  int32_t diff, n_board, n_runs, aim;
  Entry board[N_BOARD];
} Save;
#define SAVE_MAGIC 0x53543232u

int game_save(uint8_t *buf, int cap) {
  Save sv = {SAVE_MAGIC, best_ticks, diff, n_board, n_runs, aim_assist, {{0}}};
  memcpy(sv.board, board, sizeof board);
  if (cap < (int)sizeof sv) return 0;
  memcpy(buf, &sv, sizeof sv);
  return (int)sizeof sv;
}

void game_load(const uint8_t *buf, int n) {
  Save sv;
  if (n != (int)sizeof sv) return;
  memcpy(&sv, buf, sizeof sv);
  if (sv.magic != SAVE_MAGIC) return;
  best_ticks = sv.best;
  diff = clampi(sv.diff, 0, D_COUNT - 1);
  n_board = clampi(sv.n_board, 0, N_BOARD);
  n_runs = sv.n_runs;
  aim_assist = sv.aim != 0;
  memcpy(board, sv.board, sizeof board);
  for (int i = 0; i < n_board; i++)
    if (board[i].diff >= D_COUNT) board[i].diff = D_NORMAL;
}
#endif

// ---------------------------------------------------------------- outils de test
#ifdef GAME_DEBUG
int game_debug(int cmd, int a, int b) {
  switch (cmd) {
    case DBG_STATE:
      if (a == ST_MAZE) { new_run(); timer_on = true; }
      if (a == ST_STAIRS) build_stairs();
      if (a == ST_HEAVEN) build_heaven();
      state = a;
      st_t = 0;
      update_camera();
      return 0;
    case DBG_TELEPORT: P.x = (float)a; P.y = (float)b; pcx = -1; update_camera(); return 0;
    case DBG_GIVE: give_item(a, true); return 0;
    case DBG_BOSS_HP: B.hp = a; return 0;
    case DBG_TICKS: maze_ticks = (uint32_t)a; return 0;
    case DBG_GOD_MODE: god_mode = a; return 0;
    case DBG_ROOM: P.x = (float)cell_px(CW - 2); P.y = (float)cell_px(CH - 3); pcx = -1; update_camera(); return 0;
    case DBG_INFO:
      switch (a) {
        case 0: return g_pathlen;
        case 1: return g_maxdist;
        case 2: { int n = 0; for (int i = 0; i < n_en; i++) n += en[i].alive; return n; }
        case 3: return n_ch;
        case 4: return P.hp;
        case 5: return state;
        case 6: return B.hp;
        case 7: return fight;
        case 8: return (int)rank_th[0];
        case 9: return (int)P.x;
        case 10: return (int)P.y;
        case 11: return rank;
        case 12: return count_angels();
        case 16: return tgt_on;
        case 13: case 14: case 15: {  // vecteur vers l'ennemi vivant le plus proche
          float bd = 1e9f, bx = 0, by = 0;
          int bt = -1, bi = 0;
          for (int i = 0; i < n_en; i++)
            if (en[i].alive) {
              float dx = en[i].x - P.x, dy = en[i].y - P.y, d = dx * dx + dy * dy;
              if (d < bd) { bd = d; bx = dx; by = dy; bt = en[i].type; bi = i; }
            }
          (void)bi;
          return a == 15 ? bt : (int)(a == 13 ? bx : by);
        }
      }
  }
  return 0;
}
#endif
