#!/usr/bin/env python3
"""Generates the sample images used to try HexStartup.

Writes, next to this script:
  startup.png / startup.ppm  640x480, 16 colors  -> STARTUP.DAT
  notch.png   / notch.ppm    16x23, colors taken from the startup palette -> NOTCH.DAT

Requires Pillow (pip install pillow).

Every palette entry is a multiple of 4 so it survives the 8-bit -> 6-bit VGA
conversion done when the lump is written. The notch only uses colors that also
appear in the startup image, because HexStartup indexes the notch with the
startup palette.
"""
import os

from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))

PALETTE = [
    (0, 0, 0),        # 0  black
    (24, 8, 32),      # 1  deep violet
    (48, 16, 56),     # 2  violet
    (80, 24, 72),     # 3  plum
    (120, 32, 64),    # 4  wine
    (160, 40, 40),    # 5  dark red
    (200, 64, 24),    # 6  red orange
    (232, 112, 16),   # 7  orange
    (252, 176, 32),   # 8  amber
    (252, 228, 120),  # 9  pale yellow
    (252, 252, 252),  # 10 white
    (40, 40, 48),     # 11 dark stone
    (72, 72, 84),     # 12 stone
    (112, 112, 124),  # 13 light stone
    (160, 160, 168),  # 14 pale stone
    (16, 48, 40),     # 15 dark teal
]
assert len(PALETTE) == 16 and all(c % 4 == 0 for rgb in PALETTE for c in rgb)


def new_indexed(size, fill=0):
    img = Image.new("P", size, fill)
    flat = [c for rgb in PALETTE for c in rgb]
    img.putpalette(flat + [0] * (768 - len(flat)))
    return img


def load_font(size):
    for name in ("DejaVuSerif-Bold.ttf", "LiberationSerif-Bold.ttf",
                 "/usr/share/fonts/TTF/DejaVuSerif-Bold.ttf",
                 "/usr/share/fonts/liberation/LiberationSerif-Bold.ttf"):
        try:
            return ImageFont.truetype(name, size)
        except OSError:
            pass
    return ImageFont.load_default(size)


def centered_text(draw, y, text, font, fill, shadow=None):
    w = draw.textlength(text, font=font)
    x = (640 - w) // 2
    if shadow is not None:
        draw.text((x + 3, y + 3), text, font=font, fill=shadow)
    draw.text((x, y), text, font=font, fill=fill)


def make_startup():
    img = new_indexed((640, 480))
    d = ImageDraw.Draw(img)
    d.fontmode = "1"  # no antialiasing, keeps the image at 16 colors

    # Banded sky: violet at the top fading into fire near the horizon.
    bands = [1, 2, 3, 4, 5, 6, 7]
    top, horizon = 0, 330
    step = (horizon - top) / len(bands)
    for i, color in enumerate(bands):
        d.rectangle([0, int(top + i * step), 639, int(top + (i + 1) * step)], fill=color)
    # Dithered seams between bands to soften the steps.
    for i in range(1, len(bands)):
        y0 = int(top + i * step)
        for y in range(y0 - 6, y0):
            for x in range((y % 2), 640, 2):
                if (y0 - y) % 3 != 0:
                    img.putpixel((x, y), bands[i])

    # Sun.
    d.ellipse([260, 230, 380, 350], fill=8)
    d.ellipse([280, 250, 360, 330], fill=9)

    # Ground and mountains.
    d.rectangle([0, 330, 639, 479], fill=15)
    d.polygon([(0, 340), (90, 250), (170, 320), (250, 270), (330, 345), (0, 345)], fill=11)
    d.polygon([(300, 345), (420, 240), (500, 300), (560, 260), (639, 320), (639, 345)], fill=11)
    d.polygon([(90, 250), (110, 270), (70, 270)], fill=13)
    d.polygon([(420, 240), (445, 262), (398, 262)], fill=13)
    d.rectangle([0, 345, 639, 479], fill=0)

    # Stone frame around the whole screen.
    for inset, color in ((0, 13), (4, 12), (8, 11)):
        d.rectangle([inset, inset, 639 - inset, 479 - inset], outline=color, width=4)

    # Title.
    centered_text(d, 40, "HEXSTARTUP", load_font(72), fill=8, shadow=0)
    centered_text(d, 130, "sample startup screen", load_font(24), fill=10, shadow=0)

    # Progress bar trough where the notches are drawn. Hexen draws the 32 NOTCHes (16x23) from (64,441), so the inside
    # of the trough is exactly x 64..575, y 441..463.
    d.rectangle([61, 438, 578, 466], fill=11, outline=13, width=3)
    d.rectangle([64, 441, 575, 463], fill=0)
    centered_text(d, 410, "16 colors - 640x480", load_font(14), fill=14)

    return img


def make_notch():
    img = new_indexed((16, 23))
    d = ImageDraw.Draw(img)
    d.rectangle([0, 0, 15, 22], fill=5)
    d.rectangle([1, 1, 14, 21], fill=6)
    d.rectangle([3, 3, 12, 19], fill=7)
    d.rectangle([5, 6, 10, 16], fill=8)
    d.rectangle([7, 9, 8, 13], fill=9)
    d.line([(1, 1), (14, 1)], fill=10)
    d.line([(0, 22), (15, 22)], fill=4)
    return img


def save(img, name):
    img.save(os.path.join(HERE, name + ".png"), optimize=True)
    rgb = img.convert("RGB")
    with open(os.path.join(HERE, name + ".ppm"), "wb") as f:
        f.write(b"P6\n%d %d\n255\n" % rgb.size)
        f.write(rgb.tobytes())
    return {rgb_ for _, rgb_ in rgb.getcolors(256)}


def main():
    startup_colors = save(make_startup(), "startup")
    notch_colors = save(make_notch(), "notch")
    assert len(startup_colors) <= 16, len(startup_colors)
    missing = notch_colors - startup_colors
    assert not missing, "notch uses colors not in startup: %s" % missing
    print("startup: %d colors, notch: %d colors" % (len(startup_colors), len(notch_colors)))


if __name__ == "__main__":
    main()
