/* STI2D-Play : l'interface (accueil, listes, reglages) par-dessus le moteur de NumPlay. */
#ifndef STI2D_H
#define STI2D_H
#include "np.h"

/* Apps installees a cote de STI2D-Play (gros jeux, emulateurs) : lues dans la zone des apps. */
typedef struct {
  const char *name;
  void (*entry)(void);
  uint32_t size;
} sti_app_t;
#define STI_MAX_APPS 32
int sti_apps_scan(sti_app_t *out, int max);
/* Remet a neuf la RAM de l'app (apres une app lancee, ou np_session_end) */
void sti_reinit_ram(void);

/* Ce que l'accueil demande a main() */
enum { STI_QUIT = -2, STI_EXTERNAL = -3 };
extern void (*sti_external)(void);   /* l'app a lancer quand sti_home() rend STI_EXTERNAL */

void sti_load(void);                  /* reglages et favoris (sti2dplay.py) */
/* un jeu de np_games a lancer, STI_QUIT ou STI_EXTERNAL */
int sti_home(bool returning);
#endif
