"""Export through the desktop, preserving real game files and saved orders."""
import os
from pathlib import Path
import shutil
import sys
import tempfile

from run_newgame import run


def files(directory, metadata=True):
    return {p.relative_to(directory): p.read_bytes() for p in directory.rglob('*')
            if p.is_file() and (metadata or p.name != '.library')}


def main():
    executable = Path(sys.argv[1]).resolve()
    fixture = Path(__file__).parents[1] / 'scaffold/fixtures/regression/native/oneai2/checkpoints/000'
    with tempfile.TemporaryDirectory(prefix='stars-export-') as directory:
        work = Path(directory)
        source = work / 'exchange'
        source.mkdir()
        for ext in ('.xy', '.hst', '.m1', '.m2'):
            shutil.copyfile(fixture / ('game' + ext), source / ('exchange' + ext))
        backup = source / 'backup.000'
        backup.mkdir()
        shutil.copyfile(source / 'exchange.m1', backup / 'exchange.m1')
        run(executable, work, STARS_CLIENT_IMPORT_SOURCE=str(source), STARS_CLIENT_IMPORT_NAME='managed-copy')
        managed = work / 'library/Games/managed-copy'

        # Click Export Game and deliver the chosen folder through its SDL
        # callback; the real window message performs the copy.
        parent = work / 'export'
        parent.mkdir()
        before = files(managed)
        run(executable, work, STARS_CLIENT_EXPORT_GAME='managed-copy', STARS_CLIENT_EXPORT_PARENT=str(parent))
        exported = parent / 'managed-copy'
        assert files(exported) == {p: data for p, data in before.items() if p.name != '.library'}
        assert files(managed) == before

        # Canceling the chooser must not save a dirty loaded game or publish
        # files. Opening the game may update its private last-played metadata.
        canceled = work / 'canceled'
        canceled.mkdir()
        before = files(managed, metadata=False)
        run(executable, work, STARS_CLIENT_EXPORT_GAME='managed-copy', STARS_CLIENT_EXPORT_PARENT=str(canceled),
            STARS_CLIENT_EXPORT_ACTION='cancel')
        assert not list(canceled.iterdir())
        assert files(managed, metadata=False) == before

        # Save a research choice through the existing Save Turn dialog. Load
        # the exported turn in a fresh process to prove its .x1 was replayed.
        saved_parent = work / 'saved'
        saved_parent.mkdir()
        run(executable, work, STARS_CLIENT_EXPORT_GAME='managed-copy', STARS_CLIENT_EXPORT_PARENT=str(saved_parent),
            STARS_CLIENT_EXPORT_ACTION='save', STARS_CLIENT_EXPORT_RESEARCH='4')
        saved = saved_parent / 'managed-copy'
        assert (managed / 'managed-copy.x1').is_file() and (managed / 'managed-copy.h1').is_file()
        assert files(saved) == files(managed, metadata=False)
        assert not ((saved / 'managed-copy.x1').read_bytes()[17] & 1)
        run(executable, work, STARS_CLIENT_EXPORT_GAME='managed-copy', STARS_CLIENT_EXPORT_PARENT=str(saved_parent),
            STARS_CLIENT_EXPORT_ACTION='verify', STARS_CLIENT_EXPORT_RESEARCH='4',
            STARS_CLIENT_CONTINUE=str(saved / 'managed-copy.m1'))

        # Preserve every file in an existing destination, including files that
        # do not belong to the game. A collision must leave no staging folder.
        (exported / 'keep.txt').write_bytes(b'preserve existing export')
        destination_before = files(exported)
        before = files(managed)
        run(executable, work, STARS_CLIENT_EXPORT_GAME='managed-copy', STARS_CLIENT_EXPORT_PARENT=str(parent),
            STARS_CLIENT_EXPORT_EXPECTED='fail')
        assert files(exported) == destination_before and files(managed) == before
        assert list(parent.iterdir()) == [exported]

        # A second player's real saved/submitted orders survive export along
        # with the first player's orders, both histories, host state and backup.
        submitted_parent = work / 'submitted'
        submitted_parent.mkdir()
        run(executable, work, STARS_CLIENT_EXPORT_GAME='managed-copy', STARS_CLIENT_EXPORT_PARENT=str(submitted_parent),
            STARS_CLIENT_EXPORT_ACTION='submit', STARS_CLIENT_EXPORT_PLAYER='1', STARS_CLIENT_EXPORT_RESEARCH='5')
        submitted = submitted_parent / 'managed-copy'
        assert all((submitted / ('managed-copy' + ext)).is_file()
                   for ext in ('.xy', '.hst', '.m1', '.m2', '.x1', '.x2', '.h1', '.h2'))
        assert (submitted / 'backup.000/managed-copy.m1').read_bytes() == (backup / 'exchange.m1').read_bytes()
        assert files(submitted) == files(managed, metadata=False)
        # RTBOF's submitted flag is bit 8 at offset 0x0e after its record header.
        assert (submitted / 'managed-copy.x2').read_bytes()[17] & 1
        for player, research in ((0, 4), (1, 5)):
            run(executable, work, STARS_CLIENT_EXPORT_GAME='managed-copy', STARS_CLIENT_EXPORT_PARENT=str(submitted_parent),
                STARS_CLIENT_EXPORT_ACTION='verify', STARS_CLIENT_EXPORT_PLAYER=str(player),
                STARS_CLIENT_EXPORT_RESEARCH=str(research), STARS_CLIENT_CONTINUE=str(submitted / f'managed-copy.m{player + 1}'))

        # Don't Save exports the previously saved orders once, and leaves the
        # current unsaved research change in memory.
        discarded_parent = work / 'discarded'
        discarded_parent.mkdir()
        before = files(managed, metadata=False)
        run(executable, work, STARS_CLIENT_EXPORT_GAME='managed-copy', STARS_CLIENT_EXPORT_PARENT=str(discarded_parent),
            STARS_CLIENT_EXPORT_ACTION='discard', STARS_CLIENT_EXPORT_RESEARCH='2')
        assert files(discarded_parent / 'managed-copy') == before
        assert files(managed, metadata=False) == before
        run(executable, work, STARS_CLIENT_EXPORT_GAME='managed-copy', STARS_CLIENT_EXPORT_PARENT=str(discarded_parent),
            STARS_CLIENT_EXPORT_ACTION='verify', STARS_CLIENT_EXPORT_RESEARCH='4',
            STARS_CLIENT_CONTINUE=str(discarded_parent / 'managed-copy/managed-copy.m1'))

        # A real filesystem read failure after staging the top-level files must
        # remove the partial copy and leave every original file/directory alone.
        backup_file = managed / 'backup.000/managed-copy.m1'
        backup_bytes = backup_file.read_bytes()
        backup_file.unlink()
        backup_file.mkdir()
        failed_parent = work / 'copy-failure'
        failed_parent.mkdir()
        before = files(managed)
        directories_before = {p.relative_to(managed) for p in managed.rglob('*') if p.is_dir()}
        run(executable, work, STARS_CLIENT_EXPORT_GAME='managed-copy', STARS_CLIENT_EXPORT_PARENT=str(failed_parent),
            STARS_CLIENT_EXPORT_EXPECTED='fail')
        assert not list(failed_parent.iterdir())
        assert files(managed) == before
        assert {p.relative_to(managed) for p in managed.rglob('*') if p.is_dir()} == directories_before
        backup_file.rmdir()
        backup_file.write_bytes(backup_bytes)

        # A destination that became a file cannot accept a staging directory.
        # The write failure must preserve both it and the managed original.
        unavailable = work / 'unavailable'
        unavailable.write_bytes(b'preserve destination file')
        before = files(managed)
        run(executable, work, STARS_CLIENT_EXPORT_GAME='managed-copy', STARS_CLIENT_EXPORT_PARENT=str(unavailable),
            STARS_CLIENT_EXPORT_EXPECTED='fail')
        assert unavailable.read_bytes() == b'preserve destination file'
        assert files(managed) == before

        # The parent and staging directory can fit while child filenames do
        # not. The UI must reject the destination without publishing a game.
        long_name = 'managed-game-' + 'a' * 38
        long_work = work / 'long-case'
        run(executable, long_work, STARS_CLIENT_IMPORT_SOURCE=str(source), STARS_CLIENT_IMPORT_NAME=long_name)
        long_source = long_work / 'library/Games' / long_name
        before = files(long_source)
        long_parent = work / 'long-export'
        while len(os.fsencode(long_parent)) < 440:
            long_parent /= 'p' * max(1, min(90, 439 - len(os.fsencode(long_parent))))
        long_parent.mkdir(parents=True)
        run(executable, long_work, STARS_CLIENT_EXPORT_GAME=long_name, STARS_CLIENT_EXPORT_PARENT=str(long_parent),
            STARS_CLIENT_EXPORT_EXPECTED='fail')
        assert not list(long_parent.iterdir()) and files(long_source) == before
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
