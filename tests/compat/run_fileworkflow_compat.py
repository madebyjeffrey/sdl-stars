"""Replay native game/race I/O with deterministic choices on every SDL platform."""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


def main():
    executable = Path(sys.argv[1]).resolve()
    fixture = Path(__file__).resolve().parents[1] / 'scaffold/fixtures/regression/native/oneai1/checkpoints/000'
    with tempfile.TemporaryDirectory(prefix='stars-file-workflow-') as directory:
        root = Path(directory) / 'étoiles-Ω'
        root.mkdir()
        for suffix in ('.m1', '.xy', '.hst'):
            shutil.copyfile(fixture / ('game' + suffix), root / ('café-Ω' + suffix))
        race = root / 'race-long-ΩΩΩΩ-native-filename.r1'
        race_repeat = root / 'race-repeat-é-Ω.r1'
        env = dict(os.environ, STARS_SETTINGS_DIR=str(root), STARS_PROFILE_DIR=str(root),
                   STARS_TEST_GAME_PATH=str(root / 'café-Ω.m1'), STARS_TEST_RACE_PATH=str(race),
                   STARS_TEST_RACE_REPEAT_PATH=str(race_repeat), STARS_TEST_FILE_WORKFLOW_COMPAT='1',
                   SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy', STARS_SDL_RENDERER='software',
                   STARS_FILE_DIALOG_BACKEND='compat')
        env.pop('STARS_LIBRARY_DIR', None)
        subprocess.run([str(executable)], cwd=root, env=env, check=True, timeout=10)
        imported = root / 'Race Designs' / race.name
        assert imported.read_bytes() == race.read_bytes() == Path(str(race) + '.before-import').read_bytes()
        assert race_repeat.stat().st_size > 0
        print('Game open/cancel, race save/import/reload, and source immutability passed')


if __name__ == '__main__':
    main()
