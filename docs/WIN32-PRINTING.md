# Default-printer DCs and PDF/CUPS output

`PrintDlgA` and `Escape` now implement the default-printer path used by Stars'
map-printing code. Tested basic coverage rises from **175/178 (98.3%) to
177/178 (99.4%)**. This percentage describes the inventory, not complete Win32
semantics or a linked SDL game client.

## Planned Linux user flow

The agreed follow-up is **Print Map → PDF → platform PDF viewer**. Finish and
verify the complete game-map PDF, then open it with the platform's default
viewer after successful document completion. The user handles physical printing
in that viewer. Cancellation must not launch a viewer; generation failures must
not open incomplete output, and viewer-launch failures must preserve the completed
PDF and tell the user where it is.

The PDF backend already exists; automatic viewer launch and full-map qualification
remain to be implemented. Existing optional CUPS submission is not the planned
user-facing workflow. Direct printer discovery, physical-printer qualification
and an in-app printer settings dialog are not completion requirements for this
flow. Basic inventory coverage is now 178/178 (100%).

## Setup and configuration

On Ubuntu/Pop!_OS, install the added build and verification dependencies with:

```sh
sudo apt install libcairo2-dev poppler-utils cups-client
```

All three packages are already installed on this laptop. Cairo >= 1.16 with its
PDF backend is linked only into the SDL compatibility runtime. Poppler's
`pdfinfo` and `pdftoppm` verify actual generated documents when tests are enabled.
The `lp` command from `cups-client` is needed only for system-printer submission;
PDF output works without a physical printer or CUPS server. Windows and Linux
host targets retain their existing dependencies.

The printer snapshots these environment settings when `PrintDlg` creates its DC:

| Variable | Values and behavior |
| --- | --- |
| `STARS_SDL_PRINTER` | Unset or `pdf`: the virtual PDF printer. `default`: CUPS default destination. Any other value: a CUPS queue name. |
| `STARS_SDL_PRINT_FILE` | Optional PDF destination. Without it, `STARTDOC` opens the existing Save picker. A configured path is replaced only after successful document completion. |
| `STARS_SDL_PRINT_PAPER` | `letter` (default) or `a4`. |
| `STARS_SDL_PRINT_ORIENTATION` | `portrait` (default) or `landscape`. |

