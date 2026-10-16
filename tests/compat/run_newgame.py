"""Create managed games, resolve collisions, and reopen in a fresh client."""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


def run(executable, work, default_root=False, settings_root=False, **choices):
    (work / 'settings').mkdir(parents=True, exist_ok=True)
    (work / 'settings' / 'Stars.ini').unlink(missing_ok=True)
    env = dict(os.environ, STARS_SETTINGS_DIR=str(work / 'settings'),
               STARS_LIBRARY_DIR=str(work / 'library'),
               STARS_DEBUG_LIBRARY_STARTUP='1')
    if default_root or settings_root:
        env.pop('STARS_LIBRARY_DIR', None)
        if default_root:
            env.pop('STARS_SETTINGS_DIR', None)
            pref_path = work / 'pref' / 'Stars!'
            pref_path.mkdir(parents=True, exist_ok=True)
            env['STARS_TEST_PREF_PATH'] = str(pref_path) + os.sep
        env.pop('STARS_CLIENT_CONTINUE', None)
        env.pop('STARS_CLIENT_NEW_GAME', None)
        env.pop('STARS_CLIENT_NEW_ADVANCED', None)
        env.pop('STARS_CLIENT_LIBRARY_OPEN', None)
        env.update(choices)
        cwd = executable.parent if env.get('STARS_CLIENT_NEW_GAME_EXE_DIR') else work
        subprocess.run([str(executable)], cwd=cwd, env=env, check=True, timeout=45)
        return
    for key in ('STARS_CLIENT_CONTINUE', 'STARS_CLIENT_NEW_GAME',
                'STARS_CLIENT_NEW_ADVANCED', 'STARS_CLIENT_LIBRARY_OPEN'):
        env.pop(key, None)
    env.update(choices)
    cwd = executable.parent if env.get('STARS_CLIENT_NEW_GAME_EXE_DIR') else work
    subprocess.run([str(executable)], cwd=cwd, env=env, check=True, timeout=45)


