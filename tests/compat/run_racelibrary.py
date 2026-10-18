"""Exercise managed race creation, cancelled editing, import, and reload in a real client."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile


def check_cancelled_dialogs(executable):
    for mode in ("filename", "wizard"):
        with tempfile.TemporaryDirectory(prefix="stars-race-cancel-") as directory:
            env = dict(os.environ, STARS_SETTINGS_DIR=directory, STARS_PROFILE_DIR=directory,
                       STARS_CLIENT_RACE_NAME_CANCEL=mode,
                       SDL_VIDEODRIVER=os.environ.get("SDL_VIDEODRIVER", "dummy"),
                       SDL_AUDIODRIVER="dummy", STARS_SDL_RENDERER="software",
                       STARS_FILE_DIALOG_BACKEND="compat")
            subprocess.run([str(executable)], cwd=directory, env=env, check=True, timeout=20)
            assert not list(Path(directory).rglob("*.r1"))


def main():
    executable = Path(sys.argv[1]).resolve()
    check_cancelled_dialogs(executable)
    if "--dialogs-only" in sys.argv[2:]:
        return
    with tempfile.TemporaryDirectory(prefix="stars-race-roster-") as directory:
        env = dict(os.environ, STARS_SETTINGS_DIR=directory, STARS_PROFILE_DIR=directory,
                   STARS_CLIENT_RACE_ROSTER_TEST="1", SDL_VIDEODRIVER="dummy",
                   SDL_AUDIODRIVER="dummy", STARS_SDL_RENDERER="software", STARS_FILE_DIALOG_BACKEND="compat")
        subprocess.run([str(executable)], cwd=directory, env=env, check=True, timeout=25)
    with tempfile.TemporaryDirectory(prefix="stars-race-library-") as directory:
        work = Path(directory)
        settings = work / "settings"
        settings.mkdir()
        library = settings / "Race Designs"
        library.mkdir()
        (library / "Archive.r1").mkdir()
        source = work / "Managed-source.r1"
        snapshot = work / "Managed-source-snapshot.r1"
        env = dict(os.environ, STARS_SETTINGS_DIR=str(settings),
                   STARS_CLIENT_RACE_LIBRARY_TEST="1", STARS_CLIENT_RACE_IMPORT_SOURCE=str(source),
                   STARS_CLIENT_RACE_IMPORT_SNAPSHOT=str(snapshot),
                   SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy", STARS_SDL_RENDERER="software",
                   STARS_FILE_DIALOG_BACKEND="compat")
        try:
            subprocess.run([str(executable)], cwd=work, env=env, check=True, timeout=60)
        except subprocess.CalledProcessError:
            for path in sorted(library.iterdir()):
                print(f"Race library artifact: {path.name} ({path.stat().st_size} bytes)")
            raise
        assert source.is_file()
        imported = library / "Imported.r2"
        original = source.read_bytes()
        assert imported.read_bytes() == original
        assert source.read_bytes() == snapshot.read_bytes()
        assert imported.read_bytes() == snapshot.read_bytes()
        assert (library / "Managed.r3").read_bytes() == (library / "Managed.r1").read_bytes()
        assert (library / "managed-export.r1").read_bytes() == (library / "Managed.r1").read_bytes()
        assert not (library / "Managed.r2").exists()
        assert (library / "Managed.r3").is_file()
        assert source.read_bytes() == imported.read_bytes()
        env.pop("STARS_CLIENT_RACE_LIBRARY_TEST")
        env.update(STARS_CLIENT_EXTERNAL_OPEN_RACE=str(source), STARS_CLIENT_IMPORT_NAME="From-menu.r1")
        subprocess.run([str(executable)], cwd=work, env=env, check=True, timeout=15)
        assert (library / "From-menu.r1").read_bytes() == original
        assert source.read_bytes() == snapshot.read_bytes()
        assert (library / "Managed.r1.pending.r1").is_file()
        assert (library / "Imported.r2.pending.r1").is_file()


if __name__ == "__main__":
    main()
