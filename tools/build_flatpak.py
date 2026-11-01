#!/usr/bin/env python3
"""Export tracked sources and build a local Flatpak repository/bundle."""
import argparse
import io
import hashlib
import gzip
import json
import re
import subprocess
import tarfile
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=Path('dist/flatpak'))
    parser.add_argument('--prepare-only', action='store_true')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    header = output / 'version.h'
    subprocess.run(['cmake', f'-DSOURCE_DIR={root}', f'-DOUTPUT={header}',
                    '-DVERSION_BASE=2.9.0', '-P', str(root / 'cmake/version.cmake')], check=True)
    version = re.search(r'^inline constexpr char STARS_VERSION_STRING\[\] = "([^"]+)";$', header.read_text(), re.M)[1]
    untracked = subprocess.check_output(['git', '-C', str(root), 'ls-files', '--others', '--exclude-standard', '-z']).split(b'\0')
    missing = [name.decode() for name in untracked if name and Path(name.decode()).suffix in ('.c', '.cpp', '.m', '.mm', '.inc', '.h', '.cmake', '.py')]
    if missing:
        raise RuntimeError('Stage new source files before exporting: ' + ', '.join(missing))
    paths = subprocess.check_output(['git', '-C', str(root), 'ls-files', '-z']).split(b'\0')
    # Never copy .git, ignored local settings/games, private instructions or
    # build trees. Read tracked working files so reviewed local edits are included.
    with (output / 'stars-source.tar.gz').open('wb') as file:
        with gzip.GzipFile(filename='', mode='wb', fileobj=file, mtime=0) as compressed:
            with tarfile.open(fileobj=compressed, mode='w') as archive:
                for raw in paths:
                    if not raw:
                        continue
                    name = raw.decode()
                    path = root / name
                    if not path.is_file() or path.is_symlink():
                        raise RuntimeError(f'Cannot export tracked file: {name}')
                    data = path.read_bytes()
                    info = tarfile.TarInfo('stars/' + name)
                    info.size = len(data)
                    info.mode = 0o755 if path.stat().st_mode & 0o111 else 0o644
                    archive.addfile(info, io.BytesIO(data))
                info = tarfile.TarInfo('stars/.stars-source-version')
                data = (version + '\n').encode()
                info.size = len(data)
                archive.addfile(info, io.BytesIO(data))
    manifest = json.loads((root / 'packaging/flatpak/org.starsgame.Stars.json').read_text())
    manifest['modules'][-1]['sources'][0]['sha256'] = hashlib.sha256((output / 'stars-source.tar.gz').read_bytes()).hexdigest()
    manifest['modules'][-1]['build-options']['env']['STARS_VERSION'] = version
    target = output / 'org.starsgame.Stars.json'
    target.write_text(json.dumps(manifest, indent=2) + '\n')
    if args.prepare_only:
        print(target)
        return
    subprocess.run(['flatpak-builder', '--user', '--force-clean', '--disable-rofiles-fuse',
                    '--jobs=4', '--repo=' + str(output / 'repo'), str(output / 'build'), str(target)], check=True)
    subprocess.run(['flatpak', 'build-bundle', str(output / 'repo'),
                    str(output / 'stars.flatpak'), manifest['app-id'],
                    '--runtime-repo=https://dl.flathub.org/repo/flathub.flatpakrepo'], check=True)
    print(output / 'stars.flatpak')


if __name__ == '__main__':
    main()
