#include "tutankham.h"
#include "entities.h"
#include "sound.h"
#include "music.h"

/* 32x32 Tut mask - defined in tiles.c */
extern const unsigned short tut_mask_32[16][8];

u8 g_state;
u8 g_level;

static u8 s_zone;
static u8 s_pad_cur;
static u8 s_pad_prev;
static u8 s_pad_press;
static u8 s_scroll_px;
static u8 s_select_cursor;
static u8 s_player_tx;
static u8 s_player_ty;
static u8 s_player_facing;   /* 0=right 1=left */
static u8 s_walk_frame;       /* 0=frame1 1=frame2, toggles each step */
static u8 s_key_collected;
static u16 s_score;
static u16 s_hi_score;       /* persists across games */
static u8  s_konami_step;     /* Konami code progress 0-9 */

/* Intermission state */
static u16 s_intro_tick;      /* frame counter */
static u8  s_intro_px;        /* player x pixel position */
static u8  s_intro_walk;      /* walk frame toggle */
static u8  s_intro_wtimer;    /* walk animation timer */
static u8  s_looped;          /* set after completing all 8 levels */
static u8  s_zombie_mode;     /* Konami easter egg: play as mummy */
static u8  s_zombie_timer;    /* countdown seconds remaining */
static u8  s_zombie_frames;   /* frame tick counter for 1-second decrement */
static u8  s_invincible;       /* debug: timer frozen, no death */
static u8 s_lives;
static u8 s_bullets;         /* remaining shots 0-6 */
static u8 s_reloading;       /* reload animation timer */
static u8 s_blt_timer;        /* bullet move rate */
static u8 s_smoke_tick;
static u8 s_move_dir;
static u8 s_move_timer;

/* Per-level collected state */
static u8 s_key_done[MAZE_LEVELS];
/* Per-level treasure tracking - up to 4 per level */
static u8 s_treas_count[MAZE_LEVELS];      /* how many treasures found in map */
static u8 s_treas_tx[MAZE_LEVELS][8];      /* treasure col positions */
static u8 s_treas_ty[MAZE_LEVELS][8];      /* treasure row positions */
static u8 s_treas_zone[MAZE_LEVELS][8];    /* treasure zone (0 or 1) */
static u8 s_treas_done[MAZE_LEVELS];       /* bitmask: bit N = treasure N collected */

/* Bullet sprites: 4 sprite slots per bullet, 3 bullets max on screen */
/* Slots 4-15 reserved for bullets (3 x 4 slots each) */
static u8  s_blt_active[3];
static u8  s_blt_tx[3];
static u8  s_blt_ty[3];
static u8  s_blt_dir[3];  /* 0=right 1=left */

static void save_hi_score(void);
static void load_hi_score(void);
static void palettes_set_greyscale(void);
static void enter_title(void);
static void enter_intermission(void);
static void update_intermission(void);
static void enter_select(void);
static void enter_game(u8 level);
static void enter_scroll_test(void);

/* -----------------------------------------------------------------------
   INTERMISSION - chase sequence after completing all 8 levels
   32x32 Tut mask top-centre, player chased by all 5 enemy types
   Loops until animation completes then restarts game from level 1
   ----------------------------------------------------------------------- */

static void enter_intermission(void)
{
    u8 i;

    if (s_score > s_hi_score) s_hi_score = s_score;

    g_state = STATE_INTERMISSION;
    s_intro_tick   = 0;
    s_intro_px     = 0;
    s_intro_walk   = 0;
    s_intro_wtimer = 0;

    for (i = 0; i < 64; i++) UnsetSprite(i);
    ClearScreen(SCR_1_PLANE);
    ClearScreen(SCR_2_PLANE);
    SysSetSystemFont();
    tiles_install();
    SCR1_X = 0;

    /* Draw Tut mask centred in top half - tile rows 2-5 (32px tall) */
    /* Tut mask 32x32 = 4x4 tiles stored in tut_mask_32[16][8] */
    /* Install into a high VRAM region - use tile ID 232 onwards */
    InstallTileSetAt(
        (const unsigned short (*)[8])tut_mask_32,
        128,   /* 16 tiles x 8 words */
        232);

    /* Gold palette for Tut mask */
    SetPalette(SCR_1_PLANE, P_WALL,
        RGB(3,2,0), RGB(8,5,0), RGB(13,9,1), RGB(15,13,3));

    /* Draw the 4x4 tut mask grid centred: cols 8-11, rows 2-5 */
    {
        u8 tr;
        u8 tc;
        for (tr = 0; tr < 4; tr++) {
            for (tc = 0; tc < 4; tc++) {
                PutTile(SCR_1_PLANE, P_WALL,
                    (u8)(8 + tc), (u8)(2 + tr),
                    (u16)(232 + tr * 4 + tc));
            }
        }
    }

    /* Score display */
    {
        char buf[6];
        buf[0] = (u8)('0' + (s_score / 1000) % 10);
        buf[1] = (u8)('0' + (s_score / 100)  % 10);
        buf[2] = (u8)('0' + (s_score / 10)   % 10);
        buf[3] = (u8)('0' +  s_score         % 10);
        buf[4] = 0;
        PrintString(SCR_2_PLANE, P_HUD, 1, 7, "SCORE:");
        PrintString(SCR_2_PLANE, P_HUD, 8, 7, buf);
        PrintString(SCR_2_PLANE, P_HUD, 2, 8, "LEVEL COMPLETE!");
    }

    sfx_play(SFX_TELEPORT);  /* placeholder for chase music trigger */
}

