"""Regenerate the extracted bitmap review assets (requires Pillow)."""

from pathlib import Path
from PIL import Image, ImageDraw
import re, json, csv, hashlib, shutil, html, base64

root = Path(__file__).resolve().parents[2]
out = Path(__file__).resolve().parent
rc = (root / 'res/stars.rc').read_text(encoding='utf-8')
resources = re.findall(r'^([A-Z0-9_]+)\s+BITMAP\s+"([^"]+)"', rc, re.M)
assert len(resources) == 38
assert {root / 'res' / p for _, p in resources} == set((root / 'res/bitmaps').glob('*.bmp'))
rows = []
images = {}
references = {}
hulls = {}
for p in sorted((root / 'data/parts').glob('*.yaml')):
    if p.stem.startswith('starting-'):
        continue
    for block in re.split(r'(?m)^(?=- name:)', p.read_text(encoding='utf-8'))[1:]:
        symbol = re.match(r'- name: (\w+)', block).group(1)
        is_hull = p.stem in ('hulls', 'starbase-hulls')
        match = re.search(r'^\s+sz(?:Class' + ('' if is_hull else '|Name') + r'): "([^"]+)"', block, re.M)
        if not match:
            raise ValueError(f'Missing name: {symbol}')
        match_id = re.search(r'^\s+ibmp: (\d+)\s*$', block, re.M)
        ibmp = int(match_id.group(1)) if match_id else 0
        ref = dict(name=match.group(1), symbol=symbol, category=p.stem, source=str(p.relative_to(root)))
        if is_hull:
            for variant in range(4):
                hulls[ibmp + variant] = [dict(ref, variant=variant + 1)]
        else:
            references.setdefault(ibmp, []).append(ref)

def save(im, relative, kind, source, **extra):
    dest = out / relative
    dest.parent.mkdir(parents=True, exist_ok=True)
    im.save(dest)
    rows.append(dict(file=relative, kind=kind, source=source, width=im.width, height=im.height, **extra))

def slug(s):
    return re.sub('[^a-z0-9]+', '-', s.lower()).strip('-')

for symbol, relative in resources:
    p = root / 'res' / relative
    im = Image.open(p).convert('RGB')
    images[p.stem] = im
    dest = out / 'original-bmp' / p.name
    dest.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(p, dest)
    assert dest.read_bytes() == p.read_bytes()
    save(im, f'png-sheets/{p.stem}.png', 'sheet', str(p.relative_to(root)), resource=symbol,
         original_sha256=hashlib.sha256(p.read_bytes()).hexdigest())

