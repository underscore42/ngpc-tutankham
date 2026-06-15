#ifndef GAME_H
#define GAME_H

#include "ngpc.h"
#include "library.h"

/* -----------------------------------------------------------------------
   Game states
   ----------------------------------------------------------------------- */

#define STATE_TITLE   0
#define STATE_SELECT  1
#define STATE_GAME    2
#define STATE_SCROLL       3
#define STATE_INTERMISSION 4

/* -----------------------------------------------------------------------
   Input button masks
   ----------------------------------------------------------------------- */

#define J_UP     0x01
#define J_DOWN   0x02
#define J_LEFT   0x04
#define J_RIGHT  0x08
#define J_A      0x10
#define J_B      0x20
#define J_OPTION 0x40

/* -----------------------------------------------------------------------
   Game constants
   ----------------------------------------------------------------------- */

#define MAX_LIVES        3
#define MAX_BULLETS      6
#define MAX_ENEMIES      4

#define MAZE_LEVELS      8
#define MAZE_ZONES       2
#define MAZE_ROWS        9
#define MAZE_COLS        16

/* Cell types */
#define CELL_FLOOR       0
#define CELL_WALL        1
#define CELL_KEY         2
#define CELL_DOOR        3
#define CELL_TREASURE    4
#define CELL_GENERATOR   5
#define CELL_TELEPORT    6

/* Title screen bitmap */
#define TITLE_TILE_START 300
#define TITLE_TILES_W    20
#define TITLE_TILES_H    11

#define SCROLL_MAX    96   /* (16-10) cells x 16px */
#define SCROLL_SPEED   2

/* -----------------------------------------------------------------------
   Public game state
   ----------------------------------------------------------------------- */

extern u8 g_state;
extern u8 g_level;
extern const unsigned short tut_mask_32[16][8];

/* -----------------------------------------------------------------------
   Functions
   ----------------------------------------------------------------------- */

void game_init(void);
void game_update(void);

#endif
