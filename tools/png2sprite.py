#!/usr/bin/env python3
"""
png2sprite.py — Convert PNG images to NGPC tile data (u16[8] format)

Reads a PNG image and outputs C source code with tile data suitable for
InstallTileSetAt() in the ameliandev NGPC framework.

The NGPC tile format:
  - Each tile is 8x8 pixels
  - Each row is one u16 word: MSB = leftmost pixel pair
  - 2 bits per pixel (4 colours per palette): 0=transparent, 1-3=palette colours
  - For 1bpp mode (default): any non-transparent pixel becomes colour 1

Input PNG requirements:
  - Width/height must be multiples of 8
  - Supports: greyscale, RGB, RGBA, palette modes
  - Transparent (alpha=0) or black (#000000) pixels = colour 0 (transparent)
  - Other pixels mapped to palette indices 1-3 based on brightness

Tile ordering for multi-tile sprites:
  For a 16x16 sprite (2x2 tiles), tiles are ordered: TL, TR, BL, BR
  For a 24x32 sprite (3x4 tiles), tiles are left-to-right, top-to-bottom

Usage:
  python3 png2sprite.py <input.png> [options]

Options:
  --name NAME       C variable name (default: derived from filename)
  --mode 1bpp|2bpp  Colour mode (default: 1bpp)
  --threshold N     Brightness threshold for 1bpp (0-255, default: 128)
  --offset N        Tile RAM offset for InstallTileSetAt (default: 144)
  --preview         Print ASCII preview of tiles
  --header          Output .h declarations instead of .c data
"""

import sys
import os
import argparse
from PIL import Image


def pixel_to_index(r, g, b, a, mode, threshold):
    """Convert RGBA pixel to 2-bit palette index."""
    if a < 128:
        return 0  # transparent

    brightness = (r * 299 + g * 587 + b * 114) // 1000

    if mode == '1bpp':
        return 1 if brightness >= threshold else 0
    else:
        # 2bpp: map brightness to 4 levels
        if brightness < 64:
            return 0  # transparent/black
        elif brightness < 128:
            return 3  # darkest visible (palette colour 3)
        elif brightness < 192:
            return 2  # mid (palette colour 2)
        else:
            return 1  # brightest (palette colour 1)