static void update_intermission(void)
{
    u8 px;
    u8 py;
    u8 ex;
    u8 tl;
    u8 i;

    s_intro_tick++;

    /* After 600 frames (~10s) loop back to level 1 */
    if (s_intro_tick > 600) {
        {
            u16 saved_score;
            saved_score = s_score;
            s_looped = 1;
            game_init();
            s_score = saved_score;
            enter_game(0);
        }
        return;
    }

    /* Walk animation */
    s_intro_wtimer++;
    if (s_intro_wtimer >= 8) {
        s_intro_wtimer = 0;
        s_intro_walk   = (u8)(1 - s_intro_walk);
    }

    /* Move player left->right across bottom half of screen */
    /* Reset and loop */
    if (s_intro_tick % 2 == 0) {
        s_intro_px++;
    }
    if (s_intro_px > 200) {
        s_intro_px = 0;
    }

    /* Player sprite at row 11 (bottom half), scrolling x */
    py = 88;  /* pixel y = row 11 x 8px */
    px = s_intro_px;

    /* Draw player */
    if (s_intro_walk == 0) {
        tl = T_PLAY_TL;
    } else {
        tl = T_PLAY2_TL;
    }
    SetSprite(0, tl,          0, px,     py,     P_PLAYER);
    SetSprite(1, (u8)(tl+1),  0, px+8,   py,     P_PLAYER);
    SetSprite(2, (u8)(tl+2),  0, px,     py+8,   P_PLAYER);
    SetSprite(3, (u8)(tl+3),  0, px+8,   py+8,   P_PLAYER);
    SpriteControl(0, SPR_FRONT, 0);
    SpriteControl(1, SPR_FRONT, 0);
    SpriteControl(2, SPR_FRONT, 0);
    SpriteControl(3, SPR_FRONT, 0);

    /* 5 enemies chasing: scorpion, snake, bug, bird, mummy */
    /* Spaced 3 sprites (48px) apart behind player */
    {
        u8 etypes[5];
        u8 epals[5];
        u8 eflips[5];
        /* needs_flip: 1 = sprite faces left natively, flip to face right */
        etypes[0] = T_MUMMY_TL;    epals[0] = P_ENEMY_B;  eflips[0] = 1;
        etypes[1] = T_BIRD_TL;     epals[1] = P_ENEMY_B;  eflips[1] = 1;
        etypes[2] = T_BUG_TL;      epals[2] = P_ENEMY_A;  eflips[2] = 0;
        etypes[3] = T_SNAKE_TL;    epals[3] = P_BULLET;   eflips[3] = 0;
        etypes[4] = T_SCORPION_TL; epals[4] = P_ENEMY_A;  eflips[4] = 1;

        {
            u8 base;
            u8 etl;
            u8 epal;
            i = 0;
            for (; i < 5; i++) {
            /* Player gap = 48px (3 sprites), enemies 16px (1 sprite) apart */
            if (px < (u8)(48 + i * 16)) {
                ex = (u8)(px + 208 - i * 16);  /* wrap around */
            } else {
                ex = (u8)(px - 48 - i * 16);
            }
            base = (u8)(4 + i * 4);
            etl  = etypes[i];
            epal = epals[i];
            if (eflips[i]) {
                /* face right by swapping TL<->TR, BL<->BR + HFLIP */
                SetSprite(base,         (u8)(etl+1), 0, ex,    py,   epal);
                SetSprite((u8)(base+1), etl,         0, ex+8,  py,   epal);
                SetSprite((u8)(base+2), (u8)(etl+3), 0, ex,    py+8, epal);
                SetSprite((u8)(base+3), (u8)(etl+2), 0, ex+8,  py+8, epal);
                SpriteControl(base,         SPR_FRONT, SPR_HFLIP);
                SpriteControl((u8)(base+1), SPR_FRONT, SPR_HFLIP);
                SpriteControl((u8)(base+2), SPR_FRONT, SPR_HFLIP);
                SpriteControl((u8)(base+3), SPR_FRONT, SPR_HFLIP);
            } else {
                SetSprite(base,         etl,          0, ex,    py,   epal);
                SetSprite((u8)(base+1), (u8)(etl+1),  0, ex+8,  py,   epal);
                SetSprite((u8)(base+2), (u8)(etl+2),  0, ex,    py+8, epal);
                SetSprite((u8)(base+3), (u8)(etl+3),  0, ex+8,  py+8, epal);
                SpriteControl(base,         SPR_FRONT, 0);
                SpriteControl((u8)(base+1), SPR_FRONT, 0);
                SpriteControl((u8)(base+2), SPR_FRONT, 0);
                SpriteControl((u8)(base+3), SPR_FRONT, 0);
            }
            }
        }
    }
}

static void enter_title(void)
{
    u8 i;
    g_state = STATE_TITLE;
    for (i = 0; i < 64; i++) UnsetSprite(i);
    /* Clear stray HUD tiles from previous game */
    ClearScreen(SCR_2_PLANE);
    SysSetSystemFont();
    screen_draw_title();
    SCR1_X = 0;
    /* Show hi score on title */
    {
        char buf[5];
        buf[0] = (u8)('0' + (s_hi_score / 1000) % 10);
        buf[1] = (u8)('0' + (s_hi_score / 100)  % 10);
        buf[2] = (u8)('0' + (s_hi_score / 10)   % 10);
        buf[3] = (u8)('0' +  s_hi_score          % 10);
        buf[4] = 0;
        PrintString(SCR_2_PLANE, P_HUD, 6, 17, "HI:");
        PrintString(SCR_2_PLANE, P_HUD, 10, 17, buf);
    }
}

