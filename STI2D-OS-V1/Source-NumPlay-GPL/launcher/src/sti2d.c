/* STI2D-Play : l'accueil en carrousel (Matrices IA, Jeux, Favoris, Mes apps,
 * Parametres, Credits), les listes avec recherche et apercu, les reglages
 * (theme et mode clair / sombre) et les credits. Tout est dessine avec le
 * moteur de NumPlay (gfx.h : l'ecran par bandes de 16 lignes). */
#include <eadk.h>
#include <string.h>
#include "ui.h"
#include "sti2d.h"
#include "../../games/common/np_text.h"

extern const np_icon_t sti_icon_star, sti_icon_people, sti_icon_gamepad, sti_icon_matrices;

/* ------------------------------------------------------------------ themes */

typedef struct {
  const char *name;
  uint32_t acc, top, bottom;
} theme_t;
enum { TH_STI2D, TH_MANGA, TH_MINECRAFT, TH_HACKER, TH_ZELDA, TH_CITY, N_THEMES };
static const theme_t themes[N_THEMES] = {
  {"STI2D", 0xFF9800, 0xFFBE28, 0xF57C00},
  {"Manga", 0xFF5C93, 0xFFAAC8, 0xE91E63},
  {"Minecraft", 0x6EB43C, 0x7DBE46, 0x79553A},
  {"Hacker", 0x00E65A, 0x00963C, 0x00280F},
  {"Zelda", 0xE8C547, 0x50AA5A, 0x1E5E3A},
  {"City", 0x5AC8FA, 0xAA3CBE, 0x3949AB},
};

static struct {
  uint8_t theme, light, card;
  uint8_t fav[NP_MAX_GAMES];
} cfg = {TH_STI2D, 0, 1, {0}};

/* couleurs du moment (apply_theme) */
static color_t c_bg, c_panel, c_line, c_text, c_dim, c_acc, c_acc2;
static uint32_t bg888;
#define C_DARK RGB(20, 16, 10)
#define C_GREEN RGB(76, 200, 110)
#define C_RED RGB(240, 80, 70)
#define C_YELLOW RGB(255, 205, 60)

static void apply_theme(void) {
  const theme_t *t = &themes[cfg.theme];
  c_acc = gfx_rgb(t->acc);
  c_acc2 = gfx_rgb(gfx_lerp888(t->acc, 0, 90));
  if (cfg.light) {
    bg888 = 0xF0F1F5;
    c_panel = RGB(255, 255, 255), c_line = RGB(212, 215, 224), c_text = RGB(25, 28, 35), c_dim = RGB(105, 110, 125);
  } else if (cfg.theme == TH_HACKER) {
    bg888 = 0x000000;
    c_panel = RGB(0, 22, 8), c_line = RGB(0, 60, 25), c_text = RGB(170, 255, 190), c_dim = RGB(60, 160, 90);
  } else {
    bg888 = 0x12141A;
    c_panel = RGB(32, 35, 44), c_line = RGB(48, 52, 64), c_text = RGB(240, 240, 245), c_dim = RGB(140, 145, 160);
  }
  c_bg = gfx_rgb(bg888);
}

static uint32_t hash(int x, int y) {
  uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u;
  h = (h ^ (h >> 13)) * 1274126177u;
  return h ^ (h >> 16);
}

/* le motif du theme, sous la barre de titre (6 lignes) */
static color_t pattern(int x, int y) {
  switch (cfg.theme) {
    case TH_MANGA:
      return ((x + y) / 2) % 4 == 0 ? RGB(255, 240, 246) : c_acc2;
    case TH_MINECRAFT: {
      uint32_t h = hash(x / 2, y / 2) & 3;
      if (y < 2) return h ? RGB(95, 160, 50) : RGB(120, 190, 70);
      return h == 0 ? RGB(150, 108, 74) : h == 1 ? RGB(100, 70, 45) : RGB(121, 85, 58);
    }
    case TH_HACKER:
      return hash(x / 2, y / 2) & 1 ? c_acc : 0;
    case TH_ZELDA: {
      int lx = x % 16 - 8;
      return (lx < 0 ? -lx : lx) <= y ? c_acc : RGB(30, 94, 58);
    }
    case TH_CITY: {
      int hgt = 1 + (int)(hash(x / 8, 7) % 6);
      if (6 - y > hgt) return gfx_rgb(themes[TH_CITY].top);
      return (x % 4 == 1 && y % 2 == 1 && (hash(x, y) & 3) == 0) ? RGB(255, 220, 90) : RGB(25, 22, 50);
    }
    default:
      return y < 2 ? c_acc : c_acc2;
  }
}

