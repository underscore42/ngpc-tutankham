# Tutankham NGPC — Development README
## Studio So Not Kansai

---

## Overview

A port of the 1982 Konami arcade game Tutankham to the Neo Geo Pocket Color.
Platform: NGPC (TLCS-900H CPU, K2GE graphics)
Toolchain: ameliandev SDK / cc900 / tulink under Wine on Linux

---

## Build

```bash
make          # build tutankham.ngp
make clean    # clean build artefacts
```

Runs cc900.exe and tulink.exe via Wine. Output: `build/tutankham.ngp`

Test in Mednafen:
```bash
mednafen build/tutankham.ngp
```

---

## Source Files

| File | Purpose |
|------|---------|
| `src/main.c` | Boot entry, ROM header (must be first in link order) |
| `src/tiles.c` | Tile bank, maze maps, palettes, maze draw functions |
| `src/tiles.h` | Tile IDs, palette slot constants, maze function prototypes |
| `src/game.c` | State machine, player, bullets, HUD, zone transitions |
| `src/game.h` | Game states, input masks, cell types, maze dimensions, externs |
| `src/screen.c` | Title screen bitmap, select menu draw |
| `src/screen.h` | Screen function prototypes |
| `src/entities.c` | Enemy pool, AI (homing/random/patrol), spawn, respawn, draw |
| `src/entities.h` | Enemy type constants, entity state arrays, prototypes |
| `src/sound.c` | Sound effect stubs (7 SFX slots, silent until real data added) |
| `src/sound.h` | SFX ID constants, sfx_play() prototype |
| `src/tutankham.h` | Umbrella include — all TUs include this only |
| `src/carthdr.h` | ROM cartridge header |
| `common/` | Shared NGPC SDK headers (ngpc.h, library.h) |

---

## Architecture

### Game States
```
STATE_TITLE  -> press START -> STATE_GAME (level 0)
             -> Konami code -> STATE_SELECT (debug)
STATE_SELECT -> A button    -> STATE_GAME (chosen level)
             -> B button    -> STATE_TITLE
STATE_GAME   -> door exit   -> STATE_GAME (next level)
             -> all levels  -> victory screen
             -> game over   -> STATE_TITLE
STATE_SCROLL -> test mode   -> STATE_TITLE
```

### Level Structure
- 8 levels × 2 zones each
- Zone A: key + treasure + teleport pad at (14,7) → Zone B
- Zone B: return teleport at (1,7) + locked door at (14,7)
- Player enters each zone at col 1 row 7
- Maze grid: 9 rows × 16 cols, each cell = 16×16px (2×2 scroll tiles)

### Chamber Palettes (arcade-accurate)
| Levels | Chamber | Wall Colour |
|--------|---------|-------------|
| 0-1 | 1 | Blue-grey |
| 2-3 | 2 | Pink-magenta |
| 4-5 | 3 | Amber-gold |
| 6-7 | 4 | Deep blue-purple |

---

## Tile Bank

Game tiles installed at VRAM ID 148 via `InstallTileSetAt`.
Total: **84 tiles × 8 words = 672 words**

| ID Range | Content |
|----------|---------|
| 148-151 | Wall plain TL/TR/BL/BR |
| 152-155 | Player frame 1 TL/TR/BL/BR |
| 156-159 | Player frame 2 TL/TR/BL/BR |
| 160-163 | Alcove TL/TR/BL/BR |
| 164-167 | Smoke frame A TL/TR/BL/BR |
| 168-171 | Smoke frame B TL/TR/BL/BR |
| 172-175 | Smoke frame C TL/TR/BL/BR |
| 176-179 | Hieroglyph wall 1 (bird) TL/TR/BL/BR |
| 180-183 | Hieroglyph wall 2 (eye) TL/TR/BL/BR |
| 184-187 | Key TL/TR/BL/BR |
| 188-191 | Door TL/TR/BL/BR |
| 192-195 | Diamond TL/TR/BL/BR |
| 196-199 | Ruby TL/TR/BL/BR |
| 200-203 | Scarab/beetle TL/TR/BL/BR |
| 204-207 | Tut mask 16×16 TL/TR/BL/BR |
| 208 | Teleport pad (smoke frame 5 scattered) |
| 209 | Bullet in-flight (8×8 orange streak) |
| 210 | Bullet HUD icon (8×8 blue dot) |
| 211 | Hat HUD icon (8×8) |
| 212-215 | Scorpion TL/TR/BL/BR |
| 216-219 | Snake TL/TR/BL/BR |
| 220-223 | Bug TL/TR/BL/BR |
| 224-227 | Mummy TL/TR/BL/BR |
| 228-231 | Bird TL/TR/BL/BR |

Title screen bitmap: tiles 300-519 (220 tiles, installed separately)

