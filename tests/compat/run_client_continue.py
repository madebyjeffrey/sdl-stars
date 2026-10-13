"""Exercise Continue Game using copies of frozen native game fixtures."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


def main():
    executable = Path(sys.argv[1]).resolve()
    checkpoint = os.environ.get("STARS_CLIENT_CHECKPOINT", "000")
    if len(checkpoint) != 3 or not checkpoint.isascii() or not checkpoint.isdigit():
        raise ValueError("checkpoint must be three ASCII digits")
    fixture = Path(__file__).resolve().parents[1] / "scaffold/fixtures/regression/native/oneai1/checkpoints" / checkpoint
    for absolute, stem in ((False, "game"), (True, "game"), (False, "café-Ω"), (True, "café-Ω")):
        with tempfile.TemporaryDirectory(prefix="stars-continue-") as directory:
            work = Path(directory)
            if stem != "game":
                work = work / "parties-étoiles-Ω"
                work.mkdir()
            for name in ("game.m1", "game.xy", "game.hst"):
                shutil.copyfile(fixture / name, work / (stem + Path(name).suffix))
            env = dict(os.environ, STARS_SETTINGS_DIR=str(work),
                       STARS_CLIENT_CONTINUE=str(work / (stem + ".m1")) if absolute else stem + ".m1",
                       STARS_MAP_CAPTURE=str(work / "map.json"))
            subprocess.run([str(executable)], cwd=work, env=env, check=True, timeout=45)
            snapshot = json.loads((work / "map.json").read_text(encoding="utf-8"))
            assert snapshot["schema"] == 1 and snapshot["game"] != 0
            objects = snapshot["objects"]
            assert len(objects) > 1 and len({(o["kind"], o["id"]) for o in objects}) == len(objects)
            assert objects[0]["kind"] == 1 and objects[0]["id"] == 0
            assert objects[0]["onScreen"] and objects[0]["scanSelected"]
            assert all(o["name"] and len(o["screen"]) == 2 for o in objects)
    return 0


if __name__ == "__main__":
    sys.exit(main())