static void strip_rows(int y0) {
  for (int y = NP_MAX(y0, gfx_y0); y < NP_MIN(y0 + 6, gfx_y1); y++)
    for (int x = 0; x < SCREEN_W; x++) gfx_buf[(y - gfx_y0) * SCREEN_W + x] = pattern(x, y - y0);
}

#define HEAD_H 28
#define FOOT_Y 222

static void header(const char *title, const char *right, bool panel) {
  if (gfx_y0 >= HEAD_H) return;
  gfx_fill(0, 0, SCREEN_W, HEAD_H - 6, panel ? c_panel : c_acc);
  strip_rows(HEAD_H - 6);
  color_t fg = panel ? c_acc : C_DARK;
  gfx_text(&np_font_body, 10, 16, title, fg, 256);
  if (right) gfx_text_right(&np_font_small, SCREEN_W - 10, 15, right, panel ? c_dim : C_DARK, 230);
}

static void footer(const char *hint) {
  if (gfx_y1 <= FOOT_Y) return;
  gfx_fill(0, FOOT_Y, SCREEN_W, SCREEN_H - FOOT_Y, c_panel);
  gfx_fill(0, FOOT_Y, SCREEN_W, 1, c_line);
  gfx_text_center(&np_font_small, SCREEN_W / 2, FOOT_Y + 13, hint, c_dim, 256);
}

/* une carte au degrade du theme ; dim : 0..256 vers le fond */
static void card(int x, int y, int w, int h, int r, int dim) {
  const theme_t *t = &themes[cfg.theme];
  gfx_vgrad(x, y, w, h, gfx_lerp888(t->top, bg888, dim), gfx_lerp888(t->bottom, bg888, dim));
}

/* ------------------------------------------------------------------ texte */

static char *put(char *o, const char *s) {
  while (*s) *o++ = *s++;
  *o = 0;
  return o;
}

static char *put_uint(char *o, uint32_t v) {
  char d[12];
  int n = 0;
  do d[n++] = (char)('0' + v % 10), v /= 10;
  while (v);
  while (n) *o++ = d[--n];
  *o = 0;
  return o;
}

static char *put_size(char *o, uint32_t bytes) {
  uint32_t kb = (bytes + 1023) / 1024;
  if (kb < 1000) return put(put_uint(o, kb), T(" KB"));
  uint32_t t = (kb * 10 + 512) / 1024;
  o = put_uint(o, t / 10);
  *o++ = ',';
  *o++ = (char)('0' + t % 10);
  return put(o, " Mo");
}

/* ------------------------------------------------------------------ clavier */

typedef struct {
  uint64_t held;
  uint32_t next;
} keys_t;
#define KB(k) ((uint64_t)1 << (k))
#define ARROWS (KB(eadk_key_left) | KB(eadk_key_right) | KB(eadk_key_up) | KB(eadk_key_down))

static uint64_t poll(keys_t *k) {
  uint64_t s = eadk_keyboard_scan();
  uint32_t now = np_millis();
  uint64_t pressed = s & ~k->held;
  if (pressed) k->next = now + 380;
  else if ((s & ARROWS) && (int32_t)(now - k->next) >= 0) {
    pressed = s & ARROWS;
    k->next = now + 110;
  }
  k->held = s;
  return pressed;
}
static bool is_ok(uint64_t p) { return p & (KB(eadk_key_ok) | KB(eadk_key_exe)); }
static bool is_home(uint64_t p) { return p & (KB(eadk_key_home) | KB(eadk_key_on_off)); }