static void update_title(void)
{
    screen_update_title();
    /* Konami code: ↑↑↓↓←→←→BA unlocks level select */
    if (s_pad_press != 0) {
        u8 expected;
        expected = 0;
        if (s_konami_step == 0) expected = J_UP;
        if (s_konami_step == 1) expected = J_UP;
        if (s_konami_step == 2) expected = J_DOWN;
        if (s_konami_step == 3) expected = J_DOWN;
        if (s_konami_step == 4) expected = J_LEFT;
        if (s_konami_step == 5) expected = J_RIGHT;
        if (s_konami_step == 6) expected = J_LEFT;
        if (s_konami_step == 7) expected = J_RIGHT;
        if (s_konami_step == 8) expected = J_B;
        if (s_konami_step == 9) expected = J_A;
        if (s_pad_press == expected) {
            s_konami_step++;
            if (s_konami_step >= 10) {
                s_konami_step = 0;
                s_looped = 0;
                game_init();
                s_zombie_mode = 1;  /* set AFTER game_init clears it */
                enter_game(0);
                return;
            }
        } else {
            s_konami_step = 0;
            /* Check if this press starts the sequence */
            if (s_pad_press == J_UP) s_konami_step = 1;
        }
    }
    if (s_pad_press != 0 && s_konami_step == 0) {
        music_play(MUSIC_COIN);
        s_looped = 0;
        s_zombie_mode = 0;
        s_invincible  = 0;
        game_init();
        enter_game(0);
    }
}

/* -----------------------------------------------------------------------
   SELECT
   ----------------------------------------------------------------------- */

/* DEBUG: Level select - accessible via Konami code only in release */
static void enter_select(void)
{
    u8 i;
    g_state = STATE_SELECT;
    s_select_cursor = 0;
    for (i = 0; i < 64; i++) UnsetSprite(i);
    ClearScreen(SCR_1_PLANE);
    ClearScreen(SCR_2_PLANE);
    SysSetSystemFont();
    tiles_install();  /* ensure palettes installed for select screen */
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
    u8 player_px;
    u8 target;
    g_state              = STATE_GAME;
    g_level              = level;
    s_scroll_px          = 0;
    s_zone               = 0;
    s_key_collected      = s_key_done[level];
    /* treasure_collected state handled per-cell via s_treas_done bitmask */
    s_player_tx          = 1;
    s_player_ty          = 7;
    s_player_facing      = 0;
    s_walk_frame         = 0;
    s_move_dir           = 0;
    s_move_timer         = 0;
    s_reloading          = 0;
    for (i = 0; i < 3; i++) { s_blt_active[i] = 0; }

    for (i = 0; i < 64; i++) UnsetSprite(i);

    ClearScreen(SCR_1_PLANE);
    ClearScreen(SCR_2_PLANE);
    SysSetSystemFont();
    tiles_install();

    /* Snap scroll to player */
    player_px = (u8)(s_player_tx * 16);
    if (player_px > (u8)(80)) {
        target = (u8)(player_px - 80);
    } else {
        target = 0;
    }
    if (target > SCROLL_MAX) target = SCROLL_MAX;
    s_scroll_px = target;
    SCR1_X = s_scroll_px;

    if (s_zombie_mode) { palettes_set_greyscale(); } else { maze_set_wall_palette(level); }
    maze_draw_zone(level, s_zone);
    redraw_collected();
    entities_init(level, s_zone);
    hud_update();
    hud_draw_bottom();
    draw_player();
}

static void hud_update(void)
{
    char buf[6];

    /* T:XX - level number 1-16 */
    {
        u8 lnum;
        lnum = (u8)(1 + g_level + (u8)(s_looped ? 8u : 0u));
        buf[0] = 'T'; buf[1] = ':';
        if (lnum >= 10) {
            buf[2] = '1';
            buf[3] = (u8)('0' + lnum - 10);
            buf[4] = 0;
        } else {
            buf[2] = (u8)('0' + lnum);
            buf[3] = 0;
        }
        PrintString(SCR_2_PLANE, P_HUD, 0, 0, buf);
    }

    /* 4-digit score (cols 4-7) */
    buf[0] = (u8)('0' + (s_score / 1000)  % 10);
    buf[1] = (u8)('0' + (s_score / 100)   % 10);
    buf[2] = (u8)('0' + (s_score / 10)    % 10);
    buf[3] = (u8)('0' +  s_score          % 10);
    buf[4] = 0;
    PrintString(SCR_2_PLANE, P_HUD, 4, 0, buf);

    /* K indicator (col 9) */
    if (s_key_collected) {
        PrintString(SCR_2_PLANE, P_HUD, 9, 0, "K");
    } else {
        PrintString(SCR_2_PLANE, P_HUD, 9, 0, " ");
    }
}

/* Bottom HUD row (tile row 18 = pixel row 144): bullets + lives
   6 bullet slots (left side) + up to 3 hat icons (right side)
   Each icon is 8x8 = 1 tile wide, drawn on SCR_2_PLANE row 18 */
