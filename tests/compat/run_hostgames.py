"""Exercise hosted-game imports, order intake and generation in a real client."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile


def main():
    executable = Path(sys.argv[1]).resolve()
    fixture = Path(__file__).resolve().parents[1] / "scaffold/fixtures/regression/native/oneai1/checkpoints/000"
    with tempfile.TemporaryDirectory(prefix="stars-host-library-") as directory:
        work = Path(directory)
        settings = work / "settings"
        source = work / "host-source"
        player_source = work / "player-source"
        order_dir = work / "orders"
        settings.mkdir()
        source.mkdir()
        player_source.mkdir()
        order_dir.mkdir()
        for extension in ("xy", "hst"):
            (source / f"game.{extension}").write_bytes((fixture / f"game.{extension}").read_bytes())
            (player_source / f"game.{extension}").write_bytes((fixture / f"game.{extension}").read_bytes())
        (player_source / "game.m1").write_bytes((fixture / "game.m1").read_bytes())
        env = dict(os.environ, STARS_SETTINGS_DIR=str(settings),
                   STARS_CLIENT_HOST_LIBRARY_TEST="1", STARS_CLIENT_HOST_IMPORT_SOURCE=str(source),
                   STARS_CLIENT_HOST_PLAYER_IMPORT_SOURCE=str(player_source),
                   STARS_CLIENT_HOST_ORDER_BASE=str(order_dir / "submitted"),
                   SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy",
                   STARS_SDL_RENDERER="software", STARS_FILE_DIALOG_BACKEND="compat")
        subprocess.run([str(executable)], cwd=work, env=env, check=True, timeout=75)
        assert (settings / "Games" / "Hosted" / "Hosted.xy").read_bytes() == (source / "game.xy").read_bytes()
        assert (settings / "Games" / "Hosted" / "Hosted.hst").read_bytes() != (source / "game.hst").read_bytes()
        assert (settings / "Games" / "Hosted" / "Hosted.m1").is_file()
        assert (settings / "Games" / "PlayerCopy" / "PlayerCopy.xy").is_file()
        assert (settings / "Games" / "PlayerCopy" / "PlayerCopy.hst").is_file()
        assert (settings / "Games" / "PlayerCopy" / "PlayerCopy.m1").is_file()
        assert not (settings / "Games" / "Hosted" / "Hosted.x1").exists()


if __name__ == "__main__":
    main()