32×32 Tut mask: `tut_mask_32[16][8]` in tiles.c (victory screen)

---

## Palette Layout

### SCR_1 Scroll Plane (maze)
| Slot | Name | Purpose |
|------|------|---------|
| 0 | P_WALL | Chamber wall — varies per level |
| 1 | P_LOCK | Hieroglyph wall detail |
| 2 | P_KEY | Key gold |
| 3 | P_TELEPORT | Teleport pad blue glow |
| 4 | P_TREAS_DIAMOND | Diamond cyan |
| 5 | P_TREAS_RUBY | Ruby red |
| 6 | P_TREAS_BEETLE | Scarab gold |
| 7 | P_TITLE | Title screen bitmap |

### SCR_2 Scroll Plane (HUD overlay)
| Slot | Name | Purpose |
|------|------|---------|
| 0 | P_HUD | HUD text yellow |

### Sprite Plane
| Slot | Name | Purpose |
|------|------|---------|
| 0 | P_PLAYER | Explorer brown/gold |
| 1 | P_BULLET | Bullet orange/yellow |
| 2 | P_ENEMY_A | Scorpion + bug green |
| 3 | P_ENEMY_B | Mummy + bird grey/white |

---

## Sprite Slots

| Slots | Content |
|-------|---------|
| 0-3 | Player (4 tiles, 2-frame walk animation) |
| 4-15 | Bullets (3 × 4 sprites) |
| 16-31 | Enemies (4 enemies × 4 sprites each) |

---

## HUD Layout (SCR_2 row 0, 20 cols)

```
T:X SCORE K HHH BBBBBB
0   4     9 11  14
```
- `T:X` cols 0-2 — tomb/level number
- Score cols 4-7 — 4-digit score
- `K` col 9 — key collected indicator
- `HHH` cols 11-13 — hat life icons (T_HAT_HUD tile)
- `BBBBBB` cols 14-19 — bullet icons (T_BULLET_HUD / T_BULLET_FLY)

---

## Enemy Types

| Type | Constant | Palette | AI | Appears |
|------|----------|---------|-----|---------|
| Scorpion | ENT_SCORPION | P_ENEMY_A | Patrol + random turns | All levels |
| Snake | ENT_SNAKE | P_BULLET | Patrol + random turns | All levels |
| Bug | ENT_BUG | P_ENEMY_A | Patrol + random turns | All levels |
| Bird | ENT_BIRD | P_ENEMY_B | Fully random direction | All levels |
| Mummy | ENT_MUMMY | P_ENEMY_B | Homes toward player | Level 4+ |

Enemies spawn from `CELL_GENERATOR` cells, respawn 180 frames (~3s) after being killed.
Max 4 enemies active simultaneously.

---

## Cell Types

| Value | Constant | Description |
|-------|----------|-------------|
| 0 | CELL_FLOOR | Passable floor |
| 1 | CELL_WALL | Solid wall |
| 2 | CELL_KEY | Key pickup (+100 pts) |
| 3 | CELL_DOOR | Exit door (requires key) |
| 4 | CELL_TREASURE | Treasure (+500 pts) |
| 5 | CELL_GENERATOR | Enemy spawn point |
| 6 | CELL_TELEPORT | Zone teleport |

---

## Scoring

| Event | Points |
|-------|--------|
| Key collected | 100 |
| Treasure collected | 500 |
| Enemy killed | 200 |
| Level completed | 1000 |

Hi-score persists across games within a session (no flash save yet).

---

## Sound Effect IDs

All currently silent stubs. Replace `SOUNDEFFECT` entries in `sound.c`.

| ID | Constant | Trigger |
|----|----------|---------|
| 1 | SFX_SHOOT | Fire bullet |
| 2 | SFX_RELOAD | Reload (only when empty) |
| 3 | SFX_ENEMY_SPAWN | Enemy respawn |
| 4 | SFX_TELEPORT | Zone teleport |
| 5 | SFX_KEY_UNLOCK | Key collected |
| 6 | SFX_TOMB_ENTER | Level exit / door |
| 7 | SFX_PLAYER_DIE | Player death |

---

## cc900 C89 Rules (Critical)

- All variable declarations **before** any statements in every block
- No `//` comments — use `/* */` only
- No C99 features (no `for (int i=...`)
- No `volatile u16*` — use library functions
- No signed multiply or divide
- No static local variables
- Sprite tile IDs must be ≤ 255
- `const` arrays **must** be `const` — non-const globals not ROM-initialised
- Maze map must be `const` — access only via `maze_cell_get_zone()` / `maze_cell_raw()`
- SCR_2_PLANE colour 0 is opaque black — use only rows with content
- Separate scroll plane tile banks: `InstallTileSetAt` fills both planes