static char key_letter(uint64_t p) {
  static const uint8_t keys[26] = {
    eadk_key_exp, eadk_key_ln, eadk_key_log, eadk_key_imaginary, eadk_key_comma, eadk_key_power,
    eadk_key_sine, eadk_key_cosine, eadk_key_tangent, eadk_key_pi, eadk_key_sqrt, eadk_key_square,
    eadk_key_seven, eadk_key_eight, eadk_key_nine, eadk_key_left_parenthesis, eadk_key_right_parenthesis,
    eadk_key_four, eadk_key_five, eadk_key_six, eadk_key_multiplication, eadk_key_division,
    eadk_key_one, eadk_key_two, eadk_key_three, eadk_key_plus};
  for (int i = 0; i < 26; i++)
    if (p & KB(keys[i])) return (char)('a' + i);
  return 0;
}

/* ------------------------------------------------------------------ sauvegarde */
/* Un script Python : seuls ceux-ci survivent quand on installe des apps. */
#define SAVE_NAME "sti2dplay.py"

static void save(void) {
  static char buf[1024];
  char *o = buf;
  *o++ = 0; /* octet d'import automatique de Python : desactive */
  o = put(o, "# STI2D-Play : sauvegarde (ne pas modifier)\nT=");
  o = put_uint(o, cfg.theme);
  o = put(o, "\nM=");
  o = put_uint(o, cfg.light);
  o = put(o, "\nC=");
  o = put_uint(o, cfg.card);
  o = put(o, "\nF=\"");
  bool first = true;
  for (int i = 0; i < np_game_count && o < buf + sizeof buf - 40; i++) {
    if (!cfg.fav[i]) continue;
    if (!first) *o++ = ',';
    o = put(o, np_games[i].id);
    first = false;
  }
  o = put(o, "\"\n");
  o++; /* le zero final */
  np_file_write(SAVE_NAME, buf, (uint32_t)(o - buf));
}

static int value_after(const char *s, uint32_t n, const char *key) {
  int kl = (int)strlen(key);
  for (uint32_t i = 0; i + kl < n; i++)
    if ((i == 0 || s[i - 1] == '\n') && !strncmp(s + i, key, (size_t)kl)) return s[i + kl] - '0';
  return -1;
}

void sti_load(void) {
  uint32_t n = 0;
  const char *s = (const char *)np_file_read(SAVE_NAME, &n);
  if (s && n) {
    int v;
    if ((v = value_after(s, n, "T=")) >= 0 && v < N_THEMES) cfg.theme = (uint8_t)v;
    if ((v = value_after(s, n, "M=")) == 0 || v == 1) cfg.light = (uint8_t)v;
    if ((v = value_after(s, n, "C=")) >= 0 && v <= 4) cfg.card = (uint8_t)v;
    for (uint32_t i = 0; i + 3 < n; i++) {
      if (s[i] != 'F' || s[i + 1] != '=' || s[i + 2] != '"' || (i && s[i - 1] != '\n')) continue;
      const char *p = s + i + 3, *end = s + n;
      while (p < end && *p != '"') {
        const char *q = p;
        while (q < end && *q != ',' && *q != '"') q++;
        for (int g = 0; g < np_game_count; g++)
          if (strlen(np_games[g].id) == (size_t)(q - p) && !strncmp(np_games[g].id, p, (size_t)(q - p))) cfg.fav[g] = 1;
        p = q < end && *q == ',' ? q + 1 : q;
      }
    }
  }
  apply_theme();
}

/* ------------------------------------------------------------------ jeux */

static int matrices_game(void) {
  for (int i = 0; i < np_game_count; i++)
    if (!strcmp(np_games[i].id, "matrices")) return i;
  return -1;
}
static bool listed(int i) { return np_game_installed(i) && strcmp(np_games[i].id, "matrices"); }

static sti_app_t apps[STI_MAX_APPS];
static int apps_n;
void (*sti_external)(void);

/* ------------------------------------------------------------------ accueil */

enum { C_MAT, C_GAMES, C_FAV, C_SET, C_CRED, N_CARDS };

typedef struct {
  int sel;              /* sans modulo : le carrousel tourne en rond */
  float pos;
  int demo, demo_n;     /* la carte Jeux fait defiler les jeux */
  uint32_t demo_time;
  float intro;
} home_t;

static int count_games(bool favs) {
  int n = 0;
  for (int i = 0; i < np_game_count; i++)
    if (listed(i) && (!favs || cfg.fav[i])) n++;
  return n;
}

static int demo_game(int k) {
  for (int i = 0; i < np_game_count; i++)
    if (listed(i) && k-- == 0) return i;
  return -1;
}

