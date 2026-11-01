#!/usr/bin/env python3
"""Package a relocatable Apple-silicon app and discover its signing identity."""
import argparse
import os
from pathlib import Path
import plistlib
import re
import shutil
import subprocess
import sys
import tempfile


class BundleError(RuntimeError):
    pass


def run(*args, capture=False):
    return subprocess.run(args, check=True, text=True,
                          stdout=subprocess.PIPE if capture else None).stdout


def identities(text):
    # find-identity -v lists usable certificate/private-key pairs only.
    return list(dict.fromkeys(re.findall(r'^\s*\d+\)\s+([0-9A-Fa-f]{40})\s+"(.+)"\s*$', text, re.M)))


def choose_identity(found, requested):
    if requested == "ADHOC":
        return "-", "ad-hoc (local testing only)"
    if requested != "AUTO":
        matches = [(digest, name) for digest, name in found
                   if digest.lower() == requested.lower() or name == requested]
    else:
        matches = []
        for prefix in ("Developer ID Application:", "Apple Development:", "Mac Developer:"):
            matches = [(digest, name) for digest, name in found if name.startswith(prefix)]
            if matches:
                break
    if len(matches) == 1:
        return matches[0]
    if not matches:
        raise BundleError("No eligible signing identity found. Check your unlocked keychain, "
                          "or select ADHOC explicitly for a local test bundle.")
    labels = "\n".join(f"  {digest}  {name}" for digest, name in matches)
    raise BundleError("More than one signing identity matches; select one with "
                      "STARS_MACOS_SIGN_IDENTITY:\n" + labels)


def version_info(header):
    values = dict(re.findall(r'^inline constexpr (?:int|char) STARS_VERSION_(\w+)(?:\[\])? = ([^\r\n]+);$', header, re.M))
    try:
        major, minor, patch, build = [int(values[name]) for name in ("MAJOR", "MINOR", "PATCH", "BUILD")]
        text = values["STRING"].strip('"')
    except (KeyError, ValueError) as error:
        raise BundleError("Invalid generated version header") from error
    return {
        "CFBundleShortVersionString": f"{major}.{minor}.{patch}",
        "CFBundleVersion": str(max(1, build)),
        "StarsBuildVersion": text,
    }


def is_macho(path):
    if not path.is_file() or path.is_symlink():
        return False
    with path.open("rb") as stream:
        magic = stream.read(4)
    return magic in (b"\xfe\xed\xfa\xce", b"\xce\xfa\xed\xfe", b"\xfe\xed\xfa\xcf",
                     b"\xcf\xfa\xed\xfe", b"\xca\xfe\xba\xbe", b"\xbe\xba\xfe\xca",
                     b"\xca\xfe\xba\xbf", b"\xbf\xba\xfe\xca")


def macho_files(app):
    return sorted(path for path in app.rglob("*") if is_macho(path))


def dependency_names(text):
    return [line.strip().split(" (compatibility version", 1)[0]
            for line in text.splitlines()[1:] if line.strip()]


def system_dependency(name):
    return name.startswith(("/System/Library/", "/usr/lib/"))


def clean_and_validate(app, files):
    minimum = (11, 0)
    for path in files:
        arches = run("/usr/bin/lipo", "-archs", str(path), capture=True).split()
        if "arm64" not in arches:
            raise BundleError(f"Dependency has no arm64 slice: {path}")
        if len(arches) > 1:
            thin = path.with_name(path.name + ".arm64")
            run("/usr/bin/lipo", str(path), "-thin", "arm64", "-output", str(thin))
            os.chmod(thin, path.stat().st_mode)
            thin.replace(path)
        loads = run("/usr/bin/otool", "-l", str(path), capture=True)
        # Remove build-machine search paths after all dependencies are embedded.
        for rpath in re.findall(r'cmd LC_RPATH\n.*?\n\s*path (.*?) \(offset', loads):
            if rpath.startswith("/") and not system_dependency(rpath):
                run("/usr/bin/install_name_tool", "-delete_rpath", rpath, str(path))
        versions = re.findall(r'cmd LC_BUILD_VERSION\n\s*cmdsize \d+\n\s*platform \S+\n\s*minos ([0-9.]+)', loads)
        versions += re.findall(r'cmd LC_VERSION_MIN_MACOSX\n.*?\n\s*version ([0-9.]+)', loads)
        for version in versions:
            minimum = max(minimum, tuple(int(part) for part in version.split(".")))
        for name in dependency_names(run("/usr/bin/otool", "-L", str(path), capture=True)):
            if system_dependency(name):
                continue
            if name.startswith("@executable_path/"):
                resolved = app / "Contents/MacOS" / name[len("@executable_path/"):]
            elif name.startswith("@loader_path/"):
                resolved = path.parent / name[len("@loader_path/"):]
            else:
                raise BundleError(f"Nonrelocatable dependency in {path}: {name}")
            if not resolved.exists() or not resolved.resolve().is_relative_to(app.resolve()):
                raise BundleError(f"Dependency escapes or is missing from the app: {name}")
    return ".".join(str(part) for part in minimum)


