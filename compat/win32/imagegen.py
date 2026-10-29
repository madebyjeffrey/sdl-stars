#!/usr/bin/env python3
"""Compile ICON/CURSOR groups and image resources from the canonical RC assets."""
import argparse
import re
import struct
from pathlib import Path


def generate(root, output):
    definitions = dict(re.findall(r'^#define\s+(\w+)\s+(\d+)\s*$', (root / 'res/resource.h').read_text(), re.M))
    resources, images = [], []
    counts = {1: 0, 2: 0}
    for line in (root / 'res/stars.rc').read_text().splitlines():
        match = re.fullmatch(r'\s*(\w+)\s+(ICON|CURSOR)\s+"([^"]+)"\s*', line)
        if not match:
            continue
        symbol, kind, filename = match.groups()
        path = (root / 'res' / filename).resolve()
        if not path.is_relative_to((root / 'res').resolve()):
            raise ValueError('Image resource escapes res/')
        data = path.read_bytes()
        expected = 1 if kind == 'ICON' else 2
        if len(data) < 6:
            raise ValueError(f'Truncated image directory: {filename}')
        reserved, type_, count = struct.unpack_from('<HHH', data)
        if reserved or type_ != expected or count == 0 or len(data) < 6 + count * 16:
            raise ValueError(f'Invalid image directory: {filename}')
        identity = int(definitions[symbol]) if symbol in definitions else symbol.upper()
        group = bytearray(struct.pack('<HHH', 0, expected, count))
        first_image = None
        for i in range(count):
            width, height, colors, zero, first, second, size, offset = struct.unpack_from('<BBBBHHII', data, 6 + i * 16)
            if zero or offset < 6 + count * 16 or size < 40 or offset + size > len(data):
                raise ValueError(f'Invalid image entry: {filename}')
            dib = data[offset:offset + size]
            hdr, cx, cy, planes, bits, compression = struct.unpack_from('<IiiHHI', dib)
            width, height = width or 256, height or 256
            if hdr != 40 or cx != width or cy != height * 2 or planes != 1 or bits not in (1, 4, 8, 24, 32) or compression:
                raise ValueError(f'Unsupported image DIB: {filename}')
            colors_dib = struct.unpack_from('<I', dib, 32)[0] or (1 << bits if bits <= 8 else 0)
            required = 40 + colors_dib * 4 + (((cx * bits + 31) // 32) * 4 + ((cx + 31) // 32) * 4) * height
            if size < required:
                raise ValueError(f'Truncated image planes: {filename}')
            counts[expected] += 1
            ordinal = counts[expected]
            if expected == 1:
                group += struct.pack('<BBBBHHIH', width % 256, height % 256, colors, 0, planes, bits, size, ordinal)
                resource = dib
            else:
                if first >= width or second >= height:
                    raise ValueError(f'Cursor hotspot outside image: {filename}')
                resource = struct.pack('<HH', first, second) + dib
                group += struct.pack('<HHHHIH', width, height * 2, planes, bits, len(resource), ordinal)
            resources.append((3 if expected == 1 else 1, ordinal, resource))
            if first_image is None:
                first_image = resource
        group_type = 14 if expected == 1 else 12
        if any(t == group_type and n == identity for t, n, _ in resources):
            raise ValueError(f'Duplicate image group: {identity}')
        resources.append((group_type, identity, bytes(group)))
        images.append((expected == 2, identity, len(group), first_image))
    if not images:
        raise ValueError('No icon/cursor resources')
    output.mkdir(parents=True, exist_ok=True)
    source = ['/* Generated canonical ICON/CURSOR resources. */', '#include "internal.h"']
    manifest = ['/* Resource identities and first raw image for SDK comparison. */']
    for i, (_, _, data) in enumerate(resources):
        source += [f'static constinit const BYTE rgbImageResource{i}[] = {{']
        source += ['    ' + ','.join(f'0x{x:02x}' for x in data[j:j+32]) + ',' for j in range(0, len(data), 32)]
        source += ['};']
    source += ['constinit const COMPATRESOURCE rgCompatImageResources[] = {']
    for i, (type_, identity, _) in enumerate(resources):
        name = 'NULL' if isinstance(identity, int) else f'"{identity}"'
        ordinal = identity if isinstance(identity, int) else 0
        source += [f'    {{{name}, {ordinal}, rgbImageResource{i}, sizeof(rgbImageResource{i}), {type_}}},']
    source += ['};', 'constinit const size_t cCompatImageResources = sizeof(rgCompatImageResources) / sizeof(rgCompatImageResources[0]);']
    source += ['/* Desktop identity is separate from the original in-game icon resources. */']
    for condition, filename in (('#ifdef __APPLE__', 'stars-app-macos.png'), ('#else', 'stars-app.png')):
        app_icon = (root / 'res/icons' / filename).read_bytes()
        source += [condition, 'constinit const BYTE rgbCompatAppIcon[] = {']
        source += ['    ' + ','.join(f'0x{x:02x}' for x in app_icon[j:j+32]) + ',' for j in range(0, len(app_icon), 32)]
        source += ['};']
    source += ['#endif', 'constinit const size_t cbCompatAppIcon = sizeof(rgbCompatAppIcon);']
    for i, (_, _, _, data) in enumerate(images):
        manifest += [f'static constinit const BYTE rgbImageExpected{i}[] = {{']
        manifest += ['    ' + ','.join(f'0x{x:02x}' for x in data[j:j+32]) + ',' for j in range(0, len(data), 32)]
        manifest += ['};']
    manifest += ['static const struct { BOOL fCursor; LPCSTR psz; DWORD cbGroup; const BYTE *rgb; DWORD cb; } rgImageManifest[] = {']
    for i, (cursor, identity, group_size, _) in enumerate(images):
        name = f'MAKEINTRESOURCE({identity})' if isinstance(identity, int) else f'"{identity}"'
        manifest += [f'    {{{int(cursor)}, {name}, {group_size}, rgbImageExpected{i}, sizeof(rgbImageExpected{i})}},']
    manifest += ['};']
    (output / 'image_resources.cpp').write_text('\n'.join(source) + '\n')
    (output / 'image_manifest.h').write_text('\n'.join(manifest) + '\n')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('root', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    generate(args.root.resolve(), args.output.resolve())
