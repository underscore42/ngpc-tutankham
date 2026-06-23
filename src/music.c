/* music.c - coin, gate_open, stage_clear jingles.
   round_clear mapped to stage_clear. game_start mapped to coin.
*/
#include "music.h"

static const MusicStep music_coin[] = {
    {8,14},
    {9,13},
    {10,14},
    {11,53},
    {0,0}
};

static const MusicStep music_gate_open[] = {
    {12,6},
    {13,6},
    {14,6},
    {15,6},
    {16,6},
    {17,6},
    {18,6},
    {19,6},
    {20,63},
    {0,0}
};

static const MusicStep music_stage_clear[] = {
    {21,13},
    {22,13},
    {23,13},
    {24,13},
    {23,13},
    {22,13},
    {21,13},
    {25,13},
    {17,6},
    {26,6},
    {17,6},
    {26,6},
    {17,6},
    {26,6},
    {17,6},
    {26,6},
    {27,25},
    {0,0}
};

const MusicStep * const music_tracks[MUSIC_COUNT] = {
    music_coin,         /* MUSIC_COIN        */
    music_coin,         /* MUSIC_GAME_START  */
    music_gate_open,    /* MUSIC_GATE_OPEN   */
    music_stage_clear,  /* MUSIC_STAGE_CLEAR */
    music_stage_clear,  /* MUSIC_ROUND_CLEAR (stage_clear fallback) */
};
static const MusicStep *s_mus_seq=0;
static u8 s_mus_pos=0,s_mus_timer=0,s_mus_playing=0;
void music_play(u8 track)
{
    if(track>=MUSIC_COUNT)return;
    s_mus_seq=music_tracks[track];
    s_mus_pos=0; s_mus_timer=0; s_mus_playing=1;
}
void music_stop(void) { s_mus_playing=0; StopAllSound(); }
void music_update(void)
{
    const MusicStep *step;
    if(!s_mus_playing||!s_mus_seq)return;
    step=&s_mus_seq[s_mus_pos];
    if(step->slot==0&&step->dur==0){s_mus_playing=0;return;}
    if(s_mus_timer==0)PlaySound(step->slot);
    s_mus_timer++;
    if(s_mus_timer>=step->dur){s_mus_timer=0;s_mus_pos++;}
}
