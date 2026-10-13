# Print Map to PDF

The SDL client now completes **Print Map -> PDF -> platform PDF viewer**.
Choose the map's page grid, select a PDF destination in the native Save picker,
then print from your usual viewer. The existing Windows build retains its
standard default-printer path; Windows and Linux host builds do not acquire SDL,
Cairo or browser dependencies. Basic API-body coverage remains 178/178 (100%).

## Document lifecycle

`CommandHandler` previously called `STARTDOC` inside every map-page iteration
and ignored all document/page errors. It now starts one document, checks each
`NEWFRAME` and `ENDDOC`, stops on failure and releases the fonts, DC and printer
metadata on every exit. Cancelling the map dialog or PDF destination is quiet
and does not launch a viewer. Save-picker errors are distinguished from cancel.

The Cairo backend stages output beside the destination. A completed document
must successfully finish, flush, close and rename before its file URL is passed
to `SDL_OpenURL`. Aborts and generation failures preserve an existing target.
A viewer rejection preserves the completed PDF and shows its absolute UTF-8
path in a native error message, also recording it in the application log.
The launcher accepts a platform handoff; the user chooses the viewer's physical
printing options. No actual printer submission is exercised by these tests.

## Configuration

```sh
sudo apt install libcairo2-dev poppler-utils
```

These dependencies are installed on this laptop. PDF output needs neither a
physical printer nor CUPS. The default is a 300 DPI raster document with
quarter-inch margins; printed text is not searchable.

| Variable | Meaning |
| --- | --- |
| `STARS_SDL_PRINT_FILE` | Optional destination; otherwise the native Save picker asks for it. Spaces, Unicode and URI punctuation are supported. |
| `STARS_SDL_PRINT_PAPER` | `letter` (default) or `a4`. |
| `STARS_SDL_PRINT_ORIENTATION` | `portrait` (default) or `landscape`. |
| `STARS_SDL_PRINTER` | Unset or `pdf` selects this user flow. Existing opt-in `default`/queue-name CUPS behavior remains for compatibility, outside the product printing requirement. |

Map rendering retains the original planet positions, symbols, labels and
legends. Dense clusters can have overlapping labels in the original centered
layout; a larger tiled map improves readability. No turn-generation or file
format behavior changes.

## Qualification

The actual Print Map dialog and command run against a copied year-2410 game:
Letter, four-page tiled Letter and landscape A4 documents have their expected
page sizes/counts. Poppler independently locates all 128 planet marks in every
layout. The game state is unchanged; cancellation launches nothing. The map
preview is rendered for visual review.

Printer tests also cover default metadata ownership/SDK ABI, marked/blank page
completion, page clearing, abort/DC deletion, atomic destination replacement,
repeated-start rejection, reentrant DC deletion, save-picker cancellation,
encoded UTF-8 file URLs and viewer rejection with a retained document. Viewer
and error-dialog calls are intercepted during tests, so no desktop viewer opens.

The relocated CMake install and installed Flatpak also load an unprotected test
game, open File/Print Map with ordinary keyboard input, complete a Letter PDF
and close normally. A private OpenFile service receives exactly the saved PDF
bytes, which Poppler renders for visual review. These checks run sequentially
on private X11 displays; they never submit to a physical printer.

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug
ctest --test-dir dist/sdl-debug -R '^win32_print' --output-on-failure -j 1
```

For installed-product acceptance, prepare a disposable unprotected game using
an optional Linux host argument. The frozen regression race has a password, so
the harness writes a separate predefined race with the existing serializer;
the host creates a new game without modifying frozen fixtures:

```sh
python3 tests/compat/check_print_map.py dist/sdl-debug/bin/stars-compat-printmap dist/sdl-debug/print-map-test --host dist/host-debug/bin/stars-host
STARS_TEST_PRIVATE_BUS=1 dbus-run-session -- xvfb-run -a /usr/bin/python3 tests/compat/run_document_portal.py --output /tmp/stars-accepted-map.pdf /usr/bin/python3 "$PWD/tests/compat/installed_client.py" --game "$PWD/dist/sdl-debug/print-map-test/installed-game/game.m1" --print-map /tmp/stars-saved-map.pdf "$PWD/dist/install/bin/stars-sdl"
cmp /tmp/stars-accepted-map.pdf /tmp/stars-saved-map.pdf
```

Select `stars-compat-print` or `stars-compat-printmap` in CLion. The CTest map
case supplies copied files and output paths; the standalone map harness expects
that same setup. The product `stars-sdl` target uses its normal Print Map command.

Generic Win32 printer setup hooks, arbitrary paper formats, printer discovery,
physical-printer capability negotiation and direct hardware acceptance remain
outside this flow. The existing optional CUPS path uses the documented
[CUPS CLI](https://www.cups.org/doc/options.html); it is not the user-facing path.
