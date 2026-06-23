# Tutankham NGPC — Development README
## Studio So Not Kansai

---

## Overview

A port of the 1982 Konami arcade game Tutankham to the Neo Geo Pocket Color.
Platform: NGPC (TLCS-900H CPU, K2GE graphics)
Toolchain: ameliandev SDK / cc900 / tulink under Wine on Linux
Development machine: ThinkPad T440p (gandalf@raiden)
Project path: `~/projects/ngpc/devkit/tutankham/`
Reference project: `~/projects/ngpc/devkit/asteroids-neo/`

---

## Build

```bash
make          # build tutankham.ngp
make clean    # clean build artefacts
```

Runs cc900.exe and tulink.exe via Wine. Makefile uses `*.c` wildcard — all `.c`
files in `src/` are compiled automatically. Output: `build/tutankham.ngp`

Test in Mednafen:
```bash
mednafen build/tutankham.ngp
# Keys: WASD=dpad  Z=A  X=B  Enter=Option
```

Screenshots: F9 → `~/.mednafen/snaps/`

---

## Source Files

| File | Purpose |
|------|---------|
| `src/main.c` | Boot entry, ROM header — **must be first in link order** |
| `src/tiles.c` | Tile bank, maze maps, palettes, maze draw functions |
| `src/tiles.h` | Tile IDs (148–231), palette slot constants, prototypes |
| `src/game.c` | State machine, player, bullets, HUD, zone transitions, zombie mode |
| `src/game.h` | Game states, input masks, cell types, maze dimensions, externs |
| `src/screen.c` | Title screen bitmap draw, level select menu |
| `src/screen.h` | Screen function prototypes |
| `src/entities.c` | Enemy pool, AI (homing/random/patrol), spawn, respawn, draw |
| `src/entities.h` | Enemy type constants, entity state arrays, prototypes |
| `src/sound.c` | 7 SFX slots + music phrase slots (28 total), InstallSounds |
| `src/sound.h` | SFX ID constants, sfx_play() prototype |
| `src/music.c` | Arcade jingle sequencer — coin, gate open, stage clear |
| `src/music.h` | MusicStep struct, MUSIC_* constants, music_play/stop/update |
| `src/tutankham.h` | Umbrella include — all TUs include this only |
| `src/carthdr.h` | ROM cartridge header |
| `common/` | Shared NGPC SDK headers (ngpc.h, library.h, core/rom/flash.h) |

---

## Architecture

### Game States

```
STATE_TITLE       -> PRESS START        -> STATE_GAME (level 0, normal mode)
                  -> Konami code        -> STATE_GAME (level 0, ZOMBIE MODE)
STATE_SELECT      -> A button           -> STATE_GAME (chosen level)  [debug only]
                  -> B button           -> STATE_TITLE
STATE_GAME        -> door exit          -> STATE_GAME (next level)
                  -> level 8 complete   -> STATE_INTERMISSION
                  -> game over          -> STATE_TITLE
STATE_INTERMISSION-> 600 frames (~10s)  -> STATE_GAME (level 0, loops)
STATE_SCROLL      -> B button           -> STATE_TITLE  [scroll test, debug]
```

### Level Structure
- 8 levels × 2 zones each
- Zone A: key + up to 8 treasures + generators + teleport → Zone B
- Zone B: generators + door exit (requires key from Zone A)
- Player enters Zone A at col 1 row 7; Zone B at col 1 row 7
- Maze grid: 9 rows × 16 cols, each cell = 16×16px (2×2 scroll tiles)
- Scroll offset: maze is 256px wide, screen shows 160px, camera follows player

### Chamber Palettes
| Levels | Chamber | Wall Colour |
|--------|---------|-------------|
| 0–1 | 1 | Blue-grey |
| 2–3 | 2 | Pink-magenta |
| 4–5 | 3 | Amber-gold |
| 6–7 | 4 | Deep blue-purple |

---

## Map Format

Maps live in `tiles.c` as `static const u8 maze_map[8][2][9][16]`.
Generated from a Google Sheet map editor — paste output directly into tiles.c.

```
maze_map[level][zone][row][col]
```

