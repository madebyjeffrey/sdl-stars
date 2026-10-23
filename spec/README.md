# SDL-Stars module overview

Status: initial proposal for discussion and progressive specification.
Recorded: 2026-10-23.
Base: `develop` at `4ba1ce856b17f499a0238c968c7d0329bbde1ddf`.

## Purpose and scope

Identify the high-level logical modules of SDL-Stars before breaking each one
into detailed workflows, rules, and acceptance criteria. Start with the New Game
Wizard and Race Design Wizard, then work through the in-game areas.

A **module** has an interface and an implementation. Its interface includes the
inputs, outputs, state assumptions, ordering constraints, and failure behavior a
caller needs to understand. For this map, modules group coherent player or host
decisions and outcomes; their implementations may span core and UI files.

The module ownership below is proposed. The source pointers are starting points
for investigation, not proof that each module already has a clean interface.
This document records the first exploration, not a complete behavioral audit,
implementation plan, or approval to refactor the game.

## Existing technical organization

[CMakeLists.txt](../CMakeLists.txt) defines the core and desktop source groups and
the command-line host. The [roadmap](../docs/ROADMAP.md) describes their separation.

| Existing layer | Role |
| --- | --- |
| `src/core` | Shared game rules, turn generation, AI, and game-file handling. |
| `src/ui` | Desktop workflows, dialogs, drawing, and window state. |
| `src/host` | Command-line host and implementations of the game-to-UI interface for unattended operation. |
| `src/platform` | Native filesystem, timing, formatting, and encoding support. |
| `compat/win32` | SDL-backed implementation of the Win32 interface used by the desktop UI. |
| `data` and `text` | Generated static game data and text catalogs. |

The core reaches the player through [ui.h](../src/core/ui.h). The desktop uses
SDL3 through the Win32 wrapper; the command-line host remains independent of SDL.
These technical layers and the functional modules below answer different
questions: where code runs, and which behavior it owns.

## Proposed first-level module map

### Before entering a game

| ID | Module | Responsibility | Initial source pointers |
| --- | --- | --- | --- |
| M01 | Game Library | Find, organize, import, export, duplicate, rename, and delete managed games; choose a player turn or resume a session. | [libraryui.c](../src/ui/libraryui.c), [libraryzip.c](../src/ui/libraryzip.c), [library ADR](../docs/adr/0001-desktop-managed-libraries.md) |
| M02 | Race Library | Manage reusable race designs, saved versions, imports, exports, and protected designs. | [libraryui.c](../src/ui/libraryui.c), [raceui.c](../src/ui/raceui.c), [library ADR](../docs/adr/0001-desktop-managed-libraries.md) |
| M03 | New Game Wizard | Assemble game settings, player roster, race selections, and victory conditions into a valid game-creation request. | [createui.c](../src/ui/createui.c): `NewGameWizard`; [create.h](../src/core/create.h) |
| M04 | Race Design Wizard | Build a valid race through identity, traits, habitability, economy, research choices, and advantage-point accounting. | [raceui.c](../src/ui/raceui.c): `RaceCreationWizard`; [race.c](../src/core/race.c), [calculation notes](../docs/RACE-WIZARD-CALCULATIONS.md) |
| M05 | Universe Creation | Turn completed setup into planets, homeworlds, players, starting fleets, and initial game files. | [create.c](../src/core/create.c): `GenerateWorld`, `GenNewGameFromFile`, `CreateTutorWorld` |

The proposed distinction between M03 and M05 is setup versus generation. The
wizard owns editing and validating the setup; universe creation owns what the
accepted settings produce. This is a desired logical division, not a claim that
the current wizard returns a standalone request object.

M04 produces a reusable race design; M02 owns its library lifecycle. The effects
of racial traits during play belong with the gameplay rules that apply them.
Customizing a race from the New Game roster connects these workflows without
making them a single module.

### Inside a player turn

