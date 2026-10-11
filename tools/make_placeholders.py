"""Draws the placeholder assets into resources/placeholders: plain gradients and shapes, and two short beeps.

Run from the repository root: python tools/make_placeholders.py
Needs Pillow, and ffmpeg (with libvorbis) on PATH for the sounds. The final assets replace these files under
the same names, see ASSETS.md. Sprites are drawn at the UHD size: Geode makes the -hd and low ones from them.
"""

import math
import shutil
import subprocess
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter

OUT = Path(__file__).resolve().parent.parent / "resources" / "placeholders"

# The backgrounds are stretched to cover the screen, so a placeholder can be smaller than the final art
BG = (1280, 720)


def lerp(a, b, t):
    return tuple(round(x + (y - x) * t) for x, y in zip(a, b))


def vertical_gradient(size, top, bottom):
    image = Image.new("RGBA", size)
    draw = ImageDraw.Draw(image)
    for y in range(size[1]):
        draw.line([(0, y), (size[0], y)], fill=lerp(top, bottom, y / (size[1] - 1)))
    return image


def bg_far():
    # The wall: a warm dusk gradient with faint wallpaper stripes
    image = vertical_gradient(BG, (58, 32, 64, 255), (20, 11, 24, 255))
    stripes = Image.new("RGBA", BG, (0, 0, 0, 0))
    draw = ImageDraw.Draw(stripes)
    for x in range(0, BG[0], 48):
        draw.rectangle([x, 0, x + 20, BG[1]], fill=(255, 200, 230, 10))
    return Image.alpha_composite(image, stripes)


def bg_mid():
    # A window with light coming through, and a shelf on the left
    image = Image.new("RGBA", BG, (0, 0, 0, 0))
    glow = Image.new("RGBA", BG, (0, 0, 0, 0))
    ImageDraw.Draw(glow).ellipse([700, 40, 1220, 520], fill=(255, 170, 200, 70))
    image = Image.alpha_composite(image, glow.filter(ImageFilter.GaussianBlur(60)))
    draw = ImageDraw.Draw(image)
    draw.rectangle([820, 110, 1100, 430], fill=(255, 214, 230, 90))
    draw.rectangle([955, 110, 965, 430], fill=(40, 22, 46, 200))
    draw.rectangle([820, 265, 1100, 275], fill=(40, 22, 46, 200))
    draw.rectangle([810, 100, 1110, 440], outline=(40, 22, 46, 220), width=12)
    for y in (300, 420, 540):
        draw.rectangle([60, y, 360, y + 14], fill=(30, 16, 34, 200))
    return image


def bg_near():
    # The floor, and two dark shapes at the sides close to the camera
    image = Image.new("RGBA", BG, (0, 0, 0, 0))
    floor = vertical_gradient((BG[0], 170), (34, 18, 38, 200), (12, 6, 14, 250))
    image.paste(floor, (0, BG[1] - 170))
    draw = ImageDraw.Draw(image)
    draw.rounded_rectangle([-80, 380, 140, 760], radius=40, fill=(10, 5, 12, 235))
    draw.rounded_rectangle([1170, 450, 1380, 760], radius=40, fill=(10, 5, 12, 235))
    return image


def bg_vignette():
    image = Image.new("RGBA", BG, (0, 0, 0, 0))
    pixels = image.load()
    cx, cy = BG[0] / 2, BG[1] / 2
    for y in range(BG[1]):
        for x in range(BG[0]):
            d = math.hypot((x - cx) / cx, (y - cy) / cy) / math.sqrt(2)
            pixels[x, y] = (0, 0, 0, round(170 * max(0.0, (d - 0.45) / 0.55) ** 1.6))
    return image


def rounded(size, radius):
    # White, so the theme's color tints it
    image = Image.new("RGBA", size, (0, 0, 0, 0))
    ImageDraw.Draw(image).rounded_rectangle([0, 0, size[0] - 1, size[1] - 1], radius=radius,
                                            fill=(255, 255, 255, 255))
    return image


def beep(path, frequency, duration):
    subprocess.run(
        ["ffmpeg", "-y", "-loglevel", "error", "-f", "lavfi", "-i",
         f"sine=frequency={frequency}:duration={duration}", "-af",
         f"afade=t=in:d=0.005,afade=t=out:st={duration * 0.3}:d={duration * 0.7},volume=0.5",
         "-c:a", "libvorbis", "-q:a", "4", str(path)],
        check=True)


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    sprites = {
        "bg-far.png": bg_far(),
        "bg-mid.png": bg_mid(),
        "bg-near.png": bg_near(),
        "bg-vignette.png": bg_vignette(),
        "item-accent.png": rounded((8, 64), 4),
        "tray-panel.png": rounded((96, 96), 24),
        "tray-slot.png": rounded((64, 64), 16),
    }
    for name, image in sprites.items():
        image.save(OUT / name, optimize=True)
        print("drew", name)

    if shutil.which("ffmpeg"):
        beep(OUT / "hover.ogg", 1320, 0.06)
        beep(OUT / "click.ogg", 660, 0.12)
        print("made hover.ogg, click.ogg")
    else:
        print("no ffmpeg on PATH: the sounds aren't made")


if __name__ == "__main__":
    main()