| Value | Cell Type | Notes |
|-------|-----------|-------|
| 0 | CELL_FLOOR | Passable |
| 1 | CELL_WALL | Solid — varies: plain / hiero1 / hiero2 by position |
| 2 | CELL_KEY | One per Zone A — player must collect to unlock door |
| 3 | CELL_DOOR | Zone B only — exit to next level |
| 4 | CELL_TREASURE | Up to 8 per level (tracked by bitmask in s_treas_done) |
| 5 | CELL_GENERATOR | Enemy spawn point — must be on accessible floor cell |
| 6 | CELL_TELEPORT | Zone A→B and B→A warp |

**Generator placement rule:** generators must have at least one adjacent floor cell
or enemies will spawn inside walls and be invisible/inaccessible.

---

## Tile Bank

Game tiles installed at VRAM ID 148 via `InstallTileSetAt(game_tiles, 672, 148)`.
Total: **84 tiles × 8 words = 672 words**. All tile IDs ≤ 255 (hardware limit).

All 16×16 sprites occupy sequential slots: TL=N, TR=N+1, BL=N+2, BR=N+3.

| ID Range | Content |
|----------|---------|
| 148–151 | Wall plain TL/TR/BL/BR |
| 152–155 | Player frame 1 TL/TR/BL/BR |
| 156–159 | Player frame 2 TL/TR/BL/BR |
| 160–163 | Alcove TL/TR/BL/BR |
| 164–167 | Smoke A TL/TR/BL/BR |
| 168–171 | Smoke B TL/TR/BL/BR |
| 172–175 | Smoke C TL/TR/BL/BR |
| 176–179 | Hieroglyph wall 1 (bird) TL/TR/BL/BR |
| 180–183 | Hieroglyph wall 2 (eye) TL/TR/BL/BR |
| 184–187 | Key TL/TR/BL/BR |
| 188–191 | Door TL/TR/BL/BR |
| 192–195 | Diamond TL/TR/BL/BR |
| 196–199 | Ruby TL/TR/BL/BR |
| 200–203 | Scarab/beetle TL/TR/BL/BR |
| 204–207 | Tut mask 16×16 TL/TR/BL/BR |
| 208 | Teleport pad (scattered dots) |
| 209 | Bullet in-flight (8×8 orange streak) |
| 210 | Bullet HUD icon (8×8 blue dot) |
| 211 | Hat HUD icon (8×8) |
| 212–215 | Scorpion TL/TR/BL/BR |
| 216–219 | Snake TL/TR/BL/BR |
| 220–223 | Bug TL/TR/BL/BR |
| 224–227 | Mummy TL/TR/BL/BR |
| 228–231 | Bird TL/TR/BL/BR |

Title screen bitmap: tiles 300–519 (220 tiles, installed separately).
32×32 Tut mask: `tut_mask_32[16][8]` in tiles.c, installed at ID 232 for intermission.

---

## Palette Layout

### SCR_1 Scroll Plane (maze)
| Slot | Name | Purpose |
|------|------|---------|
| 0 | P_WALL | Chamber wall — varies per level pair |
| 1 | P_LOCK | Hieroglyph wall detail |
| 2 | P_KEY | Key gold |
| 3 | P_TELEPORT | Teleport pad / smoke blue |
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
| 1 | P_BULLET | Bullet orange/yellow (also snake) |
| 2 | P_ENEMY_A | Scorpion + bug |
| 3 | P_ENEMY_B | Mummy + bird grey/white |

In zombie mode all palettes switch to greyscale via `palettes_set_greyscale()`.

---

## Sprite Slots

| Slots | Content |
|-------|---------|
| 0–3 | Player (4 tiles, 2-frame walk animation) |
| 4–15 | Bullets (3 × 4 sprites) |
| 16–31 | Enemies (4 enemies × 4 sprites each) |
| 0–3 | Player (intermission — reused) |
| 4–23 | 5 enemies (intermission chase) |

**H-flip rule for 16×16 sprites:** set SPR_HFLIP AND swap TL↔TR, BL↔BR positions.
Bird and mummy sprites face left natively — flip logic inverted for these types.

---

## HUD Layout (SCR_2 row 0)

