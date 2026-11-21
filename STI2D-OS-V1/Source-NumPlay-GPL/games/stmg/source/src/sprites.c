// Tous les sprites du jeu, dessinés en caractères (voir palette dans gfx.c).
#include "sprites.h"

// --- STi2D : le petit diable (regarde vers la droite) ---
const Sprite SPR_PLAYER[2] = {
    {10, 12,
     ".W......W."
     ".WW....WW."
     "..WRRRRW.."
     "..RrrrrR.."
     "..RykykR.."
     "..RrrrrR.."
     "..RrwwrR.."
     "...RRRR..R"
     "..rRrrRr.R"
     "..r.RrR.R."
     "....R.RR.."
     "...RR.RR.."},
    {10, 12,
     ".W......W."
     ".WW....WW."
     "..WRRRRW.."
     "..RrrrrR.."
     "..RykykR.."
     "..RrrrrR.."
     "..RrwwrR.."
     "...RRRR.R."
     "..rRrrRr.R"
     "..r.RrR..R"
     "...R..RR.."
     "..RR...R.."},
};

const Sprite SPR_PLAYER_SIT = {10, 10,
                               ".W......W."
                               ".WW....WW."
                               "..WRRRRW.."
                               "..RrrrrR.."
                               "..RykykR.."
                               "..RrrrrR.."
                               "..RrwwrR.."
                               "...RRRR..."
                               ".rrRrrRrr."
                               ".RR.RR.RRR"};

const Sprite SPR_CROWN = {8, 5,
                          "y..y..y."
                          "yy.yy.yy"
                          "yyyyyyyy"
                          "yryybyry"
                          "YYYYYYYY"};

// --- EE : petit gobelin ---
const Sprite SPR_EE[2] = {
    {8, 8,
     "L......L"
     "LL.ll.LL"
     ".llllll."
     ".lellel."
     ".llllll."
     "..lkkl.."
     ".L.ll.L."
     "..L..L.."},
    {8, 8,
     "L......L"
     "LL.ll.LL"
     ".llllll."
     ".lellel."
     ".llllll."
     "..lwwl.."
     "L..ll..L"
     ".L....L."},
};

// --- SIN : Cerbère à trois têtes ---
const Sprite SPR_SIN[2] = {
    {12, 10,
     "g..g...g..g."
     "gg.gg.ggg.gg"
     "gegW.gegW.ge"
     "ggggwggggwgg"
     ".gggggggggg."
     "ggWggggggWgg"
     "gggggggggggg"
     ".ggWggggWgg."
     ".gg.gg.gg.gg"
     ".G..G...G..G"},
    {12, 10,
     "g..g...g..g."
     "gg.gg.ggg.gg"
     "gegW.gegW.ge"
     "gggwggggwggg"
     ".gggggggggg."
     "ggWggggggWgg"
     "gggggggggggg"
     ".ggWggggWgg."
     "gg.gg..gg.g."
     "G..G...G..G."},
};

// --- ITEC : Belzébuth, le seigneur des mouches ---
const Sprite SPR_ITEC[2] = {
    {14, 14,
     "..y...yy...y.."
     "..VV.VVVV.VV.."
     "...VVVVVVVV..."
     "W.VeeVVVVeeV.W"
     "WWVeeVVVVeeVWW"
     "WWWVVVwwVVVWWW"
     "WWWWVvvvvVWWWW"
     ".WWWVvVVvVWWW."
     "..WWVvvvvVWW.."
     "...VVvVVvVV..."
     "...VVvvvvVV..."
     "..V.VVVVVV.V.."
     ".V..V.VV.V..V."
     "V..V..VV..V..V"},
    {14, 14,
     "..y...yy...y.."
     "..VV.VVVV.VV.."
     "...VVVVVVVV..."
     "..VeeVVVVeeV.."
     "..VeeVVVVeeV.."
     "W..VVVwwVVV..W"
     "WWWWVvvvvVWWWW"
     "WWWWVvVVvVWWWW"
     "WWWWVvvvvVWWWW"
     ".WWVVvVVvVVWW."
     "...VVvvvvVV..."
     "..V.VVVVVV.V.."
     "..V.V.VV.V.V.."
     ".V..V.VV.V..V."},
};

