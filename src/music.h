#ifndef MUSIC_H
#define MUSIC_H
#include "ngpc.h"
typedef struct{u8 slot;u8 dur;}MusicStep;
#define MUSIC_COIN 0
#define MUSIC_GAME_START 1
#define MUSIC_GATE_OPEN 2
#define MUSIC_STAGE_CLEAR 3
#define MUSIC_ROUND_CLEAR 4
#define MUSIC_COUNT 5
extern const MusicStep * const music_tracks[MUSIC_COUNT];
void music_play(u8 track);
void music_stop(void);
void music_update(void);
#endif
