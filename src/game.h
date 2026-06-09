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
#define STATE_SCROLL  3

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

#define SCROLL_MAX    96
#define SCROLL_SPEED   2

/* -----------------------------------------------------------------------
   Public game state
   ----------------------------------------------------------------------- */

extern u8 g_state;
extern u8 g_level;

/* -----------------------------------------------------------------------
   Functions
   ----------------------------------------------------------------------- */

void game_init(void);
void game_update(void);

#endif
