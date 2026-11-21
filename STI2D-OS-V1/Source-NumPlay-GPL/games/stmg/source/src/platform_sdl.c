// Plateforme PC (SDL2) pour tester le jeu sans calculatrice.
// Flèches : bouger, Entrée/Espace/Z : OK, Échap/Retour arrière : Back, Q : quitter.
#include <SDL2/SDL.h>

#include "platform.h"

static SDL_Window *win;
static SDL_Renderer *ren;
static SDL_Texture *tx;
static uint16_t fb[320 * 240];
static bool quit;

void plat_init(void) {
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
  win = SDL_CreateWindow("STI2D vs STMG", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 960, 720, SDL_WINDOW_RESIZABLE);
  ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  SDL_RenderSetLogicalSize(ren, 320, 240);
  tx = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, 320, 240);
}

uint64_t plat_keys(void) {
  SDL_Event e;
  while (SDL_PollEvent(&e))
    if (e.type == SDL_QUIT) quit = true;
  const Uint8 *k = SDL_GetKeyboardState(NULL);
  uint64_t s = 0;
  if (k[SDL_SCANCODE_LEFT]) s |= 1ull << K_LEFT;
  if (k[SDL_SCANCODE_RIGHT]) s |= 1ull << K_RIGHT;
  if (k[SDL_SCANCODE_UP]) s |= 1ull << K_UP;
  if (k[SDL_SCANCODE_DOWN]) s |= 1ull << K_DOWN;
  if (k[SDL_SCANCODE_RETURN] || k[SDL_SCANCODE_SPACE] || k[SDL_SCANCODE_Z] || k[SDL_SCANCODE_W]) s |= 1ull << K_OK;
  if (k[SDL_SCANCODE_ESCAPE] || k[SDL_SCANCODE_BACKSPACE]) s |= 1ull << K_BACK;
  if (k[SDL_SCANCODE_Q]) quit = true;
  return s;
}

uint64_t plat_millis(void) { return SDL_GetTicks64(); }

void plat_push(int y, int h, const uint16_t *px) {
  for (int i = 0; i < 320 * h && (y * 320 + i) < 320 * 240; i++) fb[y * 320 + i] = px[i];
}

void plat_frame_done(void) {
  SDL_UpdateTexture(tx, NULL, fb, 320 * 2);
  SDL_RenderClear(ren);
  SDL_RenderCopy(ren, tx, NULL, NULL);
  SDL_RenderPresent(ren);
}

bool plat_quit(void) { return quit; }
void plat_sleep(int ms) { SDL_Delay((Uint32)ms); }
uint32_t plat_rand(void) { return (uint32_t)SDL_GetPerformanceCounter() | 1u; }