| ID | Module | Responsibility | Initial source pointers |
| --- | --- | --- | --- |
| M06 | Player Session and Turn Orders | Open a player turn, track pending changes, save or submit orders, switch players, and handle an incoming turn. | [file.c](../src/core/file.c), [save.c](../src/core/save.c), [log.c](../src/core/log.c), [fileui.c](../src/ui/fileui.c), [libraryui.c](../src/ui/libraryui.c), [mdi.c](../src/ui/mdi.c) |
| M07 | Galaxy Map and Navigation | Explore the map, select objects, search, zoom, filter, and navigate to detailed views. | [scan.c](../src/ui/scan.c), [scaninspect.c](../src/ui/scaninspect.c), [tb.c](../src/ui/tb.c) |
| M08 | Planet and Colony Management | Inspect planets and manage population, habitability, minerals, installations, defenses, and terraforming. | [planet.c](../src/core/planet.c), [planetui.c](../src/ui/planetui.c), [planetpaneui.c](../src/ui/planetpaneui.c) |
| M09 | Production Management | Edit production queues and presets; explain costs, constraints, and expected completion; connect planned production with its resolution. | [produce.c](../src/core/produce.c), [produceui.c](../src/ui/produceui.c), [turn2.c](../src/core/turn2.c): `Produce`, `CBuildProdItem` |
| M10 | Fleet Operations | Manage composition, cargo, fuel, routes, waypoints, tasks, splitting, merging, and transfers. | [ship.c](../src/core/ship.c), [ship2.c](../src/core/ship2.c), [shipui.c](../src/ui/shipui.c), [ship2ui.c](../src/ui/ship2ui.c), [turn.c](../src/core/turn.c), [turn3.c](../src/core/turn3.c) |
| M11 | Ship and Starbase Design | Assemble and validate designs from hulls and parts, calculate their properties, and manage the design roster. | [build.c](../src/core/build.c), [buildui.c](../src/ui/buildui.c), [parts.c](../src/core/parts.c) |
| M12 | Research and Technology | Set priorities and allocations; inspect progress, requirements, and available equipment. | [research.c](../src/core/research.c), [researchui.c](../src/ui/researchui.c), [parts.c](../src/core/parts.c), [turn2.c](../src/core/turn2.c): `UpdateResearchStatus` |
| M13 | Player Relations | Set relationships with other players and explain their consequences for cooperation and conflict. | [battleui.c](../src/ui/battleui.c): `RelationsDlg`; rules spread across core gameplay code |
| M14 | Battle Planning and Review | Configure battle plans, inspect results, and replay recorded battles. Planning and review are likely separate submodules later. | [battleui.c](../src/ui/battleui.c): `BattlePlansDlg`; [vcr.c](../src/ui/vcr.c), [battle.c](../src/core/battle.c) |
| M15 | Messages and Reports | Review turn events, player messages, planet/fleet summaries, scores, and exported reports; navigate to relevant objects. | [msg.c](../src/core/msg.c), [msgui.c](../src/ui/msgui.c), [report.c](../src/core/report.c), [reportui.c](../src/ui/reportui.c) |
| M16 | Tutorial and Help | Guide the player through tasks and provide contextual explanations and reference material. | [tutor.c](../src/ui/tutor.c), [tutor2.c](../src/ui/tutor2.c), [tutorial text](../text/tutorial.txt), [help implementation notes](../docs/WIN32-HELP.md) |

The map and reports present information supplied by other modules. They should
not become independent owners of colony, fleet, research, or combat rules merely
because they display those rules' results.

### Hosting and simulation

