"""Qualify actual game-map PDFs, including every planet across tiled pages."""
import argparse
import json
import os
import re
import shutil
import subprocess
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('client', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--driver', default='dummy')
    parser.add_argument('--host', type=Path, help='Prepare an unprotected game for installed-client acceptance')
    args = parser.parse_args()
    root = args.output.resolve()
    root.mkdir(parents=True, exist_ok=True)
    fixture = Path(__file__).resolve().parents[1] / 'scaffold/fixtures/regression/native/oneai1/checkpoints/010'
    for suffix in ('.m1', '.xy', '.hst'):
        shutil.copyfile(fixture / ('game' + suffix), root / ('game' + suffix))
    env = dict(os.environ, STARS_PRINTMAP_OUTPUT=str(root), STARS_SETTINGS_DIR=str(root),
               SDL_VIDEODRIVER=args.driver, SDL_AUDIODRIVER='dummy', STARS_SDL_RENDERER='software', STARS_SDL_PRINTER='pdf')
    subprocess.run([str(args.client.resolve()), 'game.m1'], cwd=root, env=env, timeout=120, check=True)
    assert (root / 'installed.r1').stat().st_size > 0
    if args.host:
        installed = root / 'installed-game'
        installed.mkdir(exist_ok=True)
        shutil.copyfile(root / 'installed.r1', installed / 'human.r1')
        shutil.copyfile(fixture.parents[3] / 'oneai1.def', installed / 'game.def')
        subprocess.run([str(args.host.resolve()), '-s12345', '-a', 'game.def'], cwd=installed, check=True, timeout=30)
        print('Installed-product fixture: ' + str(installed / 'game.m1'))
    manifest = json.loads((root / 'map.json').read_text())
    for name, nx, ny, width, height, cx, cy in (
        ('single', 1, 1, 612, 792, 2400, 3150),
        ('tiled', 2, 2, 612, 792, 2400, 3150),
        ('a4', 1, 1, 841.92, 595.2, 3358, 2330),
    ):
        text = subprocess.check_output(['pdfinfo', str(root / (name + '.pdf'))], text=True)
        assert int(re.search(r'^Pages:\s+(\d+)', text, re.M)[1]) == nx * ny
        size = re.search(r'^Page size:\s+([\d.]+) x ([\d.]+)', text, re.M)
        assert abs(float(size[1]) - width) < .03 and abs(float(size[2]) - height) < .03
        # The game's orientation rule exchanges the page grid for landscape.
        if (nx > ny) == (cx > cy): nx, ny = ny, nx
        dx, dy = cx * nx, cy * ny
        if dx < dy:
            if dy - 300 < dx: dx = dy - 300
            size_map = dx
        else:
            if dx - 450 < dy: dy = dx - 450
            size_map = dy
        scale_map = size_map - 64
        subprocess.run(['pdftoppm', '-r', '150', str(root / (name + '.pdf')), str(root / name)], check=True)
        pages = []
        for path in sorted(root.glob(name + '-*.ppm')):
            with path.open('rb') as stream:
                assert stream.readline().strip() == b'P6'
                line = stream.readline()
                while line.startswith(b'#'): line = stream.readline()
                w, h = map(int, line.split()); assert stream.readline().strip() == b'255'
                pixels = stream.read(); assert len(pixels) == w * h * 3
                pages.append((w, h, pixels))
        for planet, (ux, uy) in enumerate(manifest['planets']):
            x = int((ux - 1000) * scale_map / manifest['galaxy']) + 32
            y = int((manifest['inverse'] - 1000 - uy) * scale_map / manifest['galaxy']) + 32
            px, py = x // cx, y // cy
            assert 0 <= px < nx and 0 <= py < ny, (name, planet, x, y)
            w, h, pixels = pages[px * ny + py]
            rx, ry = int((x - px * cx + 75) / 2), int((y - py * cy + 75) / 2)
            assert any(min(pixels[(yy * w + xx) * 3:(yy * w + xx) * 3 + 3]) < 160
                       for yy in range(max(0, ry - 4), min(h, ry + 5))
                       for xx in range(max(0, rx - 4), min(w, rx + 5))), (name, planet, rx, ry)
        print(f'{name}: {nx * ny} page(s), all {len(manifest["planets"])} planet positions rendered')
    assert not list(root.glob('*.tmp-*'))
    subprocess.run(['pdftoppm', '-r', '100', '-png', str(root / 'single.pdf'), str(root / 'map-preview')], check=True)


if __name__ == '__main__':
    main()
