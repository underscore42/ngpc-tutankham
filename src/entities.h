#ifndef ENTITIES_H
#define ENTITIES_H

#include "ngpc.h"

/* -----------------------------------------------------------------------
   Enemy types
   ----------------------------------------------------------------------- */
#define ENT_NONE      0
#define ENT_SCORPION  1
#define ENT_SNAKE     2
#define ENT_BIRD      3
#define ENT_BUG       4
#define ENT_MUMMY     5

/* Max active enemies */
#define MAX_ENEMIES   4

/* Sprite slots for enemies: 16-31 (4 slots x 4 enemies) */
#define ENT_SPR_BASE  16

/* Enemy move directions */
#define DIR_LEFT  0
#define DIR_RIGHT 1
#define DIR_UP    2
#define DIR_DOWN  3

/* -----------------------------------------------------------------------
   Public state
   ----------------------------------------------------------------------- */
extern u8 ent_type[MAX_ENEMIES];
extern u8 ent_tx[MAX_ENEMIES];
extern u8 ent_ty[MAX_ENEMIES];
extern u8 ent_dir[MAX_ENEMIES];
extern u8 ent_timer[MAX_ENEMIES];
extern u8 ent_alive[MAX_ENEMIES];
extern u8 ent_respawn[MAX_ENEMIES];

/* Player position - updated by game.c each frame */
extern u8 g_player_tx;
extern u8 g_player_ty;

/* -----------------------------------------------------------------------
   Functions
   ----------------------------------------------------------------------- */
void entities_init(u8 level, u8 zone);
void entities_update(void);
void entities_draw(u8 scroll_px);
void entity_kill(u8 idx);

/* Check if any enemy is at tile position (for bullet collision) */
u8 entity_at(u8 tx, u8 ty);

#endif
