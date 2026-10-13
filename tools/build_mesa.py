#!/usr/bin/env python3
"""Build the pinned Mesa leak fix into a private, opt-in test prefix."""
import argparse
import hashlib
import pathlib
import shutil
import subprocess
import tarfile
import urllib.request


VERSION = '26.1.6'
SHA256 = '5296b88a0f1e012e2cb9ada150a2bbadf728ca81e5a4fb2ab43c83a4d2158606'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=pathlib.Path, default=pathlib.Path('dist/mesa'))
    parser.add_argument('--archive', type=pathlib.Path, help='use an offline source archive')
    parser.add_argument('--meson', default='meson', help='Meson >= 1.4 executable')
    parser.add_argument('--jobs', type=int, default=4)
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error('--jobs must be positive')
    root = args.root.resolve()
    root.mkdir(parents=True, exist_ok=True)
    archive = args.archive.resolve() if args.archive else root / f'mesa-{VERSION}.tar.xz'
    if not archive.exists():
        if args.archive:
            parser.error('offline archive does not exist')
        with urllib.request.urlopen(f'https://archive.mesa3d.org/mesa-{VERSION}.tar.xz', timeout=60) as response:
            with archive.open('wb') as output:
                shutil.copyfileobj(response, output)
    if hashlib.sha256(archive.read_bytes()).hexdigest() != SHA256:
        raise RuntimeError('Mesa archive checksum mismatch')
    source = root / f'mesa-{VERSION}'
    patch = pathlib.Path(__file__).resolve().parents[1] / 'cmake/dependencies/patches/mesa-26.1.6-rtasm-release.patch'
    stamp = source / '.stars-rtasm-patch'
    digest = hashlib.sha256(patch.read_bytes()).hexdigest()
    if not source.exists():
        with tarfile.open(archive) as tar:
            tar.extractall(root, filter='data')
    if stamp.exists():
        if stamp.read_text() != digest:
            raise RuntimeError('Patch changed; choose a fresh --root to avoid reusing stale source')
    else:
        subprocess.run(['patch', '--batch', '--forward', '-p1', '-i', str(patch)], cwd=source, check=True)
        stamp.write_text(digest)
    build = root / 'build'
    prefix = root / 'install'
    command = [args.meson, 'setup', str(build), str(source)]
    if (build / 'meson-private/coredata.dat').exists():
        command.append('--reconfigure')
    subprocess.run(command + [f'--prefix={prefix}', '--libdir=lib', '-Dplatforms=wayland',
                             '-Dgallium-drivers=llvmpipe,softpipe', '-Dvulkan-drivers=[]',
                             '-Dllvm=enabled', '-Dglx=disabled', '-Degl=enabled',
                             '-Dglvnd=enabled', '-Dgbm=enabled', '-Dgles1=disabled',
                             '-Dgles2=enabled', '-Dbuild-tests=false', '-Dvideo-codecs=[]',
                             '-Dxmlconfig=disabled'], check=True)
    subprocess.run(['ninja', '-C', str(build), '-j', str(args.jobs)], check=True)
    subprocess.run([args.meson, 'install', '-C', str(build)], check=True)
    print(f'Private software Mesa ready: STARS_TEST_MESA_PREFIX={prefix}')


if __name__ == '__main__':
    main()
