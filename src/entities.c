#include "tutankham.h"
#include "entities.h"
#include "sound.h"

/* Player position - set by game.c each frame */
u8 g_player_tx;
u8 g_player_ty;

u8 ent_type[MAX_ENEMIES];
u8 ent_tx[MAX_ENEMIES];
u8 ent_ty[MAX_ENEMIES];
u8 ent_dir[MAX_ENEMIES];
u8 ent_timer[MAX_ENEMIES];
u8 ent_alive[MAX_ENEMIES];
u8 ent_respawn[MAX_ENEMIES];  /* frames until respawn, 0=active */
static u8 s_ent_spawn_tx[MAX_ENEMIES];  /* original spawn position */
static u8 s_ent_spawn_ty[MAX_ENEMIES];
static u8 s_ent_type_orig[MAX_ENEMIES]; /* original type for respawn */
static u8 s_ent_zone;

/* Move speed in frames per step */
static const u8 ent_speed[6] = {0, 18, 22, 10, 14, 28};

static u8 ent_tl(u8 type)
{
    if (type == ENT_SCORPION) return T_SCORPION_TL;
    if (type == ENT_SNAKE)    return T_SNAKE_TL;
    if (type == ENT_BIRD)     return T_BIRD_TL;
    if (type == ENT_BUG)      return T_BUG_TL;
    if (type == ENT_MUMMY)    return T_MUMMY_TL;
    return T_PLAY_TL;
}

static u8 ent_pal(u8 type)
{
    if (type == ENT_SCORPION) return P_ENEMY_A;
    if (type == ENT_BUG)      return P_ENEMY_A;
    if (type == ENT_SNAKE)    return P_BULLET;   /* yellow */
    if (type == ENT_BIRD)     return P_ENEMY_B;
    if (type == ENT_MUMMY)    return P_ENEMY_B;
    return P_ENEMY_A;
}

static u8 pick_enemy_type(u8 level)
{
    u8 r;
    r = (u8)((level * 13 + ent_timer[0]) & 0x0F);
    if (level < 4) {
        if (r < 4)       return ENT_SCORPION;
        else if (r < 8)  return ENT_SNAKE;
        else if (r < 12) return ENT_BIRD;
        else             return ENT_BUG;
    } else {
        if (r < 3)       return ENT_SCORPION;
        else if (r < 6)  return ENT_SNAKE;
        else if (r < 9)  return ENT_BIRD;
        else if (r < 12) return ENT_BUG;
        else             return ENT_MUMMY;
    }
}

void entities_init(u8 level, u8 zone)
{
    u8 i;
    u8 row;
    u8 col;
    u8 count;
    s_ent_zone = zone;

    for (i = 0; i < MAX_ENEMIES; i++) {
        ent_type[i]  = ENT_NONE;
        ent_alive[i] = 0;
        ent_timer[i] = (u8)(i * 15);
    }

    count = 0;
    for (row = 0; row < MAZE_ROWS && count < MAX_ENEMIES; row++) {
        for (col = 0; col < MAZE_COLS && count < MAX_ENEMIES; col++) {
            if (maze_cell_raw(level, zone, col, row) == CELL_GENERATOR) {
                u8 spawn_tx;
                u8 spawn_ty;
                u8 found;
                spawn_tx = col;
                spawn_ty = row;
                found    = 0;
                /* Check each direction independently - reset both coords each time */
                if (!found && col > 0 && maze_cell_get_zone(level, zone, (u8)(col-1), row) == CELL_FLOOR) {
                    spawn_tx = (u8)(col-1); spawn_ty = row; found = 1;
                }
                if (!found && col < MAZE_COLS-1 && maze_cell_get_zone(level, zone, (u8)(col+1), row) == CELL_FLOOR) {
                    spawn_tx = (u8)(col+1); spawn_ty = row; found = 1;
                }
                if (!found && row > 0 && maze_cell_get_zone(level, zone, col, (u8)(row-1)) == CELL_FLOOR) {
                    spawn_tx = col; spawn_ty = (u8)(row-1); found = 1;
                }
                if (!found && row < MAZE_ROWS-1 && maze_cell_get_zone(level, zone, col, (u8)(row+1)) == CELL_FLOOR) {
                    spawn_tx = col; spawn_ty = (u8)(row+1); found = 1;
                }
                if (found) {
                    ent_type[count]       = pick_enemy_type(level);
                    ent_tx[count]         = spawn_tx;
                    ent_ty[count]         = spawn_ty;
                    ent_dir[count]        = DIR_RIGHT;
                    ent_timer[count]      = (u8)(count * 20 + 30);
                    ent_alive[count]      = 1;
                    ent_respawn[count]    = 0;
                    s_ent_spawn_tx[count] = spawn_tx;
                    s_ent_spawn_ty[count] = spawn_ty;
                    s_ent_type_orig[count]= ent_type[count];
                    count++;
                }
            }
        }
    }
}

