"""Build, clean and rebuild the real help target without compiling the client."""
from pathlib import Path
import subprocess
import sys
import tempfile
import re
import zipfile

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
        with zipfile.ZipFile(root / "res/help-utf8.zip") as archive:
            for item in archive.infolist():
                staged = (build / "help" / item.filename).read_bytes()
                if item.filename == "help_contexts.h":
                    header = staged.decode("utf-8")
                    assert "typedef" not in header and "UINT32_C" not in header
                    assert not re.search(r"^#define STARS_HELP_(?!CONTEXTS_H)", header, re.M)
                    # IDs, page names and file lists retain every packaged row.
                    assert re.findall(r'^    .*$', header, re.M) == re.findall(r'^    .*$', archive.read(item).decode("utf-8"), re.M)
                    for name in ("ARCHIVE_SIZE", "ARCHIVE_CRC", "BUNDLE_SIZE", "BUNDLE_CRC"):
                        source = re.search(rf"^#define STARS_HELP_{name} (.+)$", archive.read(item).decode("utf-8"), re.M)[1]
                        value = source.removeprefix("UINT32_C(").removesuffix(")")
                        assert f"STARS_HELP_{name} = {value};" in header
                    stamp = archive.read("complete.stamp").decode("ascii").strip()
                    assert f'STARS_HELP_STAMP[] = "{stamp}\\n";' in header
                else:
                    assert staged == archive.read(item), item.filename
        # Older guide versions can leave assets not named by the current stamp.
        (build / "help/stale.html").write_text("old generated asset", encoding="utf-8")
        subprocess.run([cmake, "--build", str(build), "--target", "clean"], check=True)
        assert not (build / "help").exists()
        subprocess.run([cmake, "--build", str(build), "--target", "clean"], check=True)
print("Help generation, recursive clean, repeated clean and regeneration passed")