static const char *card_title(int c) {
  static const char *const t[N_CARDS] = {T("Matrices IA"), T("Jeux NumWorks"), T("Favoris"), T("Parametres"), T("Credits")};
  return t[c];
}

static const char *card_sub(int c) {
  static char line[48];
  char *o = line;
  switch (c) {
    case C_MAT: return ".gg/STI2D IA";
    case C_GAMES: put(put_uint(o, (uint32_t)count_games(false)), T(" games inside")); break;
    case C_FAV: put(put_uint(o, (uint32_t)count_games(true)), T(" favorites")); break;
    case C_SET: return T("Theme, light or dark mode");
    default: return T("The STI2D-Play team");
  }
  return line;
}

static float absf(float v) { return v < 0 ? -v : v; }
static int wrap(int k) { return ((k % N_CARDS) + N_CARDS) % N_CARDS; }

static void draw_home_card(home_t *h, int k, float d) {
  int c = wrap(k);
  float ad = absf(d);
  float s = 1 - 0.22f * NP_MIN(ad, 1.2f);
  int w = (int)(140 * s), hh = (int)(104 * s);
  int x = (int)(160 + d * 152 - w / 2), y = 34 + (104 - hh) / 2 + (int)(6 * NP_MIN(ad, 1.f));
  if (x >= SCREEN_W || x + w <= 0) return;
  int dim = (int)(150 * NP_MIN(ad, 1.f)), r = 14;
  if (ad < 0.5f) gfx_shadow(x, y + 4, w, hh, r, 10, cfg.light ? 60 : 140);
  int a = 256 - dim * 3 / 4;
  if (c == C_GAMES && h->demo >= 0) {
    const np_shot_t *sh = &np_games[h->demo].shots[0];
    const uint8_t *px = ui_shot(h->demo, 0);
    if (px) gfx_image(px, sh->palette, sh->ncolors, SHOT_W, SHOT_H, x, y, w, hh, 0, dim);
    else card(x, y, w, hh, r, dim);
  } else {
    card(x, y, w, hh, r, dim);
    const np_icon_t *icon = c == C_MAT ? &sti_icon_matrices : c == C_FAV ? &sti_icon_star
                            : c == C_SET ? &np_icon_gear : &sti_icon_people;
    gfx_icon(icon, x + (w - icon->w) / 2, y + (hh - icon->h) / 2, 0xFFFF, a * 235 / 256);
  }
  gfx_corners(x, y, w, hh, r, c_bg);
}

static void home_scene(void *ctx) {
  home_t *h = ctx;
  gfx_fill(0, gfx_y0, SCREEN_W, gfx_y1 - gfx_y0, c_bg);
  char right[32];
  put(put_uint(right, (uint32_t)count_games(false)), T(" games"));
  header("STI2D-Play", right, true);
  int lo = (int)h->pos - 2, hi = (int)h->pos + 2;
  for (int pass = 2; pass >= 0; pass--)
    for (int k = lo; k <= hi; k++) {
      float d = k - h->pos;
      int ring = absf(d) < 0.5f ? 0 : absf(d) < 1.5f ? 1 : 2;
      if (ring == pass) draw_home_card(h, k, d);
    }
  int front = wrap((int)(h->pos + (h->pos < 0 ? -0.5f : 0.5f)));
  float away = absf(h->pos - (float)(int)(h->pos + (h->pos < 0 ? -0.5f : 0.5f)));
  int ta = (int)(256 * NP_CLAMP(1 - away * 2.4f, 0.f, 1.f));
  if (gfx_y1 > 150 && gfx_y0 < FOOT_Y) {
    gfx_text_center(&np_font_title, 160, 172, card_title(front), front == C_MAT ? c_acc : c_text, ta);
    gfx_text_center(&np_font_body, 160, 192, card_sub(front), c_dim, ta);
    int dx = 160 - N_CARDS * 7;
    for (int i = 0; i < N_CARDS; i++) {
      if (i == wrap(h->sel)) gfx_rrect(dx + i * 14 - 3, 203, 12, 5, 2, c_acc, 256);
      else gfx_rrect(dx + i * 14, 203, 6, 5, 2, c_line, 256);
    }
  }
  footer(T("< > choose    OK open    Back quit"));
}

