"""Draws the menu's own UI pieces into resources/placeholders: white shapes the theme tints.

Run from the repository root: python tools/make_placeholders.py (needs Pillow).
Only the menu's own UI pieces and effects are drawn here. The background layers (resources/background/), the
fonts (resources/fonts/) and the sounds (resources/sounds/) are real files, see ASSETS.md and LICENSES.md.
Sprites are drawn at the UHD size: Geode makes the -hd and low ones from them.
"""

import math
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter

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


def vignette(size):
    # White, transparent in the middle, opaque towards the corners: tinted dark, it darkens the edges
    image = Image.new("RGBA", (size, size), (255, 255, 255, 0))
    pixels = image.load()
    half = (size - 1) / 2
    for y in range(size):
        for x in range(size):
            d = math.hypot((x - half) / half, (y - half) / half) / math.sqrt(2)
            pixels[x, y] = (255, 255, 255, round(255 * max(0.0, (d - 0.35) / 0.65) ** 1.8))
    return image


def heart(draw, cx, cy, r):
    draw.ellipse([cx - r, cy - r * 0.9, cx, cy + r * 0.1], fill=255)
    draw.ellipse([cx, cy - r * 0.9, cx + r, cy + r * 0.1], fill=255)
    draw.polygon([(cx - r * 0.97, cy - r * 0.25), (cx + r * 0.97, cy - r * 0.25), (cx, cy + r * 1.05)], fill=255)


def star(draw, cx, cy, r):
    points = []
    for i in range(10):
        angle = -math.pi / 2 + i * math.pi / 5
        radius = r if i % 2 == 0 else r * 0.45
        points.append((cx + radius * math.cos(angle), cy + radius * math.sin(angle)))
    draw.polygon(points, fill=255)


def flower(draw, cx, cy, r):
    for i in range(5):
        angle = -math.pi / 2 + i * 2 * math.pi / 5
        px, py = cx + r * 0.55 * math.cos(angle), cy + r * 0.55 * math.sin(angle)
        draw.ellipse([px - r * 0.42, py - r * 0.42, px + r * 0.42, py + r * 0.42], fill=255)
    draw.ellipse([cx - r * 0.25, cy - r * 0.25, cx + r * 0.25, cy + r * 0.25], fill=0)


def pattern(size):
    # A tile of small hearts, stars and flowers, scattered on a staggered grid; it repeats seamlessly (each shape
    # is drawn again across the edges it touches). White on transparent, softened a little
    mask = Image.new("L", (size, size), 0)
    draw = ImageDraw.Draw(mask)
    shapes = [heart, star, flower]
    cells = 4
    step = size / cells
    for row in range(cells):
        for column in range(cells):
            cx = (column + 0.5 + (0.5 if row % 2 else 0)) * step
            cy = (row + 0.5) * step
            shape = shapes[(row * 2 + column) % 3]
            r = step * (0.16 if shape is star else 0.14)
            for dx in (-size, 0, size):
                for dy in (-size, 0, size):
                    shape(draw, cx + dx, cy + dy, r)
    mask = mask.filter(ImageFilter.GaussianBlur(0.8))
    image = Image.new("RGBA", (size, size), (255, 255, 255, 0))
    image.putalpha(mask)
    return image


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    sprites = {
        "item-accent.png": rounded((8, 64), 4),
        "tray-panel.png": rounded((96, 96), 24),
        "tray-slot.png": rounded((64, 64), 16),
        "avatar-circle.png": rounded((96, 96), 48),
        # 4 pt corners at UHD (4 px a point)
        "row-plate.png": rounded((64, 64), 16),
        "glow-soft.png": soft_glow(256),
        # A power of two on every side and at every quality (256, 128, 64), so it can repeat
        "pattern-tile.png": pattern(256),
        "vignette-soft.png": vignette(256),
    }
    for name, image in sprites.items():
        image.save(OUT / name, optimize=True)
        print("drew", name)


if __name__ == "__main__":
    main()
