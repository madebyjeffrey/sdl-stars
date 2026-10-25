#!/usr/bin/env python3
"""Retag curated ASCII fixtures in working copies; never migrate user saves.

Only use this for fixtures whose text and record schema are already modern.
Older tutorial turn assets require a load/save through their original reader.
BOF versions do not seed encryption, so retagging leaves payload bytes intact.
"""
from pathlib import Path
import argparse
import struct


def utf8_fixture_bytes(data):
    data = bytearray(data)
    offset = 0
    while offset < len(data):
        if offset + 2 > len(data):
            raise ValueError("truncated fixture header")
        header = struct.unpack_from('<H', data, offset)[0]
        kind, size = header >> 10, header & 1023
        if offset + 2 + size > len(data):
            raise ValueError("truncated fixture record")
        if kind == 8:
            if size < 16:
                raise ValueError("truncated BOF")
            version = struct.unpack_from('<H', data, offset + 10)[0]
            if version >> 12 != 2 or (version >> 5) & 127 not in (83, 84, 85):
                raise ValueError("fixture needs schema conversion before retagging")
            struct.pack_into('<H', data, offset + 10, (version & ~0xfe0) | (85 << 5))
            if data[offset + 16] == 0:  # XY: raw STARPACK data follows GAME.
                break
        offset += size + 2
    return bytes(data)


def copy_utf8_fixture(source, destination):
    Path(destination).write_bytes(utf8_fixture_bytes(Path(source).read_bytes()))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('destination', type=Path)
    args = parser.parse_args()
    copy_utf8_fixture(args.source, args.destination)
