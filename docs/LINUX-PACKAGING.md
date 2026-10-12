# Linux product packaging

The two layouts are a relocatable CMake install and a local Flatpak bundle.
Neither command publishes or uploads a release. Desktop entries launch the
title screen; filename associations are deferred while the original command
parser cannot quote filenames containing spaces.

## CMake install

Use the dependencies in [SDL development](SDL3-DEVELOPMENT.md). Build and
install just the product, host and help viewer:

```sh
cmake --preset sdl-package
cmake --build --preset sdl-package --parallel 4
cmake --install dist/sdl-package --component StarsRuntime
dist/install/bin/stars-sdl
```

The `sdl-package` preset is also available in CLion. The normal Debug and
sanitizer presets retain the qualification targets. `StarsRuntime` installs
executables in `bin`, fetched SDL's shared library in `lib`, and the guide,
archive and third-party notices in `share/stars`. Fonts, bitmaps, dialog
templates and static game data are embedded in the client. The desktop entry,
AppStream metadata and original 32-pixel icon with a scalable wrapper use
`org.starsgame.Stars`.

Linux executables find fetched SDL through an executable-relative RPATH.
Help searches both the development layout and the executable-relative
installed data directory. Move the entire prefix, not individual executables.
An installed system SDL remains a system dependency instead of being copied.
Other libraries (Cairo, Fontconfig, FreeType, zlib and libdbus) remain system
dependencies; the CMake layout is not a universal static Linux binary.

## Flatpak

Install these as separate commands (`--user` belongs to Flatpak):

```sh
sudo apt install flatpak flatpak-builder
flatpak install --user flathub org.freedesktop.Platform//26.08 org.freedesktop.Sdk//26.08
python3 tools/build_flatpak.py
flatpak install --user dist/flatpak/stars.flatpak
flatpak run org.starsgame.Stars
```

The helper exports tracked working files, without `.git`, ignored games,
local settings or build trees. Stage new source files before exporting them.
Its reproducible archive normalizes timestamps/ownership, and explicit archive
hashes invalidate stale Flatpak source caches. It preserves the Git-derived product version through `STARS_VERSION`, creates
`dist/flatpak/org.starsgame.Stars.json` with its adjacent, SHA-256-pinned source
archive, then
builds a local repository and bundle. `--prepare-only` exports without building.
The checked-in template is `packaging/flatpak/org.starsgame.Stars.json`.

SDL 3.4.18, PyYAML 6.0.3 and libwmf 0.2.16 archives have pinned SHA-256 hashes.
PyYAML and WMF conversion are build dependencies and removed from the final
application. Runtime libraries come from the matching 26.08 Platform. This
uses the [standard Flatpak manifest and bundle workflow](https://docs.flatpak.org/en/latest/first-build.html).

Permissions enable Wayland, fallback X11, GPU and sound. Home access is needed
for Stars' companion game files: choosing a single `.m1` through the portal
does not itself grant access to adjacent `.xy`, `.hst` and `.x1` files. No
network permission is requested. File selection still uses the desktop portal.
Games outside home need an explicit user filesystem override or should be
moved into home.

## Qualification and limits

Desktop and AppStream metadata validate, including successful AppStream icon
composition. The local bundle is about 4.7 MB and installs against Platform
26.08. Relocated CMake and installed Flatpak launches each show an 800x600
title and exit successfully through the normal window-close request on private
X11. Build-only Python/WMF tools are absent from the runtime; the three product
executables, SDL, help assets/fonts and notices are present. The install check runs from a fresh
working directory after moving the install prefix, with private X11/Openbox:

```sh
xvfb-run -a python3 tests/compat/installed_client.py /absolute/install/bin/stars-sdl
```

It requires a mapped, usable title window and sends the normal window-close
request, requiring successful process exit. It does not touch desktop windows. Discovery tolerates X11 BadWindow races
when SDL or Openbox removes a startup window; other protocol errors remain
failures, and process cleanup runs on diagnostic exits.
Installed help-path tests verify complete assets and context selection without
opening a browser. Full offline browser acceptance remains the deferred help
milestone; Flatpak document-portal access to the guide's relative assets also
needs desktop acceptance.

The private patched Mesa qualification prefix is separate from this package.
Flatpak uses its runtime GPU drivers; those drivers do not inherit the local
Mesa patch. System drivers and desktop settings remain unchanged.
The repository has no overall product license declaration, so AppStream
truthfully uses `LicenseRef-Unknown`; local bundle validation does not resolve
redistribution rights or constitute a Flathub submission.
