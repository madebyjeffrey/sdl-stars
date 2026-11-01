# M04 — Commands and partial commits on the six race pages

Historical evidence: inspected C11 source at `e3222eb1fbdb01d68067231b5d5ef14a71759b30`.
The retained fixtures run only against the frozen snapshot; current C++ refinement remains open.

Observed 2026-10-27 at `e3222eb1fbdb01d68067231b5d5ef14a71759b30`, with the
working collection. This companion specializes [M04-US01](M04-race-design.md#m04-us01),
[US02](M04-race-design.md#m04-us02) and [US03](M04-race-design.md#m04-us03).
It specifies every `WM_COMMAND` branch in all six `RaceWizardDlg*` callbacks,
including ordinary partial commit and cancellation. The visible page order is
1,4,5,2,3,6 by callback number, as defined by the
[top-level controller](M04-wizard-controller.md). Entry stories and M04-AC15–25
remain in the owning module. No runtime behavior, file format or baseline changes.

Canonical model: [M04PageCommands](dafny/M04-page-commands.dfy). It composes
[native race buffers](M02-native-state-and-save.md), existing M04 attribute bounds,
S01 signed narrowing/byte encoding and the canonical password hash. Source:
[raceui.c](https://github.com/madebyjeffrey/sdl-stars/blob/e3222eb1fbdb01d68067231b5d5ef14a71759b30/src/ui/raceui.c) `RaceWizardDlg1` through `RaceWizardDlg6`;
[race.c](https://github.com/madebyjeffrey/sdl-stars/blob/e3222eb1fbdb01d68067231b5d5ef14a71759b30/src/core/race.c) `GetRaceStat`, `SetRaceStat`, `SetRaceGrbit`,
`BoundsCheckPlayer`; [nativeui.c](https://github.com/madebyjeffrey/sdl-stars/blob/e3222eb1fbdb01d68067231b5d5ef14a71759b30/src/ui/nativeui.c) `CchGetUtf8WindowText`;
[globalsui.c](https://github.com/madebyjeffrey/sdl-stars/blob/e3222eb1fbdb01d68067231b5d5ef14a71759b30/src/ui/globalsui.c) `rgidRaceBtn`;
[utilgen.c](https://github.com/madebyjeffrey/sdl-stars/blob/e3222eb1fbdb01d68067231b5d5ef14a71759b30/src/core/utilgen.c) `LSaltFromSz`.
The [collection record](README.md#race-page-command-continuation) reports executed checks.

## Inputs, state, services and boundaries

`State` carries the full 192-byte native player, 16-byte password and existing
M02 binding/cache/new-flag/suggestion buffers, the raw read-only flag, selected
lesser-trait description, edit text, last combo selection request and Next enable
request. `Input.combo` separately supplies the actual later CB_GETCURSEL result;
requesting a selection does not prove the native control accepted it. `Put`,
`Stat`, `SetStat`, `Portrait`, `SetPortrait` and `SetTrait` bind reached fields to
native offsets/storage widths without changing S01 serialization. Attribute
clamps reuse M04's existing bounds. Preset replacement copies all 192 bytes,
including names, salts, runtime fields and unused storage; it is not a copy of
only the reusable logical pricing fields.

`Input` supplies LOWORD command/HIWORD notification, native check/combo return,
eight radio observations, bounded text reads, text-input rejection, seven preset
records, localized comparison names, generated plural and drawing post-state.
`First` uses the first nonzero checked radio in ascending ID order; no checked
radio has the Custom commit behavior. Comparison strings are NUL-terminated
logical strings. Only a reached fixed-buffer copy needs its corresponding byte
capacity; unrelated long catalog strings are not rejected as a new UI policy.

Native control storage, string object validity, UTF-8/rejection extraction,
localized plural generation, catalog-to-preset mapping and pointer/alias/lifetime
frames retain S01/S03/S04/S05/S06 owners. `Text` requires bytes before NUL to fit
the reached destination. `InvalidEvidence` marks unsupported receipts or an
unsafe reached native copy; it does not introduce a C validation branch. The
model describes ordinary returning, non-reentrant services. Nonlocal exceptions
and callbacks are not silently assigned the ordinary return/post-state.

For the ordinary built-in text controls, the compatibility implementation
supports the prefix/tail receipt: [GetDlgItemTextA](https://github.com/madebyjeffrey/sdl-stars/blob/e3222eb1fbdb01d68067231b5d5ef14a71759b30/compat/win32/src/control.c)
clears byte zero before dispatch; [CompatEditProc](https://github.com/madebyjeffrey/sdl-stars/blob/e3222eb1fbdb01d68067231b5d5ef14a71759b30/compat/win32/src/edit.c)
delegates WM_GETTEXT to [DefWindowProcA](https://github.com/madebyjeffrey/sdl-stars/blob/e3222eb1fbdb01d68067231b5d5ef14a71759b30/compat/win32/src/message.c), which
copies the bounded UTF-8 prefix and one NUL without clearing later bytes.
Custom subclasses, missing/destroyed controls, callbacks and valid native
window/text extraction remain S05 qualification; a buffer shape alone does not
establish those preconditions.

## Common commands and cancellation

Help is checked first and returns handled TRUE with the page's help topic.
Cancel, Back, Next and Finish map to `rgidRaceBtn` indices 0,1,2,3. These branches
do not filter the notification high word. Pages 2–6 save sticky position and
end with the index, preserving all changes already made on the page. They do
not capture or restore a race. Page-1 Cancel likewise ends immediately without
committing its current form text; earlier click-handler changes still survive.
`CancelKeepsState` proves the command-level frame. M02's outer edit/roster
wrappers, not these page handlers, own any later snapshot restoration.

Successful navigation/help returns callback TRUE; recognized setting changes
fall through to callback FALSE after performing their effects. That value does
not mean no mutation or cancelled wizard. The dialog manager's default handling
is a separate S05 concern. Unrecognized command IDs preserve this focused state.

## Identity page: preset selection versus form commit

A preset/Custom click is accepted only for notification zero and IDs 271…278.
It first zeroes both full 32-byte name fields and reads the form names, then
reads the button label into a bounded, otherwise unused buffer. It compares the
singular name against seven localized default names. When any matches and the
clicked choice is a preset, it replaces the singular with that selected default
and generates the plural using the current working race. A user-supplied name
remains. It updates the form only when these automatic replacements occur.

The click uses the selected preset's leftover allocation and portrait, or the
working race for Custom; it does **not** copy the preset design. Random disables
Next; every other choice enables it. It invalidates points and portrait regions
and returns callback FALSE. Choosing Custom does not clear Random here.
Geometry used for portrait invalidation remains a supplied native UI boundary.

A non-Cancel navigation command performs `Commit`: remember the old portrait,
choose the first checked preset or Custom, then copy that entire preset or clear
Random bit 30. It reads singular/plural with capacity **32**, despite the
initialization's edit limit 15. Native control-limit enforcement is not inferred
from these reads. Unlike the click path, these copies retain destination bytes
after their new NUL. It reads the old leftover stat even though the value is
unused, then reads the password through the actual UTF-8 text-input helper.

Rejected input sets only password byte zero to zero and returns handled TRUE
without ending the page. Preset/Custom/name changes already happened. The
remaining password bytes retain their previous values; no combo read, hash or
portrait restoration occurs. A chosen preset's portrait/salt/prefix can therefore
remain on this ordinary failure. `RejectedPasswordDoesNotClose` and
`RejectedPresetKeepsPrefix` prove these stage boundaries; failure is not rollback.

On a returning accepted password read, the combo result is narrowed through
LOWORD to signed 16 bits and then clamped to leftover range 0…6. CB_ERR (-1)
therefore selects zero. The canonical UTF-8-byte hash is written into the native
salt, the remembered portrait is restored, sticky position is saved and the
dialog ends with the navigation index. `SuccessfulCommitRestoresPortrait`
proves the successful counterpart. Top-level point/credential/save checks have
not yet occurred; the controller still owns them.

## Primary, lesser, habitat, economy and research commands

| Visible page / callback | Guard and immediate effects | Return and handoff |
| --- | --- | --- |
| 2 / `RaceWizardDlg4` | Primary IDs 271…280 require notification zero. Set the primary stat. Selecting AR (8) resets factory production/build/operate and mine production/operate to 10, mine build to 5, and clears cheap-factory bit 31. Other choices leave those fields current. | Invalidate points/description; callback FALSE. Cancel does not undo AR reset. |
| 3 / `RaceWizardDlg5` | Lesser-trait IDs 291…304 accept every notification. Narrow the check receipt to signed 16 bits and set/clear its bit using nonzero truth. Record description index 0…13. | Invalidate points/description; callback FALSE. |
| 4 / `RaceWizardDlg2` | Immunity IDs 291…293 accept every notification. Only narrowed check value exactly 1 assigns preferred/minimum/maximum -1. Every other value restores 50/20/80. | Call DrawRace2 with axis mask OR 0xff00 narrowed to signed 16 bits (-255/-254/-252), then callback FALSE with its post-state. |
| 5 / `RaceWizardDlg3` | Cheap-factory ID 291 accepts every notification. Narrow the check receipt, set bit 31 for any nonzero value. | Call DrawRace3 with 99, then callback FALSE with its post-state. |
| 6 / `RaceWizardDlg6` | Research radio IDs 271…288 require notification zero: group `(id-271)/3` selects stat 8…13 and remainder selects 0…2. Higher-starting-tech ID 291 accepts every notification and uses nonzero narrowed check truth for bit 29. | Invalidate points; callback FALSE. |

None of these command branches checks the raw read-only flag. Initialization
disables controls, but a supplied/programmatic command still reaches these
branches. This specifies their actual dispatch; it does not claim that a disabled
control normally emits a user event. `NonClickRadioIgnored` proves the radio
notification gate without generalizing it to the checkboxes.
`DrawingMaskVectors` preserves the signed-16 callee argument, rather than
mistaking the positive promoted mask expression for its received value.

`Drawing` retains an explicit pre-call state and a checked post-state receipt.
DrawRace2/3 call `DrawRaceAdvantagePoints` and therefore `CAdvantagePoints`, which
can normalize other race fields while drawing the changed control. `DrawFrame`
preserves the focused password/cache/path/new-flag/suggestion groups while
allowing the existing pricing owner to supply the race post-state. Actual
pricing/paint/global/geometry extraction is still an adapter obligation. The
receipt is not a second pricing algorithm or a promise that the race is unchanged.

## Interaction, quality and verification

M04-IR01/02 and QR01–03 remain authoritative. Command validation/feedback,
partial password failure and recovery now have these explicit state transitions.
S05 still owns native keyboard/focus, control text/rejection, accessibility,
viewport and event lifetime. No new latency, display or accessibility target is
inferred from command tests. M02 owns save/version/file recovery and S01 owns
serialized widths/credentials; runtime layout and file formats stay unchanged.

The [source fixture](../tests/spec/check_race_page_commands_source.py) compiles
the exact six WM_COMMAND arms in a small dispatch wrapper with native headers,
fresh resource IDs, actual stat/trait/hash/text-rejection helpers and
BoundsCheckPlayer. Its independent oracle checks **3,344** cases, complete
PLAYER/password bytes and tails, provider call order, checkbox versus radio
notification rules, all seven presets/Custom/no-selection, partial rejection,
31-byte/multibyte names, empty/15-byte/multibyte passwords, raw rejection truth,
hash/portrait/narrowing boundaries, AR reset and callback
versus modal results. Warnings are errors and address/undefined-behavior
sanitizers are enabled. Drawing services call actual BoundsCheckPlayer to expose
normalization; they do not execute full DrawRace2/3 or prove CAdvantagePoints.
Catalog strings/presets, control reads, geometry, plural and ordinary modal
services are scripted. This is command-arm evidence, not whole-callback/native
UI refinement. Full proof/audit and repository checks are recorded centrally.

## Remaining page work and ownership

| Unclosed inventory | Required contract/evidence and owner |
| --- | --- |
| `PszPlayerName` and preset/text extraction | [Producer contracts](S03-player-labels-and-race-titles.md) now specify selection, trimming, plural/article/fallback/grammar, reached domains and scratch frames, with actual helper/generated-catalog fixtures. `CurrentDefaultReceipt` refines this command model's supplied plural for the seven current defaults. A/V: general catalog/control extraction, native alias/reentrancy/lifetime; S03/S04/S05/S06. |
| Initialization handoff | [Initialization contracts](M04-page-initialization.md) now specify all six arms, preset/geometry/state/control requests and title lookup/argument schedule. [Title producers](S03-player-labels-and-race-titles.md) now bind current templates and the common integer-format subset; general catalog/native control extraction remains S03/S04/S05. |
| Portrait/points paint | [Pointer contracts](M04-pointer-and-message-interactions.md) now specify capture/repeat/portrait writes and cleanup. WM_PAINT, DrawRaceAdvantagePoints and InvalidateAdvPtsRect still need complete drawing/pricing/text/geometry/resource effects; M04/S05/S06. |
| DrawRace2 and native pointer handoff | Hit testing and habitat tracking now have [complete controller/numeric contracts](M04-pointer-and-message-interactions.md). Full DrawRace2 geometry/text/chance/cache and native event/capture/lifetime extraction remain M04/S04/S05/S06 work; GAP-26/27 remain explicit. |
| DrawRace3 and native economy handoff | Economy stat direction/step/repeat and redraw schedule now have contracts. AR/non-AR painted rows, geometry/count lifetime and full pricing/drawing extraction remain M04/S05/S06 work. |
| Remaining WM_PAINT and native appearance | [Pointer/message contracts](M04-pointer-and-message-interactions.md) now specify cursor/color/erase arms. Full paint geometry/control/description effects, first-paint state and native display/accessibility/lifecycle qualification remain M04/S03/S05/S06. |

These are still D/A/V work, not completed by a supplied drawing receipt or by
the top-level controller. GAP-08's policy conflict and unagreed UI/quality targets
retain their existing decision owners. No runtime repair is folded into this work.
