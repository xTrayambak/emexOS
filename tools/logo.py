#!/usr/bin/env python3
import argparse
import struct
import sys

try:
    from PIL import Image
except ImportError:
    print("error: pillow not installed. run: pip install pillow")
    sys.exit(1)


def convert(input_path, output_path, max_w=None, max_h=None):
    img = Image.open(input_path).convert("RGBA")

    w, h = img.size

    if max_w or max_h:
        scale = 1.0
        if max_w and w > max_w:
            scale = min(scale, max_w / w)
        if max_h and h > max_h:
            scale = min(scale, max_h / h)
        if scale < 1.0:
            new_w = max(1, int(w * scale))
            new_h = max(1, int(h * scale))
            img = img.resize((new_w, new_h), Image.LANCZOS)
            w, h = img.size
            print(f"  scaled to {w}x{h}")

    pixels = img.load()

    with open(output_path, "wb") as f:
        # 8-byte header
        f.write(struct.pack("<II", w, h))

        # pixel data: ARGB32 row by row
        for y in range(h):
            for x in range(w):
                r, g, b, a = pixels[x, y]
                argb = (a << 24) | (r << 16) | (g << 8) | b
                f.write(struct.pack("<I", argb))

    size = 8 + w * h * 4
    print(f"ok: {input_path} -> {output_path}")
    print(f"   {w}x{h}, {size} bytes")


def main():
    p = argparse.ArgumentParser(description="png to emexOS logo.bin converter")
    p.add_argument("input", help="input png file")
    p.add_argument("output", help="output bin file")
    p.add_argument(
        "--max-width", type=int, default=None, help="max width (keeps aspect ratio)"
    )
    p.add_argument(
        "--max-height", type=int, default=None, help="max height (keeps aspect ratio)"
    )
    args = p.parse_args()

    convert(args.input, args.output, args.max_width, args.max_height)


if __name__ == "__main__":
    main()
