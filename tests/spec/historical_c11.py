"""Run unpublished C11 witnesses against their immutable pre-migration snapshot.

These are historical evidence, separate from the linked current C++ witnesses.
The pinned commit preserves the original fixtures, helper imports and sources.
"""
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile

ROOT = Path(__file__).resolve().parents[2]
REVISION = "82679dc7ff8f3e0913ec5d05c9a2cb695673db27"
WITNESSES = (
    "check_library_name_dialog_source.py",
    "check_player_labels_source.py",
    "check_race_actions_source.py",
    "check_race_allocation_source.py",
    "check_race_appearance_source.py",
    "check_race_hit_source.py",
    "check_race_import_source.py",
    "check_race_initialization_source.py",
    "check_race_page_commands_source.py",
    "check_race_save_source.py",
    "check_race_tracking_source.py",
    "check_race_versions_source.py",
    "check_race_wizard_source.py",
    "check_race_workflows_source.py",
)


def run(names):
    if isinstance(names, str):
        names = (names,)
    if not names or any(name not in WITNESSES for name in names):
        raise ValueError("select a recorded historical C11 witness")
    with tempfile.TemporaryDirectory(prefix="stars-historical-c11-") as work:
        root = Path(work)
        archive = root / "snapshot.tar"
        with archive.open("wb") as output:
            subprocess.run(["git", "archive", REVISION, "src", "compat", "text",
                            "data", "res/resource.h", "spec", "tests/spec"],
                           cwd=ROOT, stdout=output, check=True)
        snapshot = root / "snapshot"
        snapshot.mkdir()
        with tarfile.open(archive) as source:
            source.extractall(snapshot, filter="data")
        for name in names:
            print(f"Historical C11 evidence: {name} at {REVISION}", flush=True)
            subprocess.run([sys.executable, str(snapshot / "tests/spec" / name)],
                           cwd=snapshot, check=True)


if __name__ == "__main__":
    run(sys.argv[1:] or WITNESSES)
