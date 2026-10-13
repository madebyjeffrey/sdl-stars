#!/usr/bin/env python3
"""Verify actual PDF pages, paper dimensions and rendered GDI colors with Poppler."""
import pathlib
import re
import subprocess
import sys
import tempfile

root = pathlib.Path(sys.argv[1])
pdfinfo, pdftoppm = sys.argv[2:4]
for name, pages, width, height in (
    ("pages.pdf", 2, 612, 792),
    ("implicit.pdf", 1, 612, 792),
    ("landscape-a4.pdf", 1, 841.92, 595.2),
    ("chosen.pdf", 1, 612, 792),
    ("spooled.pdf", 1, 612, 792),
):
    text = subprocess.check_output([pdfinfo, str(root / name)], text=True)
    assert int(re.search(r"^Pages:\s+(\d+)", text, re.M)[1]) == pages, (name, text)
    size = re.search(r"^Page size:\s+([\d.]+) x ([\d.]+) pts", text, re.M)
    assert size and abs(float(size[1]) - width) < .03 and abs(float(size[2]) - height) < .03, (name, text)
assert (root / "abort.pdf").read_bytes() == b"sentinel"
assert not list(root.glob("*.tmp-*")), "An aborted/deleted/finished job left staging files"

with tempfile.TemporaryDirectory(dir=root) as tmp:
    prefix = pathlib.Path(tmp) / "page"
    subprocess.run([pdftoppm, "-r", "72", str(root / "pages.pdf"), str(prefix)], check=True)
    images = []
    for path in sorted(pathlib.Path(tmp).glob("*.ppm")):
        with path.open("rb") as stream:
            assert stream.readline().strip() == b"P6"
            line = stream.readline()
            while line.startswith(b"#"):
                line = stream.readline()
            width, height = map(int, line.split())
            assert stream.readline().strip() == b"255"
            data = stream.read()
            assert len(data) == width * height * 3
            images.append((width, height, data))
    assert len(images) == 2

    def pixel(page, x, y):
        width, height, data = images[page]
        index = (y * width + x) * 3
        return tuple(data[index:index + 3])

    assert pixel(0, 50, 50) == (255, 0, 0), "Page one lost the red GDI rectangle"
    assert pixel(1, 120, 170) == (0, 0, 255), "Page two lost the blue GDI rectangle"
    assert pixel(0, 120, 170) == (255, 255, 255)
    assert pixel(1, 50, 50) == (255, 255, 255), "NEWFRAME failed to clear page one"
    assert any(max(pixel(0, x, y)) < 128 for y in range(40, 55) for x in range(185, 310)), "Printed text disappeared"
print("PDF page counts, Letter/A4 dimensions, staging cleanup, colors and text verified")
