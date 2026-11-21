#ifndef SPRITES_H
#define SPRITES_H

#include "gfx.h"

extern const Sprite SPR_PLAYER[2], SPR_PLAYER_SIT, SPR_CROWN;
extern const Sprite SPR_EE[2], SPR_SIN[2], SPR_ITEC[2], SPR_RHC[2];
extern const Sprite SPR_GOD, SPR_THRONE;
extern const Sprite SPR_CHEST, SPR_CHEST_OPEN, SPR_GOLD_CHEST, SPR_GOLD_CHEST_OPEN;
extern const Sprite SPR_HEART, SPR_HEART_BLUE, SPR_HEART_EMPTY;
extern const Sprite SPR_ICON_FIRE, SPR_ICON_BOOTS, SPR_ICON_TORCH, SPR_ICON_SHIELD, SPR_ICON_HEAL;
extern const Sprite *const SPR_ITEMS[5];

#ifdef GAME_DEBUG
int sprites_check(void);  // -1 si toutes les dimensions sont cohérentes
#endif

#endif
