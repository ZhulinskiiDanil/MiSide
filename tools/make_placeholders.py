"""Draws the menu's own UI pieces into resources/placeholders: white shapes the theme tints.

Run from the repository root: python tools/make_placeholders.py (needs Pillow).
Only the menu's own UI pieces are drawn here. The background layers (resources/background/), the font
(resources/fonts/) and the sounds (resources/sounds/) are real files, see ASSETS.md and LICENSES.md.
Sprites are drawn at the UHD size: Geode makes the -hd and low ones from them.
"""

from pathlib import Path

import math

from PIL import Image, ImageDraw

OUT = Path(__file__).resolve().parent.parent / "resources" / "placeholders"


def rounded(size, radius):
    # White, so the theme's color tints it
    image = Image.new("RGBA", size, (0, 0, 0, 0))
    ImageDraw.Draw(image).rounded_rectangle([0, 0, size[0] - 1, size[1] - 1], radius=radius,
                                            fill=(255, 255, 255, 255))
    return image


def soft_glow(size):
    # White with alpha falling off smoothly from the middle: the theme's glow color tints it
    image = Image.new("RGBA", (size, size), (255, 255, 255, 0))
    pixels = image.load()
    half = (size - 1) / 2
    for y in range(size):
        for x in range(size):
            d = min(1.0, math.hypot(x - half, y - half) / half)
            pixels[x, y] = (255, 255, 255, round(255 * (1 - d) ** 2.2))
    return image


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    sprites = {
        "item-accent.png": rounded((8, 64), 4),
        "tray-panel.png": rounded((96, 96), 24),
        "tray-slot.png": rounded((64, 64), 16),
        "avatar-circle.png": rounded((96, 96), 48),
        # 3 pt corners at UHD (4 px a point)
        "row-plate.png": rounded((48, 48), 12),
        "glow-soft.png": soft_glow(256),
    }
    for name, image in sprites.items():
        image.save(OUT / name, optimize=True)
        print("drew", name)


if __name__ == "__main__":
    main()
