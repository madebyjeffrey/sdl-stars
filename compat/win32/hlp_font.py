"""Original HC31 font descriptors and font-aware help text decoding."""
import struct

# The original guide uses just these two Adobe Symbol glyphs. Mapping reference:
# https://www.unicode.org/Public/MAPPINGS/VENDORS/ADOBE/symbol.txt
SYMBOL_CHARACTERS = {0xa8: '\u2666', 0xde: '\u21d2'}


def decode_fonts(archive):
    data = archive.stream('|FONT')
    if len(data) < 8:
        raise ValueError('Truncated WinHelp font header')
    names, count, name_offset, descriptor_offset = struct.unpack_from('<HHHH', data)
    if name_offset != 8 or names == 0 or count == 0 or descriptor_offset < name_offset:
        raise ValueError('Expected HC31 old-style font descriptors')
    name_size, remainder = divmod(descriptor_offset - name_offset, names)
    if remainder or name_size == 0 or descriptor_offset + count * 11 != len(data):
        raise ValueError('Invalid WinHelp font table dimensions')
    faces = [data[name_offset + index * name_size:name_offset + (index + 1) * name_size]
             .split(b'\0', 1)[0].decode('cp1252') for index in range(names)]
    fonts = []
    for index in range(count):
        attributes, height, family, face = struct.unpack_from('<BBBH', data, descriptor_offset + index * 11)
        foreground = tuple(data[descriptor_offset + index * 11 + 5:descriptor_offset + index * 11 + 8])
        background = tuple(data[descriptor_offset + index * 11 + 8:descriptor_offset + index * 11 + 11])
        if attributes & ~0x3f or height == 0 or not 1 <= family <= 5 or face >= names or not faces[face]:
            raise ValueError('Invalid WinHelp font descriptor')
        fonts.append({'face': faces[face], 'half_points': height, 'family': family,
                      'bold': bool(attributes & 1), 'italic': bool(attributes & 2),
                      'underline': bool(attributes & 4), 'strikeout': bool(attributes & 8),
                      'double_underline': bool(attributes & 16), 'small_caps': bool(attributes & 32),
                      'foreground': None if foreground == (1, 1, 0) else foreground,
                      'background': background})
    return fonts


def decode_text(data, font):
    if font['face'].casefold() == 'symbol':
        try:
            return ''.join(SYMBOL_CHARACTERS[value] for value in data)
        except KeyError as error:
            raise ValueError(f'Unsupported Symbol character: {error.args[0]:#x}') from error
    return data.decode('cp1252')


def style_topics(topics, fonts):
    """Attach Unicode text and the font selected before each command executes."""
    for topic in topics.values():
        index = 0
        for record in topic['display']:
            for paragraph in record['paragraphs']:
                for run in paragraph['runs']:
                    if not 0 <= index < len(fonts):
                        raise ValueError('WinHelp font index outside descriptor table')
                    run['font'] = index
                    run['unicode'] = decode_text(run['text'], fonts[index])
                    if 'font' in run['command']:
                        index = run['command']['font']
                        if not 0 <= index < len(fonts):
                            raise ValueError('WinHelp font index outside descriptor table')
    return topics


def css_style(font):
    # Named application faces use bundled Liberation Sans. Unicode-converted
    # Symbol runs also use this font, rather than depending on a legacy encoding.
    family = '"StarsHelpSans",sans-serif' if font['face'].casefold() in ('arial', 'helv', 'symbol') else {
        1: 'monospace', 2: 'serif', 3: 'sans-serif', 4: 'cursive', 5: 'fantasy'}[font['family']]
    lines = []
    if font['underline'] or font['double_underline']:
        lines.append('underline')
    if font['strikeout']:
        lines.append('line-through')
    color = font['foreground']
    return {'font-family': family, 'font-size': f"{font['half_points'] / 2:g}pt",
            'font-weight': 'bold' if font['bold'] else 'normal',
            'font-style': 'italic' if font['italic'] else 'normal',
            'font-variant': 'small-caps' if font['small_caps'] else 'normal',
            'text-decoration-line': ' '.join(lines) if lines else 'none',
            'text-decoration-style': 'double' if font['double_underline'] else 'solid',
            'color': '#000000' if color is None else '#%02x%02x%02x' % color}
