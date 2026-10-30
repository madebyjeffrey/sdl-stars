# Milestone B implementation ledger

Status: implementation active. Tracking: [#65](https://github.com/madebyjeffrey/sdl-stars/issues/65); implementation [PR #66](https://github.com/madebyjeffrey/sdl-stars/pull/66).
Base: `develop` at `417e526`, after milestone A merged in #64.

The complete [milestone B plan](CPP-MIGRATION.md#milestone-b-ownership-records-and-standard-c)
is delivered in one PR, as requested. Each checkpoint keeps focused commits and
its own verification evidence. An active or pending checkpoint is not completion.

| Checkpoint | Status | Evidence / remaining work |
| --- | --- | --- |
| B01: desktop resource owners | Complete | Typed SDL/Cairo/FreeType, image/font/menu/audio/window/DC/geometry/class owners. Local full suite, native leaks, ASan/UBSan and regression pass; all 16 Linux/Windows checks pass for ff617cb. |
| B02: dispatch and callback guards | Complete | Locks, dispatch/modal/presentation guards, deferred controls, failure rollback and foreign callback containment. Full local matrix and all 16 Linux/Windows checks pass for d5de69e. |
| B03: const-correct text and scoped views | Complete | Implementation and local matrix pass: const text/path/format/menu contracts, bounded UTF-8/output/byte views, retained mutable scratch/sentinel behavior and literal-cast audit. All 16 Linux/Windows checks pass for b799348. |
| B04: record codecs and complete storage inventory | Complete | [Source-located wire/storage inventory](CPP-WIRE-INVENTORY.md), checked byte primitives and the history-header codec pass the full local matrix and all 16 Linux/Windows checks for d6bd60f. |
| B05: production payloads | Complete | Typed pooled arrays, explicit queue/log codecs and preserved physical capacities/partial effects pass the full local matrix and all 16 Linux/Windows checks for c0e0075. |
| B06: waypoint/order payloads | Complete | Typed arrays, task aliases and explicit waypoint/order codecs preserve replay, capacities and partial effects. Full local matrix and all 16 Linux/Windows checks pass for a7614a6. |
| B07: hull/design payloads | Complete | Explicit full/partial design, HS and log codecs preserve import/export/replay contracts. Full local matrix and all 16 Linux/Windows checks pass for c2f9791. |
| B08: fleet/identifier payloads | Complete | Explicit snapshot/ID/ship-transfer codecs, scalar aliases and preserved reader/replay effects pass the full local matrix and all 16 Linux/Windows checks for d91f49a. |
| B09: battle/AI payloads | Active | Battle and AI history families pass the full local matrix. All battle, AI and battle-plan families pass the full local matrix; final Linux/Windows qualification is required. |
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


## B02 presentation guards and foreign watcher containment

The live-resize reproduction fails before the fix: a `WM_SIZE` error is pending
but painting and frame submission still run. Layout/paint failure now stops that
work. Presentation scopes balance only their own defer-counter increments,
including callbacks that destroy the window. Refresh and live-resize flags
restore during unwinding, and the SDL event watcher contains renderer failures
before returning through SDL. Show-time and scrollbar batches use the same
counter guard; normal frame batching and drawing order remain unchanged.

The existing paint-batch contract now tests layout failure, paint failure,
renderer failure in refresh and live resize, successful retry, destruction during
layout, and failed/destroying show callbacks. Existing first-frame, pane pixel,
retained exposure and native-resize layout assertions remain. This harness joins
native macOS leaks and Linux sanitizer qualification.

Results: 101/101 full macOS CTest, 45 ownership cases including 22 native leak
checks, 24/24 ASan/UBSan cases (23 ownership/callback cases plus core exception
units), warning-free builds and 519 native regression matches / 25 accepted
unused-storage warnings / zero differences, errors or skipped inputs. Baselines
and independent references remain unchanged. B02 deferred control lifetimes are
still active.


## B02 deferred control storage and reentrancy flags

List, combo and toolbar state now has typed shared ownership in its HWND, with
local leases in operations that borrow state across callbacks. Logical teardown
still detaches the control immediately. The last active operation releases its
storage afterwards, making scoped flag restoration safe when a callback destroys
the window. List reset also scopes its detached item array and remaining labels;
toolbar tooltip destruction stays in logical teardown rather than the physical
storage destructor. Silent process-exit cleanup uses the same owners.

List scroll synchronization, combo synchronization/opening/closing/sizing,
toolbar pressed state, scrollbar layout and edit-scroll synchronization now use
scope restoration. Existing normal restoration points before notifications and
publication remain. Construction uses value-initialized typed allocations and
maps allocation failure to the existing failed-creation result.

The new lifetime harness destroys list/combo/toolbar controls from their actual
owner-draw/dropdown notifications, with and without a callback exception. It
asserts immediate handle invalidation/detachment, retained borrowed storage inside
the callback, physical release at operation exit, balanced dispatch and delivery
of the original error. A combo opening frame borrows combo state; its unborrowed
list state is allowed to retire immediately. Existing complete list/combo/toolbar
behavior vectors now qualify all desktop platforms, including Windows SDL private
cases while keeping independent SDK branches intact.

Results: 109/109 full macOS CTest, 53 ownership contracts including 26 native
leak checks, 28/28 ASan/UBSan cases (27 ownership/callback cases plus core exception
units), warning-free builds and 519 native regression matches / 25 accepted
unused-storage warnings / zero differences, errors or skipped inputs. No baseline
or independent reference changed. Final B02 failure-publication checks remain
active before declaring the checkpoint complete.


## B02 resource dialog initialization stops at owner failure

The new `DialogBoxParamA` vector fails before the fix: a failed owner-disable
callback still increments the dialog initialization count. The implementation
now restores its owner and returns before creating the dialog, preserving the
first pending error. A call while that error is already pending starts no work.
The existing modal failure return values remain intact.

This and the following initialization fixes share a fully qualified batch:
109/109 macOS CTest (26 native leak checks), 28/28 ASan/UBSan cases, warning-free
builds, and 519 native regression matches / 25 accepted unused-storage warnings /
zero differences, errors or skipped inputs. Baselines and independent references
remain unchanged.


## B02 file chooser starts no work after owner failure

The private native adapter counted an unwanted chooser call after owner-disable
failure before the fix. `FFileDialog` now starts no chooser and accepts no new
operation while an error is already pending. The permanent case verifies enabled
owner, saved focus, zero native-dialog count, unchanged UTF-8 filename bytes and
preservation of the disabling error even when owner restoration also fails.
The 109-test / 26-leak / 28-sanitizer / 519-match batch above includes this case.


## B02 message-box failure produces no prompt sound

The permanent audio-hook vector fails before the fix: a message box still queues
its alert sound after its owner-disable callback fails. Setup now stops at that
failure and restores the owners. A second call with the original error pending
creates no work or sound. Independent SDK branches remain unchanged. This passes
in the same 109-test / 26-leak / 28-sanitizer / 519-match qualification batch.


## B02 combo popup publication and rollback

Dropdown-notification and owner-drawing failure vectors previously left the combo
dropped despite a pending error. Opening now stops on notification failure and
scopes rollback of a partial popup before publication. It restores capture and
selection, leaves no popup behind and permits an ordinary successful retry.
Rollback checks the exact popup it created: another vector closes/reopens it
inside owner drawing and proves the replacement stays open and usable. Existing
combo behavioral, destruction, mutation and capture cases still pass.

Results remain 109/109 full macOS CTest, 26 native leak checks, 28/28 ASan/UBSan,
warning-free builds and 519 native regression matches / 25 accepted unused-storage
warnings / zero differences, errors or skipped inputs. Baselines are unchanged.
The remaining B02 lock inventory includes profile operations before completion.


## B02 profile locks, partial loads and checked temporary publication

The final lock inventory found manual profile spin-lock pairs. A production
profile-read fault after partial loading catches its error, then the old retry
hangs (three-second reproduction timeout). Both profile entry points now use the
existing acquire/release lock policy through a C++ guard. Partial lines, parsed
chains and paths are scoped; profile saving scopes its temporary handle/path
while retaining explicit checked close, replacement and directory-sync order.
`NATIVECLEANUPSCOPE` requires a non-throwing release callable and is separate from
notification-capable callback cleanup.

The actual profile harness injects failure after a line buffer and parsed row
exist, during write-side loading, and after a save temporary has opened. Every
retry succeeds, the existing profile bytes remain unchanged, and no temporary is
left behind. A subsequent successful update still publishes normally. The full
existing profile contracts join native leaks and sanitizer qualification.
Source witnesses use the actual C++ ownership/locking headers instead of their
old atomic-only declaration. Their independent inputs, exclusions and expected
results remain unchanged.

Local checkpoint results: 111/111 full macOS CTest, 55 ownership contracts
including 27 native leak checks, 29/29 ASan/UBSan cases (28 ownership/callback cases
plus core exception units), 22/22 SDL-free host CTest, and complete desktop and
host regressions each at 519 matches / 25 accepted unused-storage warnings / zero
differences, errors or skipped inputs. No new warnings, baseline changes or frozen
reference changes. All 27 source witnesses pass: the first run passed 25 and
identified missing C++ helper declarations in the two profile witnesses; both
then passed after the include/declaration update (25 and 42 assertions).
B02 implementation and local qualification are complete; its tracker remains
active until the platform CI result is available. B03–B14 remain in scope.


## B02 platform completion gate

All 16 Linux/Windows checks pass for `d5de69e`, including source witnesses,
Linux ASan/UBSan/LSan, desktop/host regressions, tutorial coverage and Windows SDL
plus independent SDK/reference configurations. The Windows job API confirms
success (job 123581087970); the aggregate PR check listing had retained an older
in-progress state. B02 is complete. B03 is active; B04–B14 remain pending.


## B03 const text, mutable outputs and bounded views

The 24 literal-backed core/UI globals now expose immutable text. Paths, stream
writes, formatting, drawing, menu labels, compression inputs and password hashing
use const input contracts; mutable output/scratch buffers remain mutable. Report
name helpers return borrowed const text, while formatted/decompressed scratch
buffers retain their existing mutable interfaces and invalidation rules. Original
names, scalar widths, signedness and record layouts are unchanged. BTNT's text
pointer is const-qualified without changing its layout.

`encoding.h` adds call-scoped `string_view`, output `span<char>` and binary
`span<const byte>` interfaces. New views are consumed inside callback-free path
validation, message copying and legacy race-name conversion. No view is retained
across callbacks, resets, reallocations or reuse of the compressed-string cache.
A standalone, SDL-free contract executable tests nonterminated and truncated
slices, complete-code-point byte budgets, NUL rejection/termination, empty spans,
overlapping copies and UTF-16 byte parity. It joins Linux sanitizer qualification.
The compilation audit now rejects mutable string-literal casts.

`RightTextOut` has a legacy `cLen == -1` output-buffer mode. Its mutable overload
preserves that write; a separate const overload draws borrowed text without
mutating it. The real map-printing harness checks both modes and literal text.
Formatting into mutable report scratch arrays stays explicit before they are
borrowed as const display text. Source-witness adapters use const labels/inputs
or genuinely mutable local arrays; all independent expected values remain intact.

Verification: 112/112 full macOS CTest (27 native leak checks), 30/30 ASan/UBSan
cases (29 boundary/ownership cases plus core exception units), 23/23 SDL-free
host tests, all 27 source witnesses, 89 headers in both include orders, frozen
layout/enum/catalog checks and warning-free Debug/Release builds. Complete native
and host regressions each match 519 files / 25 accepted unused-storage warnings /
zero differences, errors or skipped inputs. Baselines and frozen references are
unchanged.

The first desktop/sanitizer runs encountered four workflow timeouts and one
toolbar timeout while macOS repeatedly slept; the power log records matching
sleep intervals. All five cases passed unchanged on retry. Fresh complete runs
with a temporary idle-sleep assertion passed 112/112 and 30/30 using the original
test deadlines. No product change or timeout increase was used to mask a failure.
B03 local qualification and all 16 Linux/Windows checks pass for `b799348`, including Windows SDL/SDK and Linux ASan/UBSan/LSan. B03 is complete; B04 is active.


## B04 wire inventory and first explicit codec

The generated inventory indexes all 206 owned source/header files, 46 record
names, twelve variable tails, all packing/bitfield/anonymous aggregate sites,
pointer-cast and bit-operation candidates, direct record/log/stream/resource
operations and raw copies/size expressions. Its family dispositions distinguish
wire records from runtime-only pools and genuine foreign SDK/library layouts.
The unused zero-storage `_ctype` symbol is assigned to B14. All 259 independent
C layout/prefix measurements remain unchanged. Linux and Windows CI retain the
full source-located JSON report for review; it is an index, not an alias proof.

`NATIVEBYTEREADER` and `NATIVEBYTEWRITER` preflight explicit little-endian scalar
and byte operations. Failed primitives preserve their cursor/output, and empty
and overlapping byte spans are supported. An overlapping scalar-output test
first reproduced a read that destroyed its own input; snapshotting before
publication fixes it. Independent vectors and arithmetic expectations exercise
all 65,536 paired signed-16 values, signed-32 minimum, unaligned/truncated
slices, trailing bytes, guarded outputs, size-limit skips and empty spans.
The primitive header also compiles independently with ISO C++20 pedantic errors.

The history-header read/write pair now uses explicit two-signed-16 codecs. Its
four defined bytes stay identical; encoding never dumps a native struct. The
reader deliberately retains the old reusable-buffer prefix for logically short
payloads. No stricter malformed-input admission, pointer serialization, record
size or gameplay behavior is introduced. Other families remain assigned to
B05–B11; their old layout consumers have not been declared migrated.

Qualification: 114/114 full macOS desktop CTest (55 ownership cases including
27 native leak checks), 25/25 SDL-free host tests, 31/31 ASan/UBSan cases, all
27 source witnesses and 91 headers in both include orders. Debug/Release/host
builds introduce no warnings. Complete desktop and host regressions each match
519 files / 25 accepted unused-storage warnings / zero differences, errors or
skipped inputs. Frozen references, enum/catalog contracts and baselines remain
unchanged. B04 local qualification is complete; platform CI remains required.


## B04 GCC test-construction follow-up

Linux CI rejected one test initializer that Clang accepted: GCC interpreted the
parenthesized span/first expression as a declaration. `d6bd60f` uses unambiguous
brace construction. The isolated original B04 vectors pass locally after this
test-only fix. All 16 Linux/Windows checks subsequently pass for `d6bd60f`, including
Windows SDL/SDK and Linux ASan/UBSan/LSan. B04 is complete. This does not change codec or game behavior.

## B05 production queue storage and codecs in qualification

PLPROD retains its four-byte header, byte counters, reserved bits and slot stride;
its original rgprod name now denotes a borrowed typed-array accessor. The pooled
allocator starts actual header/item lifetimes over the same complete physical
slot, including rounded spare capacity. Allocation, reallocation and release
use typed operations without treating a PLPROD as a PL. Existing free-list,
capacity truncation, preserved bytes and allocation-failure ordering remain.

PROD fields and reserved bits are encoded explicitly, and queue logs use a
two-byte ID prefix followed by four-byte items. File reads merge an incomplete
last item through the codec, preserving the prior partial-byte overwrite. Replay
retains its partial-production matching order, incoming progress mutation,
trailing-byte treatment and short-declared-record behavior. No new validation
policy or gameplay fix is included. The archived observer follows the new
accessor spelling. Frozen layouts and baselines remain unchanged.

Independent vectors cover all bit positions, 65,536 mixed production words,
reserved bits and every partial-row byte count. Real pooled-storage and log
tests cover stable/moved growth, byte-capacity wrapping, matched/unmatched
progress, input-byte mutation and zero-declared-length replay. The bounded
source witnesses use the actual typed lifetime functions with explicit physical
heap extents; their vectors, exclusions and expected receipts remain unchanged.
The first source run passed 26/27; the lifecycle adapter needed an earlier
include for its manually allocated typed array and then passed all 16,243
assertions. Full local and platform qualification remain required.


B05 final local qualification: 114/114 full macOS desktop CTest, including
55 ownership contracts and all 27 native leak checks; 25/25 SDL-free host tests;
32/32 ASan/UBSan cases including the real production log/storage tests; and a
fresh complete 27/27 source-witness run. All 92 public headers compile in both
include orders. Debug/Release/host builds introduce no warnings. Native and host
regressions each match all 519 files with 25 accepted unused-storage warnings,
zero differences/errors and no skipped inputs. Frozen 259-layout, enum/storage
and catalog contracts and all baselines are unchanged.

The remaining header-based queue moves now use actual PROD array positions
with identical byte addresses. Log replay also preserves the empty-roster
no-read path, tested with a null input pointer, and reads packed scalar outputs
through aligned temporaries. Linux sanitizer qualification now includes the
real core log/storage unit program. B05 local qualification is complete; its
platform gate remains required. B06–B14 remain in scope.


## B05 platform completion gate

All 16 Linux/Windows checks pass for `c0e0075`, including the expanded Linux
ASan/UBSan/LSan job with actual core log/storage tests, Windows SDL/SDK, source
witnesses, desktop/host regressions and tutorial coverage. B05 is complete.
B06 is active: explicit transport word masks pass all 65,536 word values and
the unchanged transport/preset vectors; order array lifetimes, task views and
wire/replay consumers are being migrated next. B06–B14 are not complete.


## B06 orders, task aliases and waypoint codecs in qualification

Transport actions use a real uint16 word with explicit 12/4-bit masks. Quantity
and action names remain accessors with width-preserving setters; constant
tutorial goals retain their original values. All 65,536 words and existing
transport/preset vectors pass. ORDER's task union becomes one actual TASKXPORT
array with call-scoped aliases to its words, and PLORD owns actual adjacent
ORDER object lifetimes through the same typed pool operations as production.

Full/partial order, waypoint and ship/index fields have explicit byte codecs.
Readers retain zeroed scratch-prefix and signed/logical admission behavior;
writers retain eight/eighteen-byte lengths and trailing-zero trimming. Legacy
replay applies bounded runtime byte effects through aligned temporaries, keeping
neighbor writes and negative-index behavior without forming invalid typed views.
Native alignment two is now checked separately for six migrated families; every
frozen size/prefix and all remaining alignment comparisons stay in force.

Focused results: wire/layout/log/util contracts pass, including every flag word,
unaligned wire slices, partial prefixes, task alias assignment, actual order
array growth/movement, byte-count wrapping and real replay mutations. The first
source run passed 26/27; its method-bearing forecast projection needed a native
C++ tag name, then passed 272,841 assertions. Providers explicitly start bounded
order arrays; independent vectors, exclusions and expected receipts are unchanged.
Full local and platform qualification remain required.


B06 final local qualification: 114/114 complete macOS desktop tests (all 27
native leak checks), 25/25 SDL-free host tests, 32/32 ASan/UBSan cases and a
fresh complete 27/27 source-witness run. All public headers compile in both
include orders. Debug/Release/host/sanitizer builds introduce no warnings.
Desktop and host regressions each match all 519 files through turn 150, with
25 accepted unused-storage warnings, no differences/errors and no skipped
inputs. All 259 frozen size/prefix contracts remain covered; the six migrated
runtime alignments are explicitly checked at two bytes. Independent frozen
inputs, catalogs and regression baselines remain unchanged. Linux/Windows
qualification remains required; B07–B14 remain pending.


## B07 ship-design payloads in qualification

HS slots have an explicit four-byte codec. Full design payloads use their
seventeen-byte prefix and decoded runtime slots; partial payloads keep the
six-byte prefix and encoded name. No typed array is formed at odd byte seventeen.
Design and change-log words encode their original flags and reserved input bits
explicitly. The design's flag word and power/visibility aliases become value
accessors over one actual representation; widths and all frozen measurements
remain unchanged. The unused zero-length RTSHDEF tail is removed.

Import retains bitmap repair, name bounds, hull/slot admission, existing-full
knowledge retention, cost/mass and engine repairs. Partial names still populate
hul.chs with their marker, including compressed lengths above sixteen. Replay
retains early rejection, accepted unknown modes, progress clearing and the
nontransactional roster-count receipt after failed replacement. Writer junk
bits that previously had indeterminate stack contents are initialized to zero;
defined fields and incoming reserved bits are unchanged. Invalid runtime full
slot counts outside the sixteen-slot HUL array cannot enter the codec; this
does not tighten the existing reader, which already rejects them.

Focused wire/layout/save/log tests pass. Independent vectors cover every category
word, every design/change flag word, all slot counts, short-prefix failures,
unaligned slots and overlapping output. Real import/export and replay tests
cover the behaviors above. The actual save/reader unit program joins Linux
ASan/UBSan/LSan qualification. Full local and platform gates remain required.


B07 final local qualification: 114/114 complete macOS desktop tests, all 27
native leak checks, 25/25 host tests, 33/33 ASan/UBSan cases including the actual
save-reader program, all 27 source witnesses, and 93 public headers in both
include orders. Debug/Release/host/sanitizer builds introduce no warnings.
Desktop and host regressions each match 519 files through turn 150 with the
same 25 accepted unused-storage warnings, zero differences/errors and no skipped
inputs. All 259 frozen size/prefix measurements and the six explicit runtime
alignment checks pass. Independent C inputs, catalogs and baseline fixtures
remain unchanged. B07's Linux/Windows gate remains required; B08–B14 remain
pending.


## B06 platform completion gate

All 16 Linux/Windows checks pass for `a7614a6`: desktop/host regressions,
Windows SDL/SDK and unit workflows, source witnesses, Linux ASan/UBSan/LSan
and tutorial coverage. B06 is complete. B07 `79b3303` has passed its complete
local matrix and is entering platform qualification. B08–B14 remain required.


## B08 fleet payloads in qualification

Fleet snapshots use explicit fourteen-byte headers, positive count masks and
byte/word variants, cargo width masks, partial direction/mass and full damage/
plan/order suffixes. Readers retain signed word counts, scratch-backed admission,
nonzero/dead logic, damage clamping and their incremental publication before a
wrong full-record tag. The damage-mask scan index still controls the original
turn-zero coordinate repair. Partial mass arithmetic and record/callback order
stay in the caller.

DV, fleet IDs, mass/damage, movement/power and direction/flag aliases now have one
actual numeric representation with width/signedness-preserving accessors. The
original scalar assignments were reviewed; compound setters repeat only stable
pointer/index reads, and function receivers are evaluated once. Fleet ID lists
are call-scoped byte views with explicit zero wire prefix, not typed integer
arrays at arbitrary byte addresses. RTFLEET has no zero-length tail. All frozen
size/prefix contracts, including RTFLEETIDS.rgid offset zero, still pass.

Split/merge and ship-transfer logs use explicit little-endian values. They retain
ID order, duplicates, odd trailing-byte treatment, the one-ID gather case, lookup
no-read paths, signed deltas and two-pass transfer order. Actual reader tests
cover zero declared length, exact next-record consumption and pre-error state;
actual split/merge and transfer writer/replay tests pass. Independent vectors
cover every header, ID and DV word, all presence masks in both count formats,
cargo width masks, high detail values, signed counts and damage clamp boundaries.
The actual ship unit program joins sanitizer qualification. Full local and
platform gates remain required.


## B07 platform completion gate

All 16 Linux/Windows checks pass for `c2f9791`, including Windows SDL/SDK, both
regressions, all source witnesses and the expanded Linux ASan/UBSan/LSan job
with actual save-reader/import/export cases. B07 is complete. B08 is in full
local qualification; B08–B14 remain required.


The first expanded B08 sanitizer run passed 33/34 programs. The new missing-ID
fixture used 0xffff, which selects FLookupFleet's existing negative-ID write-back
operation and accesses an uninitialized replay frame. The prior FRunLogRecord
and unchanged FLookupFleet use this same path; it is outside the fixture's
intended read-only lookup domain. The fixture now asserts that positive slot
511 is absent before exercising the no-read rejection. Production lookup and
admission code are unchanged. Final functional/sanitizer qualification is being
repeated; the failed run is not reported as a passing gate.


B08 final local qualification: 114/114 complete desktop tests including all
27 native leak checks, 25/25 host tests, 34/34 ASan/UBSan cases and all 27
source witnesses. The corrected positive-ID fixture passes both functional and
sanitizer reruns. All 93 public headers compile in both include orders, all
259 frozen size/prefix contracts and the six migrated runtime alignment checks
pass, and Debug/Release/host/sanitizer builds introduce no warnings. Desktop
and host regressions each match all 519 files through turn 150, with the same
25 accepted unused-storage warnings and no differences/errors/skipped inputs.
Independent reference inputs, catalogs and baselines remain unchanged. B08's
platform gate remains required; B09–B14 are pending.


## B08 platform completion gate

All 16 Linux/Windows checks pass for `d91f49a`, including Windows SDL/SDK,
desktop/host regressions, all source witnesses and the expanded Linux
ASan/UBSan/LSan job with actual ship-transfer and split/merge replay cases.
B08 is complete. B09 fixed battle codecs and scoped byte windows pass focused
wire/layout/real-heap tests; producers, replay/continuations, variable tails,
AI data and the full matrix remain required. B09–B14 are not complete.


## B09 battle record family

Battle data, events, kills and tokens have explicit 14/6/8/29-byte codecs.
Producers publish byte fields at the original points, including kill counts
before KillShips and the battle length before SendBattleMessages, with player
flags still published afterward. Runtime token/cargo/replay arrays have explicit
array lifetimes over the same allocations. RNG, tie ordering, arithmetic and
allocation order remain unchanged.

Loaders, legacy conversion, continuation writers, replay and battle reports use
scoped byte views over the physical arena. A getter reads only its field bytes;
short declared records still use existing scratch/spare backing. Signed event
counts and boundary positions retain their original traversal domain. Persistent
replay roots remain borrowed byte pointers, with fresh views on access. The
three battle zero-length tails and TOK's inactive raw-word alias are removed;
metadata widths and all frozen measurements remain unchanged. The tutorial
observer follows the borrowed battle root through the same accessor.

Real replay tests cover seeking/reset, movement, attack damage, shield counts,
round transition and summaries. The actual encrypted writer/reader verifies
34/35-token continuation boundaries, a 1023-byte event split and every payload
byte. Independent golden vectors, signed event counts, bit words, short physical
prefixes and real heap/legacy-conversion fixtures pass. Actual battle and memory
unit programs join the sanitizer gate.

Battle family local qualification: 114/114 desktop tests including all 27 native
macOS leak checks, 25/25 host tests, 36/36 ASan/UBSan cases, all 27 source
witnesses and 95 public headers in both include orders. Debug/Release/host/
sanitizer builds introduce no warnings. Desktop and host regressions each match
519 files through turn 150 with the same 25 accepted unused-storage warnings,
zero differences/errors and no skipped inputs. All 259 frozen measurements and
six migrated runtime alignment checks pass. Independent inputs, catalogs and
baselines remain unchanged. B09 AI data and platform qualification remain
required; B10–B14 are pending.


## B09 AI history in qualification

Ordinary AI history uses signed little-endian planet/freighter/ID fields and
explicit twenty-byte row codecs for compaction. Scoped byte windows retain
physical spare bytes, stale fields and the existing pre-turn-20 behavior and
later count clamps. Redistribution evaluates the donor ID before the receiver
postincrement, publishes the count before its ID, and retains unused slots.
History length is still unsigned for logging and signed for Cybertron setup.

Cybertron's persistent and temporary words have explicit bit accessors; inactive
raw-word unions are replaced by actual numeric words. Its planet-ID traversal,
low-byte field accesses, reserved bits, two-bit countdown, scratch clear lengths
and incremental updates remain at the original positions. The trailing mineral
scratch uses explicit signed 32-bit little-endian access. Macinti's opaque
history remains untouched by the ordinary validator. Raw history read/write
chunks retain their original lengths and do not acquire stricter admission.

The byte-window primitives now live in platform/byteview.h and serve both record
families. Heap adapters still borrow their physical allocations; persistent roots
remain byte pointers, with no deferred flush on normal return. Focused golden,
all-word, signed/short-prefix, real validator/heap and replay tests pass. Full
local and platform qualification remain required.


AI history local qualification: 114/114 desktop tests including all 27 native
macOS leak checks, 25/25 host tests, 36/36 ASan/UBSan cases, all 27 source
witnesses and 98 public headers in both include orders. Debug/Release/host/
sanitizer builds introduce no warnings. Desktop and host regressions each match
519 files through turn 150 with 25 accepted unused-storage warnings, zero
differences/errors and no skipped inputs. All frozen layout and migrated
alignment checks pass; independent inputs and baselines remain unchanged.
The remaining native battle-plan prefix copies are assigned to B09 by the
inventory and will migrate before B09 completion. Final platform qualification
is also required; B10–B14 remain pending.


## B09 battle-plan prefixes in qualification

The four-byte plan prefix encodes both words explicitly, including reserved
bits. Deletion logs still emit only the first two bytes. Replay reads only that
word before owner/index/deletion decisions, and reaches the target word only
after its original four-byte guard. Loading still reads the owner from the
complete reusable scratch backing before allocation/name admission. Full text
validation finishes before prefix/name publication; plan-index override and
failed-edit receipts remain unchanged. Both plan allocations now start actual
sixteen-element arrays with the same allocation and initialization policy.

Independent vectors cover every word, output/short-input guards and one-/two-
byte field views. Real save/replay tests verify exact deletion bytes, ignored
wrong-owner/index deletions, short nondeletion rejection, deletion compaction,
invalid UTF-8 preservation, new-plan publication and reserved bits. Existing
raw UTF-8/compression tests also pass. Final B09 matrix/platform qualification
remains required; B10–B14 are pending.


Final B09 local qualification: 114/114 desktop tests including all 27 native
macOS leak checks and the available tutorial smoke checkpoint, 25/25 host tests,
36/36 ASan/UBSan cases, all 27 source witnesses and 98 public headers in both
include orders. Debug/Release/host/sanitizer builds introduce no warnings.
Desktop and host regressions each match 519 files through turn 150, with the
same 25 accepted unused-storage warnings, zero differences/errors and no
skipped inputs. Frozen inputs, catalogs, baselines, all 259 size/prefix contracts
and six migrated runtime alignment contracts remain unchanged. Linux/Windows
qualification is still required before B09 completion; B10–B14 remain pending.