for kind, prefix, size, refmap in [('inventory', 'iddib_tech_icons_', 64, references),
                                  ('hulls', 'iddib_hull_icons_', 64, hulls),
                                  ('hulls-small', 'iddib_hull_icons_small_', 32, hulls)]:
    count = 7 if kind == 'inventory' else 5
    for sheet in range(count):
        stem = f'{prefix}{sheet + 1}'
        im = images[stem]
        for y in range(im.height // size):
            for x in range(im.width // size):
                ibmp = sheet * 32 + (y * 8 + x if kind == 'inventory' else x * 4 + y)
                refs = refmap.get(ibmp, [])
                suffix = slug(refs[0]['name']) if refs else 'unreferenced'
                if refs and kind != 'inventory':
                    suffix += f'-variant-{refs[0]["variant"]}'
                box = (x * size, y * size, (x + 1) * size, (y + 1) * size)
                save(im.crop(box), f'{kind}/{ibmp:03d}-{suffix}.png', kind,
                     f'res/bitmaps/{stem}.bmp', ibmp=ibmp, crop=list(box), references=refs)

for stem, size, kind in [('iddib_player_icons', 32, 'players'), ('iddib_player_icons_small', 16, 'players-small'),
                         ('iddib_player_icons_tiny', 8, 'players-tiny'), ('iddib_planet_icons', 64, 'planets'),
                         ('iddib_thing_icons', 64, 'things')]:
    im = images[stem]
    for y in range(im.height // size):
        for x in range(im.width // size):
            box = (x * size, y * size, (x + 1) * size, (y + 1) * size)
            save(im.crop(box), f'{kind}/row-{y:02d}-col-{x:02d}.png', kind,
                 f'res/bitmaps/{stem}.bmp', crop=list(box))

inventory_ids = {r['ibmp'] for r in rows if r['kind'] == 'inventory'}
assert set(references) <= inventory_ids, set(references) - inventory_ids
assert {r['ibmp'] for r in rows if r['kind'] == 'hulls'} == set(range(148))
assert len([r for r in rows if r['kind'] == 'inventory']) == 208
for r in rows:
    with Image.open(out / r['file']) as im:
        im.load()
        assert im.size == (r['width'], r['height'])
        if 'crop' in r:
            source = Image.open(root / r['source']).convert('RGB').crop(r['crop'])
            assert im.convert('RGB').tobytes() == source.tobytes()
        else:
            assert im.convert('RGB').tobytes() == Image.open(root / r['source']).convert('RGB').tobytes()

(out / 'manifest.json').write_text(json.dumps(rows, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
with (out / 'inventory.csv').open('w', newline='', encoding='utf-8') as f:
    writer = csv.writer(f)
    writer.writerow(['file', 'kind', 'bitmap_id', 'width', 'height', 'names', 'source', 'crop'])
    for r in rows:
        writer.writerow([r['file'], r['kind'], r.get('ibmp', ''), r['width'], r['height'],
                         '; '.join(x['name'] for x in r.get('references', [])), r['source'], r.get('crop', '')])

for kind in ('inventory', 'hulls'):
    group = sorted((r for r in rows if r['kind'] == kind), key=lambda r:r['ibmp'])
    cols, cw, ch = 8, 168, 108
    contact = Image.new('RGB', (cols * cw, ((len(group) + cols - 1) // cols) * ch), '#e7e7e7')
    draw = ImageDraw.Draw(contact)
    for n, r in enumerate(group):
        x, y = (n % cols) * cw, (n // cols) * ch
        contact.paste(Image.open(out / r['file']), (x + 52, y + 4))
        label = f'{r["ibmp"]:03d} ' + (r.get('references') or [{'name':'unreferenced'}])[0]['name']
        label = label.encode('ascii', 'replace').decode()
        draw.text((x + 4, y + 72), label[:26], fill='black')
        if len(label) > 26:
            draw.text((x + 4, y + 85), label[26:52], fill='black')
    contact.save(out / f'{kind}-contact-sheet.png')

sections = []
for kind in dict.fromkeys(r['kind'] for r in rows):
    cards = []
    for r in (r for r in rows if r['kind'] == kind):
        names = '; '.join(x['name'] for x in r.get('references', []))
        label = (f'{r["ibmp"]:03d} · ' if 'ibmp' in r else '') + (names or Path(r['file']).stem)
        encoded = base64.b64encode((out / r['file']).read_bytes()).decode('ascii')
        cards.append(f'<a class="card" href="{html.escape(r["file"])}" data-search="{html.escape(label.lower())}"><img src="data:image/png;base64,{encoded}" width="{r["width"]}" height="{r["height"]}" alt="{html.escape(label)}"><span>{html.escape(label)}</span><small>{r["width"]} × {r["height"]}</small></a>')
    sections.append(f'<section id="{kind}"><h2>{kind} ({len(cards)})</h2><div class="grid">{"".join(cards)}</div></section>')
(out / 'index.html').write_text('''<!doctype html><meta charset="utf-8"><title>Stars! bitmap review</title>
<style>body{font:15px system-ui;background:#ececec;color:#222;margin:32px}h1{margin-bottom:8px}nav{display:flex;gap:16px;flex-wrap:wrap;margin:24px 0}.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(180px,1fr));gap:12px}.card{background:white;border:1px solid #ccc;border-radius:6px;padding:12px;display:flex;flex-direction:column;align-items:center;gap:8px;color:inherit;text-decoration:none;overflow:hidden}.card img{image-rendering:pixelated;max-width:100%;height:auto;min-height:64px;object-fit:contain}.card span{word-break:break-word;text-align:center}small{color:#666}input{font:inherit;padding:10px;width:320px}section{margin:32px 0}a{color:#145ba4}[hidden]{display:none!important}</style>
<h1>Stars! bitmap review</h1><p>All 38 bitmap resources, plus lossless individual inventory, hull, player, planet and thing crops. Original colors and backgrounds preserved.</p><p>Inventory IDs follow the game's lookup; hull variants use the column-first lookup in DrawFleetBitmap. Unreferenced slots are included. Names come from data/parts; shared artwork lists every matching item.</p><input id="search" placeholder="Filter by name or bitmap ID"><nav>'''
 + ''.join(f'<a href="#{k}">{k}</a>' for k in dict.fromkeys(r['kind'] for r in rows)) + '</nav>' + ''.join(sections)
 + '''<script>document.querySelector('#search').addEventListener('input',e=>{let q=e.target.value.toLowerCase();document.querySelectorAll('.card').forEach(c=>c.hidden=!c.dataset.search.includes(q))})</script>''', encoding='utf-8')
counts = {kind:sum(r['kind']==kind for r in rows) for kind in dict.fromkeys(r['kind'] for r in rows)}
(out / 'README.txt').write_text('Stars! bitmap review\n\nOpen index.html for the searchable gallery. All preview images are embedded, so the gallery works without loading neighboring files.\noriginal-bmp/: 38 byte-identical source BMP files.\npng-sheets/: lossless RGB PNG conversions of all 38 resources.\ninventory/: all 208 available 64x64 technology artwork slots, including unreferenced slots.\nhulls/ and hulls-small/: all 148 hull variants at each original size.\nplayers*, planets/, things/: additional individual sprites, named by top-down source row and column.\n*-contact-sheet.png: overview images.\nmanifest.json and inventory.csv: source paths, IDs, crop rectangles and catalog names.\n\nNo resizing, recoloring, transparency removal or background edits.\nInventory source coordinates account for bottom-up DIB addressing; normal top-down PNGs match the game.\nHull artwork has four variants per catalog hull. Shared inventory art lists all catalog references.\n\nCounts: ' + json.dumps(counts) + '\n\nRegenerate from the repository root with:\n  python3 assets/bitmap-review/extract.py\nThe extraction script requires Pillow.\n\nThese are review copies of the existing res/bitmaps assets, with their existing rights; extraction does not grant a new license.\nGenerated replacement concepts and downloaded candidates are excluded.\n', encoding='utf-8')
print(json.dumps(dict(folder=str(out), counts=counts, total_pngs=len(rows)+2, catalog_items=sum(map(len,references.values())), referenced_inventory_slots=len(references)), indent=2))
