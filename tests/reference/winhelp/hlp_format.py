"""Decode HC31 paragraph/table properties and alternating text/command runs.

Independent implementation from Winterhoff's Windows Help File Format.
Keep byte strings and original opcodes: font-specific character decoding and
link-target resolution belong to the renderer, not this binary-format layer.
"""
import struct


class Cursor:
    def __init__(self, data):
        self.data = data
        self.offset = 0

    def take(self, size):
        if size < 0 or self.offset + size > len(self.data):
            raise ValueError('Truncated WinHelp formatting data')
        result = self.data[self.offset:self.offset + size]
        self.offset += size
        return result

    def number(self, fmt):
        return struct.unpack(fmt, self.take(struct.calcsize(fmt)))[0]

    def packed(self, wide=False, signed=False):
        low = self.number('<H' if wide else '<B')
        extended = low & 1
        value = low >> 1
        if extended:
            value += self.number('<H' if wide else '<B') << (15 if wide else 7)
        if signed:
            value -= (1 << (30 if wide else 14)) if extended else (1 << (14 if wide else 6))
        return value

    def text(self):
        end = self.data.find(b'\0', self.offset)
        if end < self.offset:
            raise ValueError('Unterminated WinHelp display string')
        return self.take(end - self.offset + 1)[:-1]

    def finish(self):
        if self.offset != len(self.data):
            raise ValueError('Unconsumed WinHelp formatting or text data')


def paragraph(cursor):
    unknown = cursor.take(2)
    identifier = cursor.number('<H')
    flags = cursor.number('<H')
    if flags & ~0x0f7f:
        raise ValueError('Unknown WinHelp paragraph flags')
    result = {'unknown': unknown, 'identifier': identifier, 'flags': flags,
              'alignment': 'right' if flags & 0x400 else 'center' if flags & 0x800 else 'left'}
    if flags & 1:
        result['extra'] = cursor.packed(wide=True, signed=True)
    for bit, name in enumerate(('above', 'below', 'line', 'left', 'right', 'first'), 1):
        if flags & (1 << bit):
            result[name] = cursor.packed(signed=True)
    if flags & 0x100:
        result['border'] = (cursor.number('<B'), cursor.number('<h'))
    if flags & 0x200:
        count = cursor.packed(signed=True)
        if count < 0:
            raise ValueError('Negative WinHelp tab count')
        tabs = []
        for _ in range(count):
            position = cursor.packed()
            kind = cursor.packed() if position & 0x4000 else 0
            tabs.append((position & 0x3fff, kind))
        result['tabs'] = tabs
    return result


def command(cursor):
    opcode = cursor.number('<B')
    result = {'opcode': opcode}
    if opcode in (0x81, 0x82, 0x83, 0x89, 0x8b, 0x8c, 0xff):
        pass
    elif opcode == 0x80:
        result['font'] = cursor.number('<H')
    elif opcode == 0x20:
        result['field'] = cursor.number('<I')
    elif opcode == 0x21:
        result['dtype'] = cursor.number('<H')
    elif opcode in (0x86, 0x87, 0x88):
        kind = cursor.number('<B')
        size = cursor.packed(wide=True, signed=True)
        result['picture_kind'] = kind
        if kind == 0x22:
            result['hotspot_count'] = cursor.packed()
        if kind not in (3, 0x22, 5):
            raise ValueError(f'Unknown WinHelp picture type: {kind:#x}')
        result['picture_data'] = cursor.take(size)
    elif opcode in (0xc8, 0xcc):
        size = cursor.number('<H')
        result['macro'] = cursor.take(size - 3)
    elif opcode in (0xe0, 0xe1, 0xe2, 0xe3, 0xe6, 0xe7):
        result['target'] = cursor.number('<I')
    elif opcode in (0xea, 0xeb, 0xee, 0xef):
        size = cursor.number('<H')
        result['external'] = cursor.take(size)
    else:
        raise ValueError(f'Unknown WinHelp character command: {opcode:#x}')
    return result


def decode_record(record):
    if record['kind'] not in (0x20, 0x23):
        raise ValueError('Expected a WinHelp display/table record')
    formatting = Cursor(record['formatting'])
    text = Cursor(record['text'])
    topic_size = formatting.packed(wide=True, signed=True)
    topic_length = formatting.packed()
    if topic_length != record['length']:
        raise ValueError('WinHelp topic length mismatch')
    result = {'topic_size': topic_size, 'length': topic_length, 'paragraphs': []}
    table = record['kind'] == 0x23
    if table:
        count = formatting.number('<B')
        kind = formatting.number('<B')
        if count == 0 or kind not in (0, 1, 2, 3):
            raise ValueError('Invalid WinHelp table dimensions')
        minimum = formatting.number('<h') if kind in (0, 2) else None
        columns = [(formatting.number('<h'), formatting.number('<h')) for _ in range(count)]
        result['table'] = {'kind': kind, 'minimum': minimum, 'columns': columns}
    while True:
        column = None
        if table:
            column = formatting.number('<h')
            if column == -1:
                break
            if not 0 <= column < count:
                raise ValueError('Invalid WinHelp table column')
            formatting.take(3)
        properties = paragraph(formatting)
        runs = []
        while True:
            value = text.text()
            event = command(formatting)
            runs.append({'text': value, 'command': event})
            if event['opcode'] == 0xff:
                break
        result['paragraphs'].append({'column': column, 'properties': properties, 'runs': runs})
        if not table:
            break
    formatting.finish()
    text.finish()
    return result


def decode_topics(archive):
    """Resolve the original file's links and stream-backed image placements."""
    topics = archive.topics()
    hashes = archive.context_hashes()
    for topic in topics.values():
        topic['display'] = []
        for record in topic['records']:
            decoded = decode_record(record)
            for paragraph in decoded['paragraphs']:
                for run in paragraph['runs']:
                    event = run['command']
                    opcode = event['opcode']
                    if opcode in (0xe2, 0xe3, 0xe6, 0xe7):
                        if event['target'] not in hashes:
                            raise ValueError('Missing WinHelp link context hash')
                        event['topic'] = archive.topic_at(hashes[event['target']], topics)['offset']
                    elif opcode in (0xe0, 0xe1):
                        event['topic'] = archive.topic_at(event['target'], topics)['offset']
                    elif opcode in (0x86, 0x87, 0x88):
                        payload = Cursor(event['picture_data'])
                        embedded = payload.number('<h')
                        if event['picture_kind'] not in (3, 0x22) or embedded != 0:
                            raise ValueError('Embedded WinHelp image/control not supported')
                        index = payload.number('<H')
                        payload.finish()
                        name = f'|bm{index}'
                        archive.stream(name)
                        event['image'] = name
            topic['display'].append(decoded)
    return topics
