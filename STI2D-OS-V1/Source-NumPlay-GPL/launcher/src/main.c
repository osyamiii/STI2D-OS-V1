/* STI2D-Play : les jeux de NumPlay et Matrices IA dans une seule app, plus
 * les apps installees a cote (gros jeux, emulateurs) lancees depuis l'accueil. */
#include <eadk.h>
#include "np.h"
#include "sys.h"
#include "sti2d.h"

#if PLATFORM_DEVICE && !NP_SIMULATOR
const uint32_t eadk_api_level __attribute__((section(".rodata.eadk_api_level"))) = 0;
/* Tells the games (games/common/epsilon_app.h) that the launcher takes care
 * of Home and of the RAM. */
const uint8_t numplay_launcher[1] = {1};

/* The games read the keyboard through these (the build renames their calls),
 * so the launcher knows when a game ended because of Home or On/Off. */
static bool home_pressed;
eadk_keyboard_state_t np_keyboard_scan(void) {
  eadk_keyboard_state_t s = eadk_keyboard_scan();
  if (s & ((1ull << eadk_key_home) | (1ull << eadk_key_on_off))) home_pressed = true;
  return s;
}
eadk_event_t np_event_get(int32_t *timeout) {
  eadk_event_t e = eadk_event_get(timeout);
  if (e == 6 || e == 8) home_pressed = true; /* Home, On/Off */
  return e;
}
#else
static bool home_pressed;
#endif

int main(void) {
  for (;;) {
    np_session_begin();
    np_finish_pending_uninstalls();
    /* just updated? the saves come back from their copy, then the copy is refreshed */
    np_progress_restore();
    np_progress_backup();
    sti_load();
    np_wait_release();
    bool returning = false;
    int r;
    for (;;) {
      r = sti_home(returning);
      if (r < 0) break;
      home_pressed = false;
      np_wait_release(); /* la touche OK qui lance le jeu ne doit pas compter pour lui */
      np_game_run(r);
      np_progress_backup();
      /* Home in a game goes all the way home, like everywhere on the calculator */
      if (home_pressed) return np_session_end();
      np_wait_release();
      returning = true;
    }
    if (r != STI_EXTERNAL) return np_session_end();
    /* Une app installee a cote : on rend la main au systeme comme a la sortie,
     * on la lance comme le fait l'accueil d'Epsilon, puis on repart a neuf. */
    void (*volatile entry)(void) = sti_external;
    np_session_end();
    entry();
    sti_reinit_ram();
  }
}