/* ------------------------------------------------------------------ listes */

typedef struct {
  bool apps;            /* les apps installees, sinon les jeux */
  bool favs;
  int item[NP_MAX_GAMES > STI_MAX_APPS ? NP_MAX_GAMES : STI_MAX_APPS], n;
  int sel, top;
  char query[16];
  int qn;
  float scroll;
} list_t;

#define ROW_H 21
#define LIST_W 172

static bool match(const char *name, const char *q, int qn) {
  if (!qn) return true;
  char flat[64];
  int n = 0;
  for (; *name && n < 63; name++) {
    char c = *name;
    if (c >= 'A' && c <= 'Z') c = (char)(c + 32);
    if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) flat[n++] = c;
  }
  for (int i = 0; i + qn <= n; i++)
    if (!strncmp(flat + i, q, (size_t)qn)) return true;
  return false;
}

static const char *item_name(list_t *l, int k) {
  return l->apps ? apps[l->item[k]].name : np_games[l->item[k]].title;
}

static void build_list(list_t *l) {
  l->n = 0;
  if (l->apps) {
    for (int i = 0; i < apps_n; i++)
      if (match(apps[i].name, l->query, l->qn)) l->item[l->n++] = i;
  } else {
    for (int i = 0; i < np_game_count; i++)
      if (listed(i) && (!l->favs || cfg.fav[i]) && match(np_games[i].title, l->query, l->qn)) l->item[l->n++] = i;
  }
  if (l->sel >= l->n) l->sel = l->n ? l->n - 1 : 0;
}

static int list_y0(list_t *l) { return HEAD_H + (l->qn ? 26 : 4); }
static int list_rows(list_t *l) { return (FOOT_Y - 2 - list_y0(l)) / ROW_H; }

static void preview(list_t *l) {
  int x = LIST_W + 6, w = SCREEN_W - x - 8, y = HEAD_H + 8, ih = w * 3 / 4;
  if (!l->n) return;
  int k = l->item[l->sel];
  char line[48];
  if (l->apps) {
    card(x, y, w, ih, 10, 0);
    gfx_icon(&sti_icon_gamepad, x + (w - sti_icon_gamepad.w) / 2, y + (ih - sti_icon_gamepad.h) / 2, 0xFFFF, 235);
    gfx_corners(x, y, w, ih, 10, c_bg);
    put_size(line, apps[k].size);
    gfx_text_center(&np_font_body, x + w / 2, y + ih + 18, line, c_dim, 256);
    gfx_paragraph(&np_font_small, x + w / 2, y + ih + 36, w, 13, T("Installed next to STI2D-Play. OK to launch it."), c_dim, 230);
    return;
  }
  const np_game_t *g = &np_games[k];
  const uint8_t *px = ui_shot(k, 0);
  if (px) gfx_image(px, g->shots[0].palette, g->shots[0].ncolors, SHOT_W, SHOT_H, x, y, w, ih, 10, 0);
  else card(x, y, w, ih, 10, 0);
  gfx_corners(x, y, w, ih, 10, c_bg);
  int b = gfx_paragraph(&np_font_small, x + w / 2, y + ih + 16, w, 13, g->tagline, c_text, 256);
  uint32_t missing = np_game_ram_missing(k);
  if (missing) {
    put(put_size(put(line, T("Needs ")), missing), T(" more RAM"));
    gfx_text_center(&np_font_small, x + w / 2, b + 2, line, C_RED, 256);
  } else {
    put_size(line, np_game_size(k));
    gfx_text_center(&np_font_small, x + w / 2, b + 2, line, c_dim, 256);
  }
  if (g->credit && b + 16 < FOOT_Y) gfx_paragraph(&np_font_small, x + w / 2, b + 16, w, 12, g->credit, c_dim, 170);
}

