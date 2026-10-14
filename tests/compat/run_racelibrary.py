"""Exercise managed race creation, cancelled editing, import, and reload in a real client."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile


def main():
    executable = Path(sys.argv[1]).resolve()
    with tempfile.TemporaryDirectory(prefix="stars-race-library-") as directory:
        work = Path(directory)
        settings = work / "settings"
        settings.mkdir()
        library = settings / "Race Designs"
        source = library / "Created.r1"
        env = dict(os.environ, STARS_SETTINGS_DIR=str(settings),
                   STARS_CLIENT_RACE_LIBRARY_TEST="1", STARS_CLIENT_RACE_IMPORT_SOURCE=str(source),
                   SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy", STARS_SDL_RENDERER="software",
                   STARS_FILE_DIALOG_BACKEND="compat")
        subprocess.run([str(executable)], cwd=work, env=env, check=True, timeout=60)
        assert (library / "Created.r1").is_file()
        imported = library / "Imported.r2"
        original = source.read_bytes()
        assert imported.read_bytes() == original


if __name__ == "__main__":
    main()
