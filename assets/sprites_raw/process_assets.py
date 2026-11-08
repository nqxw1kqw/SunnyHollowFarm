"""Resize actual imagegen outputs; never draw or synthesize replacement subjects."""
import json
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parent
FINAL = ROOT.parent / "sprites"
SPECIAL = {
    "player_sheet.png": (96, 128),
    "tiles_ground.png": (64, 64),
    "tiles_fence.png": (64, 32),
    "building_farmhouse.png": (96, 64),
    "building_shop_stall.png": (96, 64),
    "building_stove.png": (32, 64),
}
# Shared warm palette: outlines, earth/wood, cream, greens, reds, gold, water/denim, iron.
COLORS = """382218 573322 78492e 9b643e bc8552 d6a66a
ead0a0 fff0ce fffff0 294e2b 407332 61983e 86b848 b8d66a
753027 a83b2c d54b32 f36c3a ff9b61 9c692d c4943d e4bc55 ffe17b
26476c 386b9b 5594be 86c5d5 bce7df 303039 55555e 89858a c4bdad
bd7750 e5a778 f5c99b 6c3840 a64d4d df7270""".split()
PALETTE = Image.new("P", (1, 1))
rgb = [tuple(bytes.fromhex(c)) for c in COLORS]
PALETTE.putpalette([v for c in rgb for v in c] + [0] * (768 - len(rgb) * 3))


def finish(name):
    with Image.open(ROOT / name) as source:
        image = source.convert("RGBA")
    size = SPECIAL.get(name, (32, 32))
    if name == "tiles_ground.png":
        # The generated 2x2 subject is centered on a wider transparent canvas.
        image = image.crop((443, 15, 1331, 872))
    if name in ("building_farmhouse.png", "building_shop_stall.png", "building_stove.png", "tile_tilled_watered.png"):
        # Fit visible bounds, excluding low-alpha edge noise.
        bounds = image.getchannel("A").point(lambda a: 255 if a >= 128 else 0).getbbox()
        image = image.crop(bounds)
    if name == "ui_energy_bar.png":
        # Repack the two actual generated components into exact runtime rectangles.
        frame = image.crop((269, 21, 1485, 531)).resize((32, 16), Image.Resampling.NEAREST)
        fill = image.crop((320, 638, 1455, 830)).resize((28, 8), Image.Resampling.NEAREST)
        image = Image.new("RGBA", (32, 32))
        image.paste(frame, (0, 0))
        image.paste(fill, (2, 20))
    # Preserve the sheet's cell boundaries; do not trim the sheet to its subjects.
    image = image.resize(size, Image.Resampling.NEAREST)
    alpha = image.getchannel("A").point(lambda a: 255 if a >= 128 else 0)
    image = image.convert("RGB").quantize(palette=PALETTE, dither=Image.Dither.NONE).convert("RGBA")
    image.putalpha(alpha)
    image.save(FINAL / name)
    assert image.size == size and alpha.getextrema()[1] == 255
    print(f"{name}: {size[0]}x{size[1]}")


if __name__ == "__main__":
    FINAL.mkdir(exist_ok=True)
    for asset in json.loads((ROOT / "PROMPTS.json").read_text()):
        finish(asset["filename"])
