#!/usr/bin/env python3
"""Embed bitmap resources from the canonical RC declarations; no Windows tools."""
import argparse
import re
import struct
from pathlib import Path


def generate(root, output):
    definitions = dict(re.findall(r'^#define\s+(\w+)\s+(\d+)\s*$', (root / 'res/resource.h').read_text(), re.M))
    resources = []
    for line in (root / 'res/stars.rc').read_text().splitlines():
        if not re.search(r'\bBITMAP\b', line):
            continue
        match = re.fullmatch(r'\s*(\w+)\s+BITMAP\s+"([^"]+)"\s*', line)
        if not match:
            raise ValueError(f'Unsupported bitmap declaration: {line}')
        symbol, filename = match.groups()
        path = (root / 'res' / filename).resolve()
        if not path.is_relative_to((root / 'res/bitmaps').resolve()):
            raise ValueError(f'Bitmap outside resource directory: {filename}')
        data = path.read_bytes()
        if len(data) < 54 or data[:2] != b'BM':
            raise ValueError(f'Invalid BMP: {filename}')
        size, offset = struct.unpack_from('<IxxxxI', data, 2)
        header, width, height, planes, depth, compression = struct.unpack_from('<IiiHHI', data, 14)
        colors = struct.unpack_from('<I', data, 46)[0] or (1 << depth if depth <= 8 else 0)
        row = ((width * depth + 31) // 32) * 4
        if (size != len(data) or header != 40 or width <= 0 or height == 0 or planes != 1
                or depth not in (1, 4, 8, 24, 32) or compression != 0
                or offset != 54 + colors * 4 or offset + row * abs(height) != len(data)):
            raise ValueError(f'Unsupported/truncated BMP: {filename}')
        identity = int(definitions[symbol]) if symbol in definitions else symbol.upper()
        if identity in [r[0] for r in resources]:
            raise ValueError(f'Duplicate bitmap resource: {identity}')
        resources.append((identity, data[14:]))
    if not resources:
        raise ValueError('No bitmap resources found')
    output.mkdir(parents=True, exist_ok=True)
    source = ['/* Generated from res/stars.rc and its BMP files. */', '#include "internal.h"']
    manifest = ['/* Generated bitmap identities for native and SDK reference tests. */']
    for i, (identity, data) in enumerate(resources):
        source.append(f'_Alignas(4) static const BYTE rgbResource{i}[] = {{')
        source.extend('    ' + ','.join(f'0x{x:02x}' for x in data[j:j + 32]) + ',' for j in range(0, len(data), 32))
        source.append('};')
        identifier = f'MAKEINTRESOURCE({identity})' if isinstance(identity, int) else f'"{identity}"'
        manifest.append(f'{{{identifier}, {len(data)}}},')
    source.append('const COMPATRESOURCE rgCompatResources[] = {')
    for i, (identity, data) in enumerate(resources):
        name = 'NULL' if isinstance(identity, int) else f'"{identity}"'
        number = identity if isinstance(identity, int) else 0
        source.append(f'    {{{name}, {number}, rgbResource{i}, sizeof(rgbResource{i}), 2}},')
    source += ['};', 'const size_t cCompatResources = sizeof(rgCompatResources) / sizeof(rgCompatResources[0]);']
    (output / 'bitmap_resources.c').write_text('\n'.join(source) + '\n')
    (output / 'bitmap_manifest.h').write_text('\n'.join(manifest) + '\n')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('root', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    generate(args.root.resolve(), args.output.resolve())
