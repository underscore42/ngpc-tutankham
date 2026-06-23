#ifndef SOUND_H
#define SOUND_H
#include "ngpc.h"
#include "library.h"

/* SFX slot IDs - passed to sfx_play() */
#define SFX_SHOOT       1
#define SFX_RELOAD      2
#define SFX_ENEMY_SPAWN 3
#define SFX_TELEPORT    4
#define SFX_KEY_UNLOCK  5
#define SFX_TOMB_ENTER  6
#define SFX_PLAYER_DIE  7

void sound_install(void);
void sfx_play(u8 id);

#endif
