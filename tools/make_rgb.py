#!/usr/bin/env python3
"""Resize JPGs in to_add/ to RGB565 little-endian for the AMOLED."""
import struct
import sys
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
TO_ADD = ROOT / "to_add"
WEB = ROOT / "web"
CREAM = (242, 239, 225)

JOBS = {
    "splash": {"src": "splash.jpg", "w": 466, "h": 466, "fit": "cover"},
    "tangrowth": {"src": "tangrowth.jpg", "w": 220, "h": 220, "fit": "contain"},
}


def rgb565(r, g, b):
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def fit(im, w, h, mode):
    im = im.convert("RGB")
    if mode == "cover":
        return im.resize((w, h), Image.Resampling.LANCZOS)
    im.thumbnail((w, h), Image.Resampling.LANCZOS)
    bg = Image.new("RGB", (w, h), CREAM)
    bg.paste(im, ((w - im.width) // 2, (h - im.height) // 2))
    return bg


def write_rgb(im, dest):
    px = im.load()
    w, h = im.size
    raw = bytearray(w * h * 2)
    i = 0
    for y in range(h):
        for x in range(w):
            r, g, b = px[x, y]
            raw[i : i + 2] = struct.pack("<H", rgb565(r, g, b))
            i += 2
    dest.write_bytes(raw)
    return len(raw)


def convert(name):
    job = JOBS[name]
    src = TO_ADD / job["src"]
    if not src.exists():
        print(f"missing {src}", file=sys.stderr)
        return 1
    im = fit(Image.open(src), job["w"], job["h"], job["fit"])
    dest = TO_ADD / f"{name}.rgb"
    n = write_rgb(im, dest)
    web = WEB / f"{name}.rgb"
    web.write_bytes(dest.read_bytes())
    print(f"wrote {dest} and {web} ({n} bytes)")
    return 0


def main():
    names = sys.argv[1:] or list(JOBS)
    for name in names:
        if name not in JOBS:
            print(f"unknown {name} (have: {', '.join(JOBS)})", file=sys.stderr)
            return 1
        if convert(name):
            return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
