"""Run the real-client file workflow with a controlled native-chooser adapter."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument('--client', required=True)
args = parser.parse_args()

with tempfile.TemporaryDirectory(prefix='stars-file-workflow-compat-') as directory:
    root = Path(directory) / 'étoiles-Ω'
    root.mkdir()
    root = root.resolve()
    fixture = Path(__file__).resolve().parents[1] / 'scaffold/fixtures/regression/native/oneai1/checkpoints/000'
    managed = root / 'Games' / 'Renamed copy'
    managed.mkdir(parents=True)
    for suffix in ('.m1', '.xy', '.hst'):
        shutil.copyfile(fixture / ('game' + suffix), managed / ('Renamed copy' + suffix))
    (managed / '.library').write_text('2\nRenamed copy\n0\n0\ncafé-Ω\n', encoding='utf-8')
    race = root / 'race-long-ΩΩΩΩ-native-filename.r1'
    race_repeat = root / 'race-repeat-é-Ω.r1'
    race_export = root / 'Exports' / 'Managed.r1'
    race_export.parent.mkdir()
    export_choice = root / 'user-orders.changed-extension'
    exported = root / 'user-orders.x1'
    previous_export = root / 'previous-orders.x1'
    previous_export.write_bytes(b'previous exported orders')
    shutil.copyfile(previous_export, exported)
    assert not export_choice.exists()
    managed_orders = managed / 'Renamed copy.x1'
    env = dict(os.environ, STARS_SETTINGS_DIR=str(root), STARS_PROFILE_DIR=str(root),
               STARS_TEST_GAME_PATH=str(managed / 'Renamed copy.m1'),
               STARS_TEST_RACE_PATH=str(race), STARS_TEST_RACE_REPEAT_PATH=str(race_repeat),
               STARS_TEST_RACE_EXPORT_PATH=str(race_export),
               STARS_TEST_EXPORT_EXPECTED=str(exported), STARS_TEST_EXPORT_PREVIOUS=str(previous_export),
               STARS_TEST_MANAGED_ORDERS=str(managed_orders),
               STARS_TEST_MANAGED_TURN=str(managed / 'Renamed copy.m1'),
               STARS_TEST_EXPORT_FAILURE_TARGET=str(root / 'missing-directory' / 'cannot-write.x1'),
               STARS_TEST_EXPORT_CHOICE=str(export_choice), STARS_FILE_DIALOG_BACKEND='native',
               SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy', STARS_SDL_RENDERER='software')
    env.pop("STARS_LIBRARY_DIR", None)
    subprocess.run([str(Path(args.client).resolve())], cwd=root, env=env, check=True, timeout=75)
    assert exported.is_file() and managed_orders.is_file()
    assert exported.read_bytes() != previous_export.read_bytes()
    assert (managed / 'Renamed copy.m1').is_file()
    imported = root / "Race Designs" / race.name
    assert imported.read_bytes() == race.read_bytes() == Path(str(race) + ".before-import").read_bytes()
    assert race_repeat.is_file()
print('Native chooser game/race and managed orders export workflow passed on UTF-8 paths')
