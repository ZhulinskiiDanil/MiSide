"""Draws the menu's own UI pieces into resources/placeholders: white shapes the theme tints.

Run from the repository root: python tools/make_placeholders.py (needs Pillow).
Only the menu's own UI pieces and effects are drawn here. The fonts (resources/fonts/) and the sounds (resources/sounds/) are real files, see ASSETS.md and LICENSES.md.
Sprites are drawn at the UHD size: Geode makes the -hd and low ones from them.
"""

import math
from pathlib import Path

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


def outline(draw, points, width):
    draw.line(points + points[:1], fill=255, width=width, joint="curve")


def transformed(points, cx, cy, r, angle):
    c, s = math.cos(angle), math.sin(angle)
    return [(cx + r * (x * c - y * s), cy + r * (x * s + y * c)) for x, y in points]


def heart_points(steps=64):
    # The classic heart curve, about one unit across, its middle at the origin, point down (y grows down)
    points = []
    for i in range(steps):
        t = 2 * math.pi * i / steps
        x = 16 * math.sin(t) ** 3
        y = 13 * math.cos(t) - 5 * math.cos(2 * t) - 2 * math.cos(3 * t) - math.cos(4 * t)
        points.append((x / 17, -(y + 2.5) / 17))
    return points


def flower_points(steps=120):
    # The outline of five round petals around the middle: the farthest of the petal circles along each ray
    points = []
    distance, radius = 0.5, 0.47
    for i in range(steps):
        theta = 2 * math.pi * i / steps
        reach = 0
        for petal in range(5):
            phi = theta - (-math.pi / 2 + petal * 2 * math.pi / 5)
            side = distance * math.sin(phi)
            if radius * radius >= side * side:
                reach = max(reach, distance * math.cos(phi) + math.sqrt(radius * radius - side * side))
        points.append((reach * math.cos(theta), reach * math.sin(theta)))
    return points


def star_points():
    points = []
    for i in range(10):
        angle = -math.pi / 2 + i * math.pi / 5
        radius = 1 if i % 2 == 0 else 0.48
        points.append((radius * math.cos(angle), radius * math.sin(angle)))
    return points


def circle_points(steps=40):
    return [(math.cos(2 * math.pi * i / steps), math.sin(2 * math.pi * i / steps)) for i in range(steps)]


def scatter(size, spacing, rng, tries=40):
    # Points no closer than `spacing` to each other, the tile's edges wrapping (Bridson's Poisson disc)
    def far_enough(p, points):
        for q in points:
            dx = abs(p[0] - q[0])
            dy = abs(p[1] - q[1])
            dx, dy = min(dx, size - dx), min(dy, size - dy)
            if dx * dx + dy * dy < spacing * spacing:
                return False
        return True

    points = [(rng.uniform(0, size), rng.uniform(0, size))]
    active = [0]
    while active:
        i = rng.choice(active)
        base = points[i]
        for _ in range(tries):
            angle = rng.uniform(0, 2 * math.pi)
            reach = rng.uniform(spacing, spacing * 2)
            p = ((base[0] + reach * math.cos(angle)) % size, (base[1] + reach * math.sin(angle)) % size)
            if far_enough(p, points):
                points.append(p)
                active.append(len(points) - 1)
                break
        else:
            active.remove(i)
    return points


def pattern(size):
    # A tile of thin outlined hearts, flowers and stars, with small rings and dots between them: scattered without
    # rows, each a little bigger or smaller and turned its own way. It repeats seamlessly (each shape is drawn again
    # across the edges it touches). White on transparent, drawn twice as big and scaled down for smooth lines.
    # `size` is UHD pixels, 4 a point
    import random

    rng = random.Random(7)
    scale = 2
    big = size * scale
    mask = Image.new("L", (big, big), 0)
    draw = ImageDraw.Draw(mask)
    point = 4 * scale
    shapes = [("heart", heart_points()), ("flower", flower_points()), ("star", star_points())]
    for x, y in scatter(size, 22 * 4, rng):
        roll = rng.random()
        if roll < 0.72:
            name, unit = shapes[rng.randrange(len(shapes))]
            r = rng.uniform(3.2, 6.2) * point
            angle = math.radians(rng.uniform(-28, 28))
            width = round(0.7 * point)
            kind = "outline"
        elif roll < 0.86:
            unit, r, angle, width, kind = circle_points(), rng.uniform(1.6, 2.6) * point, 0, round(0.6 * point), "outline"
            name = "ring"
        else:
            unit, r, angle, width, kind = circle_points(), rng.uniform(0.7, 1.2) * point, 0, 0, "dot"
            name = "dot"
        for dx in (-big, 0, big):
            for dy in (-big, 0, big):
                cx, cy = x * scale + dx, y * scale + dy
                if cx < -r * 2 or cy < -r * 2 or cx > big + r * 2 or cy > big + r * 2:
                    continue
                points = transformed(unit, cx, cy, r, angle)
                if kind == "dot":
                    draw.polygon(points, fill=255)
                else:
                    outline(draw, points, width)
                if name == "flower":
                    # The flower's middle: a small dot
                    middle = transformed(circle_points(), cx, cy, r * 0.17, 0)
                    draw.polygon(middle, fill=255)
    mask = mask.resize((size, size), Image.LANCZOS)
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
        # 256 pt, a power of two on every side and at every quality (1024, 512, 256), so it can repeat
        "pattern-tile.png": pattern(1024),
        "vignette-soft.png": vignette(256),
    }
    for name, image in sprites.items():
        image.save(OUT / name, optimize=True)
        print("drew", name)


if __name__ == "__main__":
    main()
