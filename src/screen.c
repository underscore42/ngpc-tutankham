#include "tutankham.h"

static u8 s_blink_timer;
static u8 s_blink_on;

void screen_draw_title(void)
{
    u8 tx;
    u8 ty;
    u16 tid;

    ClearScreen(SCR_1_PLANE);
    ClearScreen(SCR_2_PLANE);
    /* Reinstall tiles and font after ClearScreen (Asteroids pattern) */
    SysSetSystemFont();
    tiles_install();
    /* Restore title palette - tiles_install sets slot 3 to teleport blue */
    SetPalette(SCR_1_PLANE, P_TITLE,
        RGB(0,0,0),
        RGB(11,3,3),
        RGB(2,6,8),
        RGB(7,12,14));

    for (ty = 0; ty < TITLE_TILES_H; ty++) {
        for (tx = 0; tx < TITLE_TILES_W; tx++) {
            tid = TITLE_TILE_START + ty * TITLE_TILES_W + tx;
            PutTile(SCR_1_PLANE, P_TITLE, tx, ty + 1, tid);
        }
    }

    PrintString(SCR_2_PLANE, P_HUD, 4, 15, "PRESS START");
//    PrintString(SCR_2_PLANE, P_HUD, 1, 17, "STUDIO SO NOT KANSAI");

    s_blink_timer = 0;
    s_blink_on    = 1;
}

void screen_update_title(void)
{
    s_blink_timer++;
    if (s_blink_timer >= 30) {
        s_blink_timer = 0;
        s_blink_on = s_blink_on ? 0 : 1;
        if (s_blink_on) {
            PrintString(SCR_2_PLANE, P_HUD, 4, 15, "PRESS START");
        } else {
            PrintString(SCR_2_PLANE, P_HUD, 4, 15, "           ");
        }
    }
}

void screen_clear_title(void)
{
    ClearScreen(SCR_1_PLANE);
    ClearScreen(SCR_2_PLANE);
    SysSetSystemFont();
    tiles_install();
}

void screen_draw_select(u8 cursor)
{
    u8 i;
    const char *names[9];

    names[0] = "LEVEL 1";
    names[1] = "LEVEL 2";
    names[2] = "LEVEL 3";
    names[3] = "LEVEL 4";
    names[4] = "LEVEL 5";
    names[5] = "LEVEL 6";
    names[6] = "LEVEL 7";
    names[7] = "LEVEL 8";
    names[8] = "SCROLL TEST";

    ClearScreen(SCR_1_PLANE);
    SysSetSystemFont();

    PrintString(SCR_2_PLANE, P_HUD, 5, 1, "SELECT LEVEL");

    for (i = 0; i < 9; i++) {
        if (i == cursor) {
            PrintString(SCR_2_PLANE, P_HUD, 2, i + 3, ">");
        } else {
            PrintString(SCR_2_PLANE, P_HUD, 2, i + 3, " ");
        }
        PrintString(SCR_2_PLANE, P_HUD, 4, i + 3, names[i]);
    }

    PrintString(SCR_2_PLANE, P_HUD, 2, 14, "A=SELECT  B=BACK");
}

void screen_draw_hud(u8 level)
{
    char buf[8];

    buf[0] = 'S'; buf[1] = 'T'; buf[2] = 'A';
    buf[3] = 'G'; buf[4] = 'E'; buf[5] = ':';
    buf[6] = (u8)('1' + level); buf[7] = 0;
    PrintString(SCR_2_PLANE, P_HUD, 0, 0, buf);
    PrintString(SCR_2_PLANE, P_HUD, 14, 0, "KEY:?");
}
