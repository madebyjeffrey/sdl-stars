"""Build, clean and rebuild the real help target without compiling the client."""
from pathlib import Path
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[2]
cmake, ninja = sys.argv[1:]
with tempfile.TemporaryDirectory(prefix="stars-help-clean-") as directory:
    source = Path(directory) / "source"
    build = Path(directory) / "build"
    source.mkdir()
    (source / "CMakeLists.txt").write_text(f'''cmake_minimum_required(VERSION 3.23)
project(HelpClean NONE)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
include([=[{(root / 'cmake/help-guide.cmake').as_posix()}]=])
''', encoding="utf-8")
    subprocess.run([cmake, "-S", str(source), "-B", str(build), "-G", "Ninja",
                    f"-DCMAKE_MAKE_PROGRAM={ninja}"], check=True)
    for cycle in range(2):
        subprocess.run([cmake, "--build", str(build), "--target", "stars-help-guide"], check=True)
        assert (build / "help/help_contexts.h").is_file()
        assert (build / "help/offline.html").is_file()
        assert (build / "help/fonts/LiberationSans-Regular.ttf").is_file()
        # Older guide versions can leave assets not named by the current stamp.
        (build / "help/stale.html").write_text("old generated asset", encoding="utf-8")
        subprocess.run([cmake, "--build", str(build), "--target", "clean"], check=True)
        assert not (build / "help").exists()
        subprocess.run([cmake, "--build", str(build), "--target", "clean"], check=True)
print("Help generation, recursive clean, repeated clean and regeneration passed")
