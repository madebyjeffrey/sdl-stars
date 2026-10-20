"""Reject unsafe game-library actions through the real client and native files."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

from run_newgame import run


def directory_link(link, target):
    try:
        link.symlink_to(target, target_is_directory=True)
    except OSError:
        if os.name != 'nt':
            raise
        # Directory junctions do not require Windows symlink privileges.
        subprocess.run(['cmd', '/c', 'mklink', '/J', str(link).replace('/', '\\'),
                        str(target).replace('/', '\\')],
                       check=True, capture_output=True)


def snapshot(directory):
    return {p.relative_to(directory): p.read_bytes()
            for p in directory.rglob('*') if p.is_file()}


def check(executable, case, action):
    with tempfile.TemporaryDirectory(prefix='stars-game-safety-') as directory:
        work = Path(directory)
        games = work / 'library' / 'Games'
        source = games / 'original'
        run(executable, work, STARS_CLIENT_NEW_GAME=str(source / 'original.xy'),
            STARS_CLIENT_LIBRARY_NAME='original')
        before = snapshot(source)
        external = work / 'external'
        external.mkdir()
        (external / 'important.txt').write_bytes(b'keep external files')
        link = None
        if case.startswith('collision-'):
            target = 'renamed' if action == 'rename' else 'duplicate'
            parent = source
            extension = 'x1' if case == 'collision-orders' else 'm1'
            if case == 'collision-backup':
                parent = source / 'backup.000'
                parent.mkdir()
                shutil.copyfile(source / 'original.m1', parent / 'original.m1')
            if case == 'collision-orders':
                (source / 'original.x1').write_bytes(b'current saved orders')
            name = f'{target}.{extension}'
            if case == 'collision-case':
                name = name.upper()
            if case == 'collision-directory':
                (parent / name).mkdir()
                (parent / name / 'sidecar.txt').write_bytes(b'keep sidecar directory')
            else:
                (parent / name).write_bytes(b'keep conflicting sidecar')
            before = snapshot(source)
        elif case == 'root-link':
            source.rename(external / 'original')
            directory_link(source, external / 'original')
            link = source
        elif case in ('file-link', 'dangling-link'):
            link = source / 'linked'
            try:
                link.symlink_to(external / ('important.txt' if case == 'file-link' else 'missing'))
            except OSError as error:
                if os.name != 'nt' or getattr(error, 'winerror', None) != 1314:
                    raise
                print(f'{case}: skipped without Windows file-symlink privileges')
                return
        else:
            link = source / ('backup' if case == 'backup-link' else
                             'sidecars/linked' if case == 'nested-link' else 'linked')
            link.parent.mkdir(exist_ok=True)
            directory_link(link, external)
        external_before = snapshot(external)
        run(executable, work, STARS_CLIENT_GAME_LIBRARY_TEST=action,
            STARS_CLIENT_GAME_REJECT='1', STARS_CLIENT_GAME_SOURCE_NAME='original',
            STARS_CLIENT_GAME_COPY_NAME='duplicate', STARS_CLIENT_GAME_RENAME_NAME='renamed',
            STARS_CLIENT_GAME_COLLISION_NAME='original', STARS_CLIENT_GAME_BASELINE_NAME='original')
        assert snapshot(external) == external_before, (case, action, 'external files changed')
        assert sorted(p.name for p in games.iterdir()) == ['original'], (case, action, 'partial publication')
        if case == 'root-link':
            assert snapshot(external / 'original') == before
        else:
            # Remove only the link before walking the original game tree.
            if link is not None:
                if os.name == 'nt' and not link.is_symlink():
                    link.rmdir()
                else:
                    link.unlink()
            assert snapshot(source) == before, (case, action, 'original files changed')
        print(f'{case}: {action} rejected with originals preserved')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('client', type=Path)
    cases = ('directory-link', 'backup-link', 'nested-link', 'root-link', 'file-link', 'dangling-link',
             'collision-root', 'collision-case', 'collision-backup', 'collision-directory', 'collision-orders')
    parser.add_argument('--case', choices=cases)
    args = parser.parse_args()
    for case in (args.case,) if args.case else cases:
        for action in ('rename', 'duplicate'):
            check(args.client.resolve(), case, action)


if __name__ == '__main__':
    main()
