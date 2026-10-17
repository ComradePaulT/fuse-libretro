"""Convert Fuse's GPL-licensed fuse.font into a compact built-in 8x8 font."""

from pathlib import Path

root = Path(__file__).resolve().parents[1]
data = (root / "fuse" / "fuse.font").read_bytes()
glyphs = [[0] * 8 for _ in range(96)]
offset = 0
while offset < len(data):
    code, page, metrics = data[offset : offset + 3]
    width = (metrics >> 4) & 15
    left = metrics & 7
    columns = data[offset + 3 : offset + 3 + width]
    if page == 0 and 32 <= code <= 127:
        for row in range(8):
            for col, bits in enumerate(columns):
                if bits & (0x80 >> row) and left + col < 8:
                    glyphs[code - 32][row] |= 0x80 >> (left + col)
    offset += 3 + width

lines = [
    "/* Generated from Fuse's fuse.font; covered by Fuse's GPL license. */",
    "#ifndef FUSE_LIBRETRO_TAPEDECK_FONT_H",
    "#define FUSE_LIBRETRO_TAPEDECK_FONT_H",
    "static const unsigned char tapedeck_font[96][8] = {",
]
for code, rows in enumerate(glyphs, start=32):
    lines.append("   { " + ", ".join(f"0x{row:02x}" for row in rows) + f" }}, /* {code} */")
lines += ["};", "#endif"]
(root / "src" / "tapedeck_font.h").write_text("\n".join(lines) + "\n")