static void list_scene(void *ctx) {
  list_t *l = ctx;
  gfx_fill(0, gfx_y0, SCREEN_W, gfx_y1 - gfx_y0, c_bg);
  char right[16];
  put_uint(right, (uint32_t)l->n);
  header(l->apps ? T("Mes apps") : l->favs ? T("Favoris") : T("Jeux"), right, false);
  if (l->qn) {
    gfx_rrect(6, HEAD_H + 3, LIST_W - 6, 20, 8, c_panel, 256);
    int x = gfx_text(&np_font_small, 14, HEAD_H + 17, T("Search "), c_dim, 256);
    char q[20];
    put(put(q, l->query), "_");
    gfx_text(&np_font_body, x + 2, HEAD_H + 17, q, c_acc, 256);
  }
  int y0 = list_y0(l), rows = list_rows(l);
  for (int s = 0; s < rows; s++) {
    int k = l->top + s;
    if (k >= l->n) break;
    int y = y0 + s * ROW_H;
    if (y >= gfx_y1 || y + ROW_H <= gfx_y0) continue;
    bool sel = k == l->sel;
    if (sel) gfx_rrect(4, y + 1, LIST_W - 6, ROW_H - 2, 7, c_acc, 256);
    color_t fg = sel ? C_DARK : c_text;
    int idx = l->item[k];
    bool fav = !l->apps && cfg.fav[idx];
    bool ram = !l->apps && np_game_ram_missing(idx);
    if (fav) gfx_icon(&np_icon_heart, 10, y + 6, sel ? C_DARK : C_YELLOW, 256);
    else gfx_icon(&np_icon_play, 10, y + 5, sel ? C_DARK : ram ? C_RED : C_GREEN, 256);
    gfx_text(&np_font_body, 26, y + 15, item_name(l, k), fg, 256);
  }
  if (l->n > rows) {
    int h = rows * ROW_H, bh = NP_MAX(h * rows / l->n, 12), by = y0 + (h - bh) * l->top / (l->n - rows);
    gfx_rrect(LIST_W, y0, 3, h, 1, c_panel, 256);
    gfx_rrect(LIST_W, by, 3, bh, 1, c_dim, 256);
  }
  if (!l->n) {
    const char *msg = l->favs && !l->qn ? T("No favorites yet") : l->apps && !l->qn ? T("No other app installed") : T("Nothing found");
    gfx_text_center(&np_font_body, LIST_W / 2, 110, msg, c_dim, 256);
    if (l->favs && !l->qn) gfx_paragraph(&np_font_small, LIST_W / 2, 130, LIST_W - 16, 13, T("In Games, press Shift on a game to add it."), c_dim, 220);
  }
  preview(l);
  footer(l->apps ? T("OK launch   Letters search   Back") : T("OK play   Shift favorite   Letters search   Back"));
}

/* rend un jeu, -1 pour revenir, ou -3 pour une app a lancer (sti_external) */
static int run_list(bool apps_list, bool favs) {
  static list_t l;
  memset(&l, 0, sizeof l);
  l.apps = apps_list;
  l.favs = favs;
  build_list(&l);
  keys_t keys = {eadk_keyboard_scan(), 0};
  bool dirty = true;
  for (;;) {
    uint64_t p = poll(&keys);
    int rows = list_rows(&l);
    char c;
    if (is_home(p)) return STI_QUIT;
    if (p & KB(eadk_key_back)) {
      if (!l.qn) return -1;
      l.qn = 0, l.query[0] = 0, l.sel = 0;
      build_list(&l);
      dirty = true;
    }
    if (l.n) {
      if (p & KB(eadk_key_up)) l.sel = (l.sel + l.n - 1) % l.n, dirty = true;
      if (p & KB(eadk_key_down)) l.sel = (l.sel + 1) % l.n, dirty = true;
      if (p & KB(eadk_key_left)) l.sel = NP_MAX(l.sel - rows, 0), dirty = true;
      if (p & KB(eadk_key_right)) l.sel = NP_MIN(l.sel + rows, l.n - 1), dirty = true;
      int idx = l.item[l.sel];
      if (is_ok(p)) {
        if (l.apps) {
          sti_external = apps[idx].entry;
          return STI_EXTERNAL;
        }
        if (!np_game_ram_missing(idx)) return idx;
      }
      if (!l.apps && (p & KB(eadk_key_shift))) {
        cfg.fav[idx] ^= 1;
        save();
        if (l.favs) build_list(&l);
        dirty = true;
      }
    }
    if ((p & KB(eadk_key_backspace)) && l.qn) {
      l.query[--l.qn] = 0;
      build_list(&l);
      dirty = true;
    } else if ((c = key_letter(p)) && l.qn < 12) {
      l.query[l.qn++] = c;
      l.query[l.qn] = 0;
      l.sel = 0;
      build_list(&l);
      dirty = true;
    }
    rows = list_rows(&l);
    if (l.sel < l.top) l.top = l.sel;
    if (l.sel >= l.top + rows) l.top = l.sel - rows + 1;
    if (l.top > l.n - rows) l.top = NP_MAX(l.n - rows, 0);
    if (dirty) {
      ui_frame(list_scene, &l);
      dirty = false;
    } else {
      np_sleep(12);
    }
  }
}

