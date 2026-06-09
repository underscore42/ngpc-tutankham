#include "tutankham.h"

u8 g_state;
u8 g_level;

static u8  s_pad_cur;
static u8  s_pad_prev;
static u8  s_pad_press;
static u8  s_scroll_px;
static u8  s_select_cursor;
static u8  s_player_tx;
static u8  s_player_ty;

static void enter_title(void);
static void enter_select(void);
static void enter_game(u8 level);
static void enter_scroll_test(void);
static void update_title(void);
static void update_select(void);
static void update_game(void);
static void update_scroll_test(void);
static void draw_player(void);

void game_init(void)
{
    s_pad_cur       = 0;
    s_pad_prev      = 0;
    s_pad_press     = 0;
    s_scroll_px     = 0;
    s_select_cursor = 0;
    s_player_tx     = 1;
    s_player_ty     = 1;
    enter_title();
}

void game_update(void)
{
    s_pad_prev  = s_pad_cur;
    s_pad_cur   = (u8)(JOYPAD & 0x7F);
    s_pad_press = (u8)(s_pad_cur & (~s_pad_prev));

    if (g_state == STATE_TITLE) {
        update_title();
    } else if (g_state == STATE_SELECT) {
        update_select();
    } else if (g_state == STATE_GAME) {
        update_game();
    } else if (g_state == STATE_SCROLL) {
        update_scroll_test();
    }
}

/* -----------------------------------------------------------------------
   TITLE
   ----------------------------------------------------------------------- */

static void enter_title(void)
{
    u8 i;
    g_state = STATE_TITLE;
    for (i = 0; i < 64; i++) UnsetSprite(i);
    screen_draw_title();
    SCR1_X = 0;
}

static void update_title(void)
{
    screen_update_title();
    if (s_pad_press != 0) {
        enter_select();
    }
}

/* -----------------------------------------------------------------------
   SELECT
   ----------------------------------------------------------------------- */

static void enter_select(void)
{
    u8 i;
    g_state = STATE_SELECT;
    s_select_cursor = 0;
    for (i = 0; i < 64; i++) UnsetSprite(i);
    ClearScreen(SCR_1_PLANE);
    screen_draw_select(s_select_cursor);
}

static void update_select(void)
{
    if (s_pad_press & J_UP) {
        if (s_select_cursor > 0) {
            s_select_cursor--;
            screen_draw_select(s_select_cursor);
        }
    }
    if (s_pad_press & J_DOWN) {
        if (s_select_cursor < 8) {
            s_select_cursor++;
            screen_draw_select(s_select_cursor);
        }
    }
    if (s_pad_press & J_A) {
        if (s_select_cursor == 8) {
            enter_scroll_test();
        } else {
            enter_game(s_select_cursor);
        }
    }
    if (s_pad_press & J_B) {
        enter_title();
    }
}

/* -----------------------------------------------------------------------
   GAME
   ----------------------------------------------------------------------- */

static void enter_game(u8 level)
{
    u8 i;
    g_state = STATE_GAME;
    g_level = level;
    s_scroll_px = 0;

    for (i = 0; i < 64; i++) UnsetSprite(i);

    ClearScreen(SCR_1_PLANE);
    ClearScreen(SCR_2_PLANE);
    SysSetSystemFont();
    tiles_install();

    SCR1_X = 0;
    maze_draw(level);
    screen_draw_hud(level);

    s_player_tx = 1;
    s_player_ty = 1;
    draw_player();
}

