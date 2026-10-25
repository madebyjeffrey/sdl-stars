# M04 - Race Design Wizard

Status: source-derived specification; suggested Rust/SQL models are proposals.
Reviewed: 2026-10-23. Source: `e2eae18cff0f990db20415c1c2262aed2022f95b`.


Status of coverage: data model and advantage calculation specified; full wizard
workflow and recovery audit remain incomplete. This module entry incorporates
the logical race reports first prepared on October 20, revalidated against the
current revision. It does not approve a refactor or establish a new code API.

The canonical formal definitions for race design, normalization, pricing and completion are in
[M04-race-specification.dfy](M04-race-specification.dfy). The prose and worked examples below
explain those definitions and retain their source evidence. See the
[coverage inventory](README.md#dafny-coverage-and-verification) for exact scope,
proof strength, dependency boundaries and verification commands.

## User stories and requirements

The primary actor is a player designing a reusable race, entered from the race
library or New Game roster. A developer implementing race storage/pricing is a
supporting actor. The [status conventions](README.md#requirement-ids-and-status)
apply; the existing incomplete wizard/recovery audit remains incomplete.

### Stories and workflows

<a id="m04-us01"></a>
**M04-US01 — Shape a race (Observed).** As a player, I want to choose identity,
traits, habitat, economy and research so that the race reflects my intended
play style. Start with a preset or Custom, navigate the six visible pages,
adjust choices and revisit earlier pages. Selecting Custom clears Random;
normalization may change the working record while calculating/displaying price.

<a id="m04-us02"></a>
**M04-US02 — Understand affordability (Observed).** As a player, I want an
updated advantage-point balance so that I can trade off race choices and know
whether Finish can pass the point gate. Editing feeds the canonical calculator;
Finish recalculates and keeps the active page on a negative returned balance.
The balance does not by itself validate credentials or successful saving.

<a id="m04-us03"></a>
**M04-US03 — Complete or cancel a design (Observed).** As a player, I want to
save the accepted design or return it to New Game so that I can use it in a game.
Finish checks balance and credentials, then follows the save or `fDontWrite`
path. Credential/save failure returns to page 1; Cancel returns failure without
a promise that every working byte or prior library write is restored. Read-only
viewing cannot finish a save; M02 owns version/collision/protection decisions.

<a id="m04-us04"></a>
**M04-US04 — Retain a reusable definition (Proposed representation over observed values).**
As a developer, I want race identity, gameplay settings and library version
metadata represented distinctly so that storage and pricing use the right
inputs. Map the [race data](M04-race-data-model.md) into a validated representation,
then reuse the [pricing contract](M04-race-advantage-points.md). Rust/SQL designs
are proposals; running-player progress and binary layout remain S06/S01 concerns.

### Acceptance and traceability

| ID / story / status | Observable criterion | Canonical rules and implementation evidence | Verification or gap |
| --- | --- | --- | --- |
| <a id="m04-ac01"></a>M04-AC01 / US01 / Observed | Given Random is selected, when Custom is chosen, then the Random template bit is cleared and a concrete working design remains. | [Workflow](#workflow-and-completion); `Custom` in [race model](dafny/M04-race-model.dfy); `RaceWizardDlg1`. | Existing `test_race.c` Custom regression; broader control transitions remain unaudited. |
| <a id="m04-ac02"></a>M04-AC02 / US02 / Observed | Given the complete golden input, when current binary64 pricing runs, then innate habitability is 1470287 and displayed balance is 541. | [Pricing acceptance](M04-race-advantage-points.md#acceptance-and-evidence); `FInnate`, `FPrice`, `Balance` in [pricing model](dafny/M04-race-pricing.dfy). | Native rounding witness; formal integer subtotal is conditional on the supplied innate result, not an end-to-end C proof. |
| <a id="m04-ac03"></a>M04-AC03 / US02 / Observed | Given internal total -2, when divided/narrowed for display, then returned balance is zero and the point gate permits continuation; total -3 returns -1 and blocks it. | [Final balance](M04-race-advantage-points.md#research-starting-technology-and-final-balance); `Balance`, race model `Finish`. | Defined arithmetic/transition; end-to-end boundary UI tests remain a gap. |
| <a id="m04-ac04"></a>M04-AC04 / US03 / Observed | Given editable mode and accepted balance/credentials, when Finish uses `fDontWrite`, then it can return success without saving; read-only mode cannot complete a save. | [Workflow](#workflow-and-completion); `Finish`; `RaceCreationWizard`. | Source/model; library publication is a separate M02 obligation. |
| <a id="m04-ac05"></a>M04-AC05 / US03 / Observed | Given failed credentials or cancelled/failed saving, when the wizard handles the result, then it returns to page 1; Cancel itself returns failure. | [Recovery](#cancellation-errors-and-recovery); `Finish`, `Cancel`. | Source-backed transitions; full cancellation/restore audit explicitly missing. |
| <a id="m04-ac06"></a>M04-AC06 / US04 / Proposed | Given identical pricing fields and changed identity/version metadata, when a proposed store/calculator compares price inputs, then no pricing change is inferred from metadata alone. | [Data](M04-race-data-model.md#scope-and-inputs); `FSamePriceInputs`, `MetadataIndependent`. | Formal property under its inputs; no delivered Rust/SQL implementation. |
| <a id="m04-ac07"></a>M04-AC07 / US01, US02 / Observed correction; Proposed safe boundary | Given an ordinary axis (10,99,10), when normalizing, then obtain (10,10,10); correction permits it but safe pricing rejects it. An even-width (10,99,12) normalizes to (10,11,12) and passes the width check. | `NormalizeAxis`, `FAxis`, `NormalizationWidthVectors`; `BoundsCheckPlayer` in race.c. | Formal witnesses; does not claim a runtime zero-width rejection or safe result from undefined division. |

### Interaction and quality requirements

| ID / status | Requirement, check and target | Owner / evidence / remaining decision |
| --- | --- | --- |
| <a id="m04-ir01"></a>M04-IR01 / Observed | Present the visible page order, current balance and the distinct point/credential/save outcomes in the linked workflow. Check page return and read-only/`fDontWrite` modes independently. | M04 dialogs and M02 save path; no new error/rollback protocol. |
| <a id="m04-ir02"></a>M04-IR02 / Proposed | Allow all race controls, including habitat ranges and immunity, to be operated by keyboard with visible focus and readable value/unit labels. A negative balance or invalid field has a non-color cue. | M04/S05; control-level accessibility and minimum-display audit missing. Preserve existing mouse behavior until separately changed. |
| <a id="m04-qr01"></a>M04-QR01 / Observed | Preserve UTF-8 identity byte limits, trait/storage widths and exact pricing operation order. Use multibyte boundary cases and the binary64 witness as conformance checks. | [Data companion](M04-race-data-model.md#story-linked-requirements), [pricing companion](M04-race-advantage-points.md#story-linked-requirements); S01 owns bytes. |
| <a id="m04-qr02"></a>M04-QR02 / Proposed | A new calculator validates positive ordinary habitat widths before pricing and reports normalization explicitly. Check zero-width rejection separately from observed C correction. | `FPricing`/`FAxis` in race model; integration missing. Current undefined division is not an accepted numerical outcome. |
| <a id="m04-qr03"></a>M04-QR03 / Open | Measure edit-to-balance latency across immunity masks, habitat extremes and trait combinations on a named platform; report median/p95 and failures. Target latency and accessible display profiles remain unagreed. | M04/S05; existing unit timing is not an interactive latency guarantee. |

The two companions link these stories and specialize their criteria. Remaining
policy, UI coverage and persistence decisions are owned in [Evidence and open
questions](#evidence-and-open-questions), including the protected-legacy-race conflict.

Checks for this story expansion are recorded in the
[collection verification](README.md#story-expansion-verification).

## Purpose and scope

Build a reusable race design through identity, primary and lesser traits,
habitability, economy and research choices, with advantage-point accounting.
The detailed rules are owned by two companion specifications:

- [Race data model](M04-race-data-model.md): all fields, bounds, meanings, trait
  mappings and proposed Rust/PostgreSQL representations.
- [Race advantage points](M04-race-advantage-points.md): normalization, ordered
  binary64 habitat integration, every price and interaction, and acceptance.

Keep pricing rules in the companion document rather than copying them here.
Binary record layout and trait effects during play are outside this scope.

## Inputs, outputs and owned state

Observed working state is global `vplr` (`PLAYER`) plus password input and wizard
presentation state. `RaceCreationWizard(hwndParent,fReadOnly,fDontWrite)` returns
success/failure; it does not currently expose a standalone immutable design
request. A completed design may be persisted or returned for its caller without
writing when fDontWrite is true. Race library version identity and save/reuse
decisions belong to
[M02 Race Library](M02-race-library.md#33-find-versions-and-choose-a-save-target).
Normalization can mutate the design during point calculation and display.
Sources: `RaceCreationWizard`, `DrawRaceAdvantagePoints` in
[raceui.c](../src/ui/raceui.c), `CAdvantagePoints` in [race.c](../src/core/race.c).

## Workflow and completion

Observed visible order differs from dialog function numbering:

| Step | Options | Source dialog |
| --- | --- | --- |
| 1 | Identity, preset/custom, portrait, credentials, leftover allocation | RaceWizardDlg1 |
| 2 | Primary trait | RaceWizardDlg4 |
| 3 | Lesser traits | RaceWizardDlg5 |
| 4 | Habitat, immunity and growth | RaceWizardDlg2 |
| 5 | Population, factories and mines | RaceWizardDlg3 |
| 6 | Research costs and higher starting technology | RaceWizardDlg6 |

Back/Next navigate these steps. Finish recalculates the signed balance; a
negative returned value alerts and returns to the current page. It verifies
credentials, then either saves through the current library race / standalone
save path or, with fDontWrite, returns success without saving. Read-only mode
cannot finish a save. Cancel/dialog failure returns failure. Selecting Custom
clears the Random template bit. Source: `RaceCreationWizard`, `RaceWizardDlg1`
in [raceui.c](../src/ui/raceui.c).

The historical calculation notes say fDontWrite is unused; **current source
consults it**. Preserve the current behavior when implementing a new workflow.
The historical document is not the current API contract.

## Rules and validation

The [data model](M04-race-data-model.md) defines legal logical inputs. The
[point algorithm](M04-race-advantage-points.md) defines observed correction behavior,
integer truncation, floating boundaries and finish acceptance. A proposed new
port should explicitly report normalization and invalid zero-width habitats,
rather than treating C undefined behavior as a reproducible numeric contract.
Metadata never directly changes the calculated point balance.

## Cancellation, errors and recovery

Observed outcomes in `RaceCreationWizard`: cancellation and dialog failure
return false, credential failure returns to step 1, failed/cancelled saving
returns to step 1, and insufficient points returns to the active step.
The working race is shared mutable state; this specification does not claim
that cancellation restores its original bytes. Library transaction guarantees
and overwrite protection are documented in [M02](M02-race-library.md); M04 still
needs the complete control/snapshot handoff audit.
Source: [raceui.c](../src/ui/raceui.c), [library ADR](../docs/adr/0001-desktop-managed-libraries.md).

## Interactions and proposed ownership

| Neighbor | Responsibility / handoff |
| --- | --- |
| M02 Race Library | Version selection, protected design lifecycle and persistence destination |
| M03 New Game Wizard | Select/customize a race for the roster |
| M05 Universe Creation | Consume final design, generate Random templates and apply leftover bonus |
| S01 Persistence | Read/write validation, encoding and credential import boundaries |
| S03 Static Data | Predefined race templates and labels |
| Gameplay modules | Apply racial abilities during play |

These are logical ownership proposals following the [module map](README.md),
not evidence of isolated code modules. Sources: [library ADR](../docs/adr/0001-desktop-managed-libraries.md),
`GenerateWorld` in [create.c](../src/core/create.c),
[predefined races](../data/races/predefined.yaml).

## Observable acceptance criteria

1. Reproducing the golden race in the pricing spec returns innate 1470287 and
   balance 541 using current binary64 arithmetic.
2. Zero or positive returned balance permits finish past the point gate;
   negative balance alerts and returns to the active wizard page.
3. Selecting Custom after Random clears bit 30 and retains a concrete design.
4. Ordinary habitat normalization bounds endpoints and recomputes the midpoint;
   it can retain zero, odd or even width. The proposed safe pricing boundary
   rejects zero width. See M04-AC07 for the observed/safe-boundary distinction.
5. Changing identity, portrait, credentials or allocation alone leaves price
   unchanged. Every pricing input in the data model can be retained in Rust/SQL.

Existing evidence includes the binary64 witness and high-bit/Custom regressions
in [test_race.c](../tests/unit/test_race.c). Criteria 2, 4 and 5 require broader
workflow/port coverage; listing them does not claim a new test has passed.

## Evidence and open questions

Owning sources: [race.c](../src/core/race.c), [raceui.c](../src/ui/raceui.c),
[planet.c](../src/core/planet.c), [enums.h](../src/core/enums.h),
[structs.h](../src/core/structs.h), [test_race.c](../tests/unit/test_race.c).
Historical context: [calculation notes](../docs/RACE-WIZARD-CALCULATIONS.md),
[current binary64 contract](../docs/DOUBLE-PRECISION.md).

Remaining work:

- Audit each control transition, keyboard/mouse behavior, preset replacement,
  AR economy resets and normalization timing as a full M04 workflow spec.
- Specify exact cancellation/restore guarantees with M02/M03; shared state
  means rollback cannot be assumed.
- Reconcile protected legacy import policy in the separate M02/S01 work.
  [ADR 0002](../docs/adr/0002-utf8-application-text.md) and `FWasRaceFile` describe
  replacement credentials; the module overview records the policy conflict.
- Validate the proposed SQL DDL against PostgreSQL and implement/differentially
  test a Rust calculator before describing either as a delivered interface.

Verification for this integration is limited to source review, documentation
links, formula/constant checks, Rust example compilation and whitespace checks. Earlier report test runs
are historical evidence, not checks rerun for this integration. No runtime,
baseline, file-format or gameplay changes are made.