def sign(app, files, identity, label):
    timestamp = "--timestamp" if label.startswith("Developer ID Application:") else "--timestamp=none"
    command = ("/usr/bin/codesign", "--force", "--sign", identity, "--options", "runtime", timestamp)
    main = app / "Contents/MacOS/stars-sdl"
    # Sign nested code before its containing framework/app; never use --deep
    # to sign because it can hide incorrectly packaged nested code.
    for path in files:
        if path != main:
            run(*command, str(path))
    frameworks = sorted(app.rglob("*.framework"), key=lambda path: len(path.parts), reverse=True)
    for framework in frameworks:
        if not framework.is_symlink():
            run(*command, str(framework))
    run(*command, str(app))
    run("/usr/bin/codesign", "--verify", "--deep", "--strict", "--verbose=2", str(app))


def package(args):
    found = [] if args.identity == "ADHOC" else identities(run(
        "/usr/bin/security", "find-identity", "-v", "-p", "codesigning", capture=True))
    identity, label = choose_identity(found, args.identity)
    print(f"Signing identity: {label}", flush=True)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=".stars-bundle-", dir=args.output.parent) as temporary:
        app = Path(temporary) / args.output.name
        resources = app / "Contents/Resources"
        executable = app / "Contents/MacOS/stars-sdl"
        executable.parent.mkdir(parents=True)
        resources.mkdir()
        shutil.copy2(args.executable, executable)
        shutil.copy2(args.source / "res/icons/stars-app.icns", resources)
        shutil.copytree(args.help_directory, resources / "help", ignore=shutil.ignore_patterns("*.h"))
        licenses = resources / "licenses"
        licenses.mkdir()
        for name in ("LICENSE", "LICENSE.free-api", "LICENSE.zingl"):
            shutil.copy2(args.source / "compat/win32" / name, licenses / name)
        shutil.copytree(args.source / "res/fonts", licenses / "fonts", ignore=shutil.ignore_patterns("*.ttf"))
        info = {"CFBundleName": "Stars!", "CFBundleDisplayName": "Stars!",
                "CFBundleIdentifier": "org.starsgame.Stars", "CFBundleExecutable": "stars-sdl",
                "CFBundlePackageType": "APPL", "CFBundleIconFile": "stars-app.icns",
                "NSHighResolutionCapable": True}
        info.update(version_info(args.version_header.read_text()))
        plist = app / "Contents/Info.plist"
        plist.write_bytes(plistlib.dumps(info))
        run(args.cmake, f"-DSTARS_APP={app}",
            "-DSTARS_LIBRARY_DIRS=" + ";".join(str(path) for path in args.library_directory),
            "-P", str(args.source / "cmake/macos-fixup.cmake"))
        files = macho_files(app)
        info["LSMinimumSystemVersion"] = clean_and_validate(app, files)
        plist.write_bytes(plistlib.dumps(info))
        sign(app, files, identity, label)
        # Keep the previous successful package if copying, fixup or signing fails.
        previous = Path(temporary) / "previous.app"
        if args.output.exists():
            args.output.rename(previous)
        try:
            app.rename(args.output)
        except OSError:
            if previous.exists():
                previous.rename(args.output)
            raise
    print(f"Verified signed bundle: {args.output}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("executable", "help-directory", "version-header", "source", "output"):
        parser.add_argument("--" + name, required=True, type=Path)
    parser.add_argument("--identity", default="AUTO")
    parser.add_argument("--cmake", default="cmake")
    parser.add_argument("--library-directory", action="append", default=[], type=Path)
    args = parser.parse_args()
    if sys.platform != "darwin":
        parser.error("macOS packaging requires macOS")
    try:
        package(args)
    except (BundleError, OSError, subprocess.CalledProcessError) as error:
        parser.exit(1, f"Bundle failed: {error}\n")


if __name__ == "__main__":
    main()
