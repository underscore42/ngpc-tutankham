/* tutankham.h - single include for all translation units.
 * Mirrors the include chain proven in asteroids-neo:
 * ngpc.h -> library.h (defines SCR_1_PLANE, SPRITE_PLANE, SOUNDEFFECT etc.)
 * game.h already pulls both in, so including game.h is sufficient.
 */
#ifndef TUTANKHAM_H
#define TUTANKHAM_H

#include "game.h"
#include "tiles.h"
#include "screen.h"
#include "sound.h"

#endif
