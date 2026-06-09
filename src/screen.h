#ifndef SCREEN_H
#define SCREEN_H

#include "ngpc.h"

void screen_draw_title(void);
void screen_update_title(void);     /* call each frame - handles blink */
void screen_draw_select(u8 cursor); /* game select menu */
void screen_draw_hud(u8 level);
void screen_clear_title(void);

#endif