static void hud_draw_bottom(void)
{
    u8 i;
    u16 btile;

    /* Lives as hats: cols 11-13 (space at col 10) */
    for (i = 0; i < MAX_LIVES; i++) {
        if (i < s_lives) {
            PutTile(SCR_2_PLANE, P_HUD, (u8)(11 + i), 0, T_HAT_HUD);
        } else {
            PutTile(SCR_2_PLANE, P_HUD, (u8)(11 + i), 0, 0);
        }
    }

    if (s_zombie_mode) {
        {
            char tbuf[4];
            if (s_invincible) {
                PrintString(SCR_2_PLANE, P_HUD, 13, 0, "INV  ");
            } else {
                tbuf[0] = (u8)('0' + s_zombie_timer / 10);
                tbuf[1] = (u8)('0' + s_zombie_timer % 10);
                tbuf[2] = 0;
                PrintString(SCR_2_PLANE, P_HUD, 13, 0, "T:");
                PrintString(SCR_2_PLANE, P_HUD, 15, 0, tbuf);
            }
        }
        return;
    }
    /* 6 bullet icons: cols 14-19 */
    for (i = 0; i < MAX_BULLETS; i++) {
        if (i < s_bullets) {
            btile = T_BULLET_HUD;
        } else {
            btile = T_BULLET_FLY;
        }
        PutTile(SCR_2_PLANE, P_HUD, (u8)(14 + i), 0, btile);
    }
}

static void redraw_collected(void)
{
    u8 rr;
    u8 cc;
    u8 ti;
    if (s_key_done[g_level]) {
        for (rr = 0; rr < MAZE_ROWS; rr++) {
            for (cc = 0; cc < MAZE_COLS; cc++) {
                if (maze_cell_get_zone(g_level, s_zone, cc, rr) == CELL_KEY) {
                    maze_draw_cell_as(cc, rr, CELL_FLOOR);
                }
            }
        }
    }
    ti = 0;
    for (; ti < s_treas_count[g_level]; ti++) {
        if ((s_treas_done[g_level] & (u8)(1 << ti)) &&
            s_treas_zone[g_level][ti] == s_zone) {
            cc = s_treas_tx[g_level][ti];
            rr = s_treas_ty[g_level][ti];
            maze_draw_cell_as(cc, rr, CELL_FLOOR);
        }
    }
}

