#!/usr/bin/env python3
"""Compile MENU, ACCELERATORS, DIALOG and numeric custom-file resources."""
import argparse
import ast
import re
import struct
from pathlib import Path

FLAGS_MENU = {'GRAYED': 1, 'INACTIVE': 2, 'CHECKED': 8}
FLAGS_ACCEL = {'VIRTKEY': 1, 'NOINVERT': 2, 'SHIFT': 4, 'CONTROL': 8, 'ALT': 16, 'ASCII': 0}
STRING = r'"(?:[^"\\]|\\.)*"'


def compile_resources(root):
    definitions = dict(re.findall(r'^#define\s+(\w+)\s+(0x[0-9a-fA-F]+|\d+)\s*$', (root / 'res/resource.h').read_text(), re.M))
    definitions.update(dict(re.findall(r'^#define\s+(VK_\w+)\s+(0x[0-9a-fA-F]+|\d+)\s*$', (root / 'compat/win32/include/wincompat_constants.h').read_text(), re.M)))
    constants = (root / 'compat/win32/include/wincompat_constants.h').read_text()
    for name, value in re.findall(r'^#define\s+(\w+)\s+(?:[A-Z0-9_]+\()?((?:0x[0-9a-fA-F]+|\d+))(?:\))?\s*$', constants, re.M):
        definitions.setdefault(name, value)
    lines = [line.strip() for line in (root / 'res/stars.rc').read_text().splitlines()]
    resources = []

    def number(value):
        result = int(definitions.get(value, value), 0)
        if not 0 <= result <= 65535:
            raise ValueError(f'Resource value outside WORD: {value}')
        return result

    def flags(values, mapping):
        result = 0
        for value in values:
            value = value.strip()
            if value:
                if value not in mapping:
                    raise ValueError(f'Unsupported resource flag: {value}')
                result |= mapping[value]
        return result

    def style(value, result=0):
        for name in value.split('|'):
            name = name.strip()
            negate = name.startswith('NOT ')
            if negate:
                name = name[4:].strip()
            bits = int(definitions.get(name, name), 0)
            result = result & ~bits if negate else result | bits
        return result

    def text(value):
        # RC escapes encode CP1252 bytes, not Unicode code points.
        value = ast.literal_eval(value).encode('latin1').decode('cp1252')
        return value.encode('utf-16le') + b'\0\0'

    def dialog(index, coordinates):
        dwstyle, caption, font, controls = 0, b'\0\0', None, []
        while index < len(lines) and lines[index] != 'BEGIN':
            line = lines[index]
            index += 1
            if line.startswith('STYLE '):
                dwstyle = style(line[6:])
            elif line.startswith('CAPTION '):
                caption = text(line[8:])
            elif line.startswith('FONT '):
                match = re.fullmatch(r'FONT\s+(\d+)\s*,\s*(' + STRING + ')', line)
                if not match:
                    raise ValueError(f'Unsupported dialog font: {line}')
                font = struct.pack('<H', int(match[1])) + text(match[2])
            elif line:
                raise ValueError(f'Unsupported dialog option: {line}')
        if index >= len(lines):
            raise ValueError('Missing dialog BEGIN')
        index += 1
        classes = {'button', 'edit', 'static', 'listbox', 'scrollbar', 'combobox'}
        while index < len(lines) and lines[index] != 'END':
            line = lines[index]
            index += 1
            if not line:
                continue
            match = re.fullmatch(r'CONTROL\s+(' + STRING + r')\s*,\s*(-?\w+)\s*,\s*(' + STRING + r')\s*,\s*([^,]+),\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(\d+)\s*,\s*(\d+)', line)
            if not match:
                raise ValueError(f'Unsupported dialog control: {line}')
            title, identity, class_name, flags_, *coords = match.groups()
            ident = 65535 if identity == '-1' else number(identity)
            cls = ast.literal_eval(class_name)
            # CONTROL spells out a class; windres uppercases predefined classes
            # rather than encoding the ordinals used by shorthand declarations.
            clsbytes = (cls.upper() if cls.lower() in classes else cls).encode('utf-16le') + b'\0\0'
            control = struct.pack('<IIhhhhH', style(flags_, 0x50000000), 0, *map(int, coords), ident)
            controls.append(control + clsbytes + text(title) + b'\0\0')
        if index >= len(lines) or bool(dwstyle & 0x40) != (font is not None):
            raise ValueError('Truncated dialog or inconsistent DS_SETFONT')
        data = bytearray(struct.pack('<IIHhhhh', dwstyle, 0, len(controls), *coordinates))
        data += b'\0\0\0\0' + caption + (font or b'')
        for control in controls:
            data += b'\0' * (-len(data) % 4)
            data += control
        return bytes(data), index + 1

    def menu(index, depth=0):
        if depth >= 64 or lines[index] != 'BEGIN':
            raise ValueError('Invalid menu nesting or missing BEGIN')
        index += 1
        items = []
        while index < len(lines) and lines[index] != 'END':
            line = lines[index]
            index += 1
            if not line:
                continue
            match = re.fullmatch(r'POPUP\s+(' + STRING + r')(.*)', line)
            if match:
                text = ast.literal_eval(match[1])
                mf = 0x10 | flags(match[2].split(','), FLAGS_MENU)
                children, index = menu(index, depth + 1)
                items.append((mf, None, text, children))
                continue
            match = re.fullmatch(r'MENUITEM\s+(' + STRING + r'|SEPARATOR)(?:\s*,\s*(\w+))?(.*)', line)
            if not match:
                raise ValueError(f'Unsupported menu declaration: {line}')
            if match[1] == 'SEPARATOR':
                if match[2] or match[3]:
                    raise ValueError(f'Unsupported separator: {line}')
                items.append((0, 0, '', b''))
            else:
                if not match[2]:
                    raise ValueError(f'Menu item has no command: {line}')
                items.append((flags(match[3].split(','), FLAGS_MENU), number(match[2]), ast.literal_eval(match[1]), b''))
        if index >= len(lines) or not items:
            raise ValueError('Truncated or empty menu')
        data = bytearray()
        for i, (mf, cmd, text, children) in enumerate(items):
            data += struct.pack('<H', mf | (0x80 if i == len(items) - 1 else 0))
            if cmd is not None:
                data += struct.pack('<H', cmd)
            # The canonical RC uses code page 1252; ensure representability now.
            text.encode('cp1252')
            data += text.encode('utf-16le') + b'\0\0' + children
        return bytes(data), index + 1

    index = 0
    while index < len(lines):
        binary = re.fullmatch(r'(\w+)\s+(RT_\w+|\d+)\s+("[^"]+")', lines[index])
        if binary:
            name, kind, filename = binary.groups()
            identity = number(name) if name in definitions else name.upper()
            resource_type = number(kind)
            if not 256 <= resource_type <= 65535:
                raise ValueError(f'Unsupported custom resource type: {kind}')
            path = (root / 'res' / ast.literal_eval(filename)).resolve()
            if not path.is_relative_to((root / 'res/data').resolve()):
                raise ValueError(f'Custom resource outside data directory: {filename}')
            if any(t == resource_type and n == identity for t, n, _ in resources):
                raise ValueError(f'Duplicate custom resource: {identity}')
            resources.append((resource_type, identity, path.read_bytes()))
            index += 1
            continue
        match = re.fullmatch(r'(\w+)\s+(MENU|ACCELERATORS|DIALOG)(?:\s+(-?\d+)\s*,\s*(-?\d+)\s*,\s*(\d+)\s*,\s*(\d+))?', lines[index])
        index += 1
        if not match:
            continue
        name, kind, *coords = match.groups()
        identity = number(name) if name in definitions else name.upper()
        if kind == 'DIALOG':
            data, index = dialog(index, list(map(int, coords)))
            resource_type = 5
        elif kind == 'MENU':
            data, index = menu(index)
            data = b'\0\0\0\0' + data
            resource_type = 4
        else:
            if lines[index] != 'BEGIN':
                raise ValueError('Missing accelerator BEGIN')
            index += 1
            entries = []
            while index < len(lines) and lines[index] != 'END':
                line = lines[index]
                index += 1
                if not line:
                    continue
                fields = [field.strip() for field in line.split(',')]
                if len(fields) < 2:
                    raise ValueError(f'Invalid accelerator: {line}')
                key = ast.literal_eval(fields[0]) if fields[0].startswith('"') else number(fields[0])
                if isinstance(key, str):
                    if len(key) != 1:
                        raise ValueError(f'Accelerator key must be one character: {line}')
                    key = ord(key)
                entries.append((flags(fields[2:], FLAGS_ACCEL), key, number(fields[1])))
            if index >= len(lines) or not entries:
                raise ValueError('Truncated or empty accelerator table')
            index += 1
            data = b''.join(struct.pack('<HHHH', f | (0x80 if i == len(entries) - 1 else 0), k, c, 0)
                            for i, (f, k, c) in enumerate(entries))
            resource_type = 9
        if any(t == resource_type and n == identity for t, n, _ in resources):
            raise ValueError(f'Duplicate UI resource: {identity}')
        resources.append((resource_type, identity, data))
    if not resources:
        raise ValueError('No menu/accelerator resources')
    return resources