/* ------------------------------------------------------------------ parametres */

typedef struct {
  int row;
} settings_t;

static void settings_scene(void *ctx) {
  settings_t *s = ctx;
  gfx_fill(0, gfx_y0, SCREEN_W, gfx_y1 - gfx_y0, c_bg);
  header(T("Parametres"), 0, false);
  const char *labels[3] = {T("Mode"), T("Theme"), T("Manage games")};
  const char *values[3] = {cfg.light ? T("Light") : T("Dark"), themes[cfg.theme].name, T("Reset, uninstall")};
  for (int r = 0; r < 3; r++) {
    int y = HEAD_H + 8 + r * 32;
    bool sel = r == s->row;
    gfx_rrect(10, y, SCREEN_W - 20, 26, 9, sel ? c_acc : c_panel, 256);
    color_t fg = sel ? C_DARK : c_text;
    gfx_text(&np_font_body, 22, y + 17, labels[r], fg, 256);
    if (r < 2) {
      gfx_icon(&np_icon_left, SCREEN_W - 40 - gfx_text_width(&np_font_body, values[r]) - 12, y + 6, fg, 200);
      gfx_text_right(&np_font_body, SCREEN_W - 40, y + 17, values[r], fg, 256);
      gfx_icon(&np_icon_right, SCREEN_W - 32, y + 6, fg, 200);
    } else {
      gfx_text_right(&np_font_small, SCREEN_W - 22, y + 16, values[r], sel ? C_DARK : c_dim, 230);
    }
  }
  /* apercu du theme */
  int py = HEAD_H + 108;
  card(20, py, 112, 82, 12, 0);
  gfx_icon(&sti_icon_gamepad, 20 + (112 - sti_icon_gamepad.w) / 2, py + (82 - sti_icon_gamepad.h) / 2, 0xFFFF, 235);
  gfx_corners(20, py, 112, 82, 12, c_bg);
  gfx_rrect(146, py + 4, 154, 22, 8, c_acc, 256);
  gfx_text(&np_font_body, 156, py + 19, T("Jeux"), C_DARK, 256);
  gfx_rrect(146, py + 30, 154, 22, 8, c_panel, 256);
  gfx_text(&np_font_body, 156, py + 45, T("Favoris"), c_text, 256);
  gfx_rrect(146, py + 56, 154, 22, 8, c_panel, 256);
  gfx_text(&np_font_body, 156, py + 71, T("Credits"), c_dim, 256);
  footer(T("Up/Down choose   < > change   Back"));
}

static int run_settings(void) {
  settings_t s = {0};
  keys_t keys = {eadk_keyboard_scan(), 0};
  bool dirty = true;
  for (;;) {
    uint64_t p = poll(&keys);
    if (is_home(p)) return STI_QUIT;
    if (p & KB(eadk_key_back)) return -1;
    if (p & KB(eadk_key_up)) s.row = (s.row + 2) % 3, dirty = true;
    if (p & KB(eadk_key_down)) s.row = (s.row + 1) % 3, dirty = true;
    int d = (p & KB(eadk_key_left)) ? -1 : ((p & KB(eadk_key_right)) || is_ok(p)) ? 1 : 0;
    if (d && s.row == 0) cfg.light ^= 1;
    if (d && s.row == 1) cfg.theme = (uint8_t)((cfg.theme + d + N_THEMES) % N_THEMES);
    if (d && s.row < 2) {
      apply_theme();
      save();
      dirty = true;
    }
    if (s.row == 2 && is_ok(p)) {
      np_settings(); /* effacer la progression, desinstaller des jeux (NumPlay) */
      np_progress_backup();
      ui_init();
      keys.held = eadk_keyboard_scan();
      dirty = true;
    }
    if (dirty) {
      ui_frame(settings_scene, &s);
      dirty = false;
    } else {
      np_sleep(12);
    }
  }
}

