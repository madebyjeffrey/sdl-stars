"""Create managed games, resolve collisions, and reopen in a fresh client."""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time


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
                'STARS_CLIENT_NEW_ADVANCED', 'STARS_CLIENT_LIBRARY_OPEN',
                'STARS_CLIENT_LIBRARY_OPEN_PLAYER', 'STARS_CLIENT_EXPECT_PLAYER',
                'STARS_CLIENT_EXPECT_SELECTOR', 'STARS_CLIENT_EXPECT_CONTINUE_DISABLED',
                'STARS_CLIENT_SWITCH_CANCEL_TEST', 'STARS_CLIENT_EXPECT_CONTINUE_GAME',
                'STARS_CLIENT_EXPECT_CONTINUE_YEAR', 'STARS_CLIENT_SWITCH_CHOICE',
                'STARS_CLIENT_EXPECT_SWITCHED_PLAYER'):
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
        source = work / 'exchange'
        source.mkdir()
        fixture = Path(__file__).parents[1] / 'scaffold' / 'fixtures' / 'regression' / 'native' / 'oneai2' / 'checkpoints' / '000'
        for ext in ('.xy', '.hst', '.m1', '.m2'):
            shutil.copyfile(fixture / ('game' + ext), source / ('exchange' + ext))
        backup = source / 'backup.000'
        backup.mkdir()
        shutil.copyfile(source / 'exchange.m1', backup / 'exchange.m1')
        original = {p.relative_to(source): p.read_bytes() for p in source.rglob('*') if p.is_file()}
        run(executable, work, STARS_CLIENT_IMPORT_SOURCE=str(source), STARS_CLIENT_IMPORT_NAME='managed-copy')
        imported = work / 'library' / 'Games' / 'managed-copy'
        assert (imported / '.library').is_file()
        assert all((imported / ('managed-copy' + ext)).is_file() for ext in ('.xy', '.hst', '.m1', '.m2'))
        assert (imported / 'backup.000' / 'managed-copy.m1').read_bytes() == (source / 'backup.000' / 'exchange.m1').read_bytes()
        assert {p.relative_to(source): p.read_bytes() for p in source.rglob('*') if p.is_file()} == original
        metadata = (imported / '.library').read_text(encoding='utf-8').splitlines()
        assert metadata[0] == '2' and metadata[4] == 'exchange'
        run(executable, work, STARS_CLIENT_LIBRARY_OPEN='managed-copy',
            STARS_CLIENT_LIBRARY_OPEN_PLAYER='2', STARS_CLIENT_EXPECT_PLAYER='2')
        metadata = (imported / '.library').read_text(encoding='utf-8').splitlines()
        assert metadata[2] == '1'
        turn_before = (imported / 'managed-copy.m2').read_bytes()
        run(executable, work, STARS_CLIENT_LIBRARY_OPEN='managed-copy',
            STARS_CLIENT_LIBRARY_OPEN_PLAYER='2', STARS_CLIENT_EXPECT_PLAYER='2',
            STARS_CLIENT_SWITCH_CANCEL_TEST='1')
        assert (imported / '.library').read_text(encoding='utf-8').splitlines()[2] == '1'
        assert (imported / 'managed-copy.m2').read_bytes() == turn_before
        for choice in ('save', 'discard', 'submit'):
            with tempfile.TemporaryDirectory(prefix='stars-switch-choice-') as choice_directory:
                choice_work = Path(choice_directory)
                choice_source = choice_work / 'exchange'
                choice_source.mkdir()
                for ext in ('.xy', '.hst', '.m1', '.m2'):
                    shutil.copyfile(fixture / ('game' + ext), choice_source / ('exchange' + ext))
                run(executable, choice_work, STARS_CLIENT_IMPORT_SOURCE=str(choice_source),
                    STARS_CLIENT_IMPORT_NAME='switch-choice')
                run(executable, choice_work, STARS_CLIENT_LIBRARY_OPEN='switch-choice',
                    STARS_CLIENT_LIBRARY_OPEN_PLAYER='2', STARS_CLIENT_EXPECT_PLAYER='2',
                    STARS_CLIENT_EXPECT_SWITCHED_PLAYER='1', STARS_CLIENT_SWITCH_CHOICE=choice)
        run(executable, work, STARS_CLIENT_CONTINUE=str(imported / 'managed-copy.m1'),
            STARS_CLIENT_EXPECT_PLAYER='2', STARS_CLIENT_EXPECT_CONTINUE_GAME='managed-copy',
            STARS_CLIENT_EXPECT_CONTINUE_YEAR='2400')
        (imported / 'managed-copy.m2').unlink()
        run(executable, work, STARS_CLIENT_CONTINUE=str(imported / 'managed-copy.m1'),
            STARS_CLIENT_EXPECT_SELECTOR='1')
        (imported / 'managed-copy.m1').unlink()
        run(executable, work, STARS_CLIENT_CONTINUE=str(imported / 'managed-copy.m1'),
            STARS_CLIENT_EXPECT_CONTINUE_DISABLED='1')

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

    with tempfile.TemporaryDirectory(prefix='stars-import-continue-latest-') as directory:
        work = Path(directory)
        fixture = Path(__file__).parents[1] / 'scaffold' / 'fixtures' / 'regression' / 'native' / 'oneai2' / 'checkpoints' / '000'
        for name in ('older-session', 'latest-session'):
            with tempfile.TemporaryDirectory(prefix='stars-import-continue-seed-') as seed_directory:
                seed = Path(seed_directory)
                source = seed / 'exchange'
                source.mkdir()
                for ext in ('.xy', '.hst', '.m1', '.m2'):
                    shutil.copyfile(fixture / ('game' + ext), source / ('exchange' + ext))
                run(executable, seed, STARS_CLIENT_IMPORT_SOURCE=str(source), STARS_CLIENT_IMPORT_NAME=name)
                seeded_game = seed / 'library' / 'Games' / name
                (work / 'library' / 'Games').mkdir(parents=True, exist_ok=True)
                shutil.copytree(seeded_game, work / 'library' / 'Games' / name)
        older = work / 'library' / 'Games' / 'older-session'
        latest = work / 'library' / 'Games' / 'latest-session'
        run(executable, work, STARS_CLIENT_LIBRARY_OPEN='older-session',
            STARS_CLIENT_LIBRARY_OPEN_PLAYER='2', STARS_CLIENT_EXPECT_PLAYER='2')
        # Metadata stores whole seconds; ensure the second game's open is newer.
        time.sleep(1.1)
        run(executable, work, STARS_CLIENT_LIBRARY_OPEN='latest-session',
            STARS_CLIENT_LIBRARY_OPEN_PLAYER='1', STARS_CLIENT_EXPECT_PLAYER='1')
        older_metadata = (older / '.library').read_text(encoding='utf-8').splitlines()
        latest_metadata = (latest / '.library').read_text(encoding='utf-8').splitlines()
        assert older_metadata[2] == '1' and latest_metadata[2] == '0'
        assert int(latest_metadata[3]) > int(older_metadata[3])
        run(executable, work, STARS_CLIENT_CONTINUE=str(older / 'older-session.m1'),
            STARS_CLIENT_EXPECT_PLAYER='1', STARS_CLIENT_EXPECT_CONTINUE_GAME='latest-session',
            STARS_CLIENT_EXPECT_CONTINUE_YEAR='2400')

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