Normal mode:
```
T:XX  SCORE  K  HHH  BBBBBB
0     4      9  11   14
```
- `T:XX` — level number (T:9+ after first loop)
- Score — 4 digits
- `K` — key collected indicator
- `HHH` — hat life icons
- `BBBBBB` — bullet icons

Zombie mode:
```
T:XX  SCORE  K  HHH  T:SS
                      13
```
- `T:SS` — countdown timer in seconds (replaces bullets)
- `INV` — shown when invincibility toggled on

---

## Enemy Types

| Type | Palette | AI | Notes |
|------|---------|-----|-------|
| Scorpion | P_ENEMY_A | Patrol + random turns | |
| Snake | P_BULLET | Patrol + random turns | |
| Bug | P_ENEMY_A | Patrol + random turns | |
| Bird | P_ENEMY_B | Fully random | Sprite faces left natively — flip inverted |
| Mummy | P_ENEMY_B | Homes toward player | Level 4+; also faces left natively |

Enemies spawn from CELL_GENERATOR cells. Respawn 180 frames after death.
Max 4 enemies active simultaneously.
Spawn fix: generator must have adjacent floor cell — checked at spawn time.

---

## Scoring

| Event | Points |
|-------|--------|
| Key collected | 100 |
| Treasure collected | 500 |
| Enemy killed (bullet) | 200 |
| Enemy killed (zombie touch) | 200 |
| Level completed | 1000 |

Score carries across loop (level 8 → level 9). Resets on fresh game start.
Hi-score persists within session (flash save not yet wired).

---

## Sound System

Uses `InstallSoundDriver()` / `InstallSounds()` / `PlaySound()` from library.c.
Z80 sound driver loaded into Z80 RAM. VBlank pulses Z80 each frame.
**Important:** `sound_install()` called after 2× `WaitVsync()` in main.c to allow
Z80 to stabilise before `InstallSounds()` busy-waits for ack.

### SFX Slots (1–7)
| ID | Constant | Trigger |
|----|----------|---------|
| 1 | SFX_SHOOT | Fire bullet |
| 2 | SFX_RELOAD | Reload (only when empty) |
| 3 | SFX_ENEMY_SPAWN | Enemy respawn |
| 4 | SFX_TELEPORT | Zone teleport |
| 5 | SFX_KEY_UNLOCK | Key / zombie kill |
| 6 | SFX_TOMB_ENTER | Level exit |
| 7 | SFX_PLAYER_DIE | Player death |

### Music Jingles (slots 8–27)
Converted from original AY-3-8910 VGM data (vgmrips.net).
Sequenced via `music_update()` called each frame, using `PlaySound()` per phrase.

| Constant | Track | Trigger |
|----------|-------|---------|
| MUSIC_COIN | Coin insert | PRESS START on title |
| MUSIC_GAME_START | Game start | (mapped to coin, slot limit) |
| MUSIC_GATE_OPEN | Gate open | Key collected |
| MUSIC_STAGE_CLEAR | Stage clear | Level complete |
| MUSIC_ROUND_CLEAR | Round clear | (mapped to stage_clear, slot limit) |

Total slots: 28 of 31 maximum.

---

## Zombie Mode (Easter Egg)

**Activate:** Konami code on title screen: ↑ ↑ ↓ ↓ ← → ← → B A

| Feature | Normal mode | Zombie mode |
|---------|-------------|-------------|
| Player sprite | Explorer | Mummy |
| A button | Fire bullet | Disabled |
| B button | Reload | Disabled |
| Enemy touch | Player dies | Enemy killed (+200) |
| HUD bullets | Bullet icons | Countdown timer |
| Palettes | Colour | Greyscale |
| Timer | None | 60 seconds per zone |
| Treasure | +500 pts | +500 pts +5 sec bonus |
| Door exit | +1000 pts, next level | +1000 pts +15 sec, next level |
| OPTION button | Exit to title | Toggle invincibility (INV) |

Invincibility (OPTION in zombie mode): freezes timer, enemies killed on touch
but don't count deaths — useful for map exploration and testing.

Zombie mode resets to normal on fresh PRESS START from title screen.

