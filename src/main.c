#define CARTHDR_IMPL
#include "carthdr.h"
#include "game.h"
#include "tiles.h"
#include "screen.h"
#include "sound.h"

void main(void) {
    InitNGPC();
    SysSetSystemFont();
    tiles_install();
    sound_install();
    game_init();

    while (1) {
        WaitVsync();
        game_update();
    }
}
