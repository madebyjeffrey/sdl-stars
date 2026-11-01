# Versioning

Stars! has two version numbers, and only one of them changes on `main`.

## Product version

The version of the executable, stored in its `VERSIONINFO` resource and
shown to players. `cmake/version.cmake` generates it from git into
`<build>/generated/version.h` on every build. This application header exposes
inline C++ constants, with fixed readonly character arrays at native string
boundaries. The same generation writes `version_resource.h` for the Windows
resource compiler; its six preprocessing values stay private to `stars.rc`.
The two headers carry identical version components and display bytes, and
neither is rewritten when its contents are unchanged. Packaging and release
readers consume the C++ declarations.

The generated values follow the checkout:

| Checkout | Version | Shown as | Build |
| --- | --- | --- | --- |
| Clean checkout of tag `v2.8.0` | `2.8.0` | `2.8x64` | 0 |
| Clean checkout of tag `v2.8.1` | `2.8.1` | `2.8.1x64` | 0 |
| 12 commits after `v2.8.0` | `2.8.1-dev.12+g1a2b3c4` | `2.8x64 (2.8.1-dev.12+g1a2b3c4)` | 12 |
| Same, with uncommitted changes | `2.8.1-dev.12+g1a2b3c4.dirty` | `2.8x64 (2.8.1-dev.12+g1a2b3c4.dirty)` | 12 |
| Before the first `v` tag | `2.8.0-dev.N+g…`, N counted from `2.6jrc3` | `2.8x64 (2.8.0-dev.N+g…)` | N |

The version (`STARS_VERSION_STRING`) is what tags, `FileVersion` and the
release check use; it never names a platform (a semver `-` suffix would
make it a pre-release). The name shown (`STARS_VERSION_DISPLAY`) adds the
platform: the Windows build keeps the `x64` players know, `2.8x64` for
2.8.0 and `2.8.1x64` for 2.8.1, and other builds of `stars-host` name
theirs after the version: `2.9 linux-x64`, `2.9 macos-x64`,
`2.9 macos-arm64`. CMake passes the platform (`STARS_VERSION_PLATFORM`,
`os-arch`) to `cmake/version.cmake`. Development builds append the full
version in parentheses. It appears in the About box, on the splash screen
and in report headers (`SzVersion` in `util.cpp`), in `ProductVersion`, in
release titles and in `stars-host --version`.

A development build is numbered toward the next version: the patch after the
last `vX.Y.Z` tag, or `project(stars VERSION …)` in `CMakeLists.txt` if that
is higher. To start a new minor version (2.9.0), raise the `project()`
version; patch releases need no version commit. `VERSIONINFO` carries
`MAJOR,MINOR,PATCH,BUILD`.

Source archives without git history build as `X.Y.Z-dev.0+gunknown`; set
`STARS_VERSION=2.8.0` in the environment to stamp them.

Patch releases of 2.8 are tagged on the `2.8` branch (`v2.8.1`), whose
builds count toward 2.8.1. `main` has `project()` at 2.9.0, so its builds
read `2.9.0-dev.N+g…`, counted from `v2.8.0`.

## Releasing

```sh
git tag v2.8.0
git push origin v2.8.0
```

The release workflow builds the tag, checks that it produced exactly
`2.8.0`, and publishes "Stars! 2.8x64" with `stars.exe` and `stars!.hlp`.
Every push to `main` updates the rolling `latest` prerelease, whose title
shows the name of the build. Other tags (such as `2.6jrc3` on the `2.6j`
branch) still publish releases named after the tag.

## File format version

Saves, turn files and race files carry a version in `RTBOF`
(`verMajor`/`verMinor`/`verInc`), separate from the product version. Main writes
and accepts only 2.85, which identifies UTF-8 text and UTF-8 password bytes.
Record sizes and text field widths remain unchanged; text limits are byte
budgets. Older files are rejected, without encoding detection or migration.

The 2.8 branch writes 2.84 and reads 2.49 through 2.84. The original wrote 2.83
and refused 2.84 or later. Preserve those historical contracts on their branches.