| ID | Module | Responsibility | Initial source pointers |
| --- | --- | --- | --- |
| M17 | Host Game Management | Collect and validate orders, show readiness, configure generation, and manage generated turns and backups. | [libraryui.c](../src/ui/libraryui.c), [host.c](../src/host/host.c), [turn.c](../src/core/turn.c): `VerifyTurns`, `CFindTurnsOutstanding`, `FRunCmdLine`; [hosting notes](../docs/TOTALHOST.md) |
| M18 | Turn Generation | Advance the game by one year and coordinate the ordered phases and their interactions. | [turn.c](../src/core/turn.c): `FGenerateTurn`; [turn2.c](../src/core/turn2.c), [turn3.c](../src/core/turn3.c) |
| M19 | Game Rules and Resolution | Resolve movement, production, growth, mining, research, combat, colonization, minefields, packets, and universe events. This is a provisional group to decompose. | [turn.c](../src/core/turn.c), [turn2.c](../src/core/turn2.c), [turn3.c](../src/core/turn3.c), [battle.c](../src/core/battle.c), [mine.c](../src/core/mine.c), [thing.c](../src/core/thing.c) |
| M20 | AI Players | Evaluate their situation and choose designs, research, production, fleet orders, and other actions. | [ai.c](../src/core/ai.c), [ai2.c](../src/core/ai2.c), [ai3.c](../src/core/ai3.c), [ai4.c](../src/core/ai4.c), [aiutil.c](../src/core/aiutil.c) |

M18 owns sequencing. Individual gameplay modules should own their rules. M19
keeps simulation responsibilities visible during this first pass; it is not a
proposal for a second copy of every rule. For example, production queue editing
and production resolution are related parts of M09, exercised at different times.
Desktop hosting and the command-line host are different entry points into the
shared simulation; their interaction requirements need separate treatment.

### Shared support

| ID | Module | Responsibility | Initial source pointers |
| --- | --- | --- | --- |
| S01 | Game-file Persistence and Validation | Read, validate, and write game, turn, order, history, and race records; enforce format and text contracts. | [file.c](../src/core/file.c), [save.c](../src/core/save.c), [log.c](../src/core/log.c), [structs.h](../src/core/structs.h), [encoding contracts](../docs/UNICODE-BOUNDARIES.md) |
| S02 | Application Preferences and Window Layout | Maintain application preferences, window arrangement, and session presentation. | [init.c](../src/ui/init.c), [mdi.c](../src/ui/mdi.c), [nativeui.c](../src/ui/nativeui.c) |
| S03 | Static Game Data and Text | Supply hulls, parts, race presets, AI recipes, messages, names, and tutorial text with stable identifiers. | [data generator](../data/datagen.py), [text generator](../text/textgen.py), [strings.c](../src/core/strings.c) |
| S04 | Platform Services | Provide filesystem, timing, formatting, and encoding behavior across operating systems. | [native.c](../src/platform/native.c), [filesystem.cpp](../src/platform/filesystem.cpp), [encoding.c](../src/platform/encoding.c) |
| S05 | SDL Desktop Implementation | Implement windows, controls, input, rendering, dialogs, and other desktop facilities used by the UI. | [compat/win32](../compat/win32), [SDL development guide](../docs/SDL3-DEVELOPMENT.md) |

## Findings and constraints to carry into detailed specifications

### Library and session vocabulary

Use the [root glossary](../GLOSSARY.md): a race design differs from a player turn;
a managed game can contain turns for several players; a library name differs
from the stored game title and exchange basename. These distinctions affect
imports, exports, rename operations, and session selection.

[ADR 0001](../docs/adr/0001-desktop-managed-libraries.md) records managed game and
race libraries, explicit imports, race versions, player selection, and separate
host management. Its rules are existing decisions to consult during breakdown.

### Wizard evidence

[Gameplay qualification](../docs/GAMEPLAY-QUALIFICATION.md) describes exercised
three-page New Game workflows, Back/Next/Finish, cancellation, and creation of
universe, host, and player files. It also records workflow coverage for designs,
fleets, research, production, reports, relations, and saving. This is documented
evidence from earlier qualification, not a test run performed for this overview.