// --- STMG : Dieu ---
const Sprite SPR_GOD = {16, 20,
                        "...yyyyyyyyyy..."
                        "..y..........y.."
                        "...yyyyyyyyyy..."
                        ".....pppppp....."
                        "....pppppppp...."
                        "....pcpppcpp...."
                        "....ppppPppp...."
                        "....wwppppww...."
                        "...wwwwwwwwww..."
                        "..ywwwwwwwwwwy.."
                        ".yyWwwwwwwwwWyy."
                        "pp.WwwwwwwwwW.pp"
                        "pp.WwwwyywwwW.pp"
                        "...WwwwyywwwW..."
                        "...WwwwyywwwW..."
                        "..WwwwwyywwwwW.."
                        "..WwwwwyywwwwW.."
                        ".WwwwwwyywwwwwW."
                        ".WwwwwwyywwwwwW."
                        ".WWWWWWWWWWWWWW."};

// --- RHC : ange ---
const Sprite SPR_RHC[2] = {
    {10, 8,
     "...yyyy..."
     "g..pppp..g"
     "Wg.pcpc.gW"
     "WWg.pp.gWW"
     "WWWwwwwWWW"
     ".WWwwwwWW."
     "...wwww..."
     "...p..p..."},
    {10, 8,
     "...yyyy..."
     "...pppp..."
     "...pcpc..."
     "gg..pp..gg"
     "WWWwwwwWWW"
     "WWWwwwwWWW"
     "g..wwww..g"
     "...p..p..."},
};

const Sprite SPR_THRONE = {14, 16,
                           "..yy......yy.."
                           "..yY......Yy.."
                           "..yyyyyyyyyy.."
                           "..yRRRRRRRRy.."
                           "..yRrrrrrrRy.."
                           "..yRrrrrrrRy.."
                           "..yRrrbbrrRy.."
                           "..yRrrrrrrRy.."
                           "yyyyyyyyyyyyyy"
                           "yRRRRRRRRRRRRy"
                           "yrrrrrrrrrrrry"
                           "yyyyyyyyyyyyyy"
                           ".yY........Yy."
                           ".yY........Yy."
                           ".yY........Yy."
                           "yyy........yyy"};

const Sprite SPR_CHEST = {10, 8,
                          ".NNNNNNNN."
                          "NnnnnnnnnN"
                          "NnnnnnnnnN"
                          "NYYYYYYYYN"
                          "NnnnyynnnN"
                          "NnnnnnnnnN"
                          "NnnnnnnnnN"
                          "NNNNNNNNNN"};

const Sprite SPR_CHEST_OPEN = {10, 8,
                               ".NNNNNNNN."
                               "NnnnnnnnnN"
                               "NYYYYYYYYN"
                               "NkkkkkkkkN"
                               "NYYYYYYYYN"
                               "NnnnnnnnnN"
                               "NnnnnnnnnN"
                               "NNNNNNNNNN"};

const Sprite SPR_GOLD_CHEST = {10, 8,
                               ".YYYYYYYY."
                               "YyyyyyyyyY"
                               "YyyyhhyyyY"
                               "YrrrrrrrrY"
                               "YyyyrryyyY"
                               "YyyyyyyyyY"
                               "YyyyyyyyyY"
                               "YYYYYYYYYY"};

const Sprite SPR_GOLD_CHEST_OPEN = {10, 8,
                                    ".YYYYYYYY."
                                    "YyyyyyyyyY"
                                    "YrrrrrrrrY"
                                    "YhhhhhhhhY"
                                    "YrrrrrrrrY"
                                    "YyyyyyyyyY"
                                    "YyyyyyyyyY"
                                    "YYYYYYYYYY"};

