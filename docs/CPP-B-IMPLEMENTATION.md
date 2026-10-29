# Milestone B implementation ledger

Status: implementation active. Tracking: [#65](https://github.com/madebyjeffrey/sdl-stars/issues/65); implementation [PR #66](https://github.com/madebyjeffrey/sdl-stars/pull/66).
Base: `develop` at `417e526`, after milestone A merged in #64.

The complete [milestone B plan](CPP-MIGRATION.md#milestone-b-ownership-records-and-standard-c)
is delivered in one PR, as requested. Each checkpoint keeps focused commits and
its own verification evidence. An active or pending checkpoint is not completion.

| Checkpoint | Status | Evidence / remaining work |
| --- | --- | --- |
| B01: desktop resource owners | Complete | Typed SDL/Cairo/FreeType, image/font/menu/audio/window/DC/geometry/class owners. Local full suite, native leaks, ASan/UBSan and regression pass; all 16 Linux/Windows checks pass for ff617cb. |
| B02: dispatch and callback guards | Active | Preserve cleanup-before-delivery and foreign callback containment. |
| B03: const-correct text and scoped views | Pending | Preserve UTF-8 byte budgets, NUL contracts and borrowed-buffer lifetimes. |
| B04: record codecs and complete storage inventory | Pending | Use S01's existing record/bit contracts and independent vectors; account for every raw file/resource access. |
| B05: production payloads | Pending | Separate queue storage and serialized widths without changing heap lifetime or forecast results. |
| B06: waypoint/order payloads | Pending | Preserve order edits/replay, transport presets and growth/failure receipts. |
| B07: hull/design payloads | Pending | Preserve slot strides, catalog IDs and design import/export. |
| B08: fleet/identifier payloads | Pending | Preserve split/merge order, iteration and zero-prefix wire lengths. |
| B09: battle/AI payloads | Pending | Preserve battle alignment, RNG/tie/rounding order and replay. |
| B10: message/log payloads | Pending | Preserve compression/encryption/framing and incomplete-record behavior. |
| B11: remaining packed/resource layouts | Pending | Explicit codecs replace compiler packing and anonymous aliases after all direct consumers migrate. |
| B12: typed stable game arenas | Pending | Preserve addresses, stride-derived identity, reset/invalidation rules, capacities and allocation failure. |
| B13: application/session owner | Pending | One owner; stop callbacks/timers before destroying referenced game/UI state. Update the archived observer when needed. |
| B14: strict C++20 gate | Pending | Disable owned production extensions and require strict Linux/macOS/Windows diagnostics plus the full contract matrix. |

## Ownership changes in progress

`src/platform/ownership.h` supplies move-only owners using the actual release
routine. A view or borrowed selection never enters an owner. `release()` marks
transfer into persistent ownership; cleanup destructors do not throw or call
application callbacks.

| Storage | Owner / transfer | Borrowing and invalidation |
| --- | --- | --- |
| Temporary Fontconfig patterns/character sets | Scoped owners in `FaceFallback` and `LpNewFontRaster` | Borrowed matched filename bytes are used before releasing the matching pattern. |
| Font construction and uncached glyph raster | Scoped owners until the font/cache accepts them | Faces are released after rasterization; cached glyphs keep their own copied pixels. Typed font owners now retain faces, copied glyph pixels, bounded cache chains and scaled rasters. |
| Icon/cursor XOR surface, AND bytes and native cursor | Typed `COMPATIMAGE` construction and member owners | Cached/shared handles remain module-owned. Explicit destruction clears resources but retains the handle tombstone; shutdown deletes tombstones after resetting the active cursor. |
| Temporary image/cursor/mask surfaces | Scoped owners until copied or accepted by a persistent image/GDI object | `HbmpCompatSurface` copies input pixels. Selected GDI objects remain borrowed. |
| Cairo page pixels and drawing context | Scoped owners, context released before image storage | Cairo can retain page pixels while finalizing a later PDF page. The Cairo image owns its copied pixels. |
| Print submission process | Scoped process owner | The existing wait/kill policy and synchronous event pumping remain unchanged. |
| Menu bar rectangle replacement | Scoped allocation until the new layout is complete | Existing layout remains installed on failure. The window owns the published rectangles. |

## Verification evidence

Evidence is recorded for the actual batch and revision; it is not a claim that
all B gates have passed. The first scoped-owner batch builds without warnings
and its existing macOS suite passes 58/58. The persistent-image batch builds
without warnings, passes the expanded 62/62 macOS suite (including all four
portable image/font/print/PDF contracts), and matches both full regressions:
519 files each, 25 accepted unused-storage warnings, zero differences/errors
and no skipped inputs. Address/undefined-behavior/leak instrumentation and a
focused allocation-failure test are being qualified next.

Independent frozen C/SDK inputs and native regression fixtures remain unchanged.
The baseline contract is 519 files through turn 150 for nine scenarios, with
25 accepted unused-storage warnings recorded for milestone A. Report actual
results, the first unexpected difference and skipped inputs for each B batch.

## Native macOS leak verification

The user requested `/usr/bin/leaks`. It runs with `--atExit` against native,
uninstrumented image, font, print and allocation-failure harnesses. CTest
registers all four checks on macOS, using separate print output and serial runs.
The native tool enables allocation stack logging and returns failure on leaks;
no allocation-content dumps or exclusions are used.

Initial results: image, print and allocation-failure harnesses had zero leaked
bytes. The font harness found three allocations totaling 416 bytes through
`FcDefaultSubstitute` initializing a second Fontconfig configuration. On
Fontconfig 2.17+, default substitution now uses the wrapper's existing owned
configuration through `FcConfigSetDefaultSubstitute`; older versions retain
their existing entry point. The [Fontconfig reference](https://fontconfig.pages.freedesktop.org/fontconfig/fontconfig-devel/)
records this explicit-configuration interface and its version. Repeating the
font harness reports zero leaked bytes, and all five functional ownership
contracts pass.

macOS ASan/UBSan pass the portable resource contracts. Its runtime explicitly
rejects `detect_leaks=1`, so native `leaks` is a separate macOS gate. Linux CI
retains ASan/UBSan with leak detection enabled. No global sanitizer or leak
suppression is used to hide an application allocation.

Windows runs the same PDF metadata/page/abort/publication/owner/viewer cases
with UTF-8 fixture file operations through the native file module. CUPS queue
submission is a documented Unix-only path and its real shell/process cases
remain on Linux/macOS; this migration does not add a Windows CUPS feature.

The first ownership/fault/leaks batch passes 67/67 macOS CTest cases and all
five ASan/UBSan resource contracts. Linux CI's resource ASan/UBSan/LSan job
also passes with `detect_leaks=1`. Both final local regressions again match
519 files, 25 accepted unused-storage warnings, zero differences/errors and
no skipped inputs. These results qualify this initial B01 batch, not B01's
remaining persistent owners or later B checkpoints.

## Persistent font batch

`COMPATFONT` now owns its FreeType face, the cached UTF-8 glyph chain and its
scaled font. Glyph nodes and fonts use typed construction; each glyph owns its
copied raster bytes. Cache capacity/eviction order remain unchanged, and failed
allocation retains the existing partial-effect policy (including a prior
capacity eviction). Destruction frees scaled rasters and copied pixels before
the face. Library-context shutdown still occurs after faces are released.

The glyph ownership harness loads 512 distinct code points through the bounded
cache, reloads an evicted glyph, draws its scaled raster and restarts the font
library, checking retained metrics and actual nonempty pixels. It is included in
native `leaks` and sanitizer qualification. Results: 69/69 full macOS CTest,
6/6 ASan/UBSan resource contracts, 5/5 native leak checks, and full native
regression 519 matches / 25 accepted unused-storage warnings / zero differences,
errors or skipped inputs. No file layouts or baseline fixtures changed.

## Persistent printer batch

A printer job owns its PDF surface and native file stream. The job destructor
runs the existing abort policy: release PDF callbacks before closing the file,
then remove only temporary output that the existing policy does not retain.
Transfer into a DC is explicit; closing a completed file still reports its
error before publication. Failed CUPS output retains its original recovery
receipt rather than acquiring rollback behavior.

This batch passes 69/69 macOS desktop tests, all 11 focused ownership/native
leak checks and 6/6 ASan/UBSan resource contracts. Full native regression again
matches 519 files with 25 accepted unused-storage warnings, zero differences,
errors or skipped inputs. The host sources and file layouts are unchanged.

## GDI object batch and environment compatibility

GDI objects now own their surfaces, font instances and custom palette entries;
selected objects remain borrowed. The default palette derives its borrowed
entries from the static table rather than entering an owning pointer. Typed
object construction/destruction retains tombstones and selection checks. Stock
font owners are emptied before the font library shuts down.

The existing bitmap, drawing-state and palette contracts now run on macOS and
Windows as well as Linux, with native macOS leak checks. Current local results:
75/75 complete desktop CTest, 17/17 focused ownership checks (8 native leak
harnesses reporting zero leaked bytes), 9/9 ASan/UBSan resource contracts and
519 native regression matches / 25 accepted unused-storage warnings / zero
differences, errors or skipped inputs.

Windows CI found that the historical `getenv` macro poisoned standard-library
declarations after `<cstdlib>` was introduced. `PszCompatEnvironment` keeps the
same Windows SDL/POSIX CRT lookup through an explicit function instead. The
private platform header and `<stdlib.h>` join both include-order checks. The
updated Windows build is being qualified; this does not claim completion of B01.


## Audio ownership and platform environment follow-up

A typed audio state owns its alert stream and only the SDL audio subsystem it
initialized. Cleanup closes the stream before releasing that subsystem; an
externally initialized subsystem remains live. Dummy-device checks cover actual
queued sample validity, repeated alerts, restart, muted alerts and unavailable
audio. The audio batch passes 83/83 full macOS CTest, 13/13 ASan/UBSan ownership
contracts, all 12 native leak checks and 519 native regression matches with
25 accepted unused-storage warnings, no differences/errors or skipped inputs.

Windows compiled after the environment macro removal, but three existing client
workflows exposed narrow-CRT reads of UTF-8 environment paths in harnesses that
previously inherited that macro. Those former users now explicitly call
`PszCompatEnvironment`, including shared workflow headers. The original Unicode
path/name assertions remain unchanged; Windows requalification is required.


## Menu and native window batch

Menu items own copied labels in a typed array; insertion completes allocations
before moving the installed items. Failure retains the prior labels/order and
does not attach the proposed submenu. Removal moves items without duplicating
ownership. Submenu handle trees retain their existing attach/detach and tombstone
rules. Popup frames own their rectangle storage through nested callbacks.

Windows own their SDL window, renderer, texture and canvas/composite surfaces.
A retained dialog sequence explicitly takes and returns those owners. Existing
native detach work, logical destruction, text-input shutdown, release order and
stale HWND tokens remain unchanged. Native deleter symbols keep member types
identical across private SDL test substitutions.

This batch passes 89/89 complete macOS CTest, 31/31 focused ownership contracts
including 15 native leak checks, and 16/16 ASan/UBSan ownership contracts. All
519 native regression files match (25 accepted unused-storage warnings, no
differences/errors or skipped inputs). Builds introduce no warnings. B01 still
requires library-context lifetime integration and the remaining cleanup scopes;
these results do not complete the later B checkpoints.


## B01 lifetime integration gate

Each font retains shared ownership of its FreeType library before its face; the
face and glyph/scaled-font storage are destroyed before that library reference.
Module shutdown resets its roots without making a retained face depend on
cross-translation-unit static destruction order. Fontconfig configuration storage
also has a typed owner. Allocation-failure coverage now fails the library owner
control block after FreeType initialization, checks cleanup/retry, and native
leaks verifies that failure path. A retained face also survives module-root
shutdown while a fresh font is created, with unchanged glyph metrics.

Registered classes own copied names and extra bytes. Windows own menu/update
geometry; BeginPaint transfers update geometry into its DC. Printer DCs own
their jobs and clipping/paint arrays. Temporary canvas, texture and decoded DIB
surfaces stay scoped until publication or explicit factory transfer. Borrowed
selected objects and SDK/font pattern data remain borrowed.

Final local B01 integration results: 89/89 complete macOS CTest, 31/31 focused
ownership contracts including 15 native leak checks, 16/16 ASan/UBSan resource
contracts, no new warnings, and 519 native regression matches (25 accepted
unused-storage warnings; zero differences/errors/skipped inputs). Independent
references and all regression baselines remain unchanged. Windows/Linux CI is
being requalified for the final resource commits. B02 and later checkpoints
remain required for milestone completion; the PR remains a draft.


## B02 registry and dispatch guard batch

File and memory registries retain their original acquire/release spin policy and
normal unlock boundaries, now through `std::unique_lock`. A fault inside the
real `_lread` path is caught by its C++ caller, after which the same handle must
remain usable. A separate scratch binary built with `ff617cb`'s `file.c` catches
the identical injected error and deadlocks on retry (terminated after three
seconds). The guarded implementation completes the retry and cleanup. No product
library is replaced for this comparison.

Dispatch depth and the printer's busy flag are scoped. The callback source now
includes its shared declarations, with matching `noexcept` specifications. The
fault harness verifies nested dispatch depth and that a second failure during
window destruction does not replace an already pending first failure. It builds
its Unicode filename suffix as UTF-8 rather than depending on Windows CRT argv.

The original file-sharing, 64-bit size and heap-handle contracts now run on
macOS/Windows as well as Linux. Windows SDL fixtures truncate through the native
file service; the independent SDK fixture retains its SDK calls and expectations.
Results: 95/95 full macOS CTest, 37/37 focused ownership checks including 18
native leak checks, 19/19 ASan/UBSan contracts, no new warnings, and 519 native
regression matches / 25 accepted unused-storage warnings / no differences,
errors or skipped inputs. Baselines remain unchanged.

All 16 Linux/Windows CI checks pass for the final B01 commit `ff617cb`, including
Windows Unicode workflows, native/host regressions, host units, source witnesses,
Linux ASan/UBSan/LSan and tutorial checks. B01 is complete. B02 remains active:
modal scopes, deferred control lifetimes and in-flight failure handling remain
required before its completion gate.


## B02 in-flight failure preservation

A separate reproduction showed `StarsCapturePendingError` leaving secondary error
418 pending after primary error 417 had unwound and been caught. The service now
recognizes an exception already unwinding as the primary failure. It still
captures a standalone cleanup failure and retains the first previously pending
error. This policy also applies to native chooser callback boundaries, which
use the same service.

Three core unit cases cover these distinct states. The real-window harness also
throws a primary error while its scope destroys a window whose destruction
callback fails; destruction finishes, dispatch depth returns to zero and no
secondary pending error survives. The identical standalone reproduction now
passes both in-flight and standalone-cleanup assertions.

Verification: 95/95 full macOS desktop CTest (18 native leak checks), 22/22
SDL-free host CTest, 20/20 ASan/UBSan cases (19 ownership contracts plus the
exception unit executable), warning-free builds, and both complete desktop/host
regressions at 519 matches / 25 accepted unused-storage warnings / zero
differences, errors or skipped inputs. No baseline or independent reference
changed. B02's modal and deferred control lifetime work remains active.


## B02 native chooser and dialog cleanup scopes

`COMPATCLEANUPSCOPE` contains notification-capable cleanup separately from the
non-callback resource deleters. It closes once, restores the existing normal
cleanup order, maps allocation failures at the callback boundary and preserves
an earlier pending or unwinding failure. Resource-backed dialogs scope their
window and owner/focus restoration, including retained wizard-window sequences.
Native/compatibility file selection scopes its picker, owner, native-dialog
counter, saved focus and chooser buffers.

A private native-adapter hook tests the actual production chooser path. With
`c300d5c`'s manual `filedialog.c` in a scratch binary, the injected error leaves
`enabled=0 native-dialogs=1`. The guarded implementation passes the identical
vectors: file error, bad allocation, secondary restoration failure while a
primary unwinds, standalone restoration failure, ordinary cancellation and an
already-disabled owner. Saved focus and UTF-8 filename bytes remain intact. The
independent SDK/native bridge is unchanged; the test never opens a real picker.
The implementation-hook identity mapping is checked by the full product build,
not only the private test copy. The cleanup header joins both include-order gates.

Results: 97/97 macOS full CTest, 39/39 ownership contracts including 19 native
leak checks, 21/21 ASan/UBSan cases (20 ownership contracts plus core exception
units), no new warnings, and 519 native regression matches / 25 accepted
unused-storage warnings / no differences, errors or skipped inputs. Host runtime
is unchanged from the preceding fully verified 22/22 host / 519-match batch.
B02 remains active for other modal/menu scopes and deferred control lifetimes.


## B02 popup failure stops command delivery

A production reproduction selected command 73, then threw from
`WM_UNINITMENUPOPUP`: the old result was `TrackPopupMenu=1 pending-error=1
queued-command=1 command=73`. `TrackMenu` now finishes popup/capture/notification
cleanup, then stops command/ID delivery if failure is pending. The identical
reproduction reports `TrackPopupMenu=0 pending-error=1 queued-command=0`.

The permanent fault harness fails before the fix and passes afterwards for both
posted-command and return-ID modes. It checks restored child capture, dispatch
depth and first-error delivery, then proves normal selection still posts command
73. Results: 97/97 full macOS CTest (19 native leak checks), 21/21 ASan/UBSan
cases, warning-free builds and 519 native regression matches / 25 accepted
unused-storage warnings / no differences, errors or skipped inputs. Baselines
and independent references remain unchanged. B02 remains active.


## B02 remaining modal cleanup scopes

Message boxes scope their window, icon font, disabled owners and saved focus;
their owner list is a typed pointer array. Popup tracking scopes its popup
windows, capture, exit notifications and restoration of the previous tracking
frame. The new-folder dialog scopes its window and parent/focus restoration.
Dialog factories scope templates, untransferred fonts and partial windows, with
explicit release when a successful window is handed to its caller. Normal
cleanup order, nested loops and retained wizard presentation remain unchanged.

The existing message-box vectors now run as an ownership contract on every
desktop, including Windows SDL's private audio/pixel/clipboard cases. Independent
SDK cases retain their original branches. Both message boxes and the existing
nested/unowned callback-failure harness join macOS native leaks and sanitizer
qualification; the callback harness keeps its existing functional expectations.

Results: 100/100 full macOS CTest, 43/43 ownership contracts including 21 native
leak checks, 23/23 ASan/UBSan cases (22 ownership/callback cases plus core exception
units), warning-free builds and 519 native regression matches / 25 accepted
unused-storage warnings / zero differences, errors or skipped inputs. Baselines
remain unchanged. B02 still requires failure-publication and deferred control/
presentation lifetime work before its completion gate.


## B02 modeless failure retains no live window

`HwndCreateDialog` now stops modeless initialization at a pending callback failure
and destroys the partial dialog before returning. The production reproduction
previously reported `nonnull=1 alive=1 pending-error=1`; the same reproduction
now reports `nonnull=0 alive=0 pending-error=1`. The checked C++ caller can
therefore receive its exception without losing ownership of a live window.

Permanent cases fail before the fix and pass afterwards for font assignment,
initialization, allocation failure, raw pending-error delivery and the checked
C++ wrapper. They also verify that later initialization does not run after an
earlier failure and that successful modeless creation remains usable. Existing
modal return-value and nested/timer failure cases remain unchanged.

Results: 100/100 full macOS CTest (21 native leak checks), 23/23 ASan/UBSan
cases, warning-free product builds and 519 native regression matches / 25
accepted unused-storage warnings / zero differences, errors or skipped inputs.
No baseline or independent reference changed. B02 presentation and deferred
control lifetimes remain active.
