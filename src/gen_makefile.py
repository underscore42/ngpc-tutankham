import os

TOOLCHAIN = "../../ngpcbins/T900"
GAME_NAME = "tutankham"

# main.c MUST be first to preserve ROM header at 0x200000
SRCS = [
    "src/main.c",
    "src/tiles.c",
    "src/screen.c",
    "src/game.c",
]

makefile = f"""
CC      = wine {TOOLCHAIN}/cc900.exe
LINK    = wine {TOOLCHAIN}/tulink.exe
CONV    = wine {TOOLCHAIN}/tuconv.exe
PACK    = wine {TOOLCHAIN}/s242ngp
LIB     = {TOOLCHAIN}

CFLAGS  = -cpu 900 -I./src

OBJS    = {" ".join(s.replace(".c", ".o") for s in SRCS)}
TARGET  = {GAME_NAME}.ngp

all: $(TARGET)

$(TARGET): $(OBJS)
\t$(LINK) $(OBJS) $(LIB)/system.lib $(LIB)/c900ml.lib
\t$(CONV) {GAME_NAME}.abs
\t$(PACK) {GAME_NAME}.s24
\tcp {GAME_NAME}.ngp $(TARGET)

.SUFFIXES: .c .o
.c.o:
\t$(CC) $(CFLAGS) $<

clean:
\trm -f $(OBJS) *.abs *.s24 *.ngp

run: $(TARGET)
\tmednafen $(TARGET)
""".lstrip()

with open("Makefile", "w") as f:
    f.write(makefile)

print("Makefile written.")
