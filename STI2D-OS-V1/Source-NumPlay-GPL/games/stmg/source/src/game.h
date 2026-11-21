#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include <stdint.h>

#define FPS 30

void game_init(uint32_t seed);
bool game_update(uint64_t keys);  // false = quitter l'application
void game_draw(void);             // dessine la bande courante (g_sy)

#ifdef PLATFORM_WEB
// Sauvegarde du record, de la difficulté et du classement (localStorage du navigateur).
int game_save(uint8_t *buf, int cap);  // renvoie la taille écrite
void game_load(const uint8_t *buf, int n);
#endif

#ifdef GAME_DEBUG
// Accès de test : permet de placer le jeu dans un état précis pour des captures.
enum { DBG_STATE, DBG_TELEPORT, DBG_GIVE, DBG_BOSS_HP, DBG_TICKS, DBG_INFO, DBG_GOD_MODE, DBG_ROOM };
int game_debug(int cmd, int a, int b);
#endif

#endif