static void update_game(void)
{
    u8 new_tx;
    u8 new_ty;
    u8 cell;

    /* Sync player position for entity AI */
    g_player_tx = s_player_tx;
    g_player_ty = s_player_ty;

    /* Zombie mode: countdown timer (frozen when invincible) */
    if (s_zombie_mode && !s_invincible) {
        s_zombie_frames++;
        if (s_zombie_frames >= 60) {
            s_zombie_frames = 0;
            if (s_zombie_timer > 0) {
                s_zombie_timer--;
                hud_draw_bottom();
            }
            if (s_zombie_timer == 0) {
                sfx_play(SFX_PLAYER_DIE);
                if (s_lives > 0) {
                    s_lives--;
                    s_zombie_timer  = 60;
                    s_zombie_frames = 0;
                    s_player_tx     = 1;
                    s_player_ty     = 7;
                    s_key_collected = 0;
                    draw_player();
                    hud_update();
                    hud_draw_bottom();
                } else {
                    if (s_score > s_hi_score) { s_hi_score = s_score; save_hi_score(); }
                    s_lives     = MAX_LIVES;
                    s_score     = 0;
                    s_zombie_mode = 0;
                    enter_title();
                }
                return;
            }
        }
    }

    /* Animate generators and enemies */
    update_generators();
    entities_update();
    entities_draw(s_scroll_px);

    /* Check enemy walked onto player this frame */
    if (entity_at(s_player_tx, s_player_ty) != 255) {
        u8 hit2;
        hit2 = entity_at(s_player_tx, s_player_ty);
        if (s_zombie_mode) {
            /* Zombie: touch enemy to kill it */
            entity_kill(hit2);
            s_score = (u16)(s_score + 200);
            sfx_play(SFX_KEY_UNLOCK);
            hud_update();
        } else {
            entity_kill(hit2);
            sfx_play(SFX_PLAYER_DIE);
            if (s_lives > 0) {
                s_lives--;
                s_bullets   = MAX_BULLETS;
                s_zone      = 0;
                s_player_tx = 1;
                s_player_ty = 7;
                s_scroll_px = 0;
                SCR1_X = 0;
                ClearScreen(SCR_1_PLANE);
                SysSetSystemFont();
                tiles_install();
                if (s_zombie_mode) { palettes_set_greyscale(); } else { maze_set_wall_palette(g_level); }
                maze_draw_zone(g_level, 0);
                redraw_collected();
                entities_init(g_level, 0);
                hud_update();
                hud_draw_bottom();
                draw_player();
                return;
            } else {
                if (s_score > s_hi_score) { s_hi_score = s_score; save_hi_score(); }
                s_lives = MAX_LIVES;
                s_score = 0;
                enter_title();
                return;
            }
        }
    }

    /* Animate bullets */
    update_bullets();
    draw_bullets();

    /* Option: in zombie mode toggles invincibility, otherwise exits */
    if (s_pad_press & J_OPTION) {
        if (s_zombie_mode) {
            s_invincible = (u8)(1 - s_invincible);
        } else {
            enter_title(); return;
        }
    }
    if (!s_zombie_mode && (s_pad_press & J_B)) {
        /* B = reload */
        if (s_bullets == 0) {
            s_bullets   = MAX_BULLETS;
            s_reloading = 0;
            sfx_play(SFX_RELOAD);
            hud_draw_bottom();
        }
        return;
    }

    /* A = fire (disabled in zombie mode) */
    if (!s_zombie_mode && (s_pad_press & J_A)) {
        if (s_bullets > 0 && !s_reloading) {
            fire_bullet();
            s_bullets--;
            sfx_play(SFX_SHOOT);
            hud_draw_bottom();
        }
        return;
    }

    /* Movement repeat */
    new_tx = s_player_tx;
    new_ty = s_player_ty;

    if      (s_pad_cur & J_UP)    { s_move_dir = 1; }
    else if (s_pad_cur & J_DOWN)  { s_move_dir = 2; }
    else if (s_pad_cur & J_LEFT)  { s_move_dir = 3; }
    else if (s_pad_cur & J_RIGHT) { s_move_dir = 4; }
    else                          { s_move_dir = 0; s_move_timer = 0; }

    {
        u8 do_move;
        do_move = 0;
        if (s_move_dir && (s_pad_press & (J_UP|J_DOWN|J_LEFT|J_RIGHT))) {
            do_move = 1;
            s_move_timer = 12;
        } else if (s_move_dir) {
            if (s_move_timer > 0) {
                s_move_timer--;
            } else {
                do_move = 1;
                s_move_timer = 5;
            }
        }
        if (!do_move) { return; }
    }

    if (s_move_dir == 1 && new_ty > 0)           new_ty--;
    if (s_move_dir == 2 && new_ty < MAZE_ROWS-1) new_ty++;
    if (s_move_dir == 3) { s_player_facing = 1; if (new_tx > 0)           new_tx--; }
    if (s_move_dir == 4) { s_player_facing = 0; if (new_tx < MAZE_COLS-1) new_tx++; }

    if (new_tx == s_player_tx && new_ty == s_player_ty) {
        return;
    }

    cell = maze_cell_get_zone(g_level, s_zone, new_tx, new_ty);

    if (cell == CELL_KEY      && s_key_done[g_level])      cell = CELL_FLOOR;
    /* Treat collected treasures as floor */
    if (cell == CELL_TREASURE) {
        u8 ti;
        ti = 0;
        for (; ti < s_treas_count[g_level]; ti++) {
            if (s_treas_tx[g_level][ti] == new_tx &&
                s_treas_ty[g_level][ti] == new_ty &&
                (s_treas_done[g_level] & (u8)(1 << ti))) {
                cell = CELL_FLOOR;
            }
        }
    }

    if (cell == CELL_TELEPORT) {
        u8 player_px;
        u8 target;
        if (s_zone == 0) {
            s_zone      = 1;
            s_player_tx = 1;
            s_player_ty = 7;
        } else {
            s_zone      = 0;
            s_player_tx = 13;
            s_player_ty = 7;
        }
        player_px = (u8)(s_player_tx * 16);
        if (player_px > (u8)(80)) {
            target = (u8)(player_px - 80);
        } else {
            target = 0;
        }
        if (target > SCROLL_MAX) target = SCROLL_MAX;
        s_scroll_px = target;
        sfx_play(SFX_TELEPORT);
        SCR1_X = s_scroll_px;
        ClearScreen(SCR_1_PLANE);
        ClearScreen(SCR_2_PLANE);
        SysSetSystemFont();
        tiles_install();
        if (s_zombie_mode) { palettes_set_greyscale(); } else { maze_set_wall_palette(g_level); }
        maze_draw_zone(g_level, s_zone);
        redraw_collected();
        entities_init(g_level, s_zone);
        hud_update();
        hud_draw_bottom();
        draw_player();
        return;
    }

    if (cell == CELL_KEY || cell == CELL_TREASURE || cell == CELL_DOOR) {
        if (cell == CELL_KEY && !s_key_collected) {
            s_key_collected     = 1;
            s_key_done[g_level] = 1;
            maze_draw_cell_as(new_tx, new_ty, CELL_FLOOR);
            s_score = (u16)(s_score + 100);
            hud_update();
        } else if (cell == CELL_TREASURE) {
            {
                u8 ti;
                ti = 0;
                for (; ti < s_treas_count[g_level]; ti++) {
                    if (s_treas_tx[g_level][ti]   == new_tx &&
                        s_treas_ty[g_level][ti]   == new_ty &&
                        s_treas_zone[g_level][ti] == s_zone &&
                        !(s_treas_done[g_level] & (u8)(1 << ti))) {
                        s_treas_done[g_level] |= (u8)(1 << ti);
                    }
                }
            }
            maze_draw_cell_as(new_tx, new_ty, CELL_FLOOR);
            s_score = (u16)(s_score + 500);
            if (s_zombie_mode && s_zombie_timer < 55) {
                s_zombie_timer = (u8)(s_zombie_timer + 5);  /* +5 sec bonus */
            }
            hud_update();
        } else if (cell == CELL_DOOR) {
            if (!s_key_collected) {
                return;
            }
            s_score = (u16)(s_score + 1000);
            sfx_play(SFX_TOMB_ENTER);
            music_play(MUSIC_STAGE_CLEAR);
            if (s_zombie_mode && s_zombie_timer <= 240) {
                s_zombie_timer = (u8)(s_zombie_timer + 15);
            }
            if (g_level >= MAZE_LEVELS - 1) {
                enter_intermission();
            } else {
                enter_game((u8)(g_level + 1));
            }
            return;
        }
        return;
    }

    if (cell == CELL_WALL) {
        return;
    }

    s_player_tx  = new_tx;
    s_player_ty  = new_ty;
    s_walk_frame = (u8)(1 - s_walk_frame);

    /* Camera snap */
    {
        u8 player_px;
        u8 target;
        player_px = (u8)(s_player_tx * 16);
        if (player_px > (u8)(80)) {
            target = (u8)(player_px - 80);
        } else {
            target = 0;
        }
        if (target > SCROLL_MAX) target = SCROLL_MAX;
        s_scroll_px = target;
        SCR1_X = s_scroll_px;
    }

    /* Player-enemy collision check */
    {
        u8 hit;
        hit = entity_at(s_player_tx, s_player_ty);
        if (hit != 255) {
            if (s_zombie_mode) {
                /* Zombie: kill enemy, score, never die */
                entity_kill(hit);
                s_score = (u16)(s_score + 200);
                sfx_play(SFX_KEY_UNLOCK);
                hud_update();
            } else if (s_invincible) {
                /* Invincible debug: bounce enemy away, no death */
                entity_kill(hit);
            } else {
                if (s_lives > 0) {
                    s_lives--;
                    s_bullets = MAX_BULLETS;
                    s_zone      = 0;
                    s_player_tx = 1;
                    s_player_ty = 7;
                    s_scroll_px = 0;
                    SCR1_X = 0;
                    ClearScreen(SCR_1_PLANE);
                    SysSetSystemFont();
                    tiles_install();
                    maze_set_wall_palette(g_level);
                    maze_draw_zone(g_level, 0);
                    redraw_collected();
                    entities_init(g_level, 0);
                    hud_update();
                    hud_draw_bottom();
                    draw_player();
                    return;
                } else {
                    if (s_score > s_hi_score) { s_hi_score = s_score; save_hi_score(); }
                    s_lives = MAX_LIVES;
                    s_score = 0;
                    enter_title();
                    return;
                }
            }
        }
    }

    draw_player();
}