const Sprite SPR_HEART = {9, 8,
                          ".kkk.kkk."
                          "krrrkrrrk"
                          "krwrrrrrk"
                          "krrrrrrrk"
                          ".krrrrrk."
                          "..krrrk.."
                          "...krk..."
                          "....k...."};

const Sprite SPR_HEART_BLUE = {9, 8,
                               ".kkk.kkk."
                               "kbbbkbbbk"
                               "kbcbbbbbk"
                               "kbbbbbbbk"
                               ".kbbbbbk."
                               "..kbbbk.."
                               "...kbk..."
                               "....k...."};

const Sprite SPR_HEART_EMPTY = {9, 8,
                                ".kkk.kkk."
                                "kmmmkmmmk"
                                "kmmmmmmmk"
                                "kmmmmmmmk"
                                ".kmmmmmk."
                                "..kmmmk.."
                                "...kmk..."
                                "....k...."};

const Sprite SPR_ICON_FIRE = {8, 8,
                              "...o...."
                              "..oo..o."
                              ".oyyo.o."
                              ".oyhyoo."
                              "oyhhhyo."
                              "oyhhhyo."
                              ".oyyyo.."
                              "..ooo..."};

const Sprite SPR_ICON_BOOTS = {8, 8,
                               ".NNNN..."
                               ".NnnN..."
                               ".NnnN..."
                               ".NnnN..."
                               ".NnnNNN."
                               "NnnnnnnN"
                               "NnnnnnnN"
                               "YYYYYYYY"};

const Sprite SPR_ICON_TORCH = {8, 8,
                               "...o...."
                               "..oyo..."
                               "..ohyo.."
                               "...yo..."
                               "..gWg..."
                               "...n...."
                               "...n...."
                               "...N...."};

const Sprite SPR_ICON_SHIELD = {8, 8,
                                "BBBBBBBB"
                                "BbbcbbbB"
                                "BbcccbbB"
                                "BbbcbbbB"
                                "BbbbbbbB"
                                ".BbbbbB."
                                "..BbbB.."
                                "...BB..."};

const Sprite SPR_ICON_HEAL = {8, 8,
                              ".kk..kk."
                              "krrkkrrk"
                              "krwrrrrk"
                              "krrrrrrk"
                              ".krrrrk."
                              "..krrk.."
                              "...kk..."
                              "........"};

const Sprite *const SPR_ITEMS[5] = {&SPR_ICON_HEAL, &SPR_ICON_SHIELD, &SPR_ICON_FIRE, &SPR_ICON_BOOTS, &SPR_ICON_TORCH};

#ifdef GAME_DEBUG
#include <string.h>
static const Sprite *const ALL[] = {
    &SPR_PLAYER[0], &SPR_PLAYER[1], &SPR_PLAYER_SIT, &SPR_CROWN, &SPR_EE[0], &SPR_EE[1], &SPR_SIN[0],
    &SPR_SIN[1], &SPR_ITEC[0], &SPR_ITEC[1], &SPR_GOD, &SPR_RHC[0], &SPR_RHC[1], &SPR_THRONE,
    &SPR_CHEST, &SPR_CHEST_OPEN, &SPR_GOLD_CHEST, &SPR_GOLD_CHEST_OPEN, &SPR_HEART, &SPR_HEART_BLUE,
    &SPR_HEART_EMPTY, &SPR_ICON_FIRE, &SPR_ICON_BOOTS, &SPR_ICON_TORCH, &SPR_ICON_SHIELD, &SPR_ICON_HEAL};

int sprites_check(void) {
  int bad = -1;
  for (unsigned i = 0; i < sizeof ALL / sizeof ALL[0]; i++)
    if (strlen(ALL[i]->data) != (size_t)ALL[i]->w * ALL[i]->h) bad = (int)i;
  return bad;
}
#endif
