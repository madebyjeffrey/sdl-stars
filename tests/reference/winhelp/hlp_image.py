"""Decode the original guide's RLE DIBs, metafiles and image hotspots."""
import binascii
import base64
import struct
import subprocess
import tempfile
import xml.etree.ElementTree as ET
import zlib
from pathlib import Path

from hlp_format import Cursor

MAX_PIXELS = 16 * 1024 * 1024
MAX_DATA = MAX_PIXELS * 4


def decompress(data, packing, size):
    if not 0 <= size <= MAX_DATA:
        raise ValueError('WinHelp image exceeds decompression limit')
    if packing == 0:
        result = data
    elif packing == 1:
        result = bytearray()
        cursor = Cursor(data)
        while cursor.offset < len(data):
            count = cursor.number('<B')
            if count & 0x80:
                value = cursor.take(count & 0x7f)
            else:
                value = cursor.take(1) * count
            if len(result) + len(value) > size:
                raise ValueError('WinHelp RLE output exceeds image size')
            result.extend(value)
        result = bytes(result)
    else:
        raise ValueError(f'Unsupported WinHelp image packing: {packing}')
    if len(result) != size:
        raise ValueError('WinHelp image decompressed size mismatch')
    return result


def hotspots(data):
    if not data:
        return []
    cursor = Cursor(data)
    version, count, macro_size = struct.unpack('<BHI', cursor.take(struct.calcsize('<BHI')))
    if version != 1:
        raise ValueError('Invalid WinHelp hotspot header')
    result = []
    for _ in range(count):
        opcode, flags, reserved, x, y, width, height, target = struct.unpack('<BBBHHHHI', cursor.take(struct.calcsize('<BBBHHHHI')))
        if opcode not in (0xe2, 0xe3, 0xe6, 0xe7) or flags != (4 if opcode >= 0xe6 else 0) or reserved:
            raise ValueError('Unsupported WinHelp image hotspot command')
        result.append({'opcode': opcode, 'x': x, 'y': y, 'width': width,
                       'height': height, 'target': target})
    cursor.take(macro_size)
    for item in result:
        item['name'] = cursor.text().decode('cp1252')
        item['context'] = cursor.text().decode('cp1252')
    cursor.finish()
    return result