/* -----------------------------------------------------------------------
   SCROLL TEST
   ----------------------------------------------------------------------- */

static void enter_scroll_test(void)
{
    u8 i;
    g_state     = STATE_SCROLL;
    s_scroll_px = 0;

    for (i = 0; i < 64; i++) UnsetSprite(i);

    ClearScreen(SCR_1_PLANE);
    ClearScreen(SCR_2_PLANE);
    SysSetSystemFont();
    tiles_install();

    maze_draw(0);
    maze_set_wall_palette(0);

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
        buf[4] = (u8)('0' + (s_scroll_px / 100));
        buf[5] = (u8)('0' + ((s_scroll_px / 10) % 10));
        buf[6] = (u8)('0' + (s_scroll_px % 10));
        buf[7] = 0;
        PrintString(SCR_2_PLANE, P_HUD, 12, 0, buf);
    }

    if (s_pad_press & J_B) {
        enter_title();
    }
}

/* -----------------------------------------------------------------------
   Generator smoke animation
   ----------------------------------------------------------------------- */

static void update_generators(void)
{
    u8 row;
    u8 col;
    u16 smoke_tile;
    u8 frame;
    u8 tc;
    u8 tr;

    s_smoke_tick++;
    if (s_smoke_tick >= 24) s_smoke_tick = 0;
    frame = (u8)(s_smoke_tick / 8);
    if (frame == 0)      smoke_tile = T_SMOKE_A_TL;
    else if (frame == 1) smoke_tile = T_SMOKE_B_TL;
    else                 smoke_tile = T_SMOKE_C_TL;

    for (row = 0; row < MAZE_ROWS; row++) {
        for (col = 0; col < MAZE_COLS; col++) {
            if (maze_cell_raw(g_level, s_zone, col, row) == CELL_GENERATOR) {
                tc = (u8)(col * 2);
                tr = (u8)(row * 2 + 1);
                PutTile(SCR_1_PLANE, P_TELEPORT, tc,   tr,   smoke_tile);
                PutTile(SCR_1_PLANE, P_TELEPORT, tc+1, tr,   (u16)(smoke_tile+1));
                PutTile(SCR_1_PLANE, P_TELEPORT, tc,   tr+1, (u16)(smoke_tile+2));
                PutTile(SCR_1_PLANE, P_TELEPORT, tc+1, tr+1, (u16)(smoke_tile+3));
            }
        }
    }
}

/* -----------------------------------------------------------------------
   Bullets - 3 simultaneous max, tile-based movement
   Sprite slots 4-15 (3 bullets x 4 sprites each)
   ----------------------------------------------------------------------- */

static void fire_bullet(void)
{
    u8 i;
    for (i = 0; i < 3; i++) {
        if (!s_blt_active[i]) {
            s_blt_active[i] = 1;
            s_blt_tx[i]     = s_player_tx;
            s_blt_ty[i]     = s_player_ty;
            s_blt_dir[i]    = s_player_facing;
            return;
        }
    }
}