/* ------------------------------------------------------------------ credits */

static void credits_scene(void *ctx) {
  (void)ctx;
  gfx_fill(0, gfx_y0, SCREEN_W, gfx_y1 - gfx_y0, c_bg);
  header(T("Credits"), 0, false);
  card(110, HEAD_H + 8, 100, 72, 12, 0);
  gfx_icon(&sti_icon_people, 160 - sti_icon_people.w / 2, HEAD_H + 8 + (72 - sti_icon_people.h) / 2, 0xFFFF, 240);
  gfx_corners(110, HEAD_H + 8, 100, 72, 12, c_bg);
  gfx_text_center(&np_font_title, 160, 132, "STI2D-Play", c_acc, 256);
  gfx_text_center(&np_font_body, 160, 152, "Yamii  -  Molly_612  -  Marwan  -  dj51z", c_text, 256);
  gfx_text_center(&np_font_body, 160, 170, "discord.gg/sti2d", c_acc, 256);
  gfx_text_center(&np_font_small, 160, 188, T("Games: NumPlay by Mason Chen (GPLv3), with permission"), c_dim, 256);
  gfx_text_center(&np_font_small, 160, 202, T("AI: Matrices  -  .gg/STI2D IA"), c_dim, 256);
  footer(T("Saved in the Python script sti2dplay.py"));
}

static int run_credits(void) {
  keys_t keys = {eadk_keyboard_scan(), 0};
  ui_frame(credits_scene, 0);
  for (;;) {
    uint64_t p = poll(&keys);
    if (is_home(p)) return STI_QUIT;
    if ((p & KB(eadk_key_back)) || is_ok(p)) return -1;
    np_sleep(16);
  }
}

/* ------------------------------------------------------------------ accueil */

int sti_home(bool returning) {
  ui_init();
  apply_theme();
  static home_t h;
  if (!returning || h.demo_n == 0) {
    memset(&h, 0, sizeof h);
    h.sel = cfg.card < N_CARDS ? cfg.card : C_GAMES;
    h.pos = h.sel + (returning ? 0 : 0.6f);
  }
  h.demo_n = count_games(false);
  h.demo = demo_game(h.demo_n ? (int)(np_millis() / 2600) % h.demo_n : 0);
  keys_t keys = {eadk_keyboard_scan(), 0};
  uint32_t last = np_millis();
  bool dirty = true;
  for (;;) {
    uint32_t now = np_millis();
    float dt = NP_MIN((now - last) / 1000.f, 0.05f);
    uint32_t elapsed = now - last;
    last = now;
    uint64_t p = poll(&keys);
    if (is_home(p) || (p & KB(eadk_key_back))) return STI_QUIT;
    if (p & (KB(eadk_key_left) | KB(eadk_key_up))) h.sel--, dirty = true;
    if (p & (KB(eadk_key_right) | KB(eadk_key_down))) h.sel++, dirty = true;
    if (is_ok(p) && absf(h.pos - h.sel) < 0.3f) {
      int c = wrap(h.sel), r = -1;
      cfg.card = (uint8_t)c;
      save();
      if (c == C_MAT) {
        r = matrices_game();
        if (r >= 0 && np_game_ram_missing(r)) r = -1;
      } else if (c == C_GAMES || c == C_FAV) {
        r = run_list(false, c == C_FAV);
      } else if (c == C_SET) {
        r = run_settings();
      } else {
        r = run_credits();
      }
      if (r != -1) return r;
      keys.held = eadk_keyboard_scan();
      dirty = true;
    }
    /* la carte Jeux fait defiler les captures */
    if (h.demo_n > 1) {
      h.demo_time += elapsed;
      if (h.demo_time > 2600) {
        h.demo_time = 0;
        h.demo = demo_game((int)(np_millis() / 2600) % h.demo_n);
        dirty = true;
      }
    }
    float pos = ui_approach(h.pos, (float)h.sel, dt, 0.07f);
    if (pos != h.pos) dirty = true;
    h.pos = pos;
    if (dirty) {
      ui_frame(home_scene, &h);
      dirty = false;
    } else {
      np_sleep(12);
    }
  }
}
