#!/usr/bin/env python3
"""Stage the canonical UTF-8 guide; the original HLP is archival material."""
import argparse
from pathlib import Path
import re
import zipfile


def cpp_header(data):
    """Expose the guide metadata as C++ without changing its packaged assets."""
    text = data.decode('utf-8')
    old = 'typedef struct { uint32_t idh; const char *pszPage; } STARS_HELP_CONTEXT;'
    if text.count(old) != 1:
        raise ValueError('unexpected guide context declaration')
    text = text.replace(old, '#include <cstdint>\nstruct STARS_HELP_CONTEXT { std::uint32_t idh; const char *pszPage; };')
    numeric = ('ARCHIVE_SIZE', 'ARCHIVE_CRC', 'BUNDLE_SIZE', 'BUNDLE_CRC')
    for suffix in numeric:
        pattern = rf'^#define STARS_HELP_{suffix} (\d+u|UINT32_C\((0x[0-9a-f]+)\))$'
        text, count = re.subn(pattern, lambda match: f'inline constexpr std::uint32_t STARS_HELP_{suffix} = {match[2] or match[1]};', text, flags=re.M)
        if count != 1:
            raise ValueError(f'unexpected guide {suffix}')
    for suffix in ('SHA', 'CONTENTS'):
        text, count = re.subn(rf'^#define STARS_HELP_{suffix} ("[^"\n]*")$',
                              rf'inline constexpr char STARS_HELP_{suffix}[] = \1;', text, flags=re.M)
        if count != 1:
            raise ValueError(f'unexpected guide {suffix}')
    sha = re.search(r'inline constexpr char STARS_HELP_SHA\[\] = "([0-9a-f]{64})";', text)
    if not sha:
        raise ValueError('invalid guide stamp')
    text = text.replace('static const STARS_HELP_CONTEXT rgHelpContext[]', 'inline constexpr STARS_HELP_CONTEXT rgHelpContext[]')
    text = text.replace('static const char *const rgHelpFile[]', 'inline constexpr const char *const rgHelpFile[]')
    text = text.replace('\n#endif', f'\ninline constexpr char STARS_HELP_STAMP[] = "{sha[1]}\\n";\n#endif')
    return text.encode('utf-8')


parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('archive', type=Path)
parser.add_argument('output', type=Path)
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
with zipfile.ZipFile(args.archive) as archive:
    for item in archive.infolist():
        path = Path(item.filename)
        if path.is_absolute() or '..' in path.parts:
            raise ValueError('invalid guide asset path')
        data = archive.read(item)
        if path.name == 'help_contexts.h':
            data = cpp_header(data)
        if path.suffix in ('.html', '.css', '.js', '.json', '.h') or path.name == 'complete.stamp':
            data.decode('utf-8')
        destination = args.output / path
        destination.parent.mkdir(parents=True, exist_ok=True)
        if not destination.exists() or destination.read_bytes() != data:
            destination.write_bytes(data)
