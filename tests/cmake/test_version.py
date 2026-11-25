"""Keep the C++ version API and resource/compiler metadata synchronized."""
import argparse
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
CMAKE = "cmake"


class VersionGeneration(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="stars-version-unit-")
        self.addCleanup(self.temporary.cleanup)
        self.directory = Path(self.temporary.name)
        self.source = self.directory / "source"
        self.source.mkdir()
        self.header = self.directory / "version.h"
        self.resource = self.directory / "version_resource.h"

    def generate(self, version="", platform="windows-x64", relative=False):
        env = os.environ.copy()
        env.pop("STARS_VERSION", None)
        if version:
            env["STARS_VERSION"] = version
        output = "version.h" if relative else str(self.header)
        subprocess.run([CMAKE, f"-DSOURCE_DIR={self.source}", f"-DOUTPUT={output}",
                        "-DVERSION_BASE=2.9.0", f"-DPLATFORM={platform}",
                        "-P", str(ROOT / "cmake/version.cmake")],
                       cwd=self.directory, env=env, check=True, capture_output=True, text=True)
        cpp = self.header.read_text(encoding="utf-8")
        resource = self.resource.read_text(encoding="utf-8")
        self.assertNotRegex(cpp, r"#define STARS_VERSION_(?:MAJOR|MINOR|PATCH|BUILD|STRING|DISPLAY|COMMIT|PLATFORM)\b")
        values = dict(re.findall(r'^inline constexpr (?:int|char) STARS_VERSION_(\w+)(?:\[\])? = ([^\r\n]+);$', cpp, re.M))
        raw = dict(re.findall(r'^#define STARS_VERSION_(\w+)[ \t]+([^\r\n]+)$', resource, re.M))
        self.assertEqual(set(values), {"MAJOR", "MINOR", "PATCH", "BUILD", "STRING", "DISPLAY", "COMMIT", "PLATFORM"})
        self.assertEqual(set(raw), {"MAJOR", "MINOR", "PATCH", "BUILD", "STRING", "DISPLAY"})
        self.assertEqual(raw, {key: values[key] for key in raw})
        self.assertNotIn("constexpr", resource)
        return values

    def test_source_archive(self):
        values = self.generate()
        self.assertEqual([values[k] for k in ("MAJOR", "MINOR", "PATCH", "BUILD")], ["2", "9", "0", "0"])
        self.assertEqual(values["STRING"], '"2.9.0-dev.0+gunknown"')
        self.assertEqual(values["DISPLAY"], '"2.9x64 (2.9.0-dev.0+gunknown)"')
        self.assertEqual(values["COMMIT"], '"unknown"')
        self.assertEqual(values["PLATFORM"], '"windows-x64"')

    def test_release_platform_and_patch_labels(self):
        for version, platform, display in [
                ("2.8.0", "windows-x64", "2.8x64"),
                ("2.8.1", "macos-arm64", "2.8.1 macos-arm64"),
                ("2.9.0", "linux-x64", "2.9 linux-x64")]:
            with self.subTest(version=version, platform=platform):
                values = self.generate(version, platform)
                self.assertEqual(values["STRING"], '"' + version + '"')
                self.assertEqual(values["DISPLAY"], '"' + display + '"')
                self.assertEqual(values["PATCH"], version.split(".")[2])
                self.assertEqual(values["BUILD"], "0")

    def test_development_override(self):
        text = "2.9.0-dev.41+g1234567.dirty"
        values = self.generate(text, "linux-arm64")
        self.assertEqual(values["STRING"], '"' + text + '"')
        self.assertEqual(values["DISPLAY"], '"2.9 linux-arm64 (' + text + ')"')
        self.assertEqual(values["BUILD"], "0")

    def test_relative_output_keeps_both_headers_adjacent(self):
        values = self.generate("2.8.0", relative=True)
        self.assertEqual(values["DISPLAY"], '"2.8x64"')

    def test_headers_do_not_rewrite_and_missing_resource_is_repaired(self):
        self.generate("2.8.0")
        previous = {path: (path.read_bytes(), path.stat().st_mtime_ns) for path in (self.header, self.resource)}
        self.generate("2.8.0")
        self.assertEqual(previous, {path: (path.read_bytes(), path.stat().st_mtime_ns) for path in previous})
        self.resource.unlink()
        self.generate("2.8.0")
        self.assertEqual(self.resource.read_bytes(), previous[self.resource][0])
        self.assertEqual((self.header.read_bytes(), self.header.stat().st_mtime_ns), previous[self.header])

    def test_resource_header_is_private(self):
        for directory in ("src", "compat/win32"):
            for path in (ROOT / directory).rglob("*"):
                if path.suffix in (".cpp", ".mm", ".h", ".inc"):
                    self.assertNotRegex(path.read_text(encoding="utf-8"), r'#\s*include\s*[<"]version_resource\.h[>"]', str(path))
        self.assertIn('#include "version_resource.h"', (ROOT / "res/stars.rc").read_text(encoding="utf-8"))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("--cmake", default="cmake")
    args, rest = parser.parse_known_args()
    CMAKE = args.cmake
    unittest.main(argv=[sys.argv[0], *rest])