def main():
    executable = Path(sys.argv[1]).resolve()
    for advanced, name in ((False, 'fresh-game'), (False, 'nouvelle-partie-Ω'),
                           (True, 'advanced-game'), (True, 'avancée-Ω')):
        with tempfile.TemporaryDirectory(prefix='stars-newgame-') as directory:
            work = Path(directory)
            games = work / 'library' / 'Games'
            path = games / name / name
            choices = dict(STARS_CLIENT_NEW_GAME=str(path.with_suffix('.xy')),
                           STARS_CLIENT_LIBRARY_NAME=name)
            if advanced:
                choices['STARS_CLIENT_NEW_ADVANCED'] = '1'
            run(executable, work, **choices)
            assert all(Path(str(path) + ext).stat().st_size > 0 for ext in ('.xy', '.hst', '.m1'))
            assert (path.parent / '.library').is_file()
            before = {p.name: p.read_bytes() for p in path.parent.iterdir() if p.suffix in ('.xy', '.hst', '.m1')}
            run(executable, work, STARS_CLIENT_LIBRARY_OPEN=name)
            assert all((path.parent / filename).read_bytes() == content for filename, content in before.items())
            # The wizard defaults to Game.xy; a differently cased folder is a collision.
            (games / 'gAmE').mkdir()
            choices['STARS_CLIENT_NEW_GAME'] = str(games / 'Game (2)' / 'Game (2).xy')
            choices['STARS_CLIENT_LIBRARY_NAME'] = 'Game (2)'
            choices['STARS_CLIENT_LIBRARY_SUGGESTED'] = 'Game (2)'
            choices['STARS_CLIENT_LIBRARY_REJECT'] = '1'
            run(executable, work, **choices)
            assert not list((games / 'gAmE').iterdir())
            assert (games / 'Game (2)' / '.library').is_file()
            run(executable, work, STARS_CLIENT_LIBRARY_OPEN='Game (2)')
            if not advanced and name == 'fresh-game':
                for failure in ('universe', 'host'):
                    run(executable, work, STARS_CLIENT_NEW_GAME=str(games / 'failed-game' / 'failed-game.xy'),
                        STARS_CLIENT_LIBRARY_NAME='failed-game', STARS_CLIENT_LIBRARY_FAIL_WRITE=failure)
                    assert not (games / 'failed-game').exists()
                    run(executable, work, STARS_CLIENT_LIBRARY_OPEN=name)

    with tempfile.TemporaryDirectory(prefix='stars-native-library-root-') as directory:
        work = Path(directory)
        games = work / 'pref' / 'Stars!' / 'Games'
        name = 'platform-root-game'
        path = games / name / name
        run(executable, work, default_root=True,
            STARS_CLIENT_NEW_GAME=str(path.with_suffix('.xy')),
            STARS_CLIENT_LIBRARY_NAME=name)
        assert all(Path(str(path) + ext).stat().st_size > 0 for ext in ('.xy', '.hst', '.m1'))
        assert (path.parent / '.library').is_file()
        run(executable, work, default_root=True, STARS_CLIENT_LIBRARY_OPEN=name)

    with tempfile.TemporaryDirectory(prefix='stars-settings-library-root-') as directory:
        work = Path(directory)
        games = work / 'settings' / 'Games'
        name = 'settings-root-game'
        path = games / name / name
        run(executable, work, settings_root=True,
            STARS_CLIENT_NEW_GAME=str(path.with_suffix('.xy')),
            STARS_CLIENT_LIBRARY_NAME=name)
        assert all(Path(str(path) + ext).stat().st_size > 0 for ext in ('.xy', '.hst', '.m1'))
        assert (path.parent / '.library').is_file()
        run(executable, work, settings_root=True, STARS_CLIENT_LIBRARY_OPEN=name)

    with tempfile.TemporaryDirectory(prefix='stars-import-game-') as directory:
        work = Path(directory)
        games = work / 'library' / 'Games'
        source = work / 'exchange'
        source.mkdir()
        fixture = Path(__file__).parents[1] / 'scaffold' / 'fixtures' / 'regression' / 'native' / 'oneai2' / 'checkpoints' / '000'
        for ext in ('.xy', '.hst', '.m1', '.m2'):
            shutil.copyfile(fixture / ('game' + ext), source / ('exchange' + ext))
        backup = source / 'backup.000'
        backup.mkdir()
        shutil.copyfile(source / 'exchange.m1', backup / 'exchange.m1')
        backup_turn = (backup / 'exchange.m1').read_bytes()
        original = {p.relative_to(source): p.read_bytes() for p in source.rglob('*') if p.is_file()}
        run(executable, work, STARS_CLIENT_IMPORT_SOURCE=str(source), STARS_CLIENT_IMPORT_NAME='managed-copy')
        imported = work / 'library' / 'Games' / 'managed-copy'
        assert (imported / '.library').is_file()
        assert all((imported / ('managed-copy' + ext)).is_file() for ext in ('.xy', '.hst', '.m1', '.m2'))
        assert (imported / 'backup.000' / 'managed-copy.m1').read_bytes() == (source / 'backup.000' / 'exchange.m1').read_bytes()
        assert (imported / 'backup.000' / 'managed-copy.m1').read_bytes() == backup_turn
        (imported / 'exchange.map').write_text('external universe definition', encoding='utf-8')
        assert {p.relative_to(source): p.read_bytes() for p in source.rglob('*') if p.is_file()} == original
        metadata = (imported / '.library').read_text(encoding='utf-8').splitlines()
        assert metadata[0] == '2' and metadata[4] == 'exchange'
        playable = games / 'managed-playable'
        playable_path = playable / 'managed-playable'
        run(executable, work, STARS_CLIENT_NEW_GAME=str(playable_path.with_suffix('.xy')),
            STARS_CLIENT_LIBRARY_NAME='managed-playable')
        playable_backup = playable / 'backup.000'
        playable_backup.mkdir()
        shutil.copyfile(playable / 'managed-playable.m1', playable_backup / 'managed-playable.m1')
        (playable / 'exchange.map').write_text('external universe definition', encoding='utf-8')
        playable_metadata = (playable / '.library').read_text(encoding='utf-8').splitlines()
        playable_metadata[2:4] = ['0', '42']
        (playable / '.library').write_text('\n'.join(playable_metadata) + '\n', encoding='utf-8')
        playable_turn = (playable_backup / 'managed-playable.m1').read_bytes()
        run(executable, work, STARS_CLIENT_GAME_LIBRARY_TEST='duplicate',
            STARS_CLIENT_GAME_SOURCE_NAME='managed-playable',
            STARS_CLIENT_GAME_COPY_NAME='managed-playable duplicate',
            STARS_CLIENT_GAME_RENAME_NAME='renamed-copy',
            STARS_CLIENT_GAME_COLLISION_NAME='MANAGED-COPY',
            STARS_CLIENT_GAME_BASELINE_NAME='managed-playable')
        duplicate = games / 'managed-playable duplicate'
        duplicate_metadata = (duplicate / '.library').read_text(encoding='utf-8').splitlines()
        assert duplicate_metadata[1:5] == ['managed-playable duplicate', '-1', '0', 'managed-playable']
        assert (playable / '.library').read_text(encoding='utf-8').splitlines()[2:4] == ['0', '42']
        assert (duplicate / 'managed-playable duplicate.xy').read_bytes() == (playable / 'managed-playable.xy').read_bytes()
        assert (duplicate / 'backup.000' / 'managed-playable duplicate.m1').read_bytes() == playable_turn
        assert {p.relative_to(playable): p.read_bytes() for p in playable.rglob('*') if p.is_file()} != {
            p.relative_to(duplicate): p.read_bytes() for p in duplicate.rglob('*') if p.is_file()}
        run(executable, work, STARS_CLIENT_LIBRARY_OPEN='managed-playable duplicate')

        duplicate_metadata[2:4] = ['1', '123456']
        (duplicate / '.library').write_text('\n'.join(duplicate_metadata) + '\n', encoding='utf-8')
        run(executable, work, STARS_CLIENT_GAME_LIBRARY_TEST='rename',
            STARS_CLIENT_GAME_SOURCE_NAME='managed-playable duplicate',
            STARS_CLIENT_GAME_COPY_NAME='managed-playable duplicate',
            STARS_CLIENT_GAME_RENAME_NAME='renamed-copy',
            STARS_CLIENT_GAME_COLLISION_NAME='MANAGED-COPY',
            STARS_CLIENT_GAME_BASELINE_NAME='managed-playable')
        renamed = games / 'renamed-copy'
        assert not (games / 'managed-playable duplicate').exists()
        assert (renamed / 'exchange.map').read_text(encoding='utf-8') == 'external universe definition'
        assert (renamed / 'backup.000' / 'renamed-copy.m1').is_file()
        metadata = (renamed / '.library').read_text(encoding='utf-8').splitlines()
        assert metadata[1] == 'renamed-copy' and metadata[2:4] == ['1', '123456'] and metadata[4] == 'managed-playable'
        assert (renamed / 'backup.000' / 'renamed-copy.m1').read_bytes() == playable_turn
        assert (renamed / 'renamed-copy.xy').read_bytes() == (playable / 'managed-playable.xy').read_bytes()
        assert (playable / '.library').read_text(encoding='utf-8').splitlines()[2:4] == ['0', '42']
        run(executable, work, STARS_CLIENT_LIBRARY_OPEN='renamed-copy')

        bad_source = work / 'bad-exchange'
        shutil.copytree(source, bad_source)
        turn = bad_source / 'exchange.m2'
        data = bytearray(turn.read_bytes())
        data[6] ^= 1
        turn.write_bytes(data)
        run(executable, work, STARS_CLIENT_IMPORT_SOURCE=str(bad_source),
            STARS_CLIENT_IMPORT_NAME='invalid-copy', STARS_CLIENT_IMPORT_INVALID_EXPECTED='1')
        assert not (work / 'library' / 'Games' / 'invalid-copy').exists()

        bad_type_source = work / 'bad-file-type'
        shutil.copytree(source, bad_type_source)
        turn = bad_type_source / 'exchange.m2'
        data = bytearray(turn.read_bytes())
        data[16] = 0  # .m2 must have a turn BOF, not a universe BOF.
        turn.write_bytes(data)
        run(executable, work, STARS_CLIENT_IMPORT_SOURCE=str(bad_type_source),
            STARS_CLIENT_IMPORT_NAME='invalid-type', STARS_CLIENT_IMPORT_INVALID_EXPECTED='1')
        assert not (work / 'library' / 'Games' / 'invalid-type').exists()

    with tempfile.TemporaryDirectory(prefix='stars-import-file-') as directory:
        work = Path(directory)
        source = work / 'exchange'
        source.mkdir()
        fixture = Path(__file__).parents[1] / 'scaffold' / 'fixtures' / 'regression' / 'native' / 'oneai2' / 'checkpoints' / '000'
        for ext in ('.xy', '.hst', '.m1', '.m2'):
            shutil.copyfile(fixture / ('game' + ext), source / ('exchange' + ext))
        run(executable, work, STARS_CLIENT_IMPORT_SOURCE=str(source / 'exchange.m1'),
            STARS_CLIENT_IMPORT_NAME='managed-file')
        imported = work / 'library' / 'Games' / 'managed-file'
        assert (imported / 'managed-file.m1').is_file()
        assert (imported / 'managed-file.m2').is_file()

    with tempfile.TemporaryDirectory(prefix='stars-import-open-') as directory:
        work = Path(directory)
        generated_work = work / 'generated'
        generated_work.mkdir()
        game = generated_work / 'library' / 'Games' / 'exchange' / 'exchange.xy'
        run(executable, generated_work, STARS_CLIENT_NEW_GAME=str(game), STARS_CLIENT_LIBRARY_NAME='exchange')
        source = work / 'exchange-source'
        source.mkdir()
        for ext in ('.xy', '.hst', '.m1'):
            shutil.copyfile(game.parent / ('exchange' + ext), source / ('exchange' + ext))
        client_work = work / 'client'
        client_work.mkdir()
        run(executable, client_work, STARS_CLIENT_IMPORT_SOURCE=str(source / 'exchange.m1'),
            STARS_CLIENT_IMPORT_NAME='managed-playable', STARS_CLIENT_IMPORT_EXPECTED_PLAYERS='1')
        imported = client_work / 'library' / 'Games' / 'managed-playable'
        assert all((imported / ('managed-playable' + ext)).is_file() for ext in ('.xy', '.hst', '.m1'))
        run(executable, client_work, STARS_CLIENT_LIBRARY_OPEN='managed-playable')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
