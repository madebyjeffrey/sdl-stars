Stars! bitmap review

Open index.html for the searchable gallery. All preview images are embedded, so the gallery works without loading neighboring files.
original-bmp/: 38 byte-identical source BMP files.
png-sheets/: lossless RGB PNG conversions of all 38 resources.
inventory/: all 208 available 64x64 technology artwork slots, including unreferenced slots.
hulls/ and hulls-small/: all 148 hull variants at each original size.
players*, planets/, things/: additional individual sprites, named by top-down source row and column.
*-contact-sheet.png: overview images.
manifest.json and inventory.csv: source paths, IDs, crop rectangles and catalog names.

No resizing, recoloring, transparency removal or background edits.
Inventory source coordinates account for bottom-up DIB addressing; normal top-down PNGs match the game.
Hull artwork has four variants per catalog hull. Shared inventory art lists all catalog references.

Counts: {"sheet": 38, "inventory": 208, "hulls": 148, "hulls-small": 148, "players": 32, "players-small": 32, "players-tiny": 32, "planets": 28, "things": 7}

Regenerate from the repository root with:
  python3 assets/bitmap-review/extract.py
The extraction script requires Pillow.

These are review copies of the existing res/bitmaps assets, with their existing rights; extraction does not grant a new license.
Generated replacement concepts and downloaded candidates are excluded.