def png_to_tiles(image, mode='1bpp', threshold=128):
    """Convert PIL Image to list of tiles, each tile = list of 8 u16 rows."""
    img = image.convert('RGBA')
    w, h = img.size

    if w % 8 != 0 or h % 8 != 0:
        print(f"Warning: image {w}x{h} not multiple of 8, padding with transparent", file=sys.stderr)
        new_w = ((w + 7) // 8) * 8
        new_h = ((h + 7) // 8) * 8
        padded = Image.new('RGBA', (new_w, new_h), (0, 0, 0, 0))
        padded.paste(img, (0, 0))
        img = padded
        w, h = img.size

    tiles_x = w // 8
    tiles_y = h // 8
    pixels = img.load()
    tiles = []

    # Tiles ordered left-to-right, top-to-bottom
    for ty in range(tiles_y):
        for tx in range(tiles_x):
            tile = []
            for row in range(8):
                word = 0
                for col in range(8):
                    px = tx * 8 + col
                    py = ty * 8 + row
                    r, g, b, a = pixels[px, py]
                    idx = pixel_to_index(r, g, b, a, mode, threshold)
                    # NGPC packed 2bpp: each u16 = 8 pixels, 2 bits each
                    # bits 15,14 = pixel 0 (leftmost)
                    # bits 13,12 = pixel 1
                    # ...
                    # bits 1,0 = pixel 7 (rightmost)
                    shift = 14 - col * 2
                    word |= (idx << shift)
                tile.append(word)
            tiles.append(tile)

    return tiles, tiles_x, tiles_y


def format_tile_c(tile):
    """Format a single tile (8 u16 values) as C initializer."""
    return ', '.join(f'0x{w:04X}' for w in tile)


def tile_ascii(tile):
    """Render tile as ASCII art for preview."""
    lines = []
    for word in tile:
        row = ''
        for col in range(8):
            shift = 14 - col * 2
            idx = (word >> shift) & 3
            row += ' .#@'[idx]
        lines.append(row)
    return lines


def main():
    parser = argparse.ArgumentParser(description='Convert PNG to NGPC tile data')
    parser.add_argument('input', help='Input PNG file')
    parser.add_argument('--name', help='C variable name')
    parser.add_argument('--mode', choices=['1bpp', '2bpp'], default='1bpp',
                        help='Colour mode (default: 1bpp)')
    parser.add_argument('--threshold', type=int, default=128,
                        help='Brightness threshold for 1bpp (0-255)')
    parser.add_argument('--offset', type=int, default=144,
                        help='Tile RAM offset for InstallTileSetAt')
    parser.add_argument('--preview', action='store_true',
                        help='Print ASCII preview')
    parser.add_argument('--header', action='store_true',
                        help='Output .h declaration')
    args = parser.parse_args()

    if not os.path.exists(args.input):
        print(f"Error: file not found: {args.input}", file=sys.stderr)
        sys.exit(1)

    img = Image.open(args.input)
    w, h = img.size

    if args.name:
        name = args.name
    else:
        name = os.path.splitext(os.path.basename(args.input))[0]
        name = name.replace('-', '_').replace(' ', '_')

    tiles, tiles_x, tiles_y = png_to_tiles(img, args.mode, args.threshold)
    total_tiles = len(tiles)
    total_words = total_tiles * 8

    print(f"/* Generated from {os.path.basename(args.input)} ({w}x{h}px) */")
    print(f"/* {tiles_x}x{tiles_y} tiles = {total_tiles} tiles, {total_words} words */")
    print(f"/* Mode: {args.mode}, threshold: {args.threshold} */")
    print()

    if args.preview:
        print("/* Preview:")
        for ty in range(tiles_y):
            for row in range(8):
                line = "   "
                for tx in range(tiles_x):
                    tidx = ty * tiles_x + tx
                    ascii_lines = tile_ascii(tiles[tidx])
                    line += ascii_lines[row] + " "
                print(line)
            print()
        print("*/")
        print()

    if args.header:
        print(f"#define T_{name.upper()} {args.offset}")
        print(f"#define T_{name.upper()}_TILES {total_tiles}")
        print(f"#define T_{name.upper()}_WORDS {total_words}")
        print(f"extern const u16 {name}[{total_tiles}][8];")
    else:
        if total_tiles == 1:
            # Single tile — flat array
            print(f"static const u16 {name}[8] = {{ {format_tile_c(tiles[0])} }};")
        else:
            print(f"static const u16 {name}[{total_tiles}][8] = {{")
            for i, tile in enumerate(tiles):
                tx = i % tiles_x
                ty = i // tiles_x
                if tiles_x > 1 or tiles_y > 1:
                    label = f"/* tile [{tx},{ty}] */"
                else:
                    label = ""
                print(f"    {{ {format_tile_c(tile)} }}, {label}")
            print("};")

        print()
        print(f"/* Install with: */")
        print(f"/*   InstallTileSetAt((const unsigned short (*)[8]){name}, {total_words}, {args.offset}); */")

        if total_tiles > 1:
            print(f"/*")
            print(f"   Sprite is {tiles_x}x{tiles_y} tiles ({tiles_x*8}x{tiles_y*8} px)")
            print(f"   Draw with PutTile in {tiles_x}x{tiles_y} grid:")
            print(f"   for (ty=0; ty<{tiles_y}; ty++)")
            print(f"     for (tx=0; tx<{tiles_x}; tx++)")
            print(f"       PutTile(SCR_1_PLANE, pal, x+tx, y+ty, {args.offset} + ty*{tiles_x} + tx);")
            print(f"*/")


if __name__ == '__main__':
    main()