static void update_bullets(void)
{
    u8 i;
    u8 base;
    u8 nx;

    /* Move bullets every 4 frames */
    s_blt_timer++;
    if (s_blt_timer < 4) {
        draw_bullets();
        return;
    }
    s_blt_timer = 0;

    for (i = 0; i < 3; i++) {
        if (!s_blt_active[i]) continue;

        /* Move bullet one cell in facing direction */
        nx = s_blt_tx[i];
        if (s_blt_dir[i] == 0) {
            if (nx < MAZE_COLS-1) nx++;
            else { s_blt_active[i] = 0; base = (u8)(4 + i*4); UnsetSprite(base); UnsetSprite((u8)(base+1)); UnsetSprite((u8)(base+2)); UnsetSprite((u8)(base+3)); continue; }
        } else {
            if (nx > 0) nx--;
            else { s_blt_active[i] = 0; base = (u8)(4 + i*4); UnsetSprite(base); UnsetSprite((u8)(base+1)); UnsetSprite((u8)(base+2)); UnsetSprite((u8)(base+3)); continue; }
        }

        /* Hit enemy? */
        {
            u8 hit;
            hit = entity_at(nx, s_blt_ty[i]);
            if (hit != 255) {
                entity_kill(hit);
                s_score = (u16)(s_score + 200);
                hud_update();
                s_blt_active[i] = 0;
                base = (u8)(4 + i*4);
                UnsetSprite(base); UnsetSprite((u8)(base+1));
                UnsetSprite((u8)(base+2)); UnsetSprite((u8)(base+3));
                continue;
            }
        }

        /* Hit wall? */
        if (maze_cell_get_zone(g_level, s_zone, nx, s_blt_ty[i]) == CELL_WALL) {
            s_blt_active[i] = 0;
            base = (u8)(4 + i*4);
            UnsetSprite(base); UnsetSprite((u8)(base+1));
            UnsetSprite((u8)(base+2)); UnsetSprite((u8)(base+3));
            continue;
        }

        s_blt_tx[i] = nx;
    }
}

static void draw_bullets(void)
{
    u8 i;
    u8 base;
    u8 sx;
    u8 sy;

    for (i = 0; i < 3; i++) {
        base = (u8)(4 + i*4);
        if (!s_blt_active[i]) {
            UnsetSprite(base); UnsetSprite((u8)(base+1));
            UnsetSprite((u8)(base+2)); UnsetSprite((u8)(base+3));
            continue;
        }
        sx = (u8)(s_blt_tx[i] * 16);
        if (sx >= s_scroll_px) {
            sx = (u8)(sx - s_scroll_px);
        } else {
            s_blt_active[i] = 0;
            UnsetSprite(base); UnsetSprite((u8)(base+1));
            UnsetSprite((u8)(base+2)); UnsetSprite((u8)(base+3));
            continue;
        }
        sy = (u8)(s_blt_ty[i] * 16 + 8);

        /* Bullet: single 8x8 sprite centred in cell */
        SetSprite(base, T_BULLET_FLY, 0, (u8)(sx+4), (u8)(sy+4), P_BULLET);
        SpriteControl(base, SPR_FRONT, 0);
        UnsetSprite((u8)(base+1));
        UnsetSprite((u8)(base+2));
        UnsetSprite((u8)(base+3));
    }
}

/* -----------------------------------------------------------------------
   Player sprite - 16x16 = 4 x 8x8 tiles, H-flip for left facing
   ----------------------------------------------------------------------- */

static void draw_player(void)
{
    u8 sx;
    u8 sy;
    u8 flip;

    sx = (u8)(s_player_tx * 16);
    if (sx >= s_scroll_px) {
        sx = (u8)(sx - s_scroll_px);
    } else {
        sx = 0;
    }
    sy = (u8)(s_player_ty * 16 + 8);
    flip = s_player_facing;  /* 1=left=hflip */

    {
        u8 tl;
        u8 tr;
        u8 bl;
        u8 br;
        if (s_zombie_mode) {
            tl = T_MUMMY_TL; tr = T_MUMMY_TR;
            bl = T_MUMMY_BL; br = T_MUMMY_BR;
            flip = (u8)(1 - flip);  /* mummy faces left natively, invert */
        } else if (s_walk_frame == 0) {
            tl = T_PLAY_TL;  tr = T_PLAY_TR;
            bl = T_PLAY_BL;  br = T_PLAY_BR;
        } else {
            tl = T_PLAY2_TL; tr = T_PLAY2_TR;
            bl = T_PLAY2_BL; br = T_PLAY2_BR;
        }
        if (flip) {
            SetSprite(0, tr, 0, sx,     sy,     P_PLAYER);
            SetSprite(1, tl, 0, sx + 8, sy,     P_PLAYER);
            SetSprite(2, br, 0, sx,     sy + 8, P_PLAYER);
            SetSprite(3, bl, 0, sx + 8, sy + 8, P_PLAYER);
            SpriteControl(0, SPR_FRONT, SPR_HFLIP);
            SpriteControl(1, SPR_FRONT, SPR_HFLIP);
            SpriteControl(2, SPR_FRONT, SPR_HFLIP);
            SpriteControl(3, SPR_FRONT, SPR_HFLIP);
        } else {
            SetSprite(0, tl, 0, sx,     sy,     P_PLAYER);
            SetSprite(1, tr, 0, sx + 8, sy,     P_PLAYER);
            SetSprite(2, bl, 0, sx,     sy + 8, P_PLAYER);
            SetSprite(3, br, 0, sx + 8, sy + 8, P_PLAYER);
            SpriteControl(0, SPR_FRONT, 0);
            SpriteControl(1, SPR_FRONT, 0);
            SpriteControl(2, SPR_FRONT, 0);
            SpriteControl(3, SPR_FRONT, 0);
        }
    }
}

/* -----------------------------------------------------------------------
   Public entry points called from main.c
   ----------------------------------------------------------------------- */

/* -----------------------------------------------------------------------
   Flash save - hi score persistence
   Magic number validates save data on load
   ----------------------------------------------------------------------- */
#define SAVE_MAGIC 0xA55A

static void save_hi_score(void)
{
    /* TODO: wire to save.c SaveGame() once integrated into build */
    (void)s_hi_score;
}

static void load_hi_score(void)
{
    /* TODO: wire to save.c LoadGame() once integrated into build */
    s_hi_score = 0;
}


/* -----------------------------------------------------------------------
   Zombie mode: switch all palettes to greyscale
   ----------------------------------------------------------------------- */
