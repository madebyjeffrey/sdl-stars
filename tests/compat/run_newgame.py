"""Create managed games, resolve collisions, and reopen in a fresh client."""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time
import zipfile


def run(executable, work, default_root=False, settings_root=False, **choices):
    (work / 'settings').mkdir(parents=True, exist_ok=True)
    (work / 'settings' / 'Stars.ini').unlink(missing_ok=True)
    env = dict(os.environ, STARS_SETTINGS_DIR=str(work / 'settings'),
               STARS_PROFILE_DIR=str(work / 'settings'),
               STARS_LIBRARY_DIR=str(work / 'library'),
               STARS_FILE_DIALOG_BACKEND='compat',
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


def check_first_player_turn(executable):
    fixture = Path(__file__).parents[1] / 'scaffold' / 'fixtures' / 'regression' / 'native' / 'oneai2' / 'checkpoints' / '000'
    for extensions, player in ((('.xy', '.m1'), 2), (('.xy', '.hst'), 1)):
        with tempfile.TemporaryDirectory(prefix='stars-first-player-turn-') as directory:
            work = Path(directory)
            source = work / 'exchange'
            source.mkdir()
            for extension in extensions:
                shutil.copyfile(fixture / ('game' + extension), source / ('exchange' + extension))
            run(executable, work, STARS_CLIENT_IMPORT_SOURCE=str(source),
                STARS_CLIENT_IMPORT_NAME='first-turn',
                STARS_CLIENT_IMPORT_EXPECTED_PLAYERS='1' if player == 2 else '0')
            managed = work / 'library' / 'Games' / 'first-turn'
            incoming = work / ('incoming.m' + str(player))
            shutil.copyfile(fixture / ('game.m' + str(player)), incoming)
            target = managed / ('first-turn.m' + str(player))
            for extension in ('.x1', '.x2', '.h1', '.h2'):
                (managed / ('first-turn' + extension)).write_bytes(b'preserve companion')
            before = {p.name: p.read_bytes() for p in managed.iterdir() if p.is_file()}
            original = {p.name: p.read_bytes() for p in source.iterdir()}
            target.mkdir()
            run(executable, work, STARS_CLIENT_IMPORT_UPDATE_SOURCE=str(incoming),
                STARS_CLIENT_IMPORT_UPDATE_EXPECTED='fail')
            target.rmdir()
            if player == 1:
                run(executable, work, STARS_CLIENT_IMPORT_UPDATE_SOURCE=str(incoming),
                    STARS_CLIENT_IMPORT_UPDATE_EXPECTED='fail', STARS_CLIENT_IMPORT_UPDATE_FAIL_REPLACE='1')
                assert not target.exists()
                assert not list(managed.glob('*.importing.*'))
            invalid = bytearray(incoming.read_bytes())
            invalid[14:16] = ((int.from_bytes(invalid[14:16], 'little') & ~0x1f) | 15).to_bytes(2, 'little')
            absent_player = work / 'absent.m16'
            absent_player.write_bytes(invalid)
            run(executable, work, STARS_CLIENT_IMPORT_UPDATE_SOURCE=str(absent_player),
                STARS_CLIENT_IMPORT_UPDATE_EXPECTED='fail')
            assert not (managed / 'first-turn.m16').exists()
            run(executable, work, STARS_CLIENT_IMPORT_UPDATE_SOURCE=str(incoming),
                STARS_CLIENT_IMPORT_UPDATE_EXPECTED='identical')
            assert target.read_bytes() == incoming.read_bytes()
            assert all((managed / name).read_bytes() == data for name, data in before.items())
            assert {p.name: p.read_bytes() for p in source.iterdir()} == original
            assert not list(managed.glob('*.importing.*'))
            run(executable, work, STARS_CLIENT_IMPORT_UPDATE_SOURCE=str(incoming),
                STARS_CLIENT_IMPORT_UPDATE_EXPECTED='identical')


def check_failed_switch_save(executable):
    fixture = Path(__file__).parents[1] / 'scaffold/fixtures/regression/native/oneai2/checkpoints/000'
    for choice in ('save', 'submit'):
        for failure in ('orders', 'history'):
            with tempfile.TemporaryDirectory(prefix='stars-failed-switch-save-') as directory:
                work = Path(directory)
                source = work / 'exchange'
                source.mkdir()
                for extension in ('.xy', '.hst', '.m1', '.m2'):
                    shutil.copyfile(fixture / ('game' + extension), source / ('exchange' + extension))
                run(executable, work, STARS_CLIENT_IMPORT_SOURCE=str(source),
                    STARS_CLIENT_IMPORT_NAME='save-failure')
                run(executable, work, STARS_CLIENT_LIBRARY_OPEN='save-failure',
                    STARS_CLIENT_LIBRARY_OPEN_PLAYER='2', STARS_CLIENT_EXPECT_PLAYER='2',
                    STARS_CLIENT_EXPECT_SWITCHED_PLAYER='1', STARS_CLIENT_SWITCH_CHOICE=choice,
                    STARS_CLIENT_SWITCH_FAIL_SAVE=failure)
                managed = work / 'library/Games/save-failure'
                assert (managed / 'save-failure.x2').is_file()
                assert (managed / 'save-failure.h2').is_file()


def check_uppercase_imports(executable):
    fixture = Path(__file__).parents[1] / 'scaffold/fixtures/regression/native/oneai2/checkpoints/000'
    for universe_extension in ('.XY', '.xY'):
        with tempfile.TemporaryDirectory(prefix='stars-uppercase-import-') as directory:
            work = Path(directory)
            source = work / 'exchange'
            source.mkdir()
            for extension in ('.xy', '.hst', '.m1', '.m2'):
                target_extension = universe_extension if extension == '.xy' else extension.upper()
                shutil.copyfile(fixture / ('game' + extension), source / ('exchange' + target_extension))
            before = {p.name: p.read_bytes() for p in source.iterdir()}
            archive = work / 'exchange.zip'
            with zipfile.ZipFile(archive, 'w') as output:
                for path in source.iterdir():
                    output.write(path, path.name)
            archive_before = archive.read_bytes()
            for kind, incoming in (('folder', source), ('zip', archive)):
                client_work = work / kind
                run(executable, client_work, STARS_CLIENT_IMPORT_SOURCE=str(incoming),
                    STARS_CLIENT_IMPORT_NAME='uppercase')
                managed = client_work / 'library/Games/uppercase'
                for extension in ('.xy', '.hst', '.m1', '.m2'):
                    assert (managed / ('uppercase' + extension)).read_bytes() == (fixture / ('game' + extension)).read_bytes()
                run(executable, client_work, STARS_CLIENT_LIBRARY_OPEN='uppercase',
                    STARS_CLIENT_LIBRARY_OPEN_PLAYER='2', STARS_CLIENT_EXPECT_PLAYER='2')
            assert {p.name: p.read_bytes() for p in source.iterdir()} == before
            assert archive.read_bytes() == archive_before


def main():
    executable = Path(sys.argv[1]).resolve()
    check_first_player_turn(executable)
    check_failed_switch_save(executable)
    check_uppercase_imports(executable)
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
            if not advanced and name == 'fresh-game':
                metadata_file = path.parent / '.library'
                metadata = metadata_file.read_text(encoding='utf-8').splitlines()
                metadata[0] = '1'
                metadata_file.write_text('\n'.join(metadata[:4]) + '\n', encoding='utf-8')
                names = sorted(p.name for p in games.iterdir())
                run(executable, work, STARS_CLIENT_LIBRARY_OPEN=name, STARS_CLIENT_LIBRARY_DOUBLECLICK='1')
                assert sorted(p.name for p in games.iterdir()) == names
                assert metadata_file.read_text(encoding='utf-8').splitlines()[4] == name
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
        for ext in ('.m1', '.m2'):
            shutil.copyfile(source / ('exchange' + ext), imported / ('managed-copy' + ext))
        playable = games / 'managed-playable'
        playable_path = playable / 'managed-playable'
        run(executable, work, STARS_CLIENT_NEW_GAME=str(playable_path.with_suffix('.xy')),
            STARS_CLIENT_LIBRARY_NAME='managed-playable')
        backup_names = ('backup', 'backup0', 'backup98', 'backup.000', 'backup.998', 'sidecars/nested')
        for name in backup_names:
            playable_backup = playable / name
            playable_backup.mkdir(parents=True)
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
        for name in backup_names:
            assert (duplicate / name / 'managed-playable duplicate.m1').read_bytes() == playable_turn
        assert {p.relative_to(playable): p.read_bytes() for p in playable.rglob('*') if p.is_file()} != {
            p.relative_to(duplicate): p.read_bytes() for p in duplicate.rglob('*') if p.is_file()}
        run(executable, work, STARS_CLIENT_LIBRARY_OPEN='managed-playable duplicate')
        assert (duplicate / '.library').read_text(encoding='utf-8').splitlines()[4] == 'managed-playable'

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
        for name in backup_names:
            assert (renamed / name / 'renamed-copy.m1').read_bytes() == playable_turn
        assert (renamed / 'renamed-copy.xy').read_bytes() == (playable / 'managed-playable.xy').read_bytes()
        assert (playable / '.library').read_text(encoding='utf-8').splitlines()[2:4] == ['0', '42']
        run(executable, work, STARS_CLIENT_LIBRARY_OPEN='renamed-copy')
        assert (renamed / '.library').read_text(encoding='utf-8').splitlines()[4] == 'managed-playable'
        run(executable, work, STARS_CLIENT_LIBRARY_OPEN='renamed-copy')
        assert (renamed / '.library').read_text(encoding='utf-8').splitlines()[4] == 'managed-playable'

        # Two-digit player extensions use the full one-based player number.
        player16 = bytearray((fixture / 'game.m1').read_bytes())
        player16[14:16] = ((int.from_bytes(player16[14:16], 'little') & ~0x1f) | 15).to_bytes(2, 'little')
        (imported / 'managed-copy.m16').write_bytes(player16)
        player16_later = bytearray(player16)
        player16_later[12:14] = (1).to_bytes(2, 'little')
        player16_source = work / 'player-16.m16'
        player16_source.write_bytes(player16_later)
        run(executable, work, STARS_CLIENT_IMPORT_UPDATE_SOURCE=str(player16_source),
            STARS_CLIENT_IMPORT_UPDATE_EXPECTED='player16', STARS_CLIENT_IMPORT_UPDATE_TARGET='managed-copy')
        assert (imported / 'managed-copy.m16').read_bytes() == player16_later

        # An identical turn is a no-op; a later standalone turn updates only
        # its player and retires that player's saved orders.
        orders = imported / 'managed-copy.x1'
        orders.write_bytes(b'old player orders')
        later_turn = Path(__file__).parents[1] / 'scaffold' / 'fixtures' / 'regression' / 'native' / 'oneai2' / 'checkpoints' / '025' / 'game.m1'
        later_turn_original = later_turn.read_bytes()
        run(executable, work, STARS_CLIENT_IMPORT_UPDATE_SOURCE=str(source / 'exchange.m1'),
            STARS_CLIENT_IMPORT_UPDATE_EXPECTED='identical')
        assert orders.read_bytes() == b'old player orders'

        # Exercise adjacent annual turns using real turn data with only the
        # clear BOF turn number adjusted to the 2407 -> 2408 pair.
        adjacent_turn_data = bytearray((fixture / 'game.m1').read_bytes())
        adjacent_turn_data[12:14] = (7).to_bytes(2, 'little')
        (imported / 'managed-copy.m1').write_bytes(adjacent_turn_data)
        adjacent_source = work / 'adjacent-2408.m1'
        adjacent_turn_data[12:14] = (8).to_bytes(2, 'little')
        adjacent_source.write_bytes(adjacent_turn_data)
        run(executable, work, STARS_CLIENT_IMPORT_UPDATE_SOURCE=str(adjacent_source),
            STARS_CLIENT_IMPORT_UPDATE_EXPECTED='adjacent', STARS_CLIENT_IMPORT_UPDATE_TARGET='managed-copy')
        assert (imported / 'managed-copy.m1').read_bytes() == adjacent_source.read_bytes()
        assert not orders.exists()
        orders.write_bytes(b'old player orders')

        duplicate = work / 'library' / 'Games' / 'managed-copy-duplicate'
        shutil.copytree(imported, duplicate)
        for path in list(duplicate.rglob('*')):
            if path.is_file() and path.name.startswith('managed-copy'):
                path.rename(path.with_name(path.name.replace('managed-copy', 'managed-copy-duplicate', 1)))
        metadata_path = duplicate / '.library'
        metadata = metadata_path.read_text(encoding='utf-8').splitlines()
        metadata[1] = 'managed-copy-duplicate'
        metadata_path.write_text('\n'.join(metadata) + '\n', encoding='utf-8')
        duplicate_orders = duplicate / 'managed-copy-duplicate.x1'
        duplicate_orders.write_bytes(b'old duplicate orders')
        history = duplicate / 'managed-copy-duplicate.h1'
        history.write_bytes(b'preserve player history')
        backup = duplicate / 'backup.000' / 'managed-copy-duplicate.m1'
        backup_before = backup.read_bytes()
        before_cancel = (duplicate / 'managed-copy-duplicate.m1').read_bytes()
        run(executable, work, STARS_CLIENT_IMPORT_UPDATE_SOURCE=str(later_turn),
            STARS_CLIENT_IMPORT_UPDATE_EXPECTED='cancel', STARS_CLIENT_IMPORT_UPDATE_TARGET='managed-copy-duplicate')
        assert (duplicate / 'managed-copy-duplicate.m1').read_bytes() == before_cancel
        assert duplicate_orders.read_bytes() == b'old duplicate orders'

        run(executable, work, STARS_CLIENT_IMPORT_UPDATE_SOURCE=str(later_turn),
            STARS_CLIENT_IMPORT_UPDATE_EXPECTED='later', STARS_CLIENT_IMPORT_UPDATE_TARGET='managed-copy-duplicate')
        assert (duplicate / 'managed-copy-duplicate.m1').read_bytes() == later_turn.read_bytes()
        assert not duplicate_orders.exists()
        assert history.read_bytes() == b'preserve player history'
        assert backup.read_bytes() == backup_before
        assert (duplicate / 'managed-copy-duplicate.m2').read_bytes() == (source / 'exchange.m2').read_bytes()
        assert (imported / 'managed-copy.m1').read_bytes() == adjacent_source.read_bytes()
        assert orders.read_bytes() == b'old player orders'

        failed_turn = Path(__file__).parents[1] / 'scaffold' / 'fixtures' / 'regression' / 'native' / 'oneai2' / 'checkpoints' / '050' / 'game.m1'
        duplicate_orders.write_bytes(b'orders survive staged install failure')
        before_failure = (duplicate / 'managed-copy-duplicate.m1').read_bytes()
        run(executable, work, STARS_CLIENT_IMPORT_UPDATE_SOURCE=str(failed_turn),
            STARS_CLIENT_IMPORT_UPDATE_EXPECTED='fail', STARS_CLIENT_IMPORT_UPDATE_TARGET='managed-copy-duplicate',
            STARS_CLIENT_IMPORT_UPDATE_FAIL_REPLACE='1')
        assert (duplicate / 'managed-copy-duplicate.m1').read_bytes() == before_failure
        assert duplicate_orders.read_bytes() == b'orders survive staged install failure'

        same_year_turn = work / 'changed-same-year.m1'
        # Change only the BOF salt-time bits, leaving the game and player intact.
        changed_same_year = bytearray(later_turn_original)
        changed_same_year[15] ^= 0x80
        same_year_turn.write_bytes(changed_same_year)
        run(executable, work, STARS_CLIENT_IMPORT_UPDATE_SOURCE=str(same_year_turn),
            STARS_CLIENT_IMPORT_UPDATE_EXPECTED='cancel-conflict', STARS_CLIENT_IMPORT_UPDATE_TARGET='managed-copy-duplicate')
        assert (duplicate / 'managed-copy-duplicate.m1').read_bytes() == before_failure
        assert duplicate_orders.read_bytes() == b'orders survive staged install failure'
        run(executable, work, STARS_CLIENT_IMPORT_UPDATE_SOURCE=str(same_year_turn),
            STARS_CLIENT_IMPORT_UPDATE_EXPECTED='replace-conflict', STARS_CLIENT_IMPORT_UPDATE_TARGET='managed-copy-duplicate')
        assert (duplicate / 'managed-copy-duplicate.m1').read_bytes() == changed_same_year
        assert (duplicate / 'backup.001' / 'managed-copy-duplicate.m1').read_bytes() == before_failure
        assert (duplicate / 'backup.001' / 'managed-copy-duplicate.x1').read_bytes() == b'orders survive staged install failure'
        assert duplicate_orders.read_bytes() == b'orders survive staged install failure'
        assert history.read_bytes() == b'preserve player history'
        older_turn = Path(__file__).parents[1] / 'scaffold' / 'fixtures' / 'regression' / 'native' / 'oneai2' / 'checkpoints' / '000' / 'game.m1'
        before_older_replace = (duplicate / 'managed-copy-duplicate.m1').read_bytes()
        run(executable, work, STARS_CLIENT_IMPORT_UPDATE_SOURCE=str(older_turn),
            STARS_CLIENT_IMPORT_UPDATE_EXPECTED='replace-conflict', STARS_CLIENT_IMPORT_UPDATE_TARGET='managed-copy-duplicate')
        assert (duplicate / 'managed-copy-duplicate.m1').read_bytes() == older_turn.read_bytes()
        assert (duplicate / 'backup.002' / 'managed-copy-duplicate.m1').read_bytes() == before_older_replace
        assert duplicate_orders.read_bytes() == b'orders survive staged install failure'
        assert history.read_bytes() == b'preserve player history'

        separate_turn = work / 'separate-corrected-turn.m1'
        separate_data = bytearray(older_turn.read_bytes())
        separate_data[15] ^= 0x80
        separate_turn.write_bytes(separate_data)
        before_failed_conflict = (duplicate / 'managed-copy-duplicate.m1').read_bytes()
        run(executable, work, STARS_CLIENT_IMPORT_UPDATE_SOURCE=str(separate_turn),
            STARS_CLIENT_IMPORT_UPDATE_EXPECTED='fail-conflict', STARS_CLIENT_IMPORT_UPDATE_TARGET='managed-copy-duplicate',
            STARS_CLIENT_IMPORT_UPDATE_FAIL_CONFLICT_PUBLISH='1')
        assert (duplicate / 'managed-copy-duplicate.m1').read_bytes() == before_failed_conflict
        assert duplicate_orders.read_bytes() == b'orders survive staged install failure'
        assert history.read_bytes() == b'preserve player history'
        assert not list((work / 'library' / 'Games').glob('.stars-game-old-*'))

        # A transient restore failure succeeds on retry and must not direct the
        # user to a recovery folder that the successful restore has removed.
        run(executable, work, STARS_CLIENT_IMPORT_UPDATE_SOURCE=str(separate_turn),
            STARS_CLIENT_IMPORT_UPDATE_EXPECTED='fail-conflict', STARS_CLIENT_IMPORT_UPDATE_TARGET='managed-copy-duplicate',
            STARS_CLIENT_IMPORT_UPDATE_FAIL_CONFLICT_PUBLISH='1',
            STARS_CLIENT_IMPORT_UPDATE_FAIL_CONFLICT_RESTORE='1')
        assert (duplicate / 'managed-copy-duplicate.m1').read_bytes() == before_failed_conflict
        assert duplicate_orders.read_bytes() == b'orders survive staged install failure'
        assert history.read_bytes() == b'preserve player history'
        assert not list((work / 'library' / 'Games').glob('.stars-game-old-*'))

        run(executable, work, STARS_CLIENT_IMPORT_UPDATE_SOURCE=str(separate_turn),
            STARS_CLIENT_IMPORT_UPDATE_EXPECTED='separate-conflict', STARS_CLIENT_IMPORT_UPDATE_TARGET='managed-copy-duplicate',
            STARS_CLIENT_IMPORT_COPY_NAME='corrected-copy')
        separate = work / 'library' / 'Games' / 'corrected-copy'
        assert (separate / 'corrected-copy.xy').read_bytes() == (duplicate / 'managed-copy-duplicate.xy').read_bytes()
        assert (separate / 'corrected-copy.hst').read_bytes() == (duplicate / 'managed-copy-duplicate.hst').read_bytes()
        assert (separate / 'corrected-copy.m1').read_bytes() == separate_data
        assert (separate / 'corrected-copy.m2').read_bytes() == (duplicate / 'managed-copy-duplicate.m2').read_bytes()
        assert (separate / 'corrected-copy.x1').read_bytes() == duplicate_orders.read_bytes()
        assert (separate / 'corrected-copy.h1').read_bytes() == history.read_bytes()
        assert (separate / 'backup.000' / 'corrected-copy.m1').read_bytes() == backup_before
        assert (separate / '.library').read_text(encoding='utf-8').splitlines()[4] == 'exchange'
        assert (duplicate / 'managed-copy-duplicate.m1').read_bytes() == older_turn.read_bytes()
        assert duplicate_orders.read_bytes() == b'orders survive staged install failure'
        assert (source / 'exchange.m1').read_bytes() == original[source.joinpath('exchange.m1').relative_to(source)]
        assert later_turn.read_bytes() == later_turn_original

        # If rollback itself fails, the original turn remains recoverable in
        # its retained previous file instead of being silently discarded.
        before_rollback_failure = (duplicate / 'managed-copy-duplicate.m1').read_bytes()
        run(executable, work, STARS_CLIENT_IMPORT_UPDATE_SOURCE=str(failed_turn),
            STARS_CLIENT_IMPORT_UPDATE_EXPECTED='fail', STARS_CLIENT_IMPORT_UPDATE_TARGET='managed-copy-duplicate',
            STARS_CLIENT_IMPORT_UPDATE_FAIL_ORDERS_REMOVE='1',
            STARS_CLIENT_IMPORT_UPDATE_FAIL_ROLLBACK='1')
        assert (duplicate / 'managed-copy-duplicate.m1').read_bytes() == failed_turn.read_bytes()
        assert duplicate_orders.read_bytes() == b'orders survive staged install failure'
        assert any(path.read_bytes() == before_rollback_failure
                   for path in duplicate.glob('managed-copy-duplicate.m1.previous.*'))

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

    for choice, relative in (('accept', False), ('decline', False), ('accept', True)):
        with tempfile.TemporaryDirectory(prefix='stars-recent-migration-') as directory:
            work = Path(directory)
            settings = work / 'settings'
            settings.mkdir()
            source = work if relative else work / 'external'
            source.mkdir(exist_ok=True)
            fixture = Path(__file__).parents[1] / 'scaffold' / 'fixtures' / 'regression' / 'native' / 'oneai2' / 'checkpoints' / '000'
            for ext in ('.xy', '.m1'):
                shutil.copyfile(fixture / ('game' + ext), source / ('exchange' + ext))
            original = {p.name: p.read_bytes() for p in source.iterdir() if p.is_file()}
            missing = Path('missing.m1') if relative else source / 'missing.m1'
            recent = Path('exchange.m1') if relative else source / 'exchange.m1'
            missing_player = recent.with_suffix('.m2')
            (settings / 'StarsUtf8.ini').write_text(
                f'[Files]\nFile1={missing_player}\nFile2={recent}\nFile3={missing}\n', encoding='utf-8')
            run(executable, work, STARS_CLIENT_RECENT_MIGRATION=choice,
                STARS_CLIENT_IMPORT_NAME='migrated-game', STARS_CLIENT_RECENT_MISSING_EXPECTED='2')
            migrated = work / 'library' / 'Games' / 'migrated-game'
            if choice == 'accept':
                assert (migrated / 'migrated-game.xy').is_file()
                assert (migrated / 'migrated-game.m1').is_file()
                metadata = (migrated / '.library').read_text(encoding='utf-8').splitlines()
                assert metadata[4] == 'exchange'
                assert {p.name: p.read_bytes() for p in source.iterdir() if p.is_file()} == original
            else:
                assert not migrated.exists()
            ini = (settings / 'StarsUtf8.ini').read_text(encoding='utf-8')
            assert 'RecentImportChoice=' + ('1' if choice == 'accept' else '2') in ini
            run(executable, work, STARS_CLIENT_RECENT_MIGRATION='accept',
                STARS_CLIENT_RECENT_MIGRATION_EXPECT_NO_PROMPT='1')
            if choice == 'accept':
                assert (migrated / 'migrated-game.m1').is_file()
            else:
                assert not migrated.exists()

    with tempfile.TemporaryDirectory(prefix='stars-external-open-') as directory:
        work = Path(directory)
        source = work / 'external'
        source.mkdir()
        fixture = Path(__file__).parents[1] / 'scaffold' / 'fixtures' / 'regression' / 'native' / 'oneai2' / 'checkpoints' / '000'
        for ext in ('.xy', '.m1'):
            shutil.copyfile(fixture / ('game' + ext), source / ('exchange' + ext))
        original = {p.name: p.read_bytes() for p in source.iterdir()}
        run(executable, work, STARS_CLIENT_EXTERNAL_OPEN_GAME=str(source / 'exchange.m1'),
            STARS_CLIENT_IMPORT_NAME='external-open-game')
        imported = work / 'library' / 'Games' / 'external-open-game'
        assert (imported / 'external-open-game.m1').is_file()
        assert {p.name: p.read_bytes() for p in source.iterdir()} == original

    with tempfile.TemporaryDirectory(prefix='stars-startup-import-') as directory:
        work = Path(directory)
        source = work / 'external'
        source.mkdir()
        fixture = Path(__file__).parents[1] / 'scaffold' / 'fixtures' / 'regression' / 'native' / 'oneai2' / 'checkpoints' / '000'
        for ext in ('.xy', '.m1'):
            shutil.copyfile(fixture / ('game' + ext), source / ('exchange' + ext))
        original = {p.name: p.read_bytes() for p in source.iterdir()}
        run(executable, work, STARS_CLIENT_STARTUP_SOURCE=str(source / 'exchange.m1'))
        managed = work / 'library' / 'Games' / 'startup-import' / 'startup-import.m1'
        assert managed.is_file()
        assert {p.name: p.read_bytes() for p in source.iterdir()} == original
        run(executable, work, STARS_CLIENT_STARTUP_SOURCE=str(managed), STARS_CLIENT_STARTUP_MANAGED='1')
        assert len(list((work / 'library' / 'Games').iterdir())) == 1

    return 0


if __name__ == '__main__':
    raise SystemExit(main())