/* -----------------------------------------------------------------------
   Cheap random - just a counter XORed with level and index
   ----------------------------------------------------------------------- */
static u8 s_rand_tick;

static u8 cheap_rand(u8 i)
{
    s_rand_tick++;
    return (u8)((s_rand_tick ^ (i * 37) ^ g_level) & 0x0F);
}

static u8 can_move(u8 tx, u8 ty)
{
    return (maze_cell_get_zone(g_level, s_ent_zone, tx, ty) == CELL_FLOOR) ? 1 : 0;
}

/* -----------------------------------------------------------------------
   Update - move enemies
   Mummy: homes toward player
   Scorpion/Bug: prefer current dir, random turn at junctions
   Snake: sinuous - prefers horizontal, turns at walls
   Bird: fast, random direction changes
   ----------------------------------------------------------------------- */
void entities_update(void)
{
    u8 i;
    u8 nx;
    u8 ny;
    u8 moved;
    u8 rnd;

    for (i = 0; i < MAX_ENEMIES; i++) {
        /* Respawn countdown */
        if (!ent_alive[i] && ent_respawn[i] > 0) {
            ent_respawn[i]--;
            if (ent_respawn[i] == 0) {
                ent_type[i]  = s_ent_type_orig[i];
                ent_tx[i]    = s_ent_spawn_tx[i];
                ent_ty[i]    = s_ent_spawn_ty[i];
                ent_dir[i]   = DIR_RIGHT;
                ent_alive[i] = 1;
                sfx_play(SFX_ENEMY_SPAWN);
            }
            continue;
        }
        if (!ent_alive[i] || ent_type[i] == ENT_NONE) continue;

        if (ent_timer[i] > 0) { ent_timer[i]--; continue; }
        ent_timer[i] = ent_speed[ent_type[i]];

        nx = ent_tx[i];
        ny = ent_ty[i];
        moved = 0;
        rnd = cheap_rand(i);

        if (ent_type[i] == ENT_MUMMY) {
            /* MUMMY: home toward player, prefer axis with larger distance */
            u8 dx;
            u8 dy;
            u8 px_gt;
            u8 py_gt;
            dx = (g_player_tx > ent_tx[i]) ? (u8)(g_player_tx - ent_tx[i]) : (u8)(ent_tx[i] - g_player_tx);
            dy = (g_player_ty > ent_ty[i]) ? (u8)(g_player_ty - ent_ty[i]) : (u8)(ent_ty[i] - g_player_ty);
            px_gt = (g_player_tx > ent_tx[i]) ? 1 : 0;
            py_gt = (g_player_ty > ent_ty[i]) ? 1 : 0;
            /* Try primary axis (larger distance) first */
            if (dx >= dy) {
                if (px_gt && nx < MAZE_COLS-1 && can_move((u8)(nx+1), ny)) {
                    nx++; ent_dir[i] = DIR_RIGHT; moved = 1;
                } else if (!px_gt && nx > 0 && can_move((u8)(nx-1), ny)) {
                    nx--; ent_dir[i] = DIR_LEFT; moved = 1;
                }
            }
            if (!moved) {
                if (py_gt && ny < MAZE_ROWS-1 && can_move(nx, (u8)(ny+1))) {
                    ny++; ent_dir[i] = DIR_DOWN; moved = 1;
                } else if (!py_gt && ny > 0 && can_move(nx, (u8)(ny-1))) {
                    ny--; ent_dir[i] = DIR_UP; moved = 1;
                }
            }
            /* Fallback: any open direction */
            if (!moved) {
                if (nx > 0 && can_move((u8)(nx-1), ny)) { nx--; ent_dir[i] = DIR_LEFT; moved = 1; }
                else if (nx < MAZE_COLS-1 && can_move((u8)(nx+1), ny)) { nx++; ent_dir[i] = DIR_RIGHT; moved = 1; }
                else if (ny > 0 && can_move(nx, (u8)(ny-1))) { ny--; ent_dir[i] = DIR_UP; moved = 1; }
                else if (ny < MAZE_ROWS-1 && can_move(nx, (u8)(ny+1))) { ny++; ent_dir[i] = DIR_DOWN; moved = 1; }
            }

        } else if (ent_type[i] == ENT_BIRD) {
            /* BIRD: fast random direction changes */
            u8 try_dir;
            try_dir = (u8)(rnd & 3);
            if (try_dir == 0 && nx > 0            && can_move((u8)(nx-1), ny)) { nx--; ent_dir[i] = DIR_LEFT;  moved = 1; }
            else if (try_dir == 1 && nx < MAZE_COLS-1 && can_move((u8)(nx+1), ny)) { nx++; ent_dir[i] = DIR_RIGHT; moved = 1; }
            else if (try_dir == 2 && ny > 0        && can_move(nx, (u8)(ny-1))) { ny--; ent_dir[i] = DIR_UP;    moved = 1; }
            else if (try_dir == 3 && ny < MAZE_ROWS-1 && can_move(nx, (u8)(ny+1))) { ny++; ent_dir[i] = DIR_DOWN;  moved = 1; }
            /* Fallback to current direction */
            if (!moved) {
                if (ent_dir[i] == DIR_RIGHT && nx < MAZE_COLS-1 && can_move((u8)(nx+1), ny)) { nx++; moved = 1; }
                else if (ent_dir[i] == DIR_LEFT  && nx > 0            && can_move((u8)(nx-1), ny)) { nx--; moved = 1; }
                else if (ent_dir[i] == DIR_UP    && ny > 0            && can_move(nx, (u8)(ny-1))) { ny--; moved = 1; }
                else if (ent_dir[i] == DIR_DOWN  && ny < MAZE_ROWS-1  && can_move(nx, (u8)(ny+1))) { ny++; moved = 1; }
            }

        } else {
            /* SCORPION/SNAKE/BUG: prefer current dir, random turn at junctions */
            /* Random direction change (1 in 8 chance) */
            if ((rnd & 7) == 0) {
                ent_dir[i] = (u8)(rnd & 3);
            }
            /* Try current direction */
            if (ent_dir[i] == DIR_RIGHT && nx < MAZE_COLS-1 && can_move((u8)(nx+1), ny)) { nx++; moved = 1; }
            else if (ent_dir[i] == DIR_LEFT  && nx > 0            && can_move((u8)(nx-1), ny)) { nx--; moved = 1; }
            else if (ent_dir[i] == DIR_UP    && ny > 0            && can_move(nx, (u8)(ny-1))) { ny--; moved = 1; }
            else if (ent_dir[i] == DIR_DOWN  && ny < MAZE_ROWS-1  && can_move(nx, (u8)(ny+1))) { ny++; moved = 1; }
            /* Blocked: try other dirs */
            if (!moved) {
                if (nx < MAZE_COLS-1 && can_move((u8)(nx+1), ny)) { nx++; ent_dir[i] = DIR_RIGHT; moved = 1; }
                else if (nx > 0      && can_move((u8)(nx-1), ny)) { nx--; ent_dir[i] = DIR_LEFT;  moved = 1; }
                else if (ny < MAZE_ROWS-1 && can_move(nx, (u8)(ny+1))) { ny++; ent_dir[i] = DIR_DOWN; moved = 1; }
                else if (ny > 0      && can_move(nx, (u8)(ny-1))) { ny--; ent_dir[i] = DIR_UP;   moved = 1; }
            }
        }

        if (moved) {
            ent_tx[i] = nx;
            ent_ty[i] = ny;
        }
    }
}