static void palettes_set_greyscale(void)
{
    /* Scroll plane 1 - walls, keys, treasures etc */
    SetPalette(SCR_1_PLANE, 0, RGB(0,0,0), RGB(5,5,5), RGB(10,10,10), RGB(15,15,15));
    SetPalette(SCR_1_PLANE, 1, RGB(0,0,0), RGB(4,4,4), RGB(9,9,9),  RGB(14,14,14));
    SetPalette(SCR_1_PLANE, 2, RGB(0,0,0), RGB(5,5,5), RGB(10,10,10), RGB(15,15,15));
    SetPalette(SCR_1_PLANE, 3, RGB(0,0,0), RGB(3,3,3), RGB(8,8,8),  RGB(13,13,13));
    SetPalette(SCR_1_PLANE, 4, RGB(0,0,0), RGB(4,4,4), RGB(9,9,9),  RGB(14,14,14));
    SetPalette(SCR_1_PLANE, 5, RGB(0,0,0), RGB(4,4,4), RGB(9,9,9),  RGB(14,14,14));
    SetPalette(SCR_1_PLANE, 6, RGB(0,0,0), RGB(5,5,5), RGB(10,10,10), RGB(15,15,15));
    SetPalette(SCR_1_PLANE, 7, RGB(0,0,0), RGB(4,4,4), RGB(9,9,9),  RGB(14,14,14));
    /* Scroll plane 2 - HUD (keep yellow tint for readability) */
    SetPalette(SCR_2_PLANE, 0, RGB(0,0,0), RGB(5,5,5), RGB(10,10,10), RGB(15,15,15));
    /* Sprite palettes */
    SetPalette(SPRITE_PLANE, 0, 0, RGB(5,5,5), RGB(10,10,10), RGB(15,15,15));
    SetPalette(SPRITE_PLANE, 1, 0, RGB(4,4,4), RGB(9,9,9),  RGB(14,14,14));
    SetPalette(SPRITE_PLANE, 2, 0, RGB(4,4,4), RGB(9,9,9),  RGB(14,14,14));
    SetPalette(SPRITE_PLANE, 3, 0, RGB(5,5,5), RGB(10,10,10), RGB(15,15,15));
}

void game_init(void)
{
    u8 i;
    u8 lv;
    u8 co;
    u8 ro;

    g_state              = STATE_TITLE;
    g_level              = 0;
    s_zone               = 0;
    s_pad_cur            = 0;
    s_pad_prev           = 0;
    s_pad_press          = 0;
    s_scroll_px          = 0;
    s_select_cursor      = 0;
    s_player_tx          = 1;
    s_player_ty          = 7;
    s_player_facing      = 0;
    s_walk_frame         = 0;
    s_key_collected      = 0;
    s_score              = 0;
    s_lives              = MAX_LIVES;
    s_bullets            = MAX_BULLETS;
    s_reloading          = 0;
    s_blt_timer          = 0;
    s_smoke_tick         = 0;
    s_move_dir           = 0;
    s_move_timer         = 0;
    s_konami_step        = 0;
    /* s_looped managed externally - set before game_init on loop */
    s_intro_tick         = 0;
    s_intro_px           = 0;
    s_intro_walk         = 0;
    s_intro_wtimer       = 0;

    for (i = 0; i < MAZE_LEVELS; i++) {
        s_key_done[i]     = 0;
        s_treas_done[i]   = 0;
        s_treas_count[i]  = 0;
        s_treas_zone[i][0] = 0; s_treas_zone[i][1] = 0;
        s_treas_zone[i][2] = 0; s_treas_zone[i][3] = 0;
        s_treas_zone[i][4] = 0; s_treas_zone[i][5] = 0;
        s_treas_zone[i][6] = 0; s_treas_zone[i][7] = 0;
    }
    for (i = 0; i < 3; i++) {
        s_blt_active[i]   = 0;
        s_blt_tx[i]       = 0;
        s_blt_ty[i]       = 0;
        s_blt_dir[i]      = 0;
    }

    /* Scan maps for treasure positions */
    lv = 0;
    for (; lv < MAZE_LEVELS; lv++) {
        s_treas_count[lv] = 0;
        {
            u8 zn;
            zn = 0;
            for (; zn < MAZE_ZONES; zn++) {
                ro = 0;
                for (; ro < MAZE_ROWS; ro++) {
                    co = 0;
                    for (; co < MAZE_COLS; co++) {
                        if (maze_cell_raw(lv, zn, co, ro) == CELL_TREASURE &&
                            s_treas_count[lv] < 8) {
                            s_treas_tx[lv][s_treas_count[lv]]   = co;
                            s_treas_ty[lv][s_treas_count[lv]]   = ro;
                            s_treas_zone[lv][s_treas_count[lv]] = zn;
                            s_treas_count[lv]++;
                        }
                    }
                }
            }
        }
    }

    enter_title();
}

void game_update(void)
{
    s_pad_prev  = s_pad_cur;
    s_pad_cur   = (u8)(JOYPAD & 0x7F);
    s_pad_press = (u8)(s_pad_cur & ~s_pad_prev);

    music_update();
    if      (g_state == STATE_TITLE)       { update_title(); }
    else if (g_state == STATE_SELECT)      { update_select(); }
    else if (g_state == STATE_GAME)        { update_game(); }
    else if (g_state == STATE_SCROLL)      { update_scroll_test(); }
    else if (g_state == STATE_INTERMISSION){ update_intermission(); }
}
