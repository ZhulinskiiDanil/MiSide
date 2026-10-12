"""Draws logo.png, the mod's icon in Geode's lists: Mita's portrait (the first idle frame of the close-up atlas)
over the menu's backdrop, its gradient, pattern and glow, with her pink outline. Colors are the theme's defaults
(src/Theme.cpp). Run from the repository's root: python tools/make_logo.py"""

import json

from PIL import Image, ImageChops, ImageDraw, ImageFilter

SIZE = 336
WORK = 1008  # Drawn at three times the size, then scaled down
RADIUS = 56  # Corner, at the final size

BACKDROP_LEFT = (0x0A, 0x03, 0x08)
BACKDROP_MIDDLE = (0x26, 0x0A, 0x1C)
BACKDROP_RIGHT = (0x3A, 0x0E, 0x2C)
PATTERN = (0xE0, 0x50, 0x9E)
PATTERN_OPACITY = 38
GLOW = (0xA0, 0x1A, 0x80)
OUTLINE = (0xFF, 0x4F, 0xA3)
OUTLINE_OPACITY = 210

# Her head and shoulders in the 960x1080 frame
CROP = (90, 0, 890, 800)


def mix(a, b, t):
    return tuple(round(x + (y - x) * t) for x, y in zip(a, b))


def backdrop():
    image = Image.new("RGB", (WORK, WORK))
    pixels = image.load()
    for x in range(WORK):
        for y in range(WORK):
            # Across the diagonal, as the menu's gradient runs across the screen
            t = (x + (WORK - y)) / (2 * WORK)
            pixels[x, y] = mix(BACKDROP_LEFT, BACKDROP_MIDDLE, t / 0.45) if t < 0.45 else mix(
                BACKDROP_MIDDLE, BACKDROP_RIGHT, (t - 0.45) / 0.55)
    image = image.convert("RGBA")

    tile = Image.open("resources/placeholders/pattern-tile.png").convert("RGBA").resize((WORK // 2, WORK // 2))
    alpha = tile.split()[3].point(lambda a: a * PATTERN_OPACITY // 255)
    tinted = Image.new("RGBA", tile.size, PATTERN + (0,))
    tinted.putalpha(alpha)
    for x in range(0, WORK, tile.width):
        for y in range(0, WORK, tile.height):
            image.alpha_composite(tinted, (x, y))

    glow = Image.new("L", (WORK, WORK), 0)
    ImageDraw.Draw(glow).ellipse((WORK * 0.12, WORK * 0.08, WORK * 0.88, WORK * 0.84), fill=150)
    glow = glow.filter(ImageFilter.GaussianBlur(WORK * 0.12))
    light = Image.new("RGBA", (WORK, WORK), GLOW + (0,))
    light.putalpha(glow)
    image.alpha_composite(light)
    return image


def portrait():
    manifest = json.load(open("resources/mita-closeup/mita-closeup-manifest.json", encoding="utf-8"))
    frame = manifest["frames"][0]
    x, y, w, h = frame["rect"]
    atlas = Image.open("resources/mita-closeup/mita_closeup_atlas_%02d.png" % frame["atlas"]).convert("RGBA")
    mita = atlas.crop((x, y, x + w, y + h)).crop(CROP).resize((WORK, WORK), Image.LANCZOS)

    # Her outline: the drawn pixels grown by a few, in pink, under her
    alpha = mita.split()[3]
    grown = alpha.filter(ImageFilter.MaxFilter(15)).point(lambda a: OUTLINE_OPACITY if a > 8 else 0)
    outline = Image.new("RGBA", mita.size, OUTLINE + (0,))
    outline.putalpha(ImageChops.subtract(grown, alpha))
    layer = Image.new("RGBA", mita.size, (0, 0, 0, 0))
    layer.alpha_composite(outline)
    layer.alpha_composite(mita)
    return layer


def main():
    image = backdrop()
    image.alpha_composite(portrait())
    image = image.resize((SIZE, SIZE), Image.LANCZOS)

    mask = Image.new("L", (SIZE * 4, SIZE * 4), 0)
    ImageDraw.Draw(mask).rounded_rectangle((0, 0, SIZE * 4 - 1, SIZE * 4 - 1), RADIUS * 4, fill=255)
    image.putalpha(mask.resize((SIZE, SIZE), Image.LANCZOS))
    image.save("logo.png", optimize=True)
    print("logo.png", image.size)


if __name__ == "__main__":
    main()