---

## Intermission (after Level 8)

- 32×32 gold Tut mask drawn centred in upper half (tiles 232–247)
- Score and "LEVEL COMPLETE!" displayed
- Player (explorer) walks right across bottom half
- 5 enemies chase in order: mummy → bird → bug → snake → scorpion
  - 48px gap between player and first enemy
  - 16px (1 sprite width) spacing between enemies
  - Mummy, bird, scorpion H-flipped to face right
- Runs ~10 seconds then loops back to level 1 (displayed as T:9+)
- Score carries across loop

---

## cc900 C89 Rules (Critical)

- All variable declarations **before** any statements in every block
- No `//` comments — use `/* */` only
- No C99 features — no `for (int i = 0; ...)`, no designated initialisers
- No `volatile u16*` stores — generates incorrect code
- No signed multiply or divide — crashes runtime
- No static local variables
- No `const` on SOUNDEFFECT arrays — cc900 rejects it
- No unsized arrays `[]` — must specify explicit count
- No `#define MACRO {val,val}` used as array element — expand inline
- Sprite tile IDs must be ≤ 255 (hardware limit)
- `SysSetSystemFont()` must never be called per-frame — destroys tile RAM
- `ClearScreen` after `SysSetSystemFont` destroys font — use `FillScreen` instead
- Maze map is `const` — access only via `maze_cell_get_zone()` / `maze_cell_raw()`
- Link order matters: main.c first to preserve ROM header at 0x200000

---

## Flash Save (Pending)

Flash implementation exists in `common/core/rom/flash.c` (from ameliandev SDK).
Save buffer: `gamedata[256]` (u32 array). Magic: `0xcafebabe`.
Offset: `0x1FA000`, block `0x21` (16Mb cart).

`save.c/save.h` stub exists in `src/` — wired to `save_hi_score()` / `load_hi_score()`
in game.c but currently no-ops. To activate:
1. Add `common/core/rom/flash.c` to build (currently excluded from `*.c` wildcard)
2. Call `save_init()` from `main.c` after `InitNGPC()`
3. Replace stubs in `save_hi_score()` / `load_hi_score()` with `SaveGame()` / `LoadGame()`

---

## Known Issues / TODO

### Bugs
- Enemies can still get stuck in corners (no unstick logic)
- Sound unverified on real hardware (Z80 ack timing may still hang)
- Flash save not yet wired (hi-score resets on power off)

### Missing Features
- Player run frames 3 and 4 (currently 2 walk frames only)
- Round clear jingle (dropped — slot limit; stage_clear plays instead)
- 32×32 Tut mask full render on victory screen (intermission works; standalone screen stubbed)
- Background music / NeoTracker (format unknown, no composer tool found)

### Debug Features
- **Konami code** (↑↑↓↓←→←→BA) on title → zombie mode
- **OPTION** in zombie mode → toggle invincibility (INV)
- Level select accessible after Konami if `enter_select()` re-enabled in code

---

## Sprite Sheet

Source: `sprite_sheet_-_resize_-_again_75_-_no_text_-no_lines.bmp` (263×211px, 75% scale)
Art by: volunteer artist
Extraction: Python/Pillow, luminance quantisation to 4 indices per sprite
Extracted data: `sprites_75.json` (on build machine)

Sprite layout (5 rows):
- Row 1: Key, Diamond, Ruby, Scarab
- Row 2: Player, Snake, Scorpion, Bug, Mummy
- Row 3: Wall plain, Hiero1, Hiero2, Smoke frames 1–5
- Row 4: Player run 1–4, Alcove, Bird, Door, Tut mask 16×16
- Row 5: Hat (8×8), Bullet fly (8×8), Bullet HUD (8×8), Tut mask 32×32

---

## Session History

Major milestones:
- Session 1–2: Core architecture, maze render, player movement, bullets
- Session 3–4: Enemy AI, sprite extraction, HUD, flash save stubs
- Session 5–6: Intermission, scoring, level loops, hi-score
- Session 7: Arcade VGM music conversion, sound system, zombie mode Easter egg
- Session 8: Map editor integration, multi-treasure tracking, zombie timer + invincibility
