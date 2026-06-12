/* sound.c - stub sounds, replace with real SOUNDEFFECT data later */
#include "sound.h"

static const SOUNDEFFECT game_sounds[] = {
    /* 0: dummy/silence */
    { 0, 0, 0, 0x0000, 0x0000, 0, 0, 0x0000, 0x0000, 0, 0, 0, 0, 0, 0 },
    /* 1: SFX_SHOOT - short high beep */
    { 0, 0, 0, 0x0000, 0x0000, 0, 0, 0x0000, 0x0000, 0, 0, 0, 0, 0, 0 },
    /* 2: SFX_RELOAD */
    { 0, 0, 0, 0x0000, 0x0000, 0, 0, 0x0000, 0x0000, 0, 0, 0, 0, 0, 0 },
    /* 3: SFX_ENEMY_SPAWN */
    { 0, 0, 0, 0x0000, 0x0000, 0, 0, 0x0000, 0x0000, 0, 0, 0, 0, 0, 0 },
    /* 4: SFX_TELEPORT */
    { 0, 0, 0, 0x0000, 0x0000, 0, 0, 0x0000, 0x0000, 0, 0, 0, 0, 0, 0 },
    /* 5: SFX_KEY_UNLOCK */
    { 0, 0, 0, 0x0000, 0x0000, 0, 0, 0x0000, 0x0000, 0, 0, 0, 0, 0, 0 },
    /* 6: SFX_TOMB_ENTER */
    { 0, 0, 0, 0x0000, 0x0000, 0, 0, 0x0000, 0x0000, 0, 0, 0, 0, 0, 0 },
    /* 7: SFX_PLAYER_DIE */
    { 0, 0, 0, 0x0000, 0x0000, 0, 0, 0x0000, 0x0000, 0, 0, 0, 0, 0, 0 }
};

void sound_install(void) {
    InstallSoundDriver();
    InstallSounds(game_sounds, 8);
}

void sfx_play(u8 id) {
    /* stub - call PlaySound(id) when real sounds are ready */
    (void)id;
}
