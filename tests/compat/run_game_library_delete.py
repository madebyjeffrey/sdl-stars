"""Exercise managed-game deletion through the desktop Games view."""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


def snapshot(directory):
    return {p.relative_to(directory): p.read_bytes()
            for p in directory.rglob('*') if p.is_file()}


def session_settings(directory):
    lines = (directory / 'StarsUtf8.ini').read_text(encoding='utf-8').splitlines()
    in_files = False
    files = []
    for line in lines:
        line = line.removeprefix('\ufeff')
        if line.startswith('['):
            if in_files:
                break
            in_files = line.lower() == '[files]'
        if in_files:
            files.append(line)
    return tuple(files)


def run(executable, mode):
    with tempfile.TemporaryDirectory(prefix='stars-game-delete-') as directory:
        work = Path(directory)
        settings = work / 'settings'
        games = work / 'library' / 'Games'
        selected = games / 'selected-game'
        other = games / 'other-game'
        source = work / 'imported-original'
        settings.mkdir()
        fixture = Path(__file__).parents[1] / 'scaffold' / 'fixtures' / 'regression' / 'native' / 'oneai1' / 'checkpoints' / '000'
        for root, stem in ((selected, 'selected-game'), (other, 'other-game'), (source, 'exchange')):
            root.mkdir(parents=True)
            for extension in ('.xy', '.hst', '.m1'):
                shutil.copyfile(fixture / ('game' + extension), root / (stem + extension))
            (root / '.library').write_text(f'2\n{stem}\n0\n0\n{stem}\n', encoding='utf-8')
        for root, stem in ((selected, 'selected-game'), (other, 'other-game')):
            (root / (stem + '.x1')).write_bytes(b'saved orders')
            (root / (stem + '.h1')).write_bytes(b'history')
            backup = root / 'backup.000'
            backup.mkdir()
            (backup / (stem + '.m1')).write_bytes(b'backup turn')
            (root / 'host-state').write_bytes(b'host state')
        source_before = snapshot(source)
        selected_before = snapshot(selected)
        other_before = snapshot(other)
        settings.joinpath('StarsUtf8.ini').write_text(
            f'[Files]\nFile1={selected / "selected-game.m1"}\nFile2={other / "other-game.m1"}\n',
            encoding='utf-8')
        catalog_before = snapshot(games)
        session_settings_before = session_settings(settings)
        env = dict(os.environ, STARS_SETTINGS_DIR=str(settings), STARS_PROFILE_DIR=str(settings),
                   STARS_LIBRARY_DIR=str(work / 'library'), STARS_CLIENT_GAME_DELETE_MODE=mode,
                   STARS_FILE_DIALOG_BACKEND='compat')
        env.pop('STARS_CLIENT_CONTINUE', None)
        subprocess.run([str(executable)], cwd=work, env=env, check=True, timeout=45)
        assert snapshot(source) == source_before
        assert snapshot(other) == other_before
        if mode == 'confirm':
            assert not selected.exists()
            ini = (settings / 'StarsUtf8.ini').read_text(encoding='utf-8')
            assert 'File1=' in ini and f'File1={other / "other-game.m1"}' not in ini
            assert f'File2={other / "other-game.m1"}' in ini
        elif mode == 'cancel':
            assert snapshot(selected) == selected_before
            assert snapshot(games) == catalog_before
            assert session_settings(settings) == session_settings_before
        else:
            assert selected.is_dir()
            assert (selected / 'selected-game.m1').read_bytes() == selected_before[Path('selected-game.m1')]
            ini = (settings / 'StarsUtf8.ini').read_text(encoding='utf-8')
            assert f'File1={selected / "selected-game.m1"}' in ini
        print(f'{mode}: selected managed game deletion workflow passed')


def main():
    executable = Path(sys.argv[1]).resolve()
    for mode in ('cancel', 'failure', 'confirm'):
        run(executable, mode)


if __name__ == '__main__':
    main()