def generate(root, output):
    resources = compile_resources(root)
    output.mkdir(parents=True, exist_ok=True)
    source = ['/* Generated from canonical res/stars.rc. */', '#include "internal.h"']
    manifest = ['/* Generated standard Win32 resource bytes for SDK comparison. */']
    for i, (_, _, data) in enumerate(resources):
        body = [f'static const BYTE rgbUIResource{i}[] = {{']
        body += ['    ' + ','.join(f'0x{x:02x}' for x in data[j:j+32]) + ',' for j in range(0, len(data), 32)]
        body += ['};']
        source += body
        manifest += body
    source += ['const COMPATRESOURCE rgCompatUIResources[] = {']
    manifest += ['static const struct { LPCSTR pszName, pszType; const BYTE *rgb; DWORD cb; } rgUIManifest[] = {']
    for i, (t, n, _) in enumerate(resources):
        name = 'NULL' if isinstance(n, int) else f'"{n}"'
        number = n if isinstance(n, int) else 0
        ident = f'MAKEINTRESOURCE({n})' if isinstance(n, int) else f'"{n}"'
        source += [f'    {{{name}, {number}, rgbUIResource{i}, sizeof(rgbUIResource{i}), {t}}},']
        manifest += [f'    {{{ident}, MAKEINTRESOURCE({t}), rgbUIResource{i}, sizeof(rgbUIResource{i})}},']
    source += ['};', 'const size_t cCompatUIResources = sizeof(rgCompatUIResources) / sizeof(rgCompatUIResources[0]);']
    manifest += ['};']
    (output / 'ui_resources.c').write_text('\n'.join(source) + '\n')
    (output / 'ui_manifest.h').write_text('\n'.join(manifest) + '\n')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('root', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    generate(args.root.resolve(), args.output.resolve())
