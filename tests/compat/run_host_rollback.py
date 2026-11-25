"""Generate a host turn with complete, partial, and unavailable player outputs."""
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

fixture = Path(__file__).resolve().parents[1] / 'scaffold/fixtures/regression/native/oneai1/checkpoints/000'
executable = Path(sys.argv[1]).resolve()


def generate(game, turns=1):
    return subprocess.run([executable, '-g' if turns == 1 else f'-g{turns}', 'game.hst'],
                          cwd=game, capture_output=True, timeout=45)


def portable_files(game):
    return {p.relative_to(game): p.read_bytes() for p in game.rglob('*')
            if p.is_file() and 'save-transactions' not in p.relative_to(game).parts}


with tempfile.TemporaryDirectory(prefix='stars-turn-') as temporary:
    for label, extensions in [('host', ()), ('partial', ('m1',)), ('complete', ('m1', 'm2'))]:
        game = Path(temporary) / label
        game.mkdir()
        for ext in ('hst', 'xy', *extensions):
            shutil.copyfile(fixture / f'game.{ext}', game / f'game.{ext}')
        original = portable_files(game)
        before = int.from_bytes((game / 'game.hst').read_bytes()[12:14], 'little')
        if label == 'partial':
            blocked = game / 'game.m2'
            blocked.mkdir()
            result = generate(game, 3)
            assert result.returncode == 2, (result.returncode, result.stderr)
            assert blocked.is_dir() and portable_files(game) == original
            blocked.rmdir()
        result = generate(game)
        assert result.returncode == 1, (result.returncode, result.stderr)
        assert int.from_bytes((game / 'game.hst').read_bytes()[12:14], 'little') == before + 1
        assert all((game / f'game.m{i}').stat().st_size > 0 for i in (1, 2))
        assert not (game / 'save-transactions/game/transaction').exists()

    # Resume a rollback interrupted after one original was already renamed
    # back. The receipt prevents the next opener from deleting that member.
    game = Path(temporary) / 'interrupted'
    game.mkdir()
    journal = game / 'save-transactions/game/transaction'
    (journal / 'original').mkdir(parents=True)
    (journal / 'old').mkdir()
    for ext in ('hst', 'xy', 'm1'):
        name = f'game.{ext}'
        (journal / 'original' / name).touch()
        if ext == 'xy':
            shutil.copyfile(fixture / name, game / name)
        else:
            shutil.copyfile(fixture / name, journal / 'old' / name)
            (game / name).write_bytes(b'interrupted')
    (journal / 'ready').touch()
    result = generate(game)
    assert result.returncode == 1, (result.returncode, result.stderr)
    assert (game / 'game.xy').read_bytes() == (fixture / 'game.xy').read_bytes()
    assert (game / 'backup/game.m1').read_bytes() == (fixture / 'game.m1').read_bytes()
    assert not journal.exists()
