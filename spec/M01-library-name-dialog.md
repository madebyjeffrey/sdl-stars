# M01/M02 — Shared library naming dialog

Historical evidence: inspected C11 source at `e3222eb1fbdb01d68067231b5d5ef14a71759b30`.
The retained fixtures run only against the frozen snapshot; current C++ refinement remains open.

Observed 2026-10-27 at `e3222eb1fbdb01d68067231b5d5ef14a71759b30`, with the
working collection. This technical companion specializes M01's
[import/separate-copy](M01-game-library.md#m01-us03) and
[organize/rename](M01-game-library.md#m01-us05) workflows and
[M02 import](M02-race-library.md#m02-us04). Requirements remain at the owning
entries. It specifies shared initialization/command control, game collision
suggestions and race-suggestion composition; no screen, policy or runtime
behavior is changed. S05 owns real window/control/pointer/text/display extraction;
S04 owns native paths and enumeration; S03 owns localized strings.

Canonical model: [M01LibraryNameDialog](dafny/M01-library-name-dialog.dfy), using
existing S01 `FGameName`, [M02 filename/copy state](dafny/M02-race-actions.dfy),
[race suggestions](M02-race-file-allocation.md),
[name-buffer writes](M02-race-import.md), and S01 signed C division.
Source: [libraryui.c](https://github.com/madebyjeffrey/sdl-stars/blob/e3222eb1fbdb01d68067231b5d5ef14a71759b30/src/ui/libraryui.c) `LibraryNameDlg`,
`FPortableLibraryName`, `FPortableRaceFilename`, `SuggestLibraryName`,
`SuggestRaceFilename`. Executed checks are in the
[collection record](README.md#shared-naming-and-wizard-control-continuation).

## Caller interface and entry domain

Initialization binds the caller's 256-byte name buffer through DWLP_USER. The
edit control and caller output are separate state: typing or collision suggestions
do not update the caller's buffer until successful OK. The caller owns buffer
lifetime, termination and writable capacity. A native pointer's validity cannot
be inferred from a byte-array shape. Logical strings exclude embedded NUL; text
read receipts have at most 255 bytes. S05 owns actual GetDlgItemText/UTF-8/control
mapping. The model does not invent behavior for unwritten or invalid native
string storage.

The raw signed 16-bit race-dialog flag is tested for nonzero; it is not required
to equal TRUE. Race mode uses the existing race filename guard and game mode
uses the existing game filename guard, including reserved names and root/file
budgets. Paths, native case comparison and availability remain external service
observations, not a lock against another writer. The shared callback itself does
not change the mode flag; its caller's clearing behavior is at the import owner.

## Initialization geometry and UI state

`Initialize` first queries the work area. Failure uses a rectangle from zero to
the screen width/height. C leaves the order of the two GetSystemMetrics argument
evaluations unspecified; both orders are represented. The owner rectangle is
queried only when the parent is visible; a hidden parent or failed owner query
uses the work rectangle. The parent getter therefore occurs once or twice.

The dialog rectangle is then read. Its return value is ignored by the C callback,
so admitted inputs require observed written rectangle fields. No invented zero
rectangle is assigned to a failed/unwritten read. `GeometrySafe` records every
reached signed-32 intermediate bound; false is a diagnostic for unsupported/
undefined input, not a new native rejection branch. `Position` uses signed
truncation toward zero for centering, then max/min clamping with the dialog's
existing dimensions. Oversized dialogs can still extend beyond the work area;
`PositionLowerBounds` proves only the claimed lower bounds.

The source then moves without resizing/z-order/activation changes, binds the
output pointer, sets an edit limit of 255 and sets initial text. It returns TRUE.
Focus selection, native DPI/coordinate mapping, actual keyboard input,
accessibility and fitting all displays remain S05 qualification, not consequences
of the mathematical clamp. Unsupported messages return FALSE without callback
mutation (`Unhandled`); default dialog-manager effects are separate.

## Commands, validation and retry

`Command` receives LOWORD(wParam). The callback does not filter its notification
high word. Cancel closes with result FALSE and returns handled TRUE, without
reading the output pointer or edit text. Other command IDs return unhandled FALSE.
OK reads the bound pointer and bounded edit string, then applies the mode's
portable-name guard. Invalid input reports Invalid Name and leaves the dialog
open with caller output unchanged.

An available valid name is copied with `strcpy` semantics: text plus NUL replaces
the prefix, later output bytes remain. The dialog closes with result TRUE, while
the callback itself also returns handled TRUE. A collision instead generates a
suggestion, updates only the edit control, reports Name Taken and leaves the
dialog open. It never silently accepts the suggested name. A subsequent OK
revalidates and requeries that new edit text. `InvalidOrCollisionKeepsOutput` and
`CancelLeavesBuffers` prove the output/closure distinctions.

The error body is copied into a local 256-byte buffer before fetching the caption
because both compressed-string lookups share scratch storage. Model events keep
Body → CopyBody → Caption → Message order. Source tests use an overwriting scratch
provider and distinct messages to check the retained body. Localized body strings
must fit that native buffer; complete catalog/placeholder qualification is S03.

## Suggestions and bounded exhaustion

Race mode composes canonical `Suggest`: drop the last suffix, take the 239-byte
valid UTF-8 stem prefix and query `.r1` through `.r999`. Exhaustion produces empty
edit text, then still reports Name Taken. Output remains unchanged; another OK
on that empty value reports Invalid Name. GAP-22's missing final suggestion budget
is unchanged; an oversized suggestion must pass the dialog's normal OK validation
before acceptance.

Game mode uses `SuggestGame`/`GameLoop` for the shared dialog's non-revision
`" (%d)"` suffix. It preserves the original text, uses native signed division for
`(256-rootBytes-19)/2`, and shortens the original stem to leave suffix space. The
native continuation-byte retreat is `BackBoundary`; it prevents a split in valid
UTF-8 input but does not validate arbitrary malformed sequences. This is not a
second S04 UTF-8 decoder.

The loop probes the current name before each replacement for i=2…9999. A true
probe can leave the original unchanged even after the outer collision query
returned false: these are separate observations. On exhaustion it writes the
9999 candidate and exits without probing that last candidate. It does not clear
the edit or promise availability. `FinalGameCandidateNotProbed` and the loop bound
capture that boundary. The dialog still reports Name Taken; its next OK is the
acceptance gate. Revision/localized suffix production used by other M01 callers
remains a separate caller/producer obligation.

## Quality, verification and remaining work

Existing M01/M02 interaction and quality requirements apply. Caller workflows
own correction/recovery; S05 owns focus, native control text limits and screen
qualification. Bounded query counts are not latency targets. No native alias,
cross-instance uniqueness, full Unicode comparison or atomic-publication
guarantee is added.

[Source fixture](../tests/spec/check_library_name_dialog_source.py) compiles the
whole callback and actual portable/suggestion/prefix helpers with native headers,
fresh IDs, warnings as errors and address/undefined-behavior sanitizers. An
independent oracle checks geometry, API/query order, output tails, both dialog
mode branches, scratch-message ownership and repeated OK/Cancel behavior in
342 cases. Failed dialog-rectangle replies write explicit values in these tests;
unwritten reads and arithmetic-overflow inputs are not executed. Scripted window,
text and filesystem replies do not qualify native ABI, actual displays,
assistive technology, lifetime or reentrancy.

The collection record reports full proofs/audit and repository checks. Remaining
scope is real geometry/text/control/user-data extraction and modal lifecycle with
S04/S05/S06, the other M01 localized/revision naming callers, and the full race
page controls at M04. Open display/accessibility/performance targets retain their
existing owners. No runtime validation or UX policy is silently changed.
