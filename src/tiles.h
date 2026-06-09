#ifndef TILES_H
#define TILES_H

#include "ngpc.h"

/* -----------------------------------------------------------------------
   Tile IDs
   0-143   system font (SysSetSystemFont)
   144-154 game tiles
   300-519 title screen bitmap
   ----------------------------------------------------------------------- */

#define T_WALL      148
#define T_LOCK      149
#define T_KEY       150
#define T_PLAY_TL   151
#define T_PLAY_TR   152
#define T_PLAY_BL   153
#define T_PLAY_BR   154

#define TITLE_TILE_START  300
#define TITLE_TILE_COUNT  220
#define TITLE_TILES_W     20
#define TITLE_TILES_H     11

/* -----------------------------------------------------------------------
   Palette slots
   ----------------------------------------------------------------------- */

/* SCR_1_PLANE palette slots */
#define P_WALL   0
#define P_LOCK   1
#define P_KEY    2

/* SCR_2_PLANE palette slots */
#define P_HUD    0

/* SCR_1_PLANE palette slot for title bitmap */
#define P_TITLE  3

/* SPRITE_PLANE palette slots */
#define P_PLAYER 0

/* -----------------------------------------------------------------------
   Functions
   ----------------------------------------------------------------------- */

void tiles_install(void);
void maze_draw(u8 level);
void maze_set_scroll_px(u8 px);

#endif
