#!/usr/bin/env python3
"""Build a self-contained, navigable HTML edition of the original Stars! guide."""
import argparse
import base64
import html
import json
import re
import shutil
import zlib
from pathlib import Path

from hlp import HelpArchive
from hlp_format import decode_topics
from hlp_font import css_style, decode_fonts, style_topics
from hlp_image import decode_images, png_bytes, svg_bytes

ROOT = Path(__file__).resolve().parents[3]
DIRECTORY = ROOT / "compat/win32"


def page_name(offset):
    return f'topic-{offset}.html'


def style(properties):
    return ';'.join(f'{key}:{value}' for key, value in properties.items())


def paragraph_style(properties):
    result = {'text-align': properties['alignment']}
    for key, css in (('above', 'margin-top'), ('below', 'margin-bottom'),
                     ('left', 'margin-left'), ('right', 'margin-right'),
                     ('first', 'text-indent')):
        result[css] = f"{properties.get(key, 0) / 2:g}pt"
    if properties.get('line'):
        result['line-height'] = f"{abs(properties['line']) / 2:g}pt"
    if 'border' in properties:
        flags, width = properties['border']
        if flags & 0x80:
            raise ValueError('Unsupported WinHelp paragraph border')
        # BorderWidth is a packed compiler property, not a usable CSS line
        # thickness. The explicit thick/double flags determine the stroke.
        stroke = f"{2 if flags & 0x20 else 1}px {'double' if flags & 0x40 else 'solid'} #000"
        for bit, edge in ((2, 'top'), (4, 'left'), (8, 'bottom'), (16, 'right')):
            if flags & (bit | 1):
                result['border-' + edge] = stroke
    return style(result)