/* -----------------------------------------------------------------------
   Draw
   ----------------------------------------------------------------------- */
void entities_draw(u8 scroll_px)
{
    u8 i;
    u8 base;
    u8 sx;
    u8 sy;
    u8 flip;

    for (i = 0; i < MAX_ENEMIES; i++) {
        base = (u8)(ENT_SPR_BASE + i * 4);
        if (!ent_alive[i] || ent_type[i] == ENT_NONE) {
            UnsetSprite(base);
            UnsetSprite((u8)(base+1));
            UnsetSprite((u8)(base+2));
            UnsetSprite((u8)(base+3));
            continue;
        }

        sx = (u8)(ent_tx[i] * 16);
        if (sx < scroll_px) {
            UnsetSprite(base);
            UnsetSprite((u8)(base+1));
            UnsetSprite((u8)(base+2));
            UnsetSprite((u8)(base+3));
            continue;
        }
        sx = (u8)(sx - scroll_px);
        sy = (u8)(ent_ty[i] * 16 + 8);

        {
            u8 tl;
            u8 pal;
            tl  = ent_tl(ent_type[i]);
            pal = ent_pal(ent_type[i]);
            {
                /* Bird sprite faces left natively - invert flip logic */
                u8 face_left;
                if (ent_type[i] == ENT_BIRD) {
                    face_left = (ent_dir[i] == DIR_RIGHT) ? 1 : 0;
                } else {
                    face_left = (ent_dir[i] == DIR_LEFT)  ? 1 : 0;
                }
                if (face_left) {
                    SetSprite(base,         (u8)(tl+1), 0, sx,   sy,   pal);
                    SetSprite((u8)(base+1), tl,         0, sx+8, sy,   pal);
                    SetSprite((u8)(base+2), (u8)(tl+3), 0, sx,   sy+8, pal);
                    SetSprite((u8)(base+3), (u8)(tl+2), 0, sx+8, sy+8, pal);
                    SpriteControl(base,         SPR_FRONT, SPR_HFLIP);
                    SpriteControl((u8)(base+1), SPR_FRONT, SPR_HFLIP);
                    SpriteControl((u8)(base+2), SPR_FRONT, SPR_HFLIP);
                    SpriteControl((u8)(base+3), SPR_FRONT, SPR_HFLIP);
                } else {
                    SetSprite(base,         tl,          0, sx,   sy,   pal);
                    SetSprite((u8)(base+1), (u8)(tl+1),  0, sx+8, sy,   pal);
                    SetSprite((u8)(base+2), (u8)(tl+2),  0, sx,   sy+8, pal);
                    SetSprite((u8)(base+3), (u8)(tl+3),  0, sx+8, sy+8, pal);
                    SpriteControl(base,         SPR_FRONT, 0);
                    SpriteControl((u8)(base+1), SPR_FRONT, 0);
                    SpriteControl((u8)(base+2), SPR_FRONT, 0);
                    SpriteControl((u8)(base+3), SPR_FRONT, 0);
                }
            }
        }
    }
}

void entity_kill(u8 idx)
{
    u8 base;
    if (idx >= MAX_ENEMIES) return;
    ent_alive[idx]    = 0;
    ent_type[idx]     = ENT_NONE;
    ent_respawn[idx]  = 180;  /* ~3 seconds at 60fps */
    base = (u8)(ENT_SPR_BASE + idx * 4);
    UnsetSprite(base);
    UnsetSprite((u8)(base+1));
    UnsetSprite((u8)(base+2));
    UnsetSprite((u8)(base+3));
}

u8 entity_at(u8 tx, u8 ty)
{
    u8 i;
    for (i = 0; i < MAX_ENEMIES; i++) {
        if (ent_alive[i] && ent_tx[i] == tx && ent_ty[i] == ty) {
            return i;
        }
    }
    return 255;
}
