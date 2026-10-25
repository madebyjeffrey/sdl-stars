#!/usr/bin/env python3
"""Read WinHelp archive metadata without Wine or external decompilers.

Layout reference: Manfred Winterhoff's Windows Help File Format, revision 8.
This is an independent reader; formatted text/image rendering is still separate.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path


def unpack(data, offset, fmt):
    size = struct.calcsize(fmt)
    if offset < 0 or offset + size > len(data):
        raise ValueError('Truncated WinHelp structure')
    return struct.unpack_from(fmt, data, offset)


def string(data, offset):
    end = data.find(b'\0', offset)
    if offset < 0 or end < offset:
        raise ValueError('Unterminated WinHelp string')
    return data[offset:end].decode('cp1252'), end + 1


def leaves(data):
    """Yield bounded leaf pages in their linked order, checking tree counts."""
    magic, flags, size = unpack(data, 0, '<HHH')
    root, sentinel, count, levels, entries = unpack(data, 26, '<hhhhI')
    if magic != 0x293b or not flags & 2 or size < 8 or sentinel != -1:
        raise ValueError('Invalid WinHelp B+ tree header')
    if count <= 0 or not 1 <= levels <= count or len(data) != 38 + size * count:
        raise ValueError('Invalid WinHelp B+ tree dimensions')

    def page(index):
        if not 0 <= index < count:
            raise ValueError('Invalid WinHelp B+ tree page')
        return data[38 + index * size:38 + (index + 1) * size]

    seen = set()
    index = root
    for _ in range(levels - 1):
        if index in seen:
            raise ValueError('Cyclic WinHelp B+ tree index')
        seen.add(index)
        _, number, index = unpack(page(index), 0, '<Hhh')
        if number < 0:
            raise ValueError('Invalid WinHelp index entry count')
    previous = -1
    total = 0
    while index != -1:
        if index in seen:
            raise ValueError('Cyclic WinHelp B+ tree leaves')
        seen.add(index)
        leaf = page(index)
        unused, number, back, next_index = unpack(leaf, 0, '<Hhhh')
        if number < 0 or back != previous or unused > size - 8:
            raise ValueError('Invalid WinHelp leaf header')
        total += number
        yield leaf[:size - unused], number
        previous, index = index, next_index
    if total != entries:
        raise ValueError('WinHelp B+ tree entry count mismatch')


class HelpArchive:
    def __init__(self, data):
        magic, directory, _, size = unpack(data, 0, '<IIII')
        if magic != 0x35f3f or size < 16 or size > len(data):
            raise ValueError('Invalid WinHelp file header')
        self.data = data[:size]
        self.trailing_size = len(data) - size
        self.streams = {}
        for leaf, count in leaves(self.stream_at(directory)):
            offset = 8
            for _ in range(count):
                name, offset = string(leaf, offset)
                address, = unpack(leaf, offset, '<I')
                offset += 4
                if name in self.streams:
                    raise ValueError('Duplicate WinHelp stream name')
                self.stream_at(address)
                self.streams[name] = address
            if offset != len(leaf):
                raise ValueError('Invalid WinHelp directory leaf size')

    def stream_at(self, offset):
        reserved, used, _ = unpack(self.data, offset, '<IIB')
        if offset < 16 or reserved < 9 or used > reserved - 9 or offset + reserved > len(self.data):
            raise ValueError('Invalid WinHelp stream bounds')
        return self.data[offset + 9:offset + 9 + used]

    def stream(self, name):
        if name not in self.streams:
            raise ValueError(f'Missing WinHelp stream: {name}')
        return self.stream_at(self.streams[name])

    def metadata(self):
        system = self.stream('|SYSTEM')
        magic, minor, major, timestamp, flags = unpack(system, 0, '<HHHIH')
        if magic != 0x36c or major != 1 or minor <= 16:
            raise ValueError('Expected a Windows 3.1 or later help file')
        records = {}
        offset = 12
        while offset < len(system):
            kind, size = unpack(system, offset, '<HH')
            offset += 4
            if offset + size > len(system):
                raise ValueError('Truncated WinHelp system record')
            records.setdefault(kind, []).append(system[offset:offset + size])
            offset += size
        if 1 not in records or 3 not in records:
            raise ValueError('Missing WinHelp title or contents record')
        titles = {}
        for leaf, count in leaves(self.stream('|TTLBTREE')):
            offset = 8
            for _ in range(count):
                topic, = unpack(leaf, offset, '<I')
                title, offset = string(leaf, offset + 4)
                if topic in titles:
                    raise ValueError('Duplicate WinHelp topic title')
                titles[topic] = title
            if offset != len(leaf):
                raise ValueError('Invalid WinHelp title leaf size')
        mapping = self.stream('|CTXOMAP')
        count, = unpack(mapping, 0, '<H')
        if len(mapping) != 2 + count * 8:
            raise ValueError('Invalid WinHelp context map size')
        contexts = {}
        for index in range(count):
            identifier, topic = unpack(mapping, 2 + index * 8, '<II')
            if identifier in contexts:
                raise ValueError('Duplicate WinHelp context ID')
            contexts[identifier] = topic
        return {
            'sha256': hashlib.sha256(self.data).hexdigest(),
            'archive_bytes': len(self.data), 'trailing_bytes': self.trailing_size,
            'format_minor': minor, 'generated_timestamp': timestamp, 'flags': flags,
            'title': string(records[1][0], 0)[0],
            'contents_topic': unpack(records[3][0], 0, '<I')[0],
            'streams': {name: len(self.stream(name)) for name in sorted(self.streams)},
            'titles': titles, 'contexts': contexts,
        }

    def topic_records(self):
        """Decode the original HC31 file's uncompressed record chain.

        HC31 positions use 16 KiB virtual slots even when a physical block is
        uncompressed and only 4 KiB long. Records crossing physical blocks omit
        the next block's 12-byte header. Keep formatting and text streams apart
        for the renderer instead of guessing whitespace between text runs.
        """
        metadata = self.metadata()
        if metadata['format_minor'] != 21 or metadata['flags'] != 0:
            raise ValueError('Topic decoding requires uncompressed HC31 data')
        data = self.stream('|TOPIC')
        blocks = []
        for offset in range(0, len(data), 4096):
            unpack(data, offset, '<III')
            blocks.append(data[offset + 12:offset + 4096])

        def read(position, size):
            if position < 12 or size < 0:
                raise ValueError('Invalid WinHelp topic position or size')
            index, offset = divmod(position - 12, 16384)
            parts = []
            while size:
                if not 0 <= index < len(blocks) or not 0 <= offset < len(blocks[index]):
                    raise ValueError('WinHelp topic data outside physical block')
                part = blocks[index][offset:offset + size]
                parts.append(part)
                size -= len(part)
                index += 1
                offset = 0
            return b''.join(parts)

        position, = unpack(data, 4, '<I')
        previous = 0xffffffff
        seen = set()
        block = -1
        character = 0
        while position != 0xffffffff:
            if position in seen:
                raise ValueError('Cyclic WinHelp topic chain')
            seen.add(position)
            size, text_size, back, next_position, format_size, kind = unpack(read(position, 21), 0, '<IIIIIB')
            if back != previous or not 21 <= format_size <= size or kind not in (2, 0x20, 0x23):
                raise ValueError('Invalid WinHelp topic record')
            raw = read(position, size)
            if text_size != size - format_size:
                raise ValueError('Phrase-compressed topic text is not supported')
            index = (position - 12) // 16384
            if index != block:
                block, character = index, 0
            formatting, text = raw[21:format_size], raw[format_size:]
            length = 0
            if kind == 2:
                if len(formatting) != 28:
                    raise ValueError('Invalid HC31 topic header size')
            else:
                # Skip the compressed TopicSize, then read unsigned TopicLength.
                word, = unpack(formatting, 0, '<H')
                offset = 4 if word & 1 else 2
                value, = unpack(formatting, offset, '<B')
                length = value >> 1
                if value & 1:
                    high, = unpack(formatting, offset + 1, '<B')
                    length += high * 128
                if character + length > 32768:
                    raise ValueError('WinHelp topic character offset overflow')
            yield {'position': position, 'offset': block * 32768 + character,
                   'kind': kind, 'length': length, 'formatting': formatting, 'text': text}
            character += length
            previous, position = position, next_position

    def topics(self):
        """Group records and resolve numeric contexts through character ranges."""
        metadata = self.metadata()
        topics = {}
        ranges = {}
        current = None
        for record in self.topic_records():
            if record['kind'] == 2:
                offset = record['offset']
                # HC31 also permits a title filling DataLen2 without a NUL;
                # an empty title is valid for unnamed popup topics.
                title = record['text'].split(b'\0', 1)[0].decode('cp1252')
                if offset in topics or metadata['titles'].get(offset) != title:
                    raise ValueError('WinHelp topic header/title index mismatch')
                current = {'offset': offset, 'title': title, 'header': record, 'records': [], 'contexts': []}
                topics[offset] = current
            else:
                if current is None:
                    raise ValueError('WinHelp text without a topic header')
                current['records'].append(record)
                block, character = divmod(record['offset'], 32768)
                ranges.setdefault(block, []).append((character, character + record['length'], current))
        if topics.keys() != metadata['titles'].keys():
            raise ValueError('WinHelp topic chain does not cover title index')
        for identifier, offset in metadata['contexts'].items():
            block, character = divmod(offset, 32768)
            matches = [topic for start, end, topic in ranges.get(block, []) if start <= character < end]
            if len(matches) != 1:
                raise ValueError(f'Unresolved or ambiguous WinHelp context: {identifier}')
            matches[0]['contexts'].append(identifier)
        return topics

    def context_hashes(self):
        result = {}
        for leaf, count in leaves(self.stream('|CONTEXT')):
            if len(leaf) != 8 + count * 8:
                raise ValueError('Invalid WinHelp named-context leaf size')
            for index in range(count):
                identifier, offset = unpack(leaf, 8 + index * 8, '<II')
                if identifier in result:
                    raise ValueError('Duplicate WinHelp context hash')
                result[identifier] = offset
        return result

    def topic_at(self, offset, topics):
        if offset in topics:
            return topics[offset]
        block, character = divmod(offset, 32768)
        for topic in topics.values():
            for record in topic['records']:
                record_block, start = divmod(record['offset'], 32768)
                if block == record_block and start <= character < start + record['length']:
                    return topic
        raise ValueError(f'Unresolved WinHelp topic offset: {offset}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('help_file', type=Path)
    parser.add_argument('--topics', action='store_true', help='Include topic record counts and resolved contexts')
    args = parser.parse_args()
    archive = HelpArchive(args.help_file.read_bytes())
    result = archive.metadata()
    if args.topics:
        result['topics'] = {offset: {'title': topic['title'], 'records': len(topic['records']),
                                    'contexts': topic['contexts']}
                            for offset, topic in archive.topics().items()}
    print(json.dumps(result, indent=2))
