"""Signing selection and transactional bundle assembly; no keychain access."""
import importlib.util
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("macos_bundle", ROOT / "packaging/macos/bundle.py")
bundle = importlib.util.module_from_spec(spec)
spec.loader.exec_module(bundle)
DEV = ("A" * 40, "Apple Development: Test (TEAM)")
DISTRIBUTION = ("B" * 40, "Developer ID Application: Test (TEAM)")
HEADER = '''#ifndef STARS_VERSION_H
#define STARS_VERSION_H

#define STARS_VERSION_MAJOR 2
#define STARS_VERSION_MINOR 9
#define STARS_VERSION_PATCH 0
#define STARS_VERSION_BUILD 41
#define STARS_VERSION_STRING "2.9.0-dev.41+g1234567.dirty"
#endif
'''


class SigningSelection(unittest.TestCase):
    def test_valid_identity_output(self):
        text = f'  1) {DEV[0]} "{DEV[1]}"\n  2) {DEV[0]} "{DEV[1]}"\n'
        text += f'  3) {"C" * 40} "Expired" (CSSMERR_TP_CERT_EXPIRED)\n'
        self.assertEqual(bundle.identities(text), [DEV])

    def test_distribution_preferred_over_development(self):
        self.assertEqual(bundle.choose_identity([DEV, DISTRIBUTION], "AUTO"), DISTRIBUTION)

    def test_development_fallback(self):
        self.assertEqual(bundle.choose_identity([DEV], "AUTO"), DEV)

    def test_ambiguous_preferred_identities_fail(self):
        other = ("C" * 40, "Developer ID Application: Other (OTHER)")
        with self.assertRaisesRegex(bundle.BundleError, "More than one"):
            bundle.choose_identity([DISTRIBUTION, other, DEV], "AUTO")

    def test_no_silent_adhoc_fallback(self):
        with self.assertRaisesRegex(bundle.BundleError, "No eligible"):
            bundle.choose_identity([], "AUTO")

    def test_explicit_name_and_hash(self):
        self.assertEqual(bundle.choose_identity([DEV, DISTRIBUTION], DEV[1]), DEV)
        self.assertEqual(bundle.choose_identity([DEV], DEV[0].lower()), DEV)

    def test_adhoc_is_explicit(self):
        self.assertEqual(bundle.choose_identity([], "ADHOC")[0], "-")

    def test_generated_version_with_include_guard(self):
        info = bundle.version_info(HEADER)
        self.assertEqual(info["CFBundleShortVersionString"], "2.9.0")
        self.assertEqual(info["CFBundleVersion"], "41")
        self.assertEqual(info["StarsBuildVersion"], "2.9.0-dev.41+g1234567.dirty")

    def test_invalid_version_fails_clearly(self):
        with self.assertRaises(bundle.BundleError):
            bundle.version_info("#define STARS_VERSION_H\n")