class Renderer:
    def __init__(self, archive):
        self.archive = archive
        self.fonts = decode_fonts(archive)
        self.topics = style_topics(decode_topics(archive), self.fonts)
        self.images = decode_images(archive, self.topics)
        self.contents = archive.metadata()['contents_topic']
        self.link = None
        self.link_id = 0

    def image(self, name, opcode, title):
        image = self.images[name]
        suffix = 'png' if image['kind'] == 'bitmap' else 'svg'
        filename = name[1:] + '.' + suffix
        alignment = {0x86: 'inline', 0x87: 'left', 0x88: 'right'}[opcode]
        width = image['width'] if suffix == 'png' else image['width'] * 96 / 2540
        height = image['height'] if suffix == 'png' else image['height'] * 96 / 2540
        result = [f'<span class="figure figure-{alignment}" style="width:{width:g}px;height:{height:g}px">',
                  f'<img src="images/{filename}" alt="{html.escape(title, quote=True)}" width="{width:g}" height="{height:g}">']
        for spot in image['hotspots']:
            caption = self.topics[spot['topic']]['title']
            result.append(f'<a class="image-hotspot" href="{page_name(spot["topic"])}" '
                          f'aria-label="{html.escape(caption, quote=True)}" title="{html.escape(caption, quote=True)}" '
                          f'style="left:{spot["x"]}px;top:{spot["y"]}px;width:{spot["width"]}px;height:{spot["height"]}px"></a>')
        result.append('</span>')
        return ''.join(result)

    def paragraph(self, paragraph, title):
        properties = paragraph['properties']
        attributes = (' class="help-paragraph" style="' + paragraph_style(properties) + '"'
                      + ' data-tabs="' + ','.join(str(position / 2) for position, kind in properties.get('tabs', [])
                                                  if kind == 0 and position > 0) + '"')
        if any(kind != 0 for _, kind in properties.get('tabs', [])):
            raise ValueError('Unsupported non-left WinHelp tab stop')
        result = ['<p' + attributes + '>']
        for run in paragraph['runs']:
            value = html.escape(run['unicode'])
            if value:
                value = f'<span class="f{run["font"]}">{value}</span>'
                if self.link is not None:
                    target, popup, identifier = self.link
                    value = (f'<a href="{page_name(target)}"' + (' data-popup="true"' if popup else '')
                             + f' data-help-link="{identifier}">' + value + '</a>')
                result.append(value)
            command = run['command']
            opcode = command['opcode']
            if opcode == 0x82:
                result.append('</p><p' + attributes + '>')
            elif opcode == 0x81:
                result.append('<br>')
            elif opcode == 0x83:
                result.append('<span class="tab" aria-hidden="true"></span>')
            elif opcode in (0x86, 0x87, 0x88):
                result.append(self.image(command['image'], opcode, title))
            elif opcode == 0x89:
                self.link = None
            elif opcode in (0xe0, 0xe1, 0xe2, 0xe3, 0xe6, 0xe7):
                if self.link is not None:
                    raise ValueError('Nested WinHelp text hotspot')
                self.link_id += 1
                self.link = (command['topic'], opcode in (0xe0, 0xe2, 0xe6), self.link_id)
            elif opcode == 0x8b:
                result.append('&nbsp;')
            elif opcode == 0x8c:
                result.append('&shy;')
            elif opcode not in (0x80, 0xff):
                raise ValueError(f'Unsupported rendered WinHelp command: {opcode:#x}')
        if len(result) > 1 and result[-1] == '<p' + attributes + '>':
            result.pop()  # FF after a paragraph break has no extra blank paragraph.
        else:
            result.append('</p>')
        return ''.join(result)

    def topic(self, topic):
        self.link = None
        self.link_id = 0
        result = []
        for record in topic['display']:
            if 'table' not in record:
                result.extend(self.paragraph(paragraph, topic['title']) for paragraph in record['paragraphs'])
                continue
            table = record['table']
            if table['kind'] != 1:
                raise ValueError('Unsupported variable-width WinHelp table')
            # HC31 stores the width before the gap despite the order printed
            # in the format notes. Independent RTF cell extents confirm this.
            widths = [width / 2 for width, gap in table['columns']]
            result.append('<div class="table-scroll"><table class="help-table"><colgroup>')
            result.extend(f'<col style="width:{width:g}pt">' for width in widths)
            result.append('</colgroup><tbody>')
            previous = -1
            cell_open = False
            for paragraph in record['paragraphs']:
                column = paragraph['column']
                if column != previous:
                    if cell_open:
                        result.append('</td>')
                    if column <= previous or previous == -1:
                        if previous != -1:
                            result.append('</tr>')
                        result.append('<tr>')
                    gap = table['columns'][column][1] / 2
                    result.append(f'<td data-column="{column}" style="padding-left:{gap:g}pt">')
                    cell_open = True
                result.append(self.paragraph(paragraph, topic['title']))
                previous = column
            if cell_open:
                result.append('</td></tr>')
            result.append('</tbody></table></div>')
        if self.link is not None:
            raise ValueError('Unterminated WinHelp text hotspot')
        return ''.join(result)

    def document(self, title, body):
        return ('<!doctype html><html lang="en"><head><meta charset="utf-8">'
                '<meta name="viewport" content="width=device-width,initial-scale=1">'
                '<meta http-equiv="Content-Security-Policy" content="default-src \'self\'; '
                "img-src 'self' data:; style-src 'self' 'unsafe-inline'; object-src 'none'; base-uri 'none'\">"
                f'<title>{html.escape(title)}</title><link rel="stylesheet" href="help.css">'
                '<script src="help.js" defer></script></head><body>'
                f'<nav aria-label="Guide navigation"><a href="{page_name(self.contents)}">Contents</a> '
                '<a href="index.html">Topic index</a> <button type="button" id="back">Back</button></nav>'
                f'<main aria-label="{html.escape(title, quote=True)}">{body}</main>'
                '<dialog id="popup"><button type="button" id="close-popup">Close</button>'
                '<iframe title="Help definition"></iframe></dialog></body></html>')

    def header(self, path):
        contexts = sorted((identifier, offset) for offset, topic in self.topics.items()
                          for identifier in topic['contexts'])
        files = [page_name(offset) for offset in self.topics]
        files += ['help.css', 'help.js', 'index.html', 'manifest.json', 'complete.stamp', 'offline.html']
        files += ['fonts/LiberationSans-' + name + '.ttf' for name in ('Regular', 'Bold', 'Italic', 'BoldItalic')]
        files += ['fonts/LICENSE']
        files += ['images/' + name[1:] + ('.png' if image['kind'] == 'bitmap' else '.svg')
                  for name, image in self.images.items()]
        rows = '\n'.join(f'    {{{identifier}u, "{page_name(offset)}"}},' for identifier, offset in contexts)
        assets = '\n'.join(f'    "{name}",' for name in files)
        source = ('/* Generated from the original guide; do not edit. */\n'
                  '#ifndef STARS_HELP_CONTEXTS_H\n#define STARS_HELP_CONTEXTS_H\n'
                  'typedef struct { uint32_t idh; const char *pszPage; } STARS_HELP_CONTEXT;\n'
                  f'#define STARS_HELP_ARCHIVE_SIZE {len(self.archive.data)}u\n'
                  f'#define STARS_HELP_ARCHIVE_CRC UINT32_C(0x{zlib.crc32(self.archive.data):08x})\n'
                  f'#define STARS_HELP_SHA "{self.archive.metadata()["sha256"]}"\n'
                  f'#define STARS_HELP_CONTENTS "{page_name(self.contents)}"\n'
                  f'#define STARS_HELP_BUNDLE_SIZE {len(self.bundle)}u\n'
                  f'#define STARS_HELP_BUNDLE_CRC UINT32_C(0x{zlib.crc32(self.bundle):08x})\n'
                  'static const STARS_HELP_CONTEXT rgHelpContext[] = {\n' + rows + '\n};\n'
                  'static const char *const rgHelpFile[] = {\n' + assets + '\n};\n#endif\n')
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(source, encoding='utf-8')

    def write(self, directory, converter='wmf2svg'):
        directory.mkdir(parents=True, exist_ok=True)
        (directory / 'images').mkdir(exist_ok=True)
        (directory / 'fonts').mkdir(exist_ok=True)
        for name, image in self.images.items():
            suffix = 'png' if image['kind'] == 'bitmap' else 'svg'
            data = png_bytes(image) if suffix == 'png' else svg_bytes(image, converter)
            (directory / 'images' / (name[1:] + '.' + suffix)).write_bytes(data)
        for name in ('Regular', 'Bold', 'Italic', 'BoldItalic'):
            shutil.copyfile(ROOT / 'res/fonts' / f'LiberationSans-{name}.ttf',
                            directory / 'fonts' / f'LiberationSans-{name}.ttf')
        shutil.copyfile(ROOT / 'res/fonts/LICENSE', directory / 'fonts/LICENSE')
        css = (DIRECTORY / 'help.css').read_text(encoding='utf-8')
        css += '\n' + '\n'.join(f'.f{index}{{{style(css_style(font))}}}' for index, font in enumerate(self.fonts))
        (directory / 'help.css').write_text(css, encoding='utf-8')
        shutil.copyfile(DIRECTORY / 'help.js', directory / 'help.js')
        for offset, topic in self.topics.items():
            (directory / page_name(offset)).write_text(self.document(topic['title'], self.topic(topic)), encoding='utf-8')
        rows = ''.join(f'<li><a href="{page_name(offset)}">{html.escape(topic["title"] or "Untitled topic")}</a></li>'
                       for offset, topic in sorted(self.topics.items(), key=lambda item: item[1]['title'].casefold()))
        body = ('<h1>Stars! Player’s Guide — Topic index</h1><label>Filter topics '
                '<input id="topic-filter" type="search"></label><ul id="topics">' + rows + '</ul>')
        (directory / 'index.html').write_text(self.document('Stars! Player’s Guide — Topic index', body), encoding='utf-8')
        self.write_bundle(directory, css, body)
        manifest = {'sha256': self.archive.metadata()['sha256'], 'contents': page_name(self.contents),
                    'topics': {offset: {'title': topic['title'], 'page': page_name(offset)} for offset, topic in self.topics.items()},
                    'contexts': {identifier: page_name(offset) for offset, topic in self.topics.items() for identifier in topic['contexts']}}
        (directory / 'manifest.json').write_text(json.dumps(manifest, indent=2, sort_keys=True) + '\n')
        (directory / 'complete.stamp').write_text(manifest['sha256'] + '\n')
        return manifest

    def write_bundle(self, directory, css, index):
        # A document portal exports one file, not the surrounding asset tree.
        # Keep every topic, image and font inside that file, and navigate in
        # memory so both ordinary links and definition popups remain offline.
        def data_uri(name):
            path = directory / name
            mime = {'.ttf': 'font/ttf', '.png': 'image/png', '.svg': 'image/svg+xml'}[path.suffix]
            return 'data:' + mime + ';base64,' + base64.b64encode(path.read_bytes()).decode('ascii')

        css = re.sub(r'url\((fonts/[^)]+)\)', lambda match: 'url("' + data_uri(match[1]) + '")', css)
        def portable(body):
            body = re.sub(r'src="(images/[^" ]+)"', lambda match: 'src="' + data_uri(match[1]) + '"', body)
            return re.sub(r'href="(topic-[0-9]+\.html|index\.html)"', r'href="#\1"', body)
        pages = {page_name(offset): {'title': topic['title'], 'body': portable(self.topic(topic))}
                 for offset, topic in self.topics.items()}
        pages['index.html'] = {'title': 'Stars! Player’s Guide — Topic index', 'body': portable(index)}
        payload = json.dumps({'pages': pages, 'contents': page_name(self.contents), 'css': css}).replace('<', '\\u003c')
        source = ('<!doctype html><html lang="en"><head><meta charset="utf-8">'
                  '<meta name="viewport" content="width=device-width,initial-scale=1">'
                  '<meta name="stars-help-start" content="__STARS_HELP_START__">'
                  '<title>Stars! Player’s Guide</title><style id="guide-style"></style></head><body>'
                  f'<nav aria-label="Guide navigation"><a href="#{page_name(self.contents)}">Contents</a> '
                  '<a href="#index.html">Topic index</a><button type="button" id="back">Back</button></nav>'
                  '<main></main><dialog id="popup"><button type="button" id="close-popup">Close</button>'
                  '<iframe title="Help definition"></iframe></dialog>'
                  '<script type="application/json" id="guide-data">' + payload + '</script><script>' +
                  (DIRECTORY / 'help_bundle.js').read_text(encoding='utf-8') + '</script></body></html>')
        self.bundle = source.encode('utf-8')
        (directory / 'offline.html').write_bytes(self.bundle)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('help_file', type=Path)
    parser.add_argument('output_directory', type=Path)
    parser.add_argument('--wmf2svg', default='wmf2svg')
    parser.add_argument('--header', type=Path)
    args = parser.parse_args()
    renderer = Renderer(HelpArchive(args.help_file.read_bytes()))
    manifest = renderer.write(args.output_directory, args.wmf2svg)
    if args.header:
        renderer.header(args.header)
    print(f"Generated {len(manifest['topics'])} topics and {len(manifest['contexts'])} context targets in {args.output_directory}")