These are process settings; no new save-file data is introduced. To list installed
system printers, use `lpstat -p -d`. Queue submission follows the documented
[CUPS command-line interface](https://www.cups.org/doc/options.html), including
media, orientation, title and copies arguments.

## Printer and page contracts

`PrintDlg(PD_RETURNDEFAULT | PD_RETURNDC)` returns a live printer DC plus owned,
movable `DEVMODEA` and `DEVNAMES` handles. The caller deletes the DC and frees both
handles. `PD_RETURNDEFAULT` alone returns the metadata without allocating a DC.
Existing input metadata is rejected for this default-printer call. Structure
and initialization errors use `CommDlgExtendedError`.

The virtual device is a 32-bit color raster printer at 300 DPI, with a quarter-inch
margin on each side. Letter printable resolution is 2400 by 3150 pixels; A4 and
landscape settings adjust physical and printable dimensions. `GetDeviceCaps`,
`GETPHYSPAGESIZE` and `GETPRINTINGOFFSET` report those printer dimensions. Pens,
brushes, fonts, clipping, coordinates, text and bitmap drawing use the existing
GDI raster paths. Printer canvases cannot be replaced by selecting a bitmap.

Supported escapes are `STARTDOC`, `NEWFRAME`, `ENDDOC`, `ABORTDOC`,
`QUERYESCSUPPORT`, `GETPHYSPAGESIZE` and `GETPRINTINGOFFSET`. Other escape requests
report unavailable rather than success. A repeated `STARTDOC` during an active
document reports an error and preserves the existing job. This is necessary
because Stars calls `STARTDOC` inside its map-page loop and ignores its result.

`NEWFRAME` emits the current page and clears its pixels while retaining GDI
selections/state. `ENDDOC` completes the document without adding an unwanted
blank page after the final explicit `NEWFRAME`. It emits an initial blank page
for an otherwise empty document, or a final marked page if the caller did not
explicitly end that page. `ABORTDOC`, deleting an active DC and runtime shutdown
remove incomplete staging files.

The public structures are checked against the SDK ABI. Default-printer metadata
follows Microsoft's [PRINTDLGA documentation](https://learn.microsoft.com/en-us/windows/win32/api/commdlg/ns-commdlg-printdlga).
The legacy document/page behavior is an adapter for the older calls used by Stars;
current Windows recommends newer printing calls, as described under
[Escape](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-escape).

## Output and failures

PDF output uses a private staging file in the destination directory. Only a
successful `ENDDOC`, including stream completion/close and rename, replaces the
destination. Aborting or deleting the DC leaves an existing destination intact.
Cairo owns its page-image pixels so later PDF finalization cannot access freed
SDL conversion buffers. The output contains raster pages; text is not searchable.
The PDF metadata retains the document title, converted from CP1252 when possible.

System printing first completes a private PDF, then runs `lp` with separate
arguments through SDL's process API. Queue names and filenames are not shell
commands. Submission is synchronous, with a 30-second timeout, and pumps SDL
events without dispatching caller callbacks during the process wait. A nonzero
exit or missing backend reports failure and logs the retained PDF path for
recovery. A zero exit proves spool-command acceptance, not delivery of physical
pages. Tests never submit to a real printer.

The PDF destination picker dispatches normal timers. Reentrant deletion or
another escape on its busy printer DC is rejected with `ERROR_BUSY`, preventing
a callback from freeing the context still used by `STARTDOC`. Runtime shutdown
belongs after outstanding compatibility calls return.

## Limits and remaining work

This milestone implements `PD_RETURNDEFAULT`, optionally with `PD_RETURNDC`,
which are the only flags in Stars' print call. Interactive printer selection,
input `DEVMODE` editing, page-range/collation/setup hooks, arbitrary paper sizes,
printer discovery/capability negotiation and printer information contexts remain
unsupported. The virtual driver supplies its own fixed margins and DPI; these
are not queried from hardware. Physical-printer and full game-map printing still
need qualification. Timeout handling and uncommon driver failures have less
coverage than PDF and ordinary process acceptance/rejection.

`WinHelp` is the last inventoried API without an implementation. Full-client
linking also needs the legacy `ExitWindowsEx` dependency and an entry point calling
`WinMain`. The full `stars-sdl` client remains unlinked. Finishing the API count
will not by itself complete client/runtime qualification.

## CLion and verification

Select `stars-compat-print` under `sdl-debug` or `sdl-release`. Its default
output directory is the selected CMake build's `print-test-output`, so no program
argument is needed in CLion. An optional argument selects another existing-parent
output directory. CTest supplies its directory explicitly.

```sh
cmake --preset sdl-debug
cmake --build --preset sdl-debug
ctest --test-dir dist/sdl-debug -R '^win32_print' --output-on-failure
```

The document-check fixture runs the C harness before Poppler verification,
including when only `win32_print_pdf` is selected. It checks page counts,
Letter/A4 sizes, staging cleanup, independently rendered red/blue rectangles,
printed text, and page clearing. The process test uses an executable fake `lp`
that accepts or rejects a real PDF and records argument boundaries. The harness
also covers metadata ownership, abort/deletion, destination browsing, invalid
calls, repeated `STARTDOC` and reentrant DC deletion.

The Debug and Release compatibility/smoke suites passed **54/54** tests each. AddressSanitizer,
UndefinedBehaviorSanitizer and leak detection passed the printer harness. The
CLion target was also run with no program argument, using its configured output
directory, and its final Release document fixture passed.

- MinGW Debug built; Windows units passed **49/49**, including the extended
  `DEVMODEA` SDK ABI checks.
- Linux host built and its units passed **12/12**.
- Native regression matched **519 checkpoints**, with 22 existing warnings and
  zero differences/errors or skipped inputs.
- The tutorial failed at year 2407, page 24, instruction 186, after 94 actions.
  The observer's `SendMessage` timed out while clicking the Messages-pane filter.
  The Wine log also contains `free(): invalid next size (fast)`.
  Artifacts remain in `dist/scaffold/tutorial/20261011T020241.076676Z`. This is
  an unresolved validation failure; no successful tutorial run is claimed for
  this batch. The Windows tutorial executable does not link this SDL runtime.

A fresh relocatable client link probe confirms that printing references resolve;
only `WinHelpA` and legacy `ExitWindowsEx` remain among the Win32 service
references. Game sources, serialized layouts and regression baselines were not
changed by this milestone.
