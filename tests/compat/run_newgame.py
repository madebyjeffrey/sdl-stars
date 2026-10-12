"""Create fresh ASCII/UTF-8 games through the actual Basic New Game UI."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile


def main():
    executable = Path(sys.argv[1]).resolve()
    for advanced, name in ((False, 'fresh-game'), (False, 'nouvelle-partie-Ω'),
                           (True, 'advanced-game'), (True, 'avancée-Ω')):
        with tempfile.TemporaryDirectory(prefix='stars-newgame-') as directory:
            work = Path(directory)
            env = dict(os.environ, STARS_SETTINGS_DIR=str(work),
                       STARS_CLIENT_NEW_GAME=str(work / (name + '.xy')))
            env.pop('STARS_CLIENT_CONTINUE', None)
            env.pop('STARS_CLIENT_NEW_ADVANCED', None)
            if advanced:
                env['STARS_CLIENT_NEW_ADVANCED'] = '1'
            subprocess.run([str(executable)], cwd=work, env=env, check=True, timeout=45)
            assert all((work / (name + ext)).stat().st_size > 0 for ext in ('.xy', '.hst', '.m1'))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
