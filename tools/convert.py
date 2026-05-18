import os
import sys

from PIL import Image


def convert_file(input_path, output_path):
    with Image.open(input_path) as img:
        img = img.convert("RGBA")

        img.save(output_path, format="BMP", bitmap_format="bmp", bits=32)


if len(sys.argv) == 3:
    convert_file(sys.argv[1], sys.argv[2])
    sys.exit(0)

png_dir = "tools/png"
bmp_dir = "tools/bmp"

os.makedirs(bmp_dir, exist_ok=True)

for filename in os.listdir(png_dir):
    if filename.lower().endswith(".png"):
        input_path = os.path.join(png_dir, filename)

        output_name = os.path.splitext(filename)[0] + ".bmp"
        output_path = os.path.join(bmp_dir, output_name)

        convert_file(input_path, output_path)
