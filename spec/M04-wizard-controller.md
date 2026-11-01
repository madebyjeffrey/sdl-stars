# M04 — Race wizard dispatch, Finish and modal sequence

Observed 2026-10-27 at `e3222eb1fbdb01d68067231b5d5ef14a71759b30`, with the
working collection. This companion specializes
[M04-US01](M04-race-design.md#m04-us01),
[US02](M04-race-design.md#m04-us02) and [US03](M04-race-design.md#m04-us03),
with M02/M03 consuming its completion result. It specifies the complete top-level
`RaceCreationWizard` dispatch/retry/Finish schedule. The [six page-command arms](M04-page-commands.md) now have focused contracts;
[initialization](M04-page-initialization.md) and [pointer/message](M04-pointer-and-message-interactions.md)
contracts now cover entry, hit/mouse/cursor/tracking and color/erase. Full paint
and native extraction remain separate work.
Runtime C, baseline files and pricing/credential algorithms are unchanged.

Canonical model: [M04WizardController](dafny/M04-wizard-controller.dfy), reusing
[M04 race/pricing](M04-race-specification.dfy), S01's password hash and signed
interpretation, [native filename selection](M02-native-state-and-save.md) and
[M02 import/edit state](M02-race-import.md). Source:
[raceui.c](../src/ui/raceui.c) `RaceCreationWizard`, `FSaveRace`;
[utilgen.c](../src/core/utilgen.c) `LSaltFromSz`;
[libraryui.c](../src/ui/libraryui.c) `FHasCurrentLibraryRace`,
`FSaveCurrentLibraryRace`; [window.c](../compat/win32/src/window.c)
`StarsCompatBeginDialogSequence`, `StarsCompatEndDialogSequence`.
See the [verification record](README.md#shared-naming-and-wizard-control-continuation).

## State, services and lifetime boundary

The session carries the full focused M02 state, raw signed-16 read-only mode and
active panel, parent handle identity, signed-32 salts, and local sequence/write
choices. Page, pricing, credential, save and sequence-end replies include explicit
post-state rather than a promise to restore the working race. Typed service tags
reject mismatched traces. Actual inner-page/pricing/credential/save/refinement
stays with its existing owner; no second points or hash algorithm is introduced.
The supplied post-states describe a non-reentrant modal session for parent,
read-only mode and rectangle ownership. Broader callback/reentrancy behavior is
explicit S05/S06 qualification.

Entry assigns global `vrgrcRCW` to a local **uninitialized 17-RECT stack array**,
assigns mode/parent and attempts modal-sequence ownership. `LocalFrame` means
lifetime, not initialized geometry. A returning page frees its procedure instance;
an escaping call does not reach that free. Ordinary function return expires the
local rectangle object's lifetime, and the source does not restore/clear its
global pointer. The model does not claim it is safe to read or dereference that
expired pointer. [GAP-26](README.md#gap-26) retains initialized-hit/paint, post-return
access and nested-wizard qualification with M04/S05/S06; no current use-after-
return reachability or runtime repair is asserted.

If sequence ownership was acquired, every ordinary exit calls EndDialogSequence
before returning while the local frame is still live. Otherwise it leaves the
caller/backend's sequence alone. End can run callbacks; their focused state is an
explicit service reply. `Complete`/`ReturnExpiresFrame` mark local lifetime end,
not a native pointer reset. Nonlocal escape bypasses later controller cleanup.
Actual focus/window destruction, main-thread checks and callback/global frames
are platform obligations. Passing a cleanup-call trace does not qualify them.

## Exact page dispatch

Visible order remains 1,4,5,2,3,6 by dialog procedure/resource, with active panels
1…6. Each page assigns its panel, makes a procedure instance, calls the dialog
and frees it after ordinary return. `Code` models assignment of INT_PTR to the
native unsigned-16 WizardButton, including modulo reduction of negative or large
returns. `Route` then applies the actual source branches:

| Returned code | Routing |
| --- | --- |
| 0 or 65535 | Cancel/failure from every page. |
| 1 on pages 2–6 | Previous visible page. |
| 3 on any page | Finish. |
| Any other code on page 6 | Finish by fallthrough. |
| Remaining codes on pages 1–5 | Next visible page, including Back on page 1. |

Forced/unexpected returns are a caller-contract boundary, not proof that disabled
or hidden controls can produce them normally. `RouteVectors` preserves the native
edge cases. Existing `Navigate` is only a clamped logical button projection; it
does not replace this raw dialog-return dispatcher.

## Finish, negative balance and credentials

Finish first checks the **global read-only flag**. Nonzero cancels the wizard and
returns FALSE without the direct price/hash/credential/save calls. The previous
logical `M04Race.Finish` returned Stay for this case; it now returns Failure to
match source. [GAP-25](README.md#gap-25) records that closed specification mismatch.
No runtime behavior changes.

Editable Finish recalculates the signed-16 balance, accepting the canonical
pricing owner's post-normalization state. A negative result formats its debt and
alerts, then selects the page from the active panel after pricing. Unknown panel
values use page 1. The negation occurs after C integer promotion: -32768 displays
debt 32768, not a signed-16 overflow. The model explicitly promotes before negation.
Inner native pricing/float/admitted-field extraction is still M04 pricing work;
an arbitrary successful number is not a proof of that refinement.

For nonnegative balance, the controller hashes the current bounded password,
sets `lSaltCur`, sets `lSaltLast=-5`, then checks credentials. `Salt`/`SaltRange`
reuse the canonical signed-byte, modulo-32 password algorithm. Native C-string
termination and signed-byte mapping are entry/adapter obligations. Failure goes
to page 1 with the check's current state; it is not a full rollback. Credential
side effects are retained in the supplied reply.

## Save route and ordinary/escaping completion

Credentials are checked even with nonzero `fDontWrite`. That local flag skips all
direct save/HasCurrent calls and returns TRUE. Otherwise `FHasCurrentLibraryRace`
is the current raw new flag OR first binding byte, not filename-cache presence.
It selects the current-library save when true. A false result calls standalone
save with literal `stars.r1` for an empty cache, or the cache pointer otherwise.
The existing `Suggested` adapter then models FSaveRace's pointer-sensitive choice
of the full UI suggestion. Equal text in another buffer is not pointer identity.

Returning save FALSE goes to page 1, retaining the save reply's data/artifacts.
Returning TRUE completes after owned sequence cleanup. The wizard itself never
captures/restores the outer design: M02's edit/import/roster wrappers own those
frames. `ReadOnlyFinish` and `NoWriteSkipsSave` prove the direct-call gates.
Mutable page/price/password/save/end state remains visible on cancellation and
ordinary failure. Escaped denotes a nonlocal exit, not FALSE; Pending denotes an
incomplete service trace. InvalidEvidence is a model/domain diagnostic, including
unsafe string or inconsistent service input, not a new C rejection branch.

## Interaction, quality, verification and remaining work

### Habitat growth hit-test boundary

The next page-control audit has a concrete undefined array-access boundary:
`IrcRaceDlgHitTest` searches `crcRCW` rectangles, then on the habitat page reads
`vplr.rgEnvVarMax[i / 5]` to reject immune axes. Habitat initialization sets
`crcRCW=17`; indices 15 and 16 are the growth arrows. Both therefore reach index
3 of the three-element `PLAYER.rgEnvVarMax` array. Being adjacent to
`pctIdealGrowth` in the native struct does not make that array access defined.
Read-only mode returns before the search; a different active page skips this
particular read. No supported boolean/index result is specified for the invalid
access. This is [GAP-27](README.md#gap-27), separate from the rectangle lifetime
and initialization obligations in GAP-26.

The [hit-test source fixture](../tests/spec/check_race_hit_source.py) compiles
the actual helper with native `PLAYER` and initialized, disjoint rectangle
receipts. It checks 104 defined cases and requires an array-bounds sanitizer
diagnostic for each of the two editable habitat growth hits. The helper's
post-diagnostic return is never consumed. Scripted geometry establishes the
reached invalid access, not full native mouse/paint/event reachability or original
binary parity. M04/S05/S06 own a native event trace and a separately scoped
repair with focused regression evidence; runtime source remains unchanged.

### Existing checks and unclosed page scope

M04-IR01/02 and QR01–03 remain authoritative. This controller exposes ordered
pages, debt/credential/save outcomes and retries; S05 owns actual controls,
keyboard/focus, display/accessibility and modal lifecycle. Pricing, normalization,
UTF-8/credentials and M02 file outcomes keep their canonical owners. Bounded
reply-trace evaluation terminates at a diagnostic or result; it does not prove a
user must complete the wizard or that every native retry eventually succeeds.
Performance/display targets remain Open.

[Source fixture](../tests/spec/check_race_wizard_source.py) compiles the actual
controller, password hash and cache-pointer selector with native headers/fresh
IDs, warnings as errors and address/undefined-behavior sanitizers. An independent
oracle checks 988 cases covering all pages, raw return narrowing, read-only/
no-write/sequence modes, negative/debt extremes, fallback panels, failed saves/
credentials, direct/library name selection, mutable-state retention and escaping
calls. It writes valid rectangle slots only while the controller frame is live
and never reads the rectangle pointer after return. Page/pricing/password/save/
end services are scripted; this is not a whole-page or native-UI proof.

The collection record reports focused/full proofs, links, handbook and required
repository checks. [Page commands](M04-page-commands.md) now cover preset/AR and
commit/password-recovery branches, and [initialization](M04-page-initialization.md)
now covers opening all six pages. [Pointer/message contracts](M04-pointer-and-message-interactions.md)
now specify mouse/cursor/tracking and color/erase. Remaining D work is complete
painting/drawing/pricing and general catalog/format extraction.
[Text producer contracts](S03-player-labels-and-race-titles.md) now specify
ordinary names/plurals and pinned title/default catalog/format bindings.
Native state/pricing/credential/save extraction and S05/S06 lifetime/reentrancy/
display qualification remain explicit A/V work. GAP-08 legacy policy and
GAP-21/24 state-intent questions retain their owners.
