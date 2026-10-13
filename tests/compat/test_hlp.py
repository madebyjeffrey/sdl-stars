#!/usr/bin/env python3
"""Check original help metadata and rejection of damaged archive structures."""
import hashlib
import json
import base64
import re
from collections import Counter
import struct
import sys
import tempfile
from html.parser import HTMLParser
import unittest
import xml.etree.ElementTree as ET
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'compat/win32'))
import hlp as HLP
import hlp_format as FORMAT
import hlp_font as FONT
import hlp_image as IMAGE
import hlp_render as RENDER


class GuideHTML(HTMLParser):
    def __init__(self, text):
        super().__init__()
        self.main = False
        self.text = []
        self.links = []
        self.images = []
        self.tables = 0
        self.cells = []
        self.feed(text)

    def handle_starttag(self, tag, attributes):
        attributes = dict(attributes)
        if tag == 'main':
            self.main = True
        if tag == 'a':
            self.links.append(attributes)
        if tag == 'img':
            self.images.append(attributes)
        if tag == 'table':
            self.tables += 1
        if tag == 'td':
            self.cells.append(int(attributes['data-column']))

    def handle_endtag(self, tag):
        if tag == 'main':
            self.main = False

    def handle_data(self, text):
        if self.main:
            self.text.append(text)


class HelpTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = (ROOT / 'res/stars!.hlp').read_bytes()
        cls.archive = HLP.HelpArchive(cls.data)

    def test_original_metadata(self):
        meta = self.archive.metadata()
        self.assertEqual(meta['sha256'], '43d99794da2d12e67ddbeefb3ec81b8c24309f40982b2aa7daa8e4dc5c669309')
        self.assertEqual(meta['archive_bytes'], 1662571)
        self.assertEqual(meta['trailing_bytes'], 72)
        self.assertEqual(meta['title'], "Stars! Player's Guide, v2.6b")
        self.assertEqual((meta['format_minor'], meta['flags']), (21, 0))
        self.assertEqual(len(meta['streams']), 142)
        self.assertEqual(sum(name.startswith('|bm') for name in meta['streams']), 133)
        self.assertEqual(len(meta['titles']), 414)
        self.assertEqual(len(meta['contexts']), 413)
        self.assertEqual(meta['titles'][meta['contents_topic']], "Stars! Player's Guide - Contents")
        self.assertEqual(meta['titles'][meta['contexts'][4501]], 'Welcome to Stars!')
        self.assertEqual(meta['titles'][meta['contexts'][1070]], 'Research Dialog')
        self.assertNotIn(13002, meta['contexts'])
        # Some map offsets identify a block boundary rather than the title key.
        # Resolve through display records, preserving the owning earlier topic.
        self.assertEqual({identifier for identifier, topic in meta['contexts'].items()
                          if topic not in meta['titles']}, {1514, 3004, 3130, 5514, 1017})

    def test_trailing_data_is_not_part_of_archive(self):
        self.assertEqual(HLP.HelpArchive(self.data[:1662571]).metadata()['sha256'],
                         self.archive.metadata()['sha256'])

    def test_truncation_and_bad_header(self):
        for data in (self.data[:15], self.data[:1662570], b'nope' + self.data[4:]):
            with self.subTest(length=len(data)), self.assertRaises(ValueError):
                HLP.HelpArchive(data)

    def test_stream_outside_archive(self):
        data = bytearray(self.data)
        struct.pack_into('<I', data, 4, len(data) - 4)
        with self.assertRaises(ValueError):
            HLP.HelpArchive(data)

    def test_stream_reserved_size(self):
        data = bytearray(self.data)
        struct.pack_into('<I', data, self.archive.streams['|SYSTEM'], 8)
        with self.assertRaises(ValueError):
            HLP.HelpArchive(data)

    def test_tree_bad_page_and_cycle(self):
        tree = bytearray(self.archive.stream('|TTLBTREE'))
        struct.pack_into('<h', tree, 26, 32767)
        with self.assertRaises(ValueError):
            list(HLP.leaves(tree))
        # Follow the actual root to the first leaf, then make it point to itself.
        tree = bytearray(self.archive.stream('|TTLBTREE'))
        page_size, = struct.unpack_from('<H', tree, 4)
        index, = struct.unpack_from('<h', tree, 26)
        levels, = struct.unpack_from('<h', tree, 32)
        for _ in range(levels - 1):
            index, = struct.unpack_from('<h', tree, 38 + page_size * index + 4)
        struct.pack_into('<h', tree, 38 + page_size * index + 6, index)
        with self.assertRaises(ValueError):
            list(HLP.leaves(tree))

    def test_tree_count_mismatch(self):
        tree = bytearray(self.archive.stream('|TTLBTREE'))
        struct.pack_into('<I', tree, 34, 415)
        with self.assertRaises(ValueError):
            list(HLP.leaves(tree))

    def test_system_record_truncated(self):
        data = bytearray(self.data)
        offset = self.archive.streams['|SYSTEM'] + 9
        struct.pack_into('<H', data, offset + 14, 65535)
        with self.assertRaises(ValueError):
            HLP.HelpArchive(data).metadata()

    def test_context_map_count_and_duplicate(self):
        offset = self.archive.streams['|CTXOMAP'] + 9
        data = bytearray(self.data)
        struct.pack_into('<H', data, offset, 65535)
        with self.assertRaises(ValueError):
            HLP.HelpArchive(data).metadata()

    def test_all_original_topic_records(self):
        records = list(self.archive.topic_records())
        self.assertEqual(Counter(record['kind'] for record in records), {2: 414, 0x20: 3638, 0x23: 148})
        self.assertEqual(sum(len(record['text']) for record in records), 415602)
        self.assertEqual(hashlib.sha256(b''.join(record['text'] for record in records)).hexdigest(),
                         '4f95f03c51be97302b67dfd7faa79293e0081d465b664dcb687499ef7e0f3586')

    def test_contexts_resolve_through_records(self):
        topics = self.archive.topics()
        contexts = {identifier: topic for topic in topics.values() for identifier in topic['contexts']}
        self.assertEqual(len(topics), 414)
        self.assertEqual(len(contexts), 413)
        expected = {1514: 'Location Tile', 3004: 'Population',
                    3130: 'Fleet Reports on Enemies and other Players',
                    5514: 'Load from Fleet', 1017: 'Slower Tech Advances', 1070: 'Research Dialog'}
        for identifier, title in expected.items():
            self.assertEqual(contexts[identifier]['title'], title)
        self.assertIn(b'The Research dialog displays research status',
                      b''.join(record['text'] for record in contexts[1070]['records']))

    def test_topic_record_crosses_physical_block(self):
        record = next(record for record in self.archive.topic_records() if record['position'] == 19773)
        data = self.archive.stream('|TOPIC')
        size, _, _, _, format_size = struct.unpack_from('<IIIII', data, 7485)
        self.assertGreater(7485 + size, 8192)
        raw = data[7485:8192] + data[8204:8204 + size - (8192 - 7485)]
        self.assertEqual(record['formatting'], raw[21:format_size])
        self.assertEqual(record['text'], raw[format_size:])

    def test_damaged_topic_chain(self):
        start = self.archive.streams['|TOPIC'] + 9
        for offset, fmt, value in ((12 + 12, '<I', 12), (12 + 8, '<I', 0),
                                   (12 + 12, '<I', 16395), (12 + 16, '<I', 20),
                                   (12 + 20, '<B', 255), (12 + 4, '<I', 65535)):
            data = bytearray(self.data)
            struct.pack_into(fmt, data, start + offset, value)
            with self.subTest(offset=offset, value=value), self.assertRaises(ValueError):
                list(HLP.HelpArchive(data).topic_records())

    def test_topic_title_index_disagreement(self):
        data = bytearray(self.data)
        # First header: title follows its 21-byte link and 28-byte topic header.
        data[self.archive.streams['|TOPIC'] + 9 + 12 + 49] = ord('X')
        with self.assertRaises(ValueError):
            HLP.HelpArchive(data).topics()

    def test_compressed_topic_file_rejected(self):
        data = bytearray(self.data)
        struct.pack_into('<H', data, self.archive.streams['|SYSTEM'] + 9 + 10, 4)
        with self.assertRaisesRegex(ValueError, 'uncompressed HC31'):
            list(HLP.HelpArchive(data).topic_records())

    def test_duplicate_context_id(self):
        data = bytearray(self.data)
        offset = self.archive.streams['|CTXOMAP'] + 9
        data[offset + 10:offset + 14] = data[offset + 2:offset + 6]
        with self.assertRaises(ValueError):
            HLP.HelpArchive(data).metadata()

    def test_original_display_and_table_formatting(self):
        count = 0
        paragraphs = 0
        opcodes = Counter()
        for record in self.archive.topic_records():
            if record['kind'] == 2:
                continue
            decoded = FORMAT.decode_record(record)
            count += 1
            paragraphs += len(decoded['paragraphs'])
            runs = [run for paragraph in decoded['paragraphs'] for run in paragraph['runs']]
            self.assertEqual(b''.join(run['text'] + b'\0' for run in runs), record['text'])
            opcodes.update(run['command']['opcode'] for run in runs)
        self.assertEqual(count, 3786)
        self.assertEqual(paragraphs, 4270)
        self.assertEqual(opcodes, {0x80: 7994, 0x82: 4511, 0xff: 4270,
                                  0x89: 1498, 0xe3: 1094, 0x83: 697,
                                  0xe2: 404, 0x81: 185, 0x87: 77, 0x86: 63})

    def test_original_contents_image_command(self):
        record = next(record for record in self.archive.topic_records() if record['position'] == 167)
        decoded = FORMAT.decode_record(record)
        image = next(run['command'] for run in decoded['paragraphs'][0]['runs']
                     if run['command']['opcode'] == 0x86)
        self.assertEqual(image, {'opcode': 0x86, 'picture_kind': 0x22,
                                'hotspot_count': 9, 'picture_data': b'\0\0\0\0'})

    def test_original_table_cell_structure(self):
        record = next(record for record in self.archive.topic_records() if record['position'] == 370)
        decoded = FORMAT.decode_record(record)
        self.assertEqual(decoded['table'], {'kind': 1, 'minimum': None, 'columns': [(480, 1)]})
        self.assertEqual([paragraph['column'] for paragraph in decoded['paragraphs']], [0, 0, 0, 0])
        self.assertEqual(decoded['paragraphs'][0]['properties']['below'], 24)
        self.assertEqual(decoded['paragraphs'][0]['properties']['alignment'], 'left')

    def test_packed_numbers(self):
        cases = [(b'\x00', False, False, 0), (b'\xfe', False, False, 127),
                 (b'\x01\x01', False, False, 128), (b'\x80', False, True, 0),
                 (b'\x00', False, True, -64), (b'\x01\x80', False, True, 0),
                 (b'\x08\x80', True, True, 4), (b'\x01\x00\x00\x80', True, True, 0)]
        for data, wide, signed, expected in cases:
            with self.subTest(data=data):
                cursor = FORMAT.Cursor(data)
                self.assertEqual(cursor.packed(wide=wide, signed=signed), expected)
                cursor.finish()
        for data, wide in ((b'\x01', False), (b'\x01\x00', True)):
            with self.assertRaises(ValueError):
                FORMAT.Cursor(data).packed(wide=wide)

    def test_formatter_rejects_truncation_and_trailing_data(self):
        record = next(record for record in self.archive.topic_records() if record['kind'] == 0x20)
        for key in ('formatting', 'text'):
            for value in (record[key][:-1], record[key] + b'\0'):
                changed = dict(record, **{key: value})
                with self.subTest(key=key, size=len(value)), self.assertRaises(ValueError):
                    FORMAT.decode_record(changed)

    def test_formatter_rejects_unknown_commands_and_flags(self):
        with self.assertRaisesRegex(ValueError, 'Unknown WinHelp character command'):
            FORMAT.command(FORMAT.Cursor(b'\x99'))
        with self.assertRaisesRegex(ValueError, 'Unknown WinHelp paragraph flags'):
            FORMAT.paragraph(FORMAT.Cursor(b'\0\0\0\0\x80\0'))
        with self.assertRaises(ValueError):
            FORMAT.decode_record({'kind': 2})

    def test_links_and_image_placements(self):
        topics = FORMAT.decode_topics(self.archive)
        self.assertEqual(len(self.archive.context_hashes()), 413)
        events = [run['command'] for topic in topics.values() for record in topic['display']
                  for paragraph in record['paragraphs'] for run in paragraph['runs']]
        links = [event for event in events if 'topic' in event]
        images = [event for event in events if 'image' in event]
        self.assertEqual(len(links), 1498)
        self.assertEqual(len(images), 140)
        self.assertEqual(len({event['image'] for event in images}), 133)
        self.assertTrue(all(event['topic'] in topics for event in links))
        for identifier, title in {1514: 'Location Tile', 3004: 'Population', 5514: 'Load from Fleet'}.items():
            offset = self.archive.metadata()['contexts'][identifier]
            self.assertEqual(self.archive.topic_at(offset, topics)['title'], title)
        with self.assertRaises(ValueError):
            self.archive.topic_at(0xffffffff, topics)

    def test_missing_link_target_rejected(self):
        archive = HLP.HelpArchive(self.data)
        archive.context_hashes = lambda: {}
        with self.assertRaisesRegex(ValueError, 'Missing WinHelp link context hash'):
            FORMAT.decode_topics(archive)

    def test_original_font_descriptors_and_unicode_runs(self):
        fonts = FONT.decode_fonts(self.archive)
        self.assertEqual(len(fonts), 32)
        self.assertEqual((fonts[3]['face'], fonts[3]['half_points'], fonts[3]['bold']), ('Arial', 28, True))
        self.assertIsNone(fonts[3]['foreground'])
        self.assertEqual(fonts[6]['foreground'], (0, 128, 0))
        self.assertTrue(fonts[9]['double_underline'])
        topics = FONT.style_topics(FORMAT.decode_topics(self.archive), fonts)
        runs = [run for topic in topics.values() for record in topic['display']
                for paragraph in record['paragraphs'] for run in paragraph['runs']]
        self.assertEqual(len(runs), 20793)
        text = ''.join(run['unicode'] for run in runs)
        self.assertEqual((text.count('\u2666'), text.count('\u21d2')), (256, 116))
        self.assertEqual(FONT.css_style(fonts[9])['text-decoration-style'], 'double')
        self.assertEqual(FONT.css_style(fonts[6])['color'], '#008000')

    def test_font_selection_applies_after_command(self):
        fonts = FONT.decode_fonts(self.archive)
        runs = [{'text': b'Before', 'command': {'font': 15}},
                {'text': b'\xa8', 'command': {}},
                {'text': b'\xde', 'command': {'font': 4}}]
        topic = {'display': [{'paragraphs': [{'runs': runs[:2]}]},
                             {'paragraphs': [{'runs': runs[2:]}]}]}
        FONT.style_topics({0: topic}, fonts)
        self.assertEqual([run['font'] for run in runs], [0, 15, 15])
        self.assertEqual([run['unicode'] for run in runs], ['Before', '\u2666', '\u21d2'])
        runs[0]['command']['font'] = 32
        with self.assertRaises(ValueError):
            FONT.style_topics({0: topic}, fonts)
        with self.assertRaises(ValueError):
            FONT.decode_text(b'A', fonts[15])

    def test_bad_font_descriptors(self):
        original = self.archive.stream('|FONT')
        for offset, value in ((0, 0), (4, 0), (6, 0), (3608, 0x80), (3609, 0), (3610, 0)):
            damaged = bytearray(original)
            damaged[offset] = value
            archive = HLP.HelpArchive(self.data)
            archive.stream = lambda name: bytes(damaged)
            with self.subTest(offset=offset), self.assertRaises(ValueError):
                FONT.decode_fonts(archive)

    def test_original_images_and_hotspots(self):
        topics = self.archive.topics()
        images = IMAGE.decode_images(self.archive, topics)
        self.assertEqual(Counter(item['kind'] for item in images.values()), {'bitmap': 125, 'metafile': 8})
        spots = [spot for item in images.values() for spot in item['hotspots']]
        self.assertEqual(len(spots), 28)
        self.assertEqual(Counter(spot['opcode'] for spot in spots), {0xe7: 28})
        self.assertTrue(all(spot['topic'] in topics for spot in spots))
        references = json.loads((ROOT / 'tests/compat/help_bitmap_reference.json').read_text())
        self.assertEqual(len(references), 125)
        for name, reference in references.items():
            with self.subTest(image=name):
                item = images[name]
                self.assertEqual((item['width'], item['height']), (reference['width'], reference['height']))
                self.assertEqual(hashlib.sha256(item['rgb']).hexdigest(), reference['rgb_sha256'])

    def test_image_decompression_bounds(self):
        self.assertEqual(IMAGE.decompress(b'abc', 0, 3), b'abc')
        self.assertEqual(IMAGE.decompress(b'\x83abc\x03x', 1, 6), b'abcxxx')
        self.assertEqual(IMAGE.decompress(b'\x00x\x01y', 1, 1), b'y')
        for data, packing, size in ((b'\x83ab', 1, 3), (b'\x02', 1, 2),
                                    (b'\x03x', 1, 2), (b'x', 0, 2),
                                    (b'', 2, 0), (b'', 0, IMAGE.MAX_DATA + 1)):
            with self.subTest(data=data, packing=packing, size=size), self.assertRaises(ValueError):
                IMAGE.decompress(data, packing, size)

    def test_bad_image_offsets_and_hotspots(self):
        original = self.archive.stream('|bm0')
        for data in (original[:7], original[:20], b'nope' + original[4:]):
            with self.assertRaises(ValueError):
                IMAGE.decode_image(data)
        damaged = bytearray(original)
        struct.pack_into('<I', damaged, 4, len(damaged))
        with self.assertRaises(ValueError):
            IMAGE.decode_image(damaged)
        for data in (b'\0\0\0\0\0\0\0', b'\1\1\0\0\0\0\0',
                     b'\1\0\0\0\0\0\0extra'):
            with self.assertRaises(ValueError):
                IMAGE.hotspots(data)
        archive = HLP.HelpArchive(self.data)
        archive.context_hashes = lambda: {}
        with self.assertRaises(ValueError):
            IMAGE.decode_images(archive, archive.topics())

    def test_png_preserves_rgb_and_crc(self):
        item = IMAGE.decode_image(self.archive.stream('|bm0'))
        png = IMAGE.png_bytes(item)
        self.assertEqual(png[:8], b'\x89PNG\r\n\x1a\n')
        offset, chunks, compressed = 8, [], bytearray()
        import binascii
        while offset < len(png):
            size, = struct.unpack_from('>I', png, offset)
            name, data = png[offset + 4:offset + 8], png[offset + 8:offset + 8 + size]
            crc, = struct.unpack_from('>I', png, offset + 8 + size)
            self.assertEqual(crc, binascii.crc32(name + data) & 0xffffffff)
            chunks.append(name)
            if name == b'IDAT':
                compressed.extend(data)
            offset += size + 12
        self.assertEqual(chunks, [b'IHDR', b'IDAT', b'IEND'])
        rows = zlib.decompress(compressed)
        stride = item['width'] * 3 + 1
        self.assertTrue(all(rows[y * stride] == 0 for y in range(item['height'])))
        self.assertEqual(b''.join(rows[y * stride + 1:(y + 1) * stride]
                                 for y in range(item['height'])), item['rgb'])

    def test_original_wmf_conversion(self):
        for name, item in IMAGE.decode_images(self.archive, self.archive.topics()).items():
            if item['kind'] != 'metafile':
                continue
            with self.subTest(image=name):
                header = IMAGE.placeable_wmf(item)
                self.assertEqual(header[22:], item['wmf'])
                self.assertEqual(struct.unpack_from('<I', header)[0], 0x9ac6cdd7)
                checksum = 0
                for word in struct.unpack('<11H', header[:22]):
                    checksum ^= word
                self.assertEqual(checksum, 0)
                root = ET.fromstring(IMAGE.svg_bytes(item))
                self.assertEqual(root.tag, '{http://www.w3.org/2000/svg}svg')
                self.assertAlmostEqual(float(root.attrib['width']), item['width'] * 96 / 2540, places=4)
                images = list(root.iter('{http://www.w3.org/2000/svg}image'))
                self.assertEqual(len(images), 1)
                image = images[0]
                uri = image.attrib['{http://www.w3.org/1999/xlink}href']
                self.assertTrue(uri.startswith('data:image/png;base64,'))
                png = base64.b64decode(uri.split(',', 1)[1])
                width, height = struct.unpack_from('>II', png, 16)
                self.assertEqual((int(image.attrib['width']), int(image.attrib['height'])), (width, height))
                transform = image.attrib['transform']
                self.assertTrue(transform.startswith('matrix('))
                a, b, c, d, x, y = map(float, transform[7:-1].split())
                self.assertEqual((b, c, x, y), (0, 0, 0, 0))
                _, _, viewport_width, viewport_height = map(float, root.attrib['viewBox'].split())
                self.assertAlmostEqual(a * width, viewport_width, places=3)
                self.assertAlmostEqual(d * height, viewport_height, places=3)

    def test_rendered_original_text_links_images_and_tables(self):
        renderer = RENDER.Renderer(self.archive)
        image_count, table_count, hotspots = 0, 0, 0
        logical_links, popup_links = set(), set()
        pages = {RENDER.page_name(key) for key in renderer.topics}
        for offset, topic in renderer.topics.items():
            with self.subTest(topic=offset):
                parsed = GuideHTML(renderer.document(topic['title'], renderer.topic(topic)))
                expected = ''.join(run['unicode'] for record in topic['display']
                                   for paragraph in record['paragraphs'] for run in paragraph['runs'])
                self.assertEqual(''.join(parsed.text), expected)
                for link in parsed.links:
                    self.assertTrue(link['href'] == 'index.html' or link['href'] in pages)
                    if 'data-help-link' in link:
                        logical_links.add((offset, link['data-help-link']))
                        if 'data-popup' in link:
                            popup_links.add((offset, link['data-help-link']))
                image_count += len(parsed.images)
                table_count += parsed.tables
                hotspots += sum(link.get('class') == 'image-hotspot' for link in parsed.links)
        self.assertEqual((image_count, table_count), (140, 148))
        self.assertEqual(hotspots, 28)
        self.assertEqual((len(logical_links), len(popup_links)), (1498, 404))

    def test_generated_guide_contexts_and_local_assets(self):
        renderer = RENDER.Renderer(self.archive)
        with tempfile.TemporaryDirectory(prefix='stars-help-test-') as directory:
            output = Path(directory)
            manifest = renderer.write(output)
            renderer.header(output / 'help_contexts.h')
            header = (output / 'help_contexts.h').read_text()
            pairs = dict((int(identifier), page) for identifier, page in
                         re.findall(r'\{(\d+)u, "(topic-\d+\.html)"\}', header))
            self.assertEqual(pairs, manifest['contexts'])
            self.assertIn(f'0x{zlib.crc32(self.archive.data):08x}', header)
            self.assertEqual((len(manifest['topics']), len(manifest['contexts'])), (414, 413))
            self.assertNotIn(13002, manifest['contexts'])
            self.assertEqual(manifest['topics'][int(manifest['contexts'][4501][6:-5])]['title'], 'Welcome to Stars!')
            self.assertEqual(len(list((output / 'images').iterdir())), 133)
            self.assertEqual(len(list((output / 'fonts').glob('*.ttf'))), 4)
            self.assertTrue((output / 'fonts/LICENSE').is_file())
            self.assertEqual((output / 'complete.stamp').read_text().strip(), manifest['sha256'])
            bundle = json.loads(re.search(r'id="guide-data">(.*?)</script>', (output / 'offline.html').read_text(), re.S)[1])
            self.assertEqual(len(bundle['pages']), 415)
            self.assertEqual(bundle['css'].count('data:font/ttf;base64,'), 4)
            for page, value in bundle['pages'].items():
                parsed = GuideHTML('<main>' + value['body'] + '</main>')
                self.assertEqual(parsed.text, GuideHTML((output / page).read_text()).text)
                for link in parsed.links:
                    self.assertTrue(link['href'].startswith('#'))
                    self.assertIn(link['href'][1:], bundle['pages'])
                for image in parsed.images:
                    self.assertTrue(image['src'].startswith(('data:image/png;base64,', 'data:image/svg+xml;base64,')))
            for path in output.glob('*.html'):
                parsed = GuideHTML(path.read_text())
                for link in parsed.links:
                    self.assertTrue(path.name == 'offline.html' and link['href'].startswith('#') and link['href'][1:] in bundle['pages'] or (output / link['href']).is_file())
                for image in parsed.images:
                    self.assertTrue((output / image['src']).is_file())

    def test_renderer_escapes_text_and_topic_titles(self):
        renderer = RENDER.Renderer(self.archive)
        topic = {'title': '<unsafe "title">', 'display': [{'paragraphs': [{
            'properties': {'alignment': 'left'}, 'runs': [{'unicode': '<script>&', 'font': 4,
                                                        'command': {'opcode': 0xff}}]}]}]}
        body = renderer.topic(topic)
        document = renderer.document(topic['title'], body)
        self.assertIn('&lt;script&gt;&amp;', document)
        self.assertNotIn('<unsafe', document)
        self.assertIn('aria-label="&lt;unsafe &quot;title&quot;&gt;"', document)
        self.assertEqual(''.join(GuideHTML(document).text), '<script>&')

    def test_renderer_preserves_table_column_groups(self):
        renderer = RENDER.Renderer(self.archive)
        topic = renderer.topics[247]
        table = next(record for record in topic['display'] if 'table' in record)
        self.assertEqual(table['table']['columns'], [(435, 1), (385, 11)])
        parsed = GuideHTML(renderer.topic(topic))
        self.assertEqual(parsed.cells, [0, 1])
        self.assertEqual(parsed.tables, 1)

    def test_renderer_rejects_unsupported_commands(self):
        renderer = RENDER.Renderer(self.archive)
        paragraph = {'properties': {'alignment': 'left'}, 'runs': [
            {'unicode': '', 'font': 4, 'command': {'opcode': 0xc8, 'macro': b'example'}}]}
        with self.assertRaises(ValueError):
            renderer.paragraph(paragraph, 'Example')


if __name__ == '__main__':
    unittest.main()