The [race calculation notes](../docs/RACE-WIZARD-CALCULATIONS.md) describe six
visible steps: identity; primary trait; lesser traits; habitat and growth;
economy; and research. They distinguish wizard pricing from the later gameplay
effects of racial traits. The notes are explicitly historical: their arithmetic
analysis concerns pre-migration extended intermediates. Current production uses
binary64 as documented in [DOUBLE-PRECISION.md](../docs/DOUBLE-PRECISION.md).
Revalidate detailed formulas and state transitions against current source before
promoting historical descriptions into current specifications.

### Information and order ownership

Distinguish authoritative universe state, information known to a player,
historical observations, and estimates. Map, reports, and detailed views need
consistent visibility and stale-information rules. Whether this needs a distinct
Intelligence and Visibility module remains open.

Distinguish editing an order, saving it, submitting it, exporting it, and having
the host execute it. Cancellation and failure need explicit outcomes for each
stage. Library operations, the active player session, and persistence all
participate; their final ownership must be established during detailed work.

### Compatibility and determinism

Carry the root [repository rules](../AGENTS.md) into every detailed spec. Saved
record widths remain fixed, text uses UTF-8 byte budgets, and current game writes
use format 2.85. Sorting, RNG use, floating operation boundaries, and phase order
can affect generated turns. Documenting current behavior is separate from
proposing an intentional behavior change and its regression-baseline update.

There is an existing documentation conflict to resolve before specifying legacy
protected-race import: root AGENTS.md says protected legacy races are unsupported,
while [ADR 0002](../docs/adr/0002-utf8-application-text.md) describes importing them
with a confirmed replacement password. This overview does not choose a new
policy or revise either document. The detailed race-import work must reconcile
the instructions, accepted decision, and current implementation explicitly.

## Open decomposition questions

- Should Intelligence and Visibility become its own module shared by map,
  reports, AI, and turn generation?
- Where should colony management end and production management begin,
  particularly for terraforming, defenses, and mineral accounting?
- Should Research Planning and the Technology Browser become separate modules?
- Should battle plans, combat resolution, and battle replay be three explicit
  modules, with a shared recorded-battle contract?
- Should messages, reports, scoring, and victory evaluation be split? Scores are
  displayed through reports but calculated during generation; victory conditions
  are configured during game creation.
- How should minefields, packets, wormholes, salvage, and random events be grouped
  when M19 is decomposed?
- Which session transitions own unsaved-order prompts and import conflicts, and
  which operations are delegated to libraries and persistence?
- Which AI decisions reuse the same validation rules as human orders, and where
  does the implementation currently differ?

These questions mark future investigation; they do not block this first-level map.

## Outline for each detailed module specification

1. **Purpose and scope:** user or host goal, included responsibilities, and named
   neighboring modules that own related behavior.
2. **Inputs, outputs, and owned state:** persistent data, working state, derived
   values, player-visible knowledge, and interface assumptions.
3. **User actions and workflow:** entry points, actions, navigation, transitions,
   completion, and automated or host entry points where applicable.
4. **Rules, calculations, and validation:** constraints, formulas, rounding,
   ordering, prerequisites, and shared rules referenced by name.
5. **Cancellation, errors, and recovery:** state preserved or committed, partial
   failure handling, retry behavior, and user-visible outcomes.
6. **Interactions with other modules:** responsibility handoffs and dependencies,
   keeping planning, execution, presentation, and persistence explicit.
7. **Observable acceptance criteria:** concrete examples and existing test
   evidence; identify coverage gaps without claiming they pass.
8. **Evidence and open questions:** inspected revision, source/function links,
   relevant ADRs, known bugs, proposed changes, and unresolved decisions.

## Suggested next steps

First review and refine this map. Then specify M03 New Game Wizard and M04 Race
Design Wizard to establish the level of detail and document format. Continue
with the player session and in-game modules, decomposing M19 alongside the rules
owned by each gameplay area. Add focused documents only as those modules are
worked on; retain this document as the overview and index.
