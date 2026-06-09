/* sound.c */
#include "sound.h"

static const SOUNDEFFECT game_sounds[] = {
    /* dummy entry - Phase 2: replace with real sounds */
    { 0, 0, 0, 0x0000, 0x0000, 0, 0, 0x0000, 0x0000, 0, 0, 0, 0, 0, 0 }
};

void sound_install(void) {
    InstallSoundDriver();
    InstallSounds(game_sounds, 1);
}