def decode_image(data):
    cursor = Cursor(data)
    magic, count = struct.unpack('<HH', cursor.take(struct.calcsize('<HH')))
    if magic not in (0x506c, 0x706c) or count != 1:
        raise ValueError('Expected a single WinHelp picture variant')
    offset = cursor.number('<I')
    if offset < cursor.offset or offset >= len(data):
        raise ValueError('WinHelp picture offset outside stream')
    cursor = Cursor(data[offset:])
    kind, packing = struct.unpack('<BB', cursor.take(struct.calcsize('<BB')))
    if kind == 6:
        xdpi, ydpi = cursor.packed(wide=True), cursor.packed(wide=True)
        planes, bits = cursor.packed(), cursor.packed()
        width, height, colors, transparent, size, hotspot_size = [cursor.packed(wide=True) for _ in range(6)]
        if planes != 1 or bits not in (1, 4, 8) or transparent:
            raise ValueError('Unsupported WinHelp DIB format')
        if not width or not height or width * height > MAX_PIXELS:
            raise ValueError('Invalid WinHelp bitmap dimensions')
        if colors > 1 << bits:
            raise ValueError('Invalid WinHelp bitmap palette size')
        image_offset, hotspot_offset = struct.unpack('<II', cursor.take(struct.calcsize('<II')))
        palette = [tuple(cursor.take(4)) for _ in range(colors or 1 << bits)]
        stride = ((width * bits + 31) // 32) * 4
        expected = stride * height
        result = {'kind': 'bitmap', 'width': width, 'height': height, 'dpi': (xdpi, ydpi)}
    elif kind == 8:
        mapping = cursor.packed()
        width, height = struct.unpack('<HH', cursor.take(struct.calcsize('<HH')))
        expected, size, hotspot_size = [cursor.packed(wide=True) for _ in range(3)]
        image_offset, hotspot_offset = struct.unpack('<II', cursor.take(struct.calcsize('<II')))
        if not width or not height or mapping != 8:
            raise ValueError('Unsupported WinHelp metafile mapping')
        result = {'kind': 'metafile', 'width': width, 'height': height, 'mapping': mapping}
    else:
        raise ValueError(f'Unsupported WinHelp picture type: {kind}')
    if image_offset < cursor.offset or image_offset + size > len(cursor.data):
        raise ValueError('WinHelp image data outside picture')
    pixels = decompress(cursor.data[image_offset:image_offset + size], packing, expected)
    if hotspot_size:
        if hotspot_offset < image_offset + size or hotspot_offset + hotspot_size > len(cursor.data):
            raise ValueError('WinHelp hotspots outside picture')
        result['hotspots'] = hotspots(cursor.data[hotspot_offset:hotspot_offset + hotspot_size])
    elif hotspot_offset:
        raise ValueError('WinHelp hotspot offset without data')
    else:
        result['hotspots'] = []
    if kind == 8:
        result['wmf'] = pixels
    else:
        rgb = bytearray()
        for y in range(height - 1, -1, -1):
            row = pixels[y * stride:(y + 1) * stride]
            for x in range(width):
                index = (row[x * bits // 8] >> (8 - bits - (x * bits % 8))) & ((1 << bits) - 1)
                if index >= len(palette):
                    raise ValueError('WinHelp pixel outside palette')
                blue, green, red, _ = palette[index]
                rgb.extend((red, green, blue))
        result['rgb'] = bytes(rgb)
    return result


def decode_images(archive, topics):
    hashes = archive.context_hashes()
    result = {}
    for name in archive.streams:
        if name.startswith('|bm'):
            item = decode_image(archive.stream(name))
            for spot in item['hotspots']:
                if spot['target'] not in hashes:
                    raise ValueError('Unresolved WinHelp image hotspot')
                spot['topic'] = archive.topic_at(hashes[spot['target']], topics)['offset']
                if spot['x'] + spot['width'] > item['width'] or spot['y'] + spot['height'] > item['height']:
                    raise ValueError('WinHelp hotspot outside image')
            result[name] = item
    return result


def png_bytes(item):
    width, height = item['width'], item['height']
    rgb = item['rgb']
    if len(rgb) != width * height * 3:
        raise ValueError('Invalid RGB image size')
    def chunk(name, data):
        return struct.pack('>I', len(data)) + name + data + struct.pack('>I', binascii.crc32(name + data) & 0xffffffff)
    rows = b''.join(b'\0' + rgb[y * width * 3:(y + 1) * width * 3] for y in range(height))
    return (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 2, 0, 0, 0))
            + chunk(b'IDAT', zlib.compress(rows)) + chunk(b'IEND', b''))


def placeable_wmf(item):
    # The guide stores an ordinary WMF with separate HIMETRIC extents. Aldus'
    # placeable header supplies the bounds to the standalone converter.
    width, height = item['width'], item['height']
    if width > 32767 or height > 32767:
        raise ValueError('WMF dimensions exceed placeable bounds')
    header = struct.pack('<IHhhhhHI', 0x9ac6cdd7, 0, 0, 0, width, height, 2540, 0)
    checksum = 0
    for word in struct.unpack('<10H', header):
        checksum ^= word
    return header + struct.pack('<H', checksum) + item['wmf']


def svg_bytes(item, converter='wmf2svg'):
    with tempfile.TemporaryDirectory(prefix='stars-help-wmf-') as directory:
        source, output = Path(directory) / 'picture.wmf', Path(directory) / 'picture.svg'
        source.write_bytes(placeable_wmf(item))
        subprocess.run([converter, '--inline', '-o', str(output), str(source)],
                       check=True, capture_output=True, timeout=30)
        data = output.read_bytes()
    # libwmf 0.2.13 emits xlink attributes without declaring either SVG
    # namespace. Supply those declarations before parsing its XML output.
    data = data.replace(b'<svg ', b'<svg xmlns="http://www.w3.org/2000/svg" '
                        b'xmlns:xlink="http://www.w3.org/1999/xlink" ', 1)
    root = ET.fromstring(data)
    if root.tag != '{http://www.w3.org/2000/svg}svg':
        raise ValueError('WMF converter did not produce SVG')
    for image in root.iter('{http://www.w3.org/2000/svg}image'):
        uri = image.attrib.get('{http://www.w3.org/1999/xlink}href', '')
        if not uri.startswith('data:image/png;base64,'):
            raise ValueError('WMF converter produced an external image')
        png = base64.b64decode(''.join(uri.split(',', 1)[1].split()), validate=True)
        if len(png) < 24 or png[:8] != b'\x89PNG\r\n\x1a\n' or png[12:16] != b'IHDR':
            raise ValueError('WMF converter produced an invalid embedded PNG')
        # libwmf's SVG writer sets the already scaled image dimensions AND
        # a scale transform. Restore the intrinsic pixels so it scales once.
        width, height = struct.unpack_from('>II', png, 16)
        image.set('width', str(width))
        image.set('height', str(height))
    root.set('viewBox', f"0 0 {root.attrib['width']} {root.attrib['height']}")
    root.set('width', f"{item['width'] * 96 / 2540:g}")
    root.set('height', f"{item['height'] * 96 / 2540:g}")
    ET.register_namespace('', 'http://www.w3.org/2000/svg')
    ET.register_namespace('xlink', 'http://www.w3.org/1999/xlink')
    return ET.tostring(root, encoding='utf-8', xml_declaration=True)
