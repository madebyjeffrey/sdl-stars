# Bevy starting-screen experiment

This experiment is suspended and retained for reference. The four actions
reach the existing game functionality, but rendering after modal dialogs is
unresolved: the title/version can disappear, and some attempts lost the whole
screen. Recreating the Bevy renderer and macOS Metal view did not resolve it.
The intended next direction is to recreate portions of the game in Rust,
using existing C functionality through library interfaces.

The optional `STARS_BEVY_STARTUP` build setting links a Rust static library
using Bevy 0.20.0 into the existing `stars-sdl` executable. It is off by
default. Rust 1.97.1 or newer and Cargo are required; the first build downloads
the dependencies pinned in `src/ui/bevy_start/Cargo.lock`.

On Apple silicon macOS:

```sh
cmake --preset macos-sdl-debug -B dist/macos-bevy-debug -DSTARS_BEVY_STARTUP=ON
cmake --build dist/macos-bevy-debug --target stars-sdl
dist/macos-bevy-debug/bin/stars-sdl
```

For Linux use `sdl-debug`, and for native MSYS2 UCRT64 Windows use
`windows-sdl-debug` with the Rust `x86_64-pc-windows-gnu` target installed.
Cross-compilation and macOS universal binaries are outside this experiment.

## Scope and ownership

SDL creates the starting window and pumps its events on the main thread.
Bevy renders through its native window handle, with no Winit event loop.
The Rust library owns layout, text, hover/pressed appearance and selection.
The C bridge calls `TitleWndProc` with the same four action IDs as the original
buttons. The existing title stays hidden; the original dialogs open over a
paused Bevy backdrop. After cancellation, the bridge recreates the renderer
and resumes input, subject to the rendering defect above.
The Bevy renderer is dropped before SDL destroys its borrowed window.

New Game opens the existing wizard. Open Game opens the existing picker.
Continue Game retains the existing availability check and opens the recent
game. Exit and closing the starting window use the existing exit handler.
Cancelling a wizard or picker returns to Bevy. Tab/Shift-Tab change focus;
Enter/Space activate it. N/O/C/X select the corresponding action, and Escape
exits. Disabled Continue is skipped by keyboard navigation.

Only the initial interactive starting screen uses Bevy. Direct command-line
game launches, headless runs, the host, and subsequent return-to-title paths
retain their existing behavior. An unsupported native window backend or a
failed Bevy initialization falls back to the SDL title screen.

The Rust crate uses a small selection of Bevy features without audio, 3D or
Winit and embeds the existing Liberation Sans font. The Rust library currently
uses Cargo's development profile, including in CMake release configurations;
its static library adds substantial build time and executable size (about
390 MB for the macOS Debug executable). No game state or save-file layout crosses this UI interface, and turn
generation requires no new baseline.

## Qualification

These results record checks during the experiment. The full regressions and
unit tests preceded the final renderer-recreation attempt; the final build
compiled successfully, but its live cancellation check still lost the headings.

- Apple silicon macOS SDL Debug build and Rust formatting/compilation pass.
  The touched files introduce no compiler warnings; existing packed UI table
  linker warnings remain.
- Live desktop checks cover New Game creation, wizard cancellation, native
  picker cancellation, Open Game loading, Continue Game loading, Exit,
  disabled Continue and keyboard navigation. The old title remains hidden
  while the wizard/picker is open, and cancelled actions return to Bevy.
- The 17 SDL unit tests pass.
- The full native regression, both with the ordinary client and with Bevy
  linked in, matches all 519 comparisons through turn 150. The ordinary build
  reports 67 warning-only matches; the Bevy Debug build reports 69. There are
  no differences/errors or skipped reference inputs. Baselines are unchanged.
- `make tutorial` finds no registered tutorial tests on the macOS preset;
  this is not a tutorial pass.

Windows and Linux graphical execution still require platform qualification;
their native handle bridges are included, but macOS is the development target
for this experiment. The development app bundle used for desktop checks is in
`dist/macos-bevy-debug/Stars Bevy Experiment.app` and uses private settings in
`dist/bevy-manual/config` so it can be tried separately from the normal client.
