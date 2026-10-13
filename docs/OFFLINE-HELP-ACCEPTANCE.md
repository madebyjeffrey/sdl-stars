# Offline help acceptance

WinHelp now exports a self-contained HTML guide before handing its file URL to
the platform browser. All **414 original topics and 413 context mappings** are
retained, together with the original bitmap diagrams, eight converted WMF
diagrams, four embedded font faces, tables and definition popups. No content
server or network connection is needed.

## Why the export is one file

The former guide used relative HTML, image, CSS, JavaScript and font files.
That works beside the development executable, but a sandbox's
[OpenFile portal](https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.OpenURI.html)
hands off one descriptor, not its surrounding asset tree. It also does not
carry a URL fragment identifying the initial help topic.

The generated `offline.html` embeds every resource and topic. WinHelp verifies
its compiled size/CRC, embeds the requested initial topic and atomically writes
an export in SDL's per-user `stars/help` data directory. The file's name includes
the bundle CRC and topic identity. These are generated browser exports, not
saved-game data. Topic navigation uses hashes inside the loaded guide; popup
frames use embedded content. Browser Back, the topic-index filter and links
inside definitions remain local. JavaScript must be enabled for navigation.

The original archive CRC and complete-asset checks remain. Invalid contexts,
missing/damaged assets, unwritable export directories and launch failure return
failure. Existing explicit archive-path validation is preserved. The portable
launcher initializes SDL platform services even for the standalone help viewer.

## Automated browser acceptance

A separate headless Firefox profile opens a single copied HTML file in an
otherwise empty directory. It walks all 414 topics, waits for images/fonts,
checks vector diagrams contain visible pixels, clicks the Contents image's
hotspots, opens/closes a definition popup, filters the topic index and uses Back.
No HTTP server hosts the guide; resource checks reject HTTP/HTTPS dependencies.
The private OpenFile test receives a real descriptor and copies only that file,
then the browser verifies that its requested Research context survives the
renamed export. User browser profiles and desktop windows are untouched.

For local acceptance, install Mozilla's standalone geckodriver in your PATH,
or supply its absolute path. The test driver used here is 0.37.1 from
[Mozilla's releases](https://github.com/mozilla/geckodriver/releases).

```sh
cmake --preset sdl-debug -DSTARS_GECKODRIVER_EXECUTABLE=/absolute/path/geckodriver
cmake --build --preset sdl-debug
ctest --test-dir dist/sdl-debug -R '^win32_help' --output-on-failure -j 1
```

Browser acceptance is registered when CMake finds geckodriver; ordinary build
and archive/context tests still work without it. Firefox is only a development
test dependency. The installed product opens the user's chosen browser:

```sh
dist/install/bin/stars-compat-help-viewer --contents
flatpak run --command=stars-compat-help-viewer org.starsgame.Stars --contents
```

Relocated CMake and installed Flatpak Research-context exports each pass the
full 414-topic browser walk. The complete non-tutorial fatal sanitizer suite
passes 124/124; Windows units pass 55/55 and native regression remains 519
matches. These private-bus tests qualify OpenFile handoff, not a desktop
portal backend or user browser choice. Native macOS/other browser and real desktop/GPU acceptance remain
part of platform qualification, not an HTTP-hosting dependency of help.