static void update_game(void)
{
    u8 new_tx;
    u8 new_ty;

    new_tx = s_player_tx;
    new_ty = s_player_ty;

    if (s_pad_press & J_UP)    { if (new_ty > 0)  new_ty--; }
    if (s_pad_press & J_DOWN)  { if (new_ty < 8)  new_ty++; }
    if (s_pad_press & J_LEFT)  { if (new_tx > 0)  new_tx--; }
    if (s_pad_press & J_RIGHT) { if (new_tx < 31) new_tx++; }

    if (new_tx != s_player_tx || new_ty != s_player_ty) {
        s_player_tx = new_tx;
        s_player_ty = new_ty;
        draw_player();
    }

    /* Camera: scroll when player nears edges of visible window */
    {
        u8 screen_x;
        screen_x = (u8)(s_player_tx * 16);
        if (screen_x > s_scroll_px + 5 * 16 && s_scroll_px < SCROLL_MAX) {
            s_scroll_px = (u8)(s_scroll_px + SCROLL_SPEED);
            if (s_scroll_px > SCROLL_MAX) s_scroll_px = SCROLL_MAX;
            SCR1_X = s_scroll_px;
        } else if (screen_x < s_scroll_px + 4 * 16 && s_scroll_px > 0) {
            if (s_scroll_px >= SCROLL_SPEED) {
                s_scroll_px = (u8)(s_scroll_px - SCROLL_SPEED);
            } else {
                s_scroll_px = 0;
            }
            SCR1_X = s_scroll_px;
        }
    }

    if (s_pad_press & J_B) {
        enter_select();
    }
}

/* -----------------------------------------------------------------------
   SCROLL TEST
   ----------------------------------------------------------------------- */

static void enter_scroll_test(void)
{
    u8 i;
    g_state = STATE_SCROLL;
    s_scroll_px = 0;

    for (i = 0; i < 64; i++) UnsetSprite(i);

    ClearScreen(SCR_1_PLANE);
    ClearScreen(SCR_2_PLANE);
    SysSetSystemFont();
    tiles_install();

    maze_draw(0);

    /* Hardcode 32px offset to confirm register works on first frame */
    SCR1_X = 32;
    s_scroll_px = 32;

    screen_draw_hud(0);
    PrintString(SCR_2_PLANE, P_HUD, 0, 2, "SCROLL TEST");
    PrintString(SCR_2_PLANE, P_HUD, 0, 3, "LR=SCROLL B=BACK");
}

static void update_scroll_test(void)
{
    u8 changed;
    char buf[8];

    changed = 0;

    if (s_pad_cur & J_LEFT) {
        if (s_scroll_px >= SCROLL_SPEED) {
            s_scroll_px = (u8)(s_scroll_px - SCROLL_SPEED);
        } else {
            s_scroll_px = 0;
        }
        changed = 1;
    }
    if (s_pad_cur & J_RIGHT) {
        if (s_scroll_px <= (u8)(SCROLL_MAX - SCROLL_SPEED)) {
            s_scroll_px = (u8)(s_scroll_px + SCROLL_SPEED);
        } else {
            s_scroll_px = SCROLL_MAX;
        }
        changed = 1;
    }

    if (changed) {
        SCR1_X = s_scroll_px;
        buf[0] = 'S'; buf[1] = 'C'; buf[2] = 'R'; buf[3] = ':';
        buf[4] = '0' + (s_scroll_px / 100);
        buf[5] = '0' + ((s_scroll_px / 10) % 10);
        buf[6] = '0' + (s_scroll_px % 10);
        buf[7] = 0;
        PrintString(SCR_2_PLANE, P_HUD, 12, 0, buf);
    }

    if (s_pad_press & J_B) {
        enter_select();
    }
}

/* -----------------------------------------------------------------------
   Player sprite - 16x16 = 4 x 8x8 tiles
   Asteroids pattern: pass raw pixel coords, no +16 offset
   ----------------------------------------------------------------------- */

static void draw_player(void)
{
    u8 sx;
    u8 sy;

    /* Convert tile coords to pixel coords, subtract scroll offset */
    sx = (u8)(s_player_tx * 16);
    if (sx >= s_scroll_px) {
        sx = (u8)(sx - s_scroll_px);
    } else {
        sx = 0;
    }
    sy = (u8)(s_player_ty * 16 + 8);  /* +8 for HUD row */

    SetSprite(0, T_PLAY_TL, 0, sx,     sy,     P_PLAYER);
    SetSprite(1, T_PLAY_TR, 0, sx + 8, sy,     P_PLAYER);
    SetSprite(2, T_PLAY_BL, 0, sx,     sy + 8, P_PLAYER);
    SetSprite(3, T_PLAY_BR, 0, sx + 8, sy + 8, P_PLAYER);

    SpriteControl(0, SPR_FRONT, 0);
    SpriteControl(1, SPR_FRONT, 0);
    SpriteControl(2, SPR_FRONT, 0);
    SpriteControl(3, SPR_FRONT, 0);
}
