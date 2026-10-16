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

        # Folder export carries every managed player file and its backup,
        # while keeping private library metadata and the managed copy intact.
        export_parent = work / 'export'
        export_parent.mkdir()
        run(executable, work, STARS_CLIENT_EXPORT_GAME='managed-copy', STARS_CLIENT_EXPORT_PARENT=str(export_parent),
            STARS_CLIENT_EXPORT_COLLISION='1')
        exported = export_parent / 'managed-copy'
        source_before_export = {p.relative_to(imported): p.read_bytes() for p in imported.rglob('*') if p.is_file()}
        expected_export = {path: content for path, content in source_before_export.items() if path.name != '.library'}
        expected_export[Path('keep.txt')] = b'preserve existing export'
        assert {p.relative_to(exported): p.read_bytes() for p in exported.rglob('*') if p.is_file()} == expected_export
        assert (exported / 'keep.txt').read_text(encoding='utf-8') == 'preserve existing export'
        assert {p.relative_to(imported): p.read_bytes() for p in imported.rglob('*') if p.is_file()} == source_before_export
        run(executable, work, STARS_CLIENT_CONTINUE=str(exported / 'managed-copy.m1'))

        # Saved orders and history are included with their contents intact.
        (imported / 'managed-copy.x1').write_bytes(b'saved player one orders')
        (imported / 'managed-copy.h2').write_bytes(b'player two history')
        (imported / 'managed-copy.x2').write_bytes(b'saved player two orders')
        source_before_export = {p.relative_to(imported): p.read_bytes() for p in imported.rglob('*') if p.is_file()}
        orders_export_parent = work / 'export-with-orders'
        orders_export_parent.mkdir()
        run(executable, work, STARS_CLIENT_EXPORT_GAME='managed-copy', STARS_CLIENT_EXPORT_PARENT=str(orders_export_parent))
        orders_export = orders_export_parent / 'managed-copy'
        assert {p.relative_to(orders_export): p.read_bytes() for p in orders_export.rglob('*') if p.is_file()} == {
            path: content for path, content in source_before_export.items() if path.name != '.library'
        }

        # A destination write failure after staging must leave the managed
        # game and the pre-existing destination file untouched.
        failed_export_parent = work / 'export-failure'
        failed_export_parent.mkdir()
        blocked_destination = failed_export_parent / 'managed-copy'
        blocked_destination.write_text('keep failed export destination', encoding='utf-8')
        run(executable, work, STARS_CLIENT_EXPORT_GAME='managed-copy',
            STARS_CLIENT_EXPORT_PARENT=str(failed_export_parent), STARS_CLIENT_EXPORT_FAIL='1')
        assert blocked_destination.read_text(encoding='utf-8') == 'keep failed export destination'
        assert {p.relative_to(imported): p.read_bytes() for p in imported.rglob('*') if p.is_file()} == source_before_export
        assert not list(failed_export_parent.glob('.stars-game-export-*'))

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
        changed_same_year = bytearray(later_turn_original)
        changed_same_year[25] ^= 1
        same_year_turn.write_bytes(changed_same_year)
        run(executable, work, STARS_CLIENT_IMPORT_UPDATE_SOURCE=str(same_year_turn),
            STARS_CLIENT_IMPORT_UPDATE_EXPECTED='fail', STARS_CLIENT_IMPORT_UPDATE_TARGET='managed-copy-duplicate')
        older_turn = Path(__file__).parents[1] / 'scaffold' / 'fixtures' / 'regression' / 'native' / 'oneai2' / 'checkpoints' / '000' / 'game.m1'
        run(executable, work, STARS_CLIENT_IMPORT_UPDATE_SOURCE=str(older_turn),
            STARS_CLIENT_IMPORT_UPDATE_EXPECTED='fail', STARS_CLIENT_IMPORT_UPDATE_TARGET='managed-copy-duplicate')
        assert (duplicate / 'managed-copy-duplicate.m1').read_bytes() == before_failure
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
