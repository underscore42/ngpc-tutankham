#ifndef TILES_H
#define TILES_H

#include "ngpc.h"

/* -----------------------------------------------------------------------
   Tile IDs  (game_tiles starts at VRAM tile 148)
   ----------------------------------------------------------------------- */
#define T_WALL_TL              148
#define T_WALL_TR              149
#define T_WALL_BL              150
#define T_WALL_BR              151
#define T_PLAY_TL              152
#define T_PLAY_TR              153
#define T_PLAY_BL              154
#define T_PLAY_BR              155
#define T_PLAY2_TL             156
#define T_PLAY2_TR             157
#define T_PLAY2_BL             158
#define T_PLAY2_BR             159
#define T_ALCOVE_TL            160
#define T_ALCOVE_TR            161
#define T_ALCOVE_BL            162
#define T_ALCOVE_BR            163
#define T_SMOKE_A_TL           164
#define T_SMOKE_A_TR           165
#define T_SMOKE_A_BL           166
#define T_SMOKE_A_BR           167
#define T_SMOKE_B_TL           168
#define T_SMOKE_B_TR           169
#define T_SMOKE_B_BL           170
#define T_SMOKE_B_BR           171
#define T_SMOKE_C_TL           172
#define T_SMOKE_C_TR           173
#define T_SMOKE_C_BL           174
#define T_SMOKE_C_BR           175
#define T_HIERO1_TL            176
#define T_HIERO1_TR            177
#define T_HIERO1_BL            178
#define T_HIERO1_BR            179
#define T_HIERO2_TL            180
#define T_HIERO2_TR            181
#define T_HIERO2_BL            182
#define T_HIERO2_BR            183
#define T_KEY_TL               184
#define T_KEY_TR               185
#define T_KEY_BL               186
#define T_KEY_BR               187
#define T_DOOR_TL              188
#define T_DOOR_TR              189
#define T_DOOR_BL              190
#define T_DOOR_BR              191
#define T_DIAMOND_TL           192
#define T_DIAMOND_TR           193
#define T_DIAMOND_BL           194
#define T_DIAMOND_BR           195
#define T_RUBY_TL              196
#define T_RUBY_TR              197
#define T_RUBY_BL              198
#define T_RUBY_BR              199
#define T_SCARAB_TL            200
#define T_SCARAB_TR            201
#define T_SCARAB_BL            202
#define T_SCARAB_BR            203
#define T_TUTMASK_TL           204
#define T_TUTMASK_TR           205
#define T_TUTMASK_BL           206
#define T_TUTMASK_BR           207
#define T_TELEPORT             208
#define T_BULLET_FLY           209
#define T_BULLET_HUD           210
#define T_HAT_HUD              211
#define T_SCORPION_TL          212
#define T_SCORPION_TR          213
#define T_SCORPION_BL          214
#define T_SCORPION_BR          215
#define T_SNAKE_TL             216
#define T_SNAKE_TR             217
#define T_SNAKE_BL             218
#define T_SNAKE_BR             219
#define T_BUG_TL               220
#define T_BUG_TR               221
#define T_BUG_BL               222
#define T_BUG_BR               223
#define T_MUMMY_TL             224
#define T_MUMMY_TR             225
#define T_MUMMY_BL             226
#define T_MUMMY_BR             227
#define T_BIRD_TL              228
#define T_BIRD_TR              229
#define T_BIRD_BL              230
#define T_BIRD_BR              231

/* Convenience aliases */
#define T_WALL           T_WALL_TL
#define T_KEY            T_KEY_TL
#define T_LOCK           T_DOOR_TL
#define T_ALCOVE_EMPTY   T_ALCOVE_TL
#define T_ALCOVE_KEY     T_ALCOVE_TR
#define T_ALCOVE_TREAS   T_ALCOVE_BL

/* -----------------------------------------------------------------------
   SCR_1 scroll plane palette slots
   ----------------------------------------------------------------------- */
#define P_WALL           0   /* chamber wall colour (varies per level) */
#define P_LOCK           1   /* hieroglyph walls */
#define P_KEY            2   /* key gold */
#define P_TELEPORT       3   /* teleport pad / smoke blue */
#define P_TREAS_DIAMOND  4   /* diamond cyan */
#define P_TREAS_RUBY     5   /* ruby red */
#define P_TREAS_BEETLE   6   /* scarab/beetle gold */
#define P_TITLE          7   /* title screen bitmap */

/* SCR_2 scroll plane palette slots */
#define P_HUD            0   /* HUD text yellow */

/* -----------------------------------------------------------------------
   Sprite palettes (4 slots: 0-3)
   ----------------------------------------------------------------------- */
#define P_PLAYER         0   /* orange/brown explorer */
#define P_BULLET         1   /* yellow/orange bullet */
#define P_ENEMY_A        2   /* scorpion, bug */
#define P_ENEMY_B        3   /* mummy, bird */

/* Enemy palette aliases */
#define P_ENEMY_SCORPION P_ENEMY_A
#define P_ENEMY_BUG      P_ENEMY_A
#define P_ENEMY_SNAKE    P_BULLET
#define P_ENEMY_BIRD     P_ENEMY_B
#define P_ENEMY_MUMMY    P_ENEMY_B

/* -----------------------------------------------------------------------
   Functions
   ----------------------------------------------------------------------- */
void tiles_install(void);
void maze_draw(u8 level);
void maze_set_wall_palette(u8 level);
u8   maze_cell_get(u8 level, u8 col, u8 row);
void maze_draw_cell(u8 level, u8 col, u8 row);
void maze_draw_cell_as(u8 col, u8 row, u8 cell);
u8   maze_cell_get_zone(u8 level, u8 zone, u8 col, u8 row);
u8   maze_cell_raw(u8 level, u8 zone, u8 col, u8 row);
void maze_draw_zone(u8 level, u8 zone);
void maze_draw_cell_zone(u8 level, u8 zone, u8 col, u8 row);
u16  maze_treasure_tile(u8 level);
u8   maze_treasure_pal(u8 level);

#endif
