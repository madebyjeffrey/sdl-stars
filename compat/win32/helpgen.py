#!/usr/bin/env python3
"""Stage the canonical UTF-8 guide; the original HLP is archival material."""
import argparse
from pathlib import Path
import zipfile

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
        if path.suffix in ('.html', '.css', '.js', '.json', '.h') or path.name == 'complete.stamp':
            data.decode('utf-8')
        destination = args.output / path
        destination.parent.mkdir(parents=True, exist_ok=True)
        if not destination.exists() or destination.read_bytes() != data:
            destination.write_bytes(data)
