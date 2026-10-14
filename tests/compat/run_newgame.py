"""Create managed games, resolve collisions, and reopen in a fresh client."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile


def run(executable, work, default_root=False, **choices):
    (work / 'settings').mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, STARS_SETTINGS_DIR=str(work / 'settings'),
               STARS_LIBRARY_DIR=str(work / 'library'))
    if default_root:
        env.pop('STARS_LIBRARY_DIR', None)
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
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