class BundleAssembly(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="stars-bundle-unit-")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        source = self.root / "source"
        for name in ("res/icons/stars-app.icns", "res/fonts/LICENSE",
                     "res/fonts/ORIGIN.json", "compat/win32/LICENSE", "compat/win32/LICENSE.free-api",
                     "compat/win32/LICENSE.zingl"):
            path = source / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text("fixture")
        executable = self.root / "stars-sdl"
        executable.write_bytes(b"fixture")
        help_directory = self.root / "help"
        help_directory.mkdir()
        (help_directory / "index.html").write_text("offline help")
        (help_directory / "complete.stamp").write_text("integrity stamp")
        (help_directory / "help_contexts.h").write_text("build header")
        version = self.root / "version.h"
        version.write_text(HEADER)
        self.args = SimpleNamespace(source=source, executable=executable, help_directory=help_directory,
                                    version_header=version, output=self.root / "Stars!.app",
                                    identity="AUTO", cmake="cmake", library_directory=[])
        self.args.output.mkdir()
        (self.args.output / "previous").write_text("successful package")

    def test_failure_preserves_previous_package(self):
        with patch.object(bundle, "run", return_value=f'1) {DEV[0]} "{DEV[1]}"\n'), \
             patch.object(bundle, "clean_and_validate", return_value="14.0"), \
             patch.object(bundle, "sign", side_effect=bundle.BundleError("sign failed")):
            with self.assertRaisesRegex(bundle.BundleError, "sign failed"):
                bundle.package(self.args)
        self.assertEqual((self.args.output / "previous").read_text(), "successful package")
        self.assertEqual(list(self.root.glob(".stars-bundle-*")), [])

    def test_success_contains_resources_and_selected_identity(self):
        with patch.object(bundle, "run", return_value=f'1) {DEV[0]} "{DEV[1]}"\n'), \
             patch.object(bundle, "clean_and_validate", return_value="14.0"), \
             patch.object(bundle, "sign") as sign:
            bundle.package(self.args)
        self.assertEqual(sign.call_args.args[2:], DEV)
        self.assertTrue((self.args.output / "Contents/Resources/stars-app.icns").is_file())
        self.assertTrue((self.args.output / "Contents/Resources/help/index.html").is_file())
        self.assertTrue((self.args.output / "Contents/Resources/help/complete.stamp").is_file())
        self.assertFalse((self.args.output / "Contents/Resources/help/help_contexts.h").exists())
        self.assertFalse((self.args.output / "previous").exists())

    def test_nested_signing_order_and_verification(self):
        app = self.args.output
        framework = app / "Contents/Frameworks/SDL3.framework"
        framework.mkdir(parents=True)
        leaf = framework / "Versions/A/SDL3"
        main = app / "Contents/MacOS/stars-sdl"
        with patch.object(bundle, "run") as run:
            bundle.sign(app, [leaf, main], *DISTRIBUTION)
        commands = [call.args for call in run.call_args_list]
        self.assertEqual([command[-1] for command in commands],
                         [str(leaf), str(framework), str(app), str(app)])
        self.assertTrue(all("--timestamp" in command and "--deep" not in command for command in commands[:-1]))
        self.assertIn("--deep", commands[-1])
        self.assertIn("--strict", commands[-1])

    def test_development_signing_has_no_timestamp_service(self):
        with patch.object(bundle, "run") as run:
            bundle.sign(self.args.output, [], *DEV)
        self.assertIn("--timestamp=none", run.call_args_list[0].args)

    def test_external_dependency_is_rejected(self):
        with patch.object(bundle, "run", side_effect=["arm64", "", "main:\n /opt/homebrew/lib/extra.dylib (compatibility version 1.0.0)"]):
            with self.assertRaisesRegex(bundle.BundleError, "Nonrelocatable"):
                bundle.clean_and_validate(self.args.output, [self.args.executable])

    def test_dependency_escape_is_rejected(self):
        main = self.args.output / "Contents/MacOS/stars-sdl"
        outside = self.root / "outside.dylib"
        outside.write_text("outside")
        with patch.object(bundle, "run", side_effect=["arm64", "", "main:\n @loader_path/../../../outside.dylib (compatibility version 1.0.0)"]):
            with self.assertRaisesRegex(bundle.BundleError, "escapes"):
                bundle.clean_and_validate(self.args.output, [main])

    def test_minimum_os_from_build_command(self):
        loads = "cmd LC_BUILD_VERSION\n cmdsize 32\n platform 1\n minos 15.0\n sdk 26.0\n"
        with patch.object(bundle, "run", side_effect=["arm64", loads, "main:\n /usr/lib/libSystem.B.dylib (compatibility version 1.0.0)"]):
            self.assertEqual(bundle.clean_and_validate(self.args.output, [self.args.executable]), "15.0")

    def test_non_arm_dependency_is_rejected(self):
        with patch.object(bundle, "run", return_value="x86_64"):
            with self.assertRaisesRegex(bundle.BundleError, "no arm64"):
                bundle.clean_and_validate(self.args.output, [self.args.executable])


if __name__ == "__main__":
    unittest.main()
