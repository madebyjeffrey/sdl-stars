# SDL-Stars module overview

Status: source-derived detailed module collection; logical ownership remains proposed.
Recorded: 2026-10-23.
Base: `develop` at `4ba1ce856b17f499a0238c968c7d0329bbde1ddf`.

## Consolidated evidence after the C++ migration

The unpublished M01/M02/M04/S03/S04 continuations were reconciled with
`develop` at `b6491ed2` on 2026-10-31. Their observations and dated runs below
refer to C11 source at `e3222eb1`; they do not qualify the current C++ bodies.
Fourteen newly preserved fixture commands use [the historical runner](../tests/spec/historical_c11.py)
to materialize source, fixtures and models from immutable commit `82679dc`.
They never extract or rewrite current production. Existing linked C++ witnesses,
current strict compiler policy and C++ qualification remain in place. Porting
these additional historical cases to linked C++ tests remains open.

## Purpose and scope

Identify the high-level logical modules of SDL-Stars and progressively specify
their actors, goals, workflows, requirements, rules and acceptance evidence.
All 26 module-map entries now have detailed specifications, including stable user
stories, acceptance criteria, interaction/quality requirements, source evidence,
canonical Dafny definitions and explicitly owned coverage limits. Technical companions specialize shared contracts, gameplay algorithms and versioned data.

The [completeness review and completion backlog](#completeness-review-and-completion-backlog)
separates documented modules from remaining rule/model work, runtime qualification
and policy decisions. The completion goal is active; the collection is not yet
a complete executable behavioral specification.

Module documents and Dafny files begin with their uppercase module ID, such as
`M05-universe-creation.md` or `S01-game-file-formats.md`. `README.md` and
`AGENTS.md` cover the whole specification collection.

A **module** has an interface and an implementation. Its interface includes the
inputs, outputs, state assumptions, ordering constraints, and failure behavior a
caller needs to understand. For this map, modules group coherent player or host
decisions and outcomes; their implementations may span core and UI files.

The module ownership below is proposed. The source pointers are starting points
for investigation, not proof that each module already has a clean interface.
The original map records the first exploration; detailed entries now provide
source-backed coverage with explicit proof and test boundaries. This is not
approval to refactor the game or a claim of exhaustive C/UI verification.

## Linked witness evidence update (Milestone C)

The 27 existing witness commands now run normal linked production targets.
[The C05 inventory](../docs/CPP-C05-WITNESSES.md) maps every command, original
case/domain/exclusion/oracle and current fixture to its specification owners.
Earlier run records below describe their historical extraction mechanism;
the C05 inventory supersedes that mechanism while retaining the recorded
independent expectations and coverage limits. Canonical Dafny rules are unchanged.

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
| M01 | [Game Library](M01-game-library.md) | Find, organize, import, export, duplicate, rename, and delete managed games; choose a player turn or resume a session. | [libraryui.cpp](../src/ui/libraryui.cpp), [libraryzip.cpp](../src/ui/libraryzip.cpp), [library ADR](../docs/adr/0001-desktop-managed-libraries.md) |
| M02 | [Race Library](M02-race-library.md) | Manage reusable race designs, saved versions, imports, exports, and protected designs. | [libraryui.cpp](../src/ui/libraryui.cpp), [raceui.cpp](../src/ui/raceui.cpp), [library ADR](../docs/adr/0001-desktop-managed-libraries.md) |
| M03 | [New Game Wizard](M03-new-game-wizard.md) | Assemble game settings, player roster, race selections, and victory conditions into a valid game-creation request. | [createui.cpp](../src/ui/createui.cpp): `NewGameWizard`; [create.h](../src/core/create.h) |
| M04 | [Race Design Wizard](M04-race-design.md) | Build a valid race through identity, traits, habitability, economy, research choices, and advantage-point accounting. | [raceui.cpp](../src/ui/raceui.cpp): `RaceCreationWizard`; [race.cpp](../src/core/race.cpp), [calculation notes](../docs/RACE-WIZARD-CALCULATIONS.md) |
| M05 | [Universe Creation](M05-universe-creation.md) | Turn completed setup into planets, homeworlds, players, starting fleets, and initial game files. | [create.cpp](../src/core/create.cpp): `GenerateWorld`, `GenNewGameFromFile`, `CreateTutorWorld` |

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
| M06 | [Player Session and Turn Orders](M06-player-session-and-turn-orders.md) | Open a player turn, track pending changes, save or submit orders, switch players, and handle an incoming turn. | [file.cpp](../src/core/file.cpp), [save.cpp](../src/core/save.cpp), [log.cpp](../src/core/log.cpp), [fileui.cpp](../src/ui/fileui.cpp), [libraryui.cpp](../src/ui/libraryui.cpp), [mdi.cpp](../src/ui/mdi.cpp) |
| M07 | [Galaxy Map and Navigation](M07-galaxy-map-and-navigation.md) | Explore the map, select objects, search, zoom, filter, and navigate to detailed views. | [scan.cpp](../src/ui/scan.cpp), [scaninspect.cpp](../src/ui/scaninspect.cpp), [tb.cpp](../src/ui/tb.cpp) |
| M08 | [Planet and Colony Management](M08-planet-and-colony-management.md) | Inspect planets and manage population, habitability, minerals, installations, defenses, and terraforming. | [planet.cpp](../src/core/planet.cpp), [planetui.cpp](../src/ui/planetui.cpp), [planetpaneui.cpp](../src/ui/planetpaneui.cpp) |
| M09 | [Production Management](M09-production-management.md) | Edit production queues and presets; explain costs, constraints, and expected completion; connect planned production with its resolution. | [produce.cpp](../src/core/produce.cpp), [produceui.cpp](../src/ui/produceui.cpp), [turn2.cpp](../src/core/turn2.cpp): `Produce`, `CBuildProdItem` |
| M10 | [Fleet Operations](M10-fleet-operations.md) | Manage composition, cargo, fuel, routes, waypoints, tasks, splitting, merging, and transfers. | [ship.cpp](../src/core/ship.cpp), [ship2.cpp](../src/core/ship2.cpp), [shipui.cpp](../src/ui/shipui.cpp), [ship2ui.cpp](../src/ui/ship2ui.cpp), [turn.cpp](../src/core/turn.cpp), [turn3.cpp](../src/core/turn3.cpp) |
| M11 | [Ship and Starbase Design](M11-ship-and-starbase-design.md) | Assemble and validate designs from hulls and parts, calculate their properties, and manage the design roster. | [build.cpp](../src/core/build.cpp), [buildui.cpp](../src/ui/buildui.cpp), [parts.cpp](../src/core/parts.cpp) |
| M12 | [Research and Technology](M12-research-and-technology.md) | Set priorities and allocations; inspect progress, requirements, and available equipment. | [research.cpp](../src/core/research.cpp), [researchui.cpp](../src/ui/researchui.cpp), [parts.cpp](../src/core/parts.cpp), [turn2.cpp](../src/core/turn2.cpp): `UpdateResearchStatus` |
| M13 | [Player Relations](M13-player-relations.md) | Set relationships with other players and explain their consequences for cooperation and conflict. | [battleui.cpp](../src/ui/battleui.cpp): `RelationsDlg`; rules spread across core gameplay code |
| M14 | [Battle Planning and Review](M14-battle-planning-and-review.md) | Configure battle plans, inspect results, and replay recorded battles. Planning and review are likely separate submodules later. | [battleui.cpp](../src/ui/battleui.cpp): `BattlePlansDlg`; [vcr.cpp](../src/ui/vcr.cpp), [battle.cpp](../src/core/battle.cpp) |
| M15 | [Messages and Reports](M15-messages-and-reports.md) | Review turn events, player messages, planet/fleet summaries, scores, and exported reports; navigate to relevant objects. | [msg.cpp](../src/core/msg.cpp), [msgui.cpp](../src/ui/msgui.cpp), [report.cpp](../src/core/report.cpp), [reportui.cpp](../src/ui/reportui.cpp) |
| M16 | [Tutorial and Help](M16-tutorial-and-help.md) | Guide the player through tasks and provide contextual explanations and reference material. | [tutor.cpp](../src/ui/tutor.cpp), [tutor2.cpp](../src/ui/tutor2.cpp), [tutorial text](../text/tutorial.txt), [help implementation notes](../docs/WIN32-HELP.md) |

The map and reports present information supplied by other modules. They should
not become independent owners of colony, fleet, research, or combat rules merely
because they display those rules' results.

### Hosting and simulation

| ID | Module | Responsibility | Initial source pointers |
| --- | --- | --- | --- |
| M17 | [Host Game Management](M17-host-game-management.md) | Collect and validate orders, show readiness, configure generation, and manage generated turns and backups. | [libraryui.cpp](../src/ui/libraryui.cpp), [host.cpp](../src/host/host.cpp), [turn.cpp](../src/core/turn.cpp): `VerifyTurns`, `CFindTurnsOutstanding`, `FRunCmdLine`; [hosting notes](../docs/TOTALHOST.md) |
| M18 | [Turn Generation](M18-turn-generation.md) | Advance the game by one year and coordinate the ordered phases and their interactions. | [turn.cpp](../src/core/turn.cpp): `FGenerateTurn`; [turn2.cpp](../src/core/turn2.cpp), [turn3.cpp](../src/core/turn3.cpp) |
| M19 | [Game Rules and Resolution](M19-game-rules-and-resolution.md) | Resolve movement, production, growth, mining, research, combat, colonization, minefields, packets, and universe events. This is a provisional group to decompose. | [turn.cpp](../src/core/turn.cpp), [turn2.cpp](../src/core/turn2.cpp), [turn3.cpp](../src/core/turn3.cpp), [battle.cpp](../src/core/battle.cpp), [mine.cpp](../src/core/mine.cpp), [thing.cpp](../src/core/thing.cpp) |
| M20 | [AI Players](M20-ai-players.md) | Evaluate their situation and choose designs, research, production, fleet orders, and other actions. | [ai.cpp](../src/core/ai.cpp), [ai2.cpp](../src/core/ai2.cpp), [ai3.cpp](../src/core/ai3.cpp), [ai4.cpp](../src/core/ai4.cpp), [aiutil.cpp](../src/core/aiutil.cpp) |

M18 owns sequencing. Individual gameplay modules should own their rules. M19
keeps simulation responsibilities visible during this first pass; it is not a
proposal for a second copy of every rule. For example, production queue editing
and production resolution are related parts of M09, exercised at different times.
Desktop hosting and the command-line host are different entry points into the
shared simulation; their interaction requirements need separate treatment.

### Shared support

| ID | Module | Responsibility | Initial source pointers |
| --- | --- | --- | --- |
| S01 | [Game-file Formats, Persistence and Validation](S01-game-file-formats.md) | Specify file families and binary records, map them to shared logical state, and define read/write validation and compatibility. | [file.cpp](../src/core/file.cpp), [save.cpp](../src/core/save.cpp), [log.cpp](../src/core/log.cpp), [structs.h](../src/core/structs.h), [encoding contracts](../docs/UNICODE-BOUNDARIES.md) |
| S02 | [Application Preferences and Window Layout](S02-application-preferences-and-window-layout.md) | Maintain application preferences, window arrangement, and session presentation. | [init.cpp](../src/ui/init.cpp), [mdi.cpp](../src/ui/mdi.cpp), [nativeui.cpp](../src/ui/nativeui.cpp) |
| S03 | [Static Game Data and Text](S03-static-game-data-and-text.md) | Specify the catalogs, schemas, values, units and stable identifiers required by generation and gameplay. | [data generator](../data/datagen.py), [text generator](../text/textgen.py), [strings.cpp](../src/core/strings.cpp) |
| S04 | [Platform Services](S04-platform-services.md) | Provide filesystem, timing, formatting, and encoding behavior across operating systems. | [native.cpp](../src/platform/native.cpp), [filesystem.cpp](../src/platform/filesystem.cpp), [encoding.cpp](../src/platform/encoding.cpp) |
| S05 | [SDL Desktop Implementation](S05-sdl-desktop-implementation.md) | Implement windows, controls, input, rendering, dialogs, and other desktop facilities used by the UI. | [compat/win32](../compat/win32), [SDL development guide](../docs/SDL3-DEVELOPMENT.md) |
| S06 | [Shared Game State and In-memory Data Model](S06-shared-game-state.md) | Define logical entities, identities, relationships, typed values, invariants and state lifetimes shared by the functional modules. | [structs.h](../src/core/structs.h), [enums.h](../src/core/enums.h), [globals.h](../src/core/globals.h), [race model](M04-race-data-model.md) |

S01 and S03 retain their original IDs with expanded scope; S06 is new. These
are specification boundaries, not a requirement to reorganize the C sources.
S01 now has a [detailed persistence specification](S01-game-file-formats.md).
S06 now has a [detailed shared-state specification](S06-shared-game-state.md) and
[Dafny contracts](dafny/S06-shared-state.dfy), checked against
`e2eae18cff0f990db20415c1c2262aed2022f95b` on 2026-10-24. S03 now specifies catalog schemas, stable identities, text generation and
[a complete versioned input inventory](S03-catalog-inventory.md). The local M03/M04 definitions remain canonical for setup and races.

## Shared data contracts

The existing M03 and M04 specifications describe setup and race data but explicitly
exclude binary layouts. S01 now specifies serialized records and workflows;
S03 supplies schemas and precise versioned catalog values. S06 defines the
shared logical model between those boundaries and the gameplay modules.

Keep three representations separate: **serialized files** (S01), **logical
in-memory state** (S06), and **static reference data** (S03). A file record need
not correspond to one logical struct. For example, current `PLAYER` combines race
attributes with runtime player state, while `WriteRtPlr` and `ReadRtPlr` encode
and decode its stored representation. A modern model may separate these concerns
without changing the existing file contract.

### S01: Game-file Formats, Persistence and Validation

The complete source-derived entry point is [Game-file Formats, Persistence and
Validation](S01-game-file-formats.md), checked against
`e2eae18cff0f990db20415c1c2262aed2022f95b` on 2026-10-23. It separates
canonical writer output, observed reader behavior and proposed stricter validation.

| Reference | Coverage |
| --- | --- |
| [Dafny specification](S01-game-file-specification.dfy) | Foundation plus codecs, packed records, orders, file groups, external formats and recovery conditions; explicit proof/dependency scope in S01. |
| [S01 entry](S01-game-file-formats.md) | File families, exact sequencing, companion requirements, state mappings, history/appended turns, compatibility and recovery. |
| [Binary records](S01-game-file-records.md) | Complete current tag inventory, widths/bitfields, packed entity variants, UTF-8/nibble codec, XOR stream, race checksum, battle/message/AI data. |
| [Orders and replay](S01-game-file-orders.md) | All current commands, byte encodings, state effects, generation-only changes, ordering and failure semantics. |
| [External formats](S01-game-file-external.md) | Batch definition lines, report columns, metadata schemas, library publication and ZIP subset. |
| [Validation and acceptance](S01-game-file-validation.md) | Checks by entry point, concrete vectors, actual test evidence and explicit implementation/policy gaps. |

The specification records the protected-legacy-race policy conflict, incomplete
bounded validation, nontransactional saves/replay, and the mismatch between core
multi-year turn support and library import. It does not fix these or change
runtime/file-format behavior. S06 owns the logical types and S03 owns the stable
catalog IDs referenced by these byte mappings.

### S06: Shared Game State and In-memory Data Model

The [shared-state specification](S06-shared-game-state.md) covers the six model areas
below. It distinguishes current C behavior, storage domains and proposed
validated boundaries. Its [Dafny model](dafny/S06-shared-state.dfy) specifies selected
conditions and proves selected consequences; it does not verify the C runtime
or propose a source reorganization.

| Model area | Detailed coverage |
| --- | --- |
| Identity, time and units | Scoped IDs, indices, optional/sentinel adapters, gift-recipient encoding, calendar time, coordinates, cargo, resources, fuel and arithmetic boundaries (§2). |
| Setup and players | Canonical M03/M04 links; mutable progress, relations, plans, templates, counts and design state (§3). |
| Universe entities | Planets/colonies, designs, fleets, cargo, production and waypoint state; references, cardinality, defaults and transitional exceptions (§§3–4). |
| Variant entities and commands | All declared object classes, ten task codes, ten transport actions and four THING cases; salvage and unknown discriminators distinguished (§§4–5). |
| Knowledge and results | Partial/stale/estimated information, ownership, messages, scores, recorded battles and AI memory (§6). |
| Execution and session state | Creation-facing conditions, RNG streams/stack, pending commands, phase-local state, caches, editor copies and destruction (§§7–8). |

[Dafny verification and acceptance](S06-shared-game-state.md#9-formal-verification-and-acceptance)
records the command, proof obligations, positive/negative witnesses and limits.
Logical types are not serialization layouts: S01 owns the adapters and S03 the
catalogs. M03/M04 remain the existing setup/race definitions; gameplay modules
own legal mutations and resolution. Stronger validated-boundary conditions do
not silently change the C loader or the regression baseline.

### S03: Static Game Data and Text

The [S03 entry](S03-static-game-data-and-text.md) specifies generator/schema
boundaries, stable positional IDs, derived fields, AI packing and compressed
text. Its [versioned inventory](S03-catalog-inventory.md) enumerates all 21 ordinary
tables, six recipe tables, preferences/research inputs and text catalogs, with
exact source hashes and generator/schema/embedded-rule references. Source hashes
establish identity, not semantic correctness; native parser/compiler, runtime
formatting and rendering gaps remain explicit. Gameplay consumers own formulas
and effects, S01 saved references, S06 mutable instances.

### Shared contracts needed by M05 Universe Creation

M05 uses the following minimum dependency set. Its detailed specification now
links the shared contracts and pins catalog/helper inputs. The complete S03
inventory now supplements M05’s bounded creation-facing references.

| Dependency | Contract M05 should reference |
| --- | --- |
| M03 / M04 | Accepted setup, roster and race snapshots; defaults and validation; victory settings and leftover-point choices. |
| S06 | Generated game/player/planet/design/fleet/space-object state, identities and references, initial values, units, and ownership before/after creation. |
| S03 / M11 | Race presets, starting hulls/parts/templates, default plans, planet names, required lookup tables, and design calculations. |
| S06 / S04 | RNG algorithm and full entry state, draw order, clock-derived identity and tutorial/batch differences. A seed alone is not the complete reproducibility contract. |
| S01 / M01 | Initial `.xy`, `.hst` and `.mN` outputs, completeness checks, and what survives a failed write or library finalization. |

The generation rules now live in [M05 Universe Creation](M05-universe-creation.md),
with a bounded, versioned catalog/helper contract. M03 §4.7 retains a handoff
link; S03 remains owner of the full catalog specification. M05 acceptance
examples should check both logical initial state and the corresponding decoded file outputs; deterministic tests
must control clock and RNG inputs as well as catalog versions.

## Detailed specifications

| Module | Entry point | Supporting specifications | Coverage |
| --- | --- | --- | --- |
| M01 | [Game Library](M01-game-library.md) | [Dafny contracts](M01-library-specification.dfy), [import publication/recovery](M01-import-publication.md), shared names, S01 persistence and M06 session handoffs | Listing/Continue, new-game publication, imports/conflicts, copy/rename/delete, export and host orders; partial-failure boundaries explicit. |
| M02 | [Race Library](M02-race-library.md) | [Dafny contracts](M02-library-specification.dfy), shared names, M04 design and S01 bytes | Destination binding, stored-name version scans, reuse/update/new-version decisions, import/duplicate/export/delete and legacy policy conflict. |
| M03 | [New Game Wizard](M03-new-game-wizard.md) | Setup model, roster/defaults, victory controls, scoring and M05 handoff | Source-derived workflow and calculations, dependency boundaries, port guidance and acceptance vectors. |
| M04 | [Race Design Wizard](M04-race-design.md) | [Race data model and storage mapping](M04-race-data-model.md); [Advantage-point calculation](M04-race-advantage-points.md); [Wizard control](M04-wizard-controller.md); [Page commands](M04-page-commands.md); [Initialization](M04-page-initialization.md); [Pointer/messages](M04-pointer-and-message-interactions.md) | Logical inputs, pricing, dispatch, command/initialization, mouse/cursor/tracking and color/erase specified; complete painting/helper/native extraction remains open. |
| M05 | [Universe Creation](M05-universe-creation.md) | M03/M04 inputs, S06 state, S01 files and versioned S03/M11 dependencies | Source-derived placement, starting assets, RNG/order, tutorial/batch paths, publication/failure behavior and acceptance matrix. |
| M06 | [Player Session and Turn Orders](M06-player-session-and-turn-orders.md) | Player selection, pending orders, saving, export and incoming turns | Source-derived workflows, partial-failure boundaries and modeled session transitions. |
| M07 | [Galaxy Map and Navigation](M07-galaxy-map-and-navigation.md) | Selection, search, zoom/pan, filters, gesture handoffs and semantic inspection | Source-derived navigation rules, formal properties and selected scanner workflow evidence. |
| M08 | [Planet and Colony Management](M08-planet-and-colony-management.md) | [Dafny contracts](M08-colony-specification.dfy), [native economy](M08-colony-economy.md), [mass-driver capability](M08-mass-driver-capability.md), [terraforming selection](M08-terraform-selection.md), [capability producers](M08-colony-capabilities.md), shared state, arithmetic and persistence | Colony selection, native capacity/output/installations, growth/budget handoffs, terraforming, orbital facilities and pane behavior; mining/refinement and platform boundaries explicit. |
| M09 | [Production Management](M09-production-management.md) | [Dafny contracts](M09-production-specification.dfy), [native cost vectors](M09-production-costs.md), [forecast controller](M09-production-forecast.md), [builder](M09-production-builder.md), [host handoffs](M09-production-execution.md), shared state, arithmetic and persistence | Queue editing/templates, commit scope, cost/forecast controllers and native builder steps; outer host application and refinement remain open. |
| M10 | [Fleet Operations](M10-fleet-operations.md) | [Dafny contracts](M10-fleet-specification.dfy), shared state, arithmetic and persistence | Fleet orders, cargo, split/merge, overflow, fuel and task-resolution handoffs; host simulation and packed-damage gaps explicit. |
| M11 | [Ship and Starbase Design](M11-ship-and-starbase-design.md) | [Dafny contracts](M11-design-specification.dfy), shared state, arithmetic and persistence | Roster/copy/edit/delete, availability, slots and cost/armor projections; catalog and cascading deletion boundaries explicit. |
| M12 | [Research and Technology](M12-research-and-technology.md) | [Dafny contracts](M12-research-specification.dfy), [native development/budget quotes](M12-development-and-budget-quotes.md), [host research](M12-host-research-resolution.md) | Research policy, ordered development quote/native cost, budget preview, technology browser and host allocation/breakthrough handoffs; shared costs retained. |
| M13 | [Player Relations](M13-player-relations.md) | [Dafny contracts](M13-relations-specification.dfy) | Directed relations, immediate edits, close/log semantics and attack/access consumers. |
| M14 | [Battle Planning and Review](M14-battle-planning-and-review.md) | [Dafny contracts](M14-battle-review-specification.dfy) | Plan editing/copy/delete, fleet-reference remapping and bounded recorded-battle playback. |
| M15 | [Messages and Reports](M15-messages-and-reports.md) | [Dafny contracts](M15-reports-specification.dfy) | Event filtering, outgoing drafts, report eligibility/caches, exports and viewpoint limits. |
| M16 | [Tutorial and Help](M16-tutorial-and-help.md) | [Dafny contracts](M16-tutorial-specification.dfy) | Tutorial page/task progression, exit/restoration and packaged HTML help; full tutorial qualification remains open. |
| M17 | [Host Game Management](M17-host-game-management.md) | [Dafny contracts](M17-host-specification.dfy) | Readiness precedence, wait/try dispatch, AI completion, backups and partial-output recovery. |
| M18 | [Turn Generation](M18-turn-generation.md) | [Dafny contracts](M18-generation-specification.dfy) | Ordered annual phases, replay/RNG prerequisites, publication and failure cleanup boundaries. |
| M19 | [Game Rules and Resolution](M19-game-rules-and-resolution.md) | [Dafny contracts](M19-resolution-specification.dfy), [construction effects](M19-construction-effects.md), [mining/depletion](M19-mining-and-depletion.md) | Complete resolution-family ownership inventory, native colony economy/mining/depletion contracts and explicit combat/task/object obligations. |
| M20 | [AI Players](M20-ai-players.md) | [Dafny contracts](M20-ai-specification.dfy) | AI entry/failure paths, exact strategy IDs, traversal/RNG, policy inventory and persistence limits. |
| S01 | [Game-file Formats, Persistence and Validation](S01-game-file-formats.md) | [Dafny specification](S01-game-file-specification.dfy), [records/codecs](S01-game-file-records.md), [orders](S01-game-file-orders.md), [external formats](S01-game-file-external.md), [validation](S01-game-file-validation.md) | Complete current emitted-format and persistence-workflow specification; observed validation/recovery limits and policy conflicts explicit. |
| S02 | [Application Preferences and Window Layout](S02-application-preferences-and-window-layout.md) | [Dafny contracts](S02-preferences-specification.dfy), [preference contracts](S02-preference-contracts.md) | Active key/default/width/write inventory, desktop profile grammar/list edits, exact tile and placement parsing, normalization and native boundaries. |
| S03 | [Static Game Data and Text](S03-static-game-data-and-text.md) | [Dafny contracts](S03-catalogs-specification.dfy), [versioned inventory](S03-catalog-inventory.md) | Catalog schemas/identities, AI packing, compressed text and 67 exact source/generator/schema references. |
| S04 | [Platform Services](S04-platform-services.md) | [Dafny contracts](S04-platform-specification.dfy) | Filesystem/buffer results, UTF-8, formatting/sort/arithmetic, temp directories and clock boundaries. |
| S05 | [SDL Desktop Implementation](S05-sdl-desktop-implementation.md) | [Dafny contracts](S05-desktop-specification.dfy) | Desktop queue/filter/timer projections, focus/capture/lifetimes and full compatibility-facility qualification inventory. |
| S06 | [Shared Game State and In-memory Data Model](S06-shared-game-state.md) | [Dafny contracts](dafny/S06-shared-state.dfy), [planet editor contracts](S06-planet-editor-contracts.md), identity/unit maps, entity/variant schemas, knowledge and lifetimes | Six model areas specified; selected structural conditions and proofs; C/refinement and gameplay proof limits explicit. |

The M03 specification was checked against `e2eae18cff0f990db20415c1c2262aed2022f95b`
on 2026-10-23. Its generation contract moved to [M05](M05-universe-creation.md),
rechecked at the same revision on 2026-10-24 with tutorial, catalog and failure
contracts added; race pricing remains linked to M04.

The M04 documents integrate the earlier race reports into this module map and
were checked against `e2eae18cff0f990db20415c1c2262aed2022f95b` on 2026-10-23.
They distinguish observed behavior from proposed port interfaces. The map's
original base revision above still describes its initial exploration.

## Traceability views

Recorded 2026-10-25. The [handbook traceability views](../handbook/traceability.md)
are generated from the collection's existing anchors, parent-story references,
acceptance rows/paragraphs, named rules, model includes and evidence resources.
They connect **116 stories** to **278 acceptance criteria** through **309 authored
parent references**, including companion criteria, across all 26 modules.
Per-module views preserve acceptance/rule/evidence/limit text and support story
and text filters, shared criterion links and reading without JavaScript. Model
and test/resource backlinks provide the reverse reading path; downloadable JSON
records the same navigation and build revision.

Model links distinguish authored criterion-file links from exact declaration-name
matches in document-linked models and their include closure. All **80 formal
files** have readable views/raw downloads; a lexical declaration index supplies
stable anchors and source-line links. Ambiguous names remain unresolved. Test
navigation resolves authored links or explicit/unique filenames under `tests/`;
broader document context stays separate from criterion-level associations.

These views introduce no acceptance rule, proof obligation, test result or
coverage score. Existing behavior statuses, source inspection revisions, dated
runs, external obligations and gaps remain authoritative in the owning specs.
Finding a declaration or test file does not establish C refinement, successful
test execution or full coverage. Missing file navigation does not imply absent
tests. Interaction/quality requirements remain in the requirement index.
Update mappings and evidence in their owning specs; generated copies are never
the source of truth. Runtime/formal definitions and existing IDs are unchanged.

Validation: **25 handbook tests** and **9 specification-checker tests** pass,
including parent/companion ownership, escaped table content, ambiguous names,
tracked-test discovery, regeneration/removal and moved-fragment links. Zensical
builds **159 pages** with zero source/rendered-link errors. Browser checks cover
all 26 trace views and parent edges, story/text filters, empty results, keyboard
reset, deep-link recovery, declaration/source-line and reverse links, mobile and
JavaScript-disabled reading; all **72 existing diagrams** and handbook
search/anchors/raw downloads still pass. The JSON export and build manifest share
revision, source fingerprint and working-tree status. Existing spec anchors and
all **559 requirement IDs** are retained.

`python3 tests/spec/verify.py --time-limit 120` with Dafny **4.11.0** verifies all
80 formal files and include closure: **4019 verified, 0 errors; audit 0 findings**.
SDL Debug builds without warnings; **17 unit tests** and the available tutorial
smoke checkpoint pass. Full native regression through checkpoint 150 reports
**519 matches, 21 existing warnings, 0 differences/errors and 0 skipped reference
inputs**. No runtime source, formal definitions, file formats or baselines changed.

## Shared-state and file relationship diagrams

Recorded 2026-10-25; selected relationships rechecked at
`7abe8083788d2f991e54eb9fc108a5468f82d2a1`. Eight data-model/relationship diagrams
live in the owning S06 entry and S01 entry/record/orders/external companions.
They show full-host entities and scoped references, task/thing variants,
observations/results/lifetimes, file companions, snapshot record groups, binary
framing, orders and library metadata/exchange copies. The
[data-model index](../handbook/data-models.md) discovers diagrams, titles and
stable anchors from those canonical sections; the [module atlas](../handbook/diagrams.md)
also links entry-level data diagrams. Edits to the specs regenerate navigation
without keeping another diagram copy.

These projections select existing contracts; they do not add logical rules,
database tables, C classes or disk fields. Native widths/layouts, Proposed typed
boundaries, partial-view knowledge, field-specific history merging, implicit
record ownership/order, reader-validation gaps and non-atomic publication retain
their existing scope. Full-host relationship closure is not imposed on filtered
player views or mutation intermediates. Existing IDs, anchors, canonical formal
definitions, runtime behavior and Open decisions remain unchanged.

Presentation validation: **17 handbook tests** and **9 specification-checker
tests** pass. The Zensical build has **130 pages** with zero source/rendered-link
errors. Chromium renders all **72 diagrams**, including eight new data models,
across **29 specification pages** and checks desktop/mobile layouts, both display
modes, scrolling, generated indexes/anchors, search and raw model downloads.
Prior specification anchors and all **559 requirement IDs** are retained. SDL
Debug builds without warnings; all **17 unit tests** and the available tutorial
smoke checkpoint pass. Full native regression through checkpoint 150 reports
**519 matches, 21 existing warnings, 0 differences/errors and 0 skipped reference
inputs**. No runtime source, formal definitions, file layouts or baselines changed.

## Cross-module sequence diagrams

Recorded 2026-10-25; handoffs rechecked at
`c3259e255f37af9c1fad0b7c749d36db157ee60e`. Seven cross-module sequences now live
in the owning M01, M02, M03, M06, M17 and M18 entries. They cover race-version
saving, new-game creation, opening/planning/exporting a player turn, importing
its successor, desktop hosting, CLI validation/generation and shared year
resolution/publication. The [sequence index](../handbook/sequences.md) and
[module atlas](../handbook/diagrams.md) discover titles, stable anchors and counts
from the canonical specs on every build; no diagram copies are maintained.

These are source-derived reading projections of the existing rules and external
obligations. They preserve entry-specific cancellation, guarded versus legacy
save behavior, partial publication, AI writes during CLI validation and ignored
generation-output results. Logical lifelines do not introduce services, new
runtime APIs, persistence rules or stronger C/model-refinement claims. Existing
requirement IDs, anchors, formal definitions and Open decisions remain unchanged.

Presentation validation: all **15 handbook tests** and **9 specification-checker
tests** pass. The Zensical build has **129 pages** and zero source/rendered-link
errors. Chromium renders all **64 diagrams**, including the seven new sequences,
and checks their generated index/anchors, desktop/mobile layouts, Fit/Actual size,
horizontal scrolling, search and raw downloads. Existing specification anchors
and all **559 requirement IDs** are retained. SDL Debug builds without warnings;
all **17 unit tests** and the available tutorial smoke checkpoint pass. Full
native regression through checkpoint 150 reports **519 matches, 21 existing
warnings, 0 differences/errors and 0 skipped reference inputs**. No game source,
file formats, formal definitions or regression baselines changed.

## Module workflow and state diagrams

Recorded 2026-10-25. Each of the 26 first-level module entries now contains a
workflow diagram and a state/recovery diagram: **52 module diagrams**, plus the
five existing handbook overview/traceability diagrams. The
[handbook diagram atlas](../handbook/diagrams.md) is generated from those canonical
Markdown blocks; edit the owning module once to update the rendered page and index.

The diagrams summarize existing source-derived prose and canonical Dafny contracts.
They distinguish copied-draft cancellation, close-and-commit dialogs, partial
publication, ignored write results, unsupported cancellation and undefined native
access. State names are explanatory milestones rather than new saved enums.
The module scope notes retain external obligations and Open decisions; diagram
presence does not close algorithm, C/model-refinement or platform-qualification gaps.
The [reading guide](../handbook/reading.md#logical-ownership-and-implementation)
explains those distinctions. Existing story/criterion IDs and anchors are retained.

Validation for this presentation milestone: Dafny 4.11.0 verified all 80 formal
files and their include closure (**4019 verified, 0 errors; audit 0 findings**).
The nine specification-checker tests and twelve handbook tests pass. Chromium
renders all 57 diagrams and checks every module at desktop/mobile widths, including
Fit to width / Actual size and horizontal scrolling, plus search/anchors/raw downloads.
The SDL Debug build has no warnings; 17 unit tests and the available tutorial smoke
checkpoint pass. Full native regression has **519 matches, 21 existing warnings,
0 differences/errors and 0 skipped reference inputs**. No runtime code, formal
definitions, file formats or regression baselines changed.

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
The [M04 advantage specification](M04-race-advantage-points.md) now records the
formulas checked against current source; the [data model](M04-race-data-model.md)
contains Dafny logical fields and proposed Rust/SQL mapping guidance. The
[module entry](M04-race-design.md) records the verified top-level workflow and its
remaining control-transition/recovery audit. Current `RaceCreationWizard`
honors `fDontWrite`, superseding the historical notes on that parameter.

### Information and order ownership

Distinguish authoritative universe state, information known to a player,
historical observations, and estimates. Map, reports, and detailed views need
consistent visibility and stale-information rules. Whether this needs a distinct
Intelligence and Visibility module remains open.

Distinguish editing an order, saving it, submitting it, exporting it, and having
the host execute it. Cancellation and failure need explicit outcomes for each
stage. Library operations, the active player session, and persistence all
participate; their final ownership must be established during detailed work.

[M06](M06-player-session-and-turn-orders.md) now specifies the player workflow,
with [Dafny contracts](M06-session-specification.dfy) checked against
`e2eae18cff0f990db20415c1c2262aed2022f95b`. It distinguishes guarded departure
from legacy direct-save/destruction paths, in-memory submission intent from
saved orders, staged export from delivery, and library import from live reload.
S01/S06 retain their shared definitions; GUI callbacks, concurrent-host races
and failed filesystem restoration remain explicit boundaries.

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
- Should the guarded departure in M06 cover every direct-open, close and reload
  path, including message edits finalized during destruction? Its current
  library/persistence handoffs are documented; a uniform transition is not implemented.
- Which AI decisions reuse the same validation rules as human orders, and where
  does the implementation currently differ?

These questions mark future investigation; they do not block this first-level map.

## Outline for each detailed module specification

Organize a module from **intent → workflow → observable acceptance → rules and
state → evidence**. Existing numbered rule sections and anchors remain stable;
a story/requirements section may precede them and link to the detailed material.
A companion specializes the owning entry's stories rather than copying them.

1. **Purpose and scope:** actor goals, included responsibilities, explicit
   exclusions, neighboring owners and whether the document is an entry or a
   technical companion. State the inspected revision and coverage limits.
2. **Actors, context and stories:** stable story IDs, actor, desired capability,
   benefit, trigger and prerequisites. Use “As a …, I want …, so that …” when
   it communicates the need. Support modules can name a developer or host
   operator and the player-facing outcome they enable.
3. **Workflows:** normal steps from entry to completion, alternatives and their
   triggers, cancellation, invalid input, partial failure, recovery and the
   observable end state. Link shared steps to their owner.
4. **Acceptance criteria:** stable IDs, Given/When/Then or equivalent explicit
   precondition/action/outcome; include relevant negative and boundary cases.
   Separate current behavior from a proposed stronger contract.
5. **Interaction requirements:** input methods, keyboard/focus, selection,
   feedback, discoverability and accessibility. Record known platform/viewport
   assumptions. A headless module explains its caller/error interface and
   links the UI owner; interaction can be inapplicable with that reason.
6. **Quality requirements:** relevant responsiveness, determinism, text/encoding,
   data integrity, resource limits and supported-display behavior. Record the
   scenario, measurement/check method, target, status, evidence and owner.
   Where no target was agreed or measurement performed, say so explicitly.
7. **Inputs, outputs and owned state:** persistent, working and derived state,
   knowledge/provenance, units, identity and lifetime. Reference S06 logical
   types, S03 catalogs and S01 serialization independently.
8. **Rules and calculations:** canonical Dafny definitions, constraints, operation
   order, rounding/RNG boundaries, checked conditions and external obligations.
   Reuse definitions; a story must not introduce a second algorithm in prose.
9. **Dependencies and handoffs:** responsibility and data exchanged with each
   owner, including planning versus execution, UI versus core, persistence and
   recovery. A proposed module split is not an approved refactor.
10. **Traceability:** story → criterion → rule/model → implementation evidence →
    test/proof or gap. A table can combine acceptance and traceability columns.
    Link to dated verification rather than implying that a named test ran.
11. **Evidence and verification:** source revision, named functions, actual
    commands/results, property proof scope, existing test scope and missing
    coverage. A checked model is not proof of C refinement or full UI behavior.
12. **Open decisions:** unanswered policy, usability, performance and ownership
    questions, the owner who must resolve each, and what depends on the answer.

### Requirement IDs and status

Use stable module-scoped IDs: `M07-US01` (story), `M07-AC01` (acceptance),
`M07-IR01` (interaction), `M07-QR01` (quality). Technical companions add a short
scope, for example `S01-REC-AC01`. Keep existing IDs such as `S01-A01`; link them
instead of renumbering. Give referenced IDs explicit lowercase HTML anchors so
renaming a heading does not break a reference. Keep IDs unique across the collection.

**Behavior status** has three values:

| Status | Meaning |
| --- | --- |
| Observed | Describes current source behavior at the cited revision, including its limitations. |
| Proposed | Desired interface/behavior or acceptance target; not automatically implemented or approved for a runtime change. |
| Open | A policy, target or behavior is unresolved; name the decision owner and the consequence. |

**Evidence** is a separate dimension: source reviewed; named model definition;
proved lemma and assumptions; existing test and its scope; dated passing/failing
run; or missing coverage. A story can describe observed behavior while its
end-to-end acceptance test remains missing. Historical runs remain dated evidence.
Stories inferred from source express a plausible goal, not measured user demand.

### Interaction and quality review

Each entry and companion accounts for these concerns in its own context. Reuse
an owner's requirement by link when it supplies the whole contract; explain an
inapplicable concern. Avoid inventing performance budgets, platform guarantees,
accessibility conformance or transaction guarantees from an implementation detail.

| Concern | Required specification detail |
| --- | --- |
| Input and focus | Supported mouse/keyboard/caller paths, navigation and cancellation, focus after validation/failure; identify incomplete audits. |
| Feedback and discovery | How users identify the current object, action, units, completion and failure; link control/help/message owners. |
| Accessibility and display | Keyboard alternatives, non-color cues, readable labels and relevant viewport/platform cases; distinguish proposals from demonstrated support. |
| Text | UTF-8 byte budgets, safe truncation and exact comparison behavior; link existing S01/M04/native contracts rather than assuming Unicode case folding. |
| Responsiveness and resources | Workload and environment, timing/memory metric, measurement method and agreed threshold or Open target. Retry loops and allocation failures need explicit limits. |
| Integrity and reproducibility | What is preserved on success/failure, scoped identities, ordering, RNG/clock dependencies and comparison oracle. State partial-write and rollback limitations. |

New representable acceptance rules require checked Dafny definitions under
[spec/AGENTS.md](AGENTS.md). Reworded criteria point to existing definitions.
UI accessibility, rendering, filesystem durability and timing criteria identify
specific platform/test obligations where the formal collection has no model;
labeling something a quality requirement does not establish a proof.

### Story coverage inventory

The 26 detailed entry specifications own stories; their seven companions own
focused criteria and link back. Every first-level module now has an entry;
remaining work is the explicitly named native/formal/interaction coverage, not
unwritten overview rows. Older research documents remain dated evidence.

| Owner | Entry stories | Companion responsibilities |
| --- | --- | --- |
| M01 | [Game library](M01-game-library.md#user-stories-and-requirements) | S01 owns bytes/publication; M06 owns active-session saves and Orders Export. |
| M02 | [Race library](M02-race-library.md#user-stories-and-requirements) | M04 owns design/pricing; S01 owns race records and conversion; shared name predicates have one definition. |
| M03 | [Game setup and roster](M03-new-game-wizard.md#user-stories-and-requirements) | Batch and creation handoffs link S01/M05. |
| M04 | [Race design](M04-race-design.md#user-stories-and-requirements) | [Data](M04-race-data-model.md#story-linked-requirements) and [pricing](M04-race-advantage-points.md#story-linked-requirements). |
| M05 | [Universe creation](M05-universe-creation.md#user-stories-and-requirements) | Catalogs, persistence and initial-state contracts remain shared owners. |
| M06 | [Player sessions](M06-player-session-and-turn-orders.md#user-stories-and-requirements) | S01 retains bytes/replay/publication. |
| M07 | [Map navigation](M07-galaxy-map-and-navigation.md#user-stories-and-requirements) | M08/M10 retain gameplay edits; S05 retains platform input/rendering. |
| M08 | [Planet and Colony Management](M08-planet-and-colony-management.md#user-stories-and-requirements) | Colony selection, capacity/output, terraforming, orbital facilities and pane behavior; growth/mining and platform boundaries explicit. |
| M09 | [Production Management](M09-production-management.md#user-stories-and-requirements) | Queue editing/templates, commit scope, cost/forecast controllers and native builder steps; outer host application and refinement remain open. |
| M10 | [Fleet Operations](M10-fleet-operations.md#user-stories-and-requirements) | Fleet orders, cargo, split/merge, overflow, fuel and task-resolution handoffs; host simulation and packed-damage gaps explicit. |
| M11 | [Ship and Starbase Design](M11-ship-and-starbase-design.md#user-stories-and-requirements) | Roster/copy/edit/delete, availability, slots and cost/armor projections; catalog and cascading deletion boundaries explicit. |
| M12 | [Research and Technology](M12-research-and-technology.md#user-stories) | §§3–6 trace workflows, criteria, definitions, dependencies and owned coverage gaps. |
| M13 | [Player Relations](M13-player-relations.md#user-stories) | §§3–6 trace workflows, criteria, definitions, dependencies and owned coverage gaps. |
| M14 | [Battle Planning and Review](M14-battle-planning-and-review.md#user-stories) | §§3–6 trace workflows, criteria, definitions, dependencies and owned coverage gaps. |
| M15 | [Messages and Reports](M15-messages-and-reports.md#user-stories) | §§3–6 trace workflows, criteria, definitions, dependencies and owned coverage gaps. |
| M16 | [Tutorial and Help](M16-tutorial-and-help.md#user-stories) | §§3–6 trace workflows, criteria, definitions, dependencies and owned coverage gaps. |
| M17 | [Host Game Management](M17-host-game-management.md#user-stories) | §§3–6 trace workflows, criteria, definitions, dependencies and owned coverage gaps. |
| M18 | [Turn Generation](M18-turn-generation.md#user-stories) | §§3–6 trace workflows, criteria, definitions, dependencies and owned coverage gaps. |
| M19 | [Game Rules and Resolution](M19-game-rules-and-resolution.md#user-stories) | §§3–6 trace workflows, criteria, definitions, dependencies and owned coverage gaps. |
| M20 | [AI Players](M20-ai-players.md#user-stories) | §§3–6 trace workflows, criteria, definitions, dependencies and owned coverage gaps. |
| S01 | [Persistence](S01-game-file-formats.md#user-stories-and-requirements) | [Records](S01-game-file-records.md#story-linked-requirements), [orders](S01-game-file-orders.md#story-linked-requirements), [external formats](S01-game-file-external.md#story-linked-requirements), [validation](S01-game-file-validation.md#story-linked-requirements). |
| S02 | [Application Preferences and Window Layout](S02-application-preferences-and-window-layout.md#user-stories) | [Preference contracts](S02-preference-contracts.md#acceptance-and-evidence) add focused criteria for profile parsing, mutation, layout and storage widths. |
| S03 | [Static Game Data and Text](S03-static-game-data-and-text.md#user-stories) | [Catalog inventory](S03-catalog-inventory.md) pins complete source inputs; §§3–6 trace workflows, criteria, definitions, dependencies and owned coverage gaps. |
| S04 | [Platform Services](S04-platform-services.md#user-stories) | §§3–6 trace workflows, criteria, definitions, dependencies and owned coverage gaps. |
| S05 | [SDL Desktop Implementation](S05-sdl-desktop-implementation.md#user-stories) | §§3–6 trace workflows, criteria, definitions, dependencies and owned coverage gaps. |
| S06 | [Shared state](S06-shared-game-state.md#user-stories-and-requirements) | Shared typed values and lifetimes consumed by the functional modules. |

## Suggested next steps

All first-level entries are documented. Follow the named coverage inventories to
prioritize further work: M04’s detailed workflow/recovery audit; M08–M13 and M19
complete host-effect refinement; M20 strategy-specific decision/visibility
fixtures; S03 parser/consumer equivalence; and S05 real-backend accessibility,
IME, rendering and display qualification. M16’s full 80-page tutorial remains
unfinished. None of these gaps is closed by a passing symbolic witness or dummy
backend test. Future behavior changes remain separate runtime tasks with the
repository’s baseline and changelog discipline. Retain this document as the index.

## Dafny coverage and verification

Migration inventory, 2026-10-24, based on the detailed specifications at
`e2eae18cff0f990db20415c1c2262aed2022f95b`. Dafny is the canonical notation for
modeled values and rules; Markdown retains explanations, provenance, module
ownership, worked examples and unresolved decisions. This is a specification
migration, not generated game code or a proof that the C implementation refines
the model. Earlier verification tables in module documents describe their
original batches; this section records the collection-wide migration.

M01/M02 and M08–M11 were added after the original migration; their coverage
and dated verification are recorded below. The overview-only M12–M20 and S02/S03/S05
entries identify responsibilities,
not complete algorithms to translate. Existing concrete rules embedded in M03
(such as scoring and research) are included below without changing ownership.

| Existing specification area | Canonical definitions and coverage |
| --- | --- |
| M03 §2, §3.1–3.2: settings, selectors, cache, roster edits, navigation and initialization | [M03 setup](dafny/M03-setup.dfy): `Setup`, `Slot`, `Selector`, `DecodeSelector`, `FPrefix`, `FSetup`, `EditSlot`, `CacheAllocation`, `Wizard`, `LeavePage`, `FCanLeave`, `FInitialize`. Requested selectors remain distinct from resolved controllers. |
| M03 §3.3: normalization | [M03 normalization](dafny/M03-normalization.dfy): `Substitute`, `InitialPass`, `MissingNames`, `FNameProbe`, `FDuplicateName`, `Portraits`, `DropPortrait`, `FFillPortrait`. Substitution retains pricing normalization on non-AI races; supplied points come from M04 `FPrice`. Substitution and missing names interleave per player, before duplicate-name and portrait passes. `ResolveAI` draws random level before kind; `ResolveExpansion` is separate. |
| M03 §4.1 / S06 §8.1: arithmetic and RNG | [S04 arithmetic](dafny/S04-arithmetic.dfy): `LMulDiv`, `Trunc`, `Take`, `SeedTable`, `Seed`, `FDraws`; reuse S01 signed narrowing/division and S06 normalized recurrence/context stack. Literal seed table retains 279. |
| M03 §4.2: default roster | `CountBranches`, `ChooseCount`, `DefaultCount`, `DefaultSelector`, `ShuffleLater`, `DefaultRoster`; thresholds, conditional fresh draws and strictly-later shuffle are represented. |
| M03 §4.3 / §4.8: victory controls and batch conversions | `VCMax`, `VCBase`, `VCStep`, `DefaultVC`, `EnabledCount`, `VCValue`, `Toggle`, `SetVC`, `ClickVC`, `BatchRaw`, `FBatchValue`; [S01 external](dafny/S01-external.dfy) retains the batch grammar. Batch starts from zero raw state rather than desktop defaults. |
| M03 §4.4: victory | [M03 scoring](dafny/M03-scoring.dfy): `Rank`, `FLeader`, `FSecond`, `Conditions`, `FConditionMatrix`, `Met`, `FDead`, `Victory`. Tied ranks, highest-index leader, zero-score lead ties, minimum-year/count gates, elimination and the single-participant exception stay distinct. |
| M03 §4.5: score, resources, ship power and speed | `ScoredPopulation`, `PlanetPoints`, `TechPoints`, `ShipPoints`, `Score`, `Pack`, `Decode`, `Capacity`, `EffectivePopulation`, `Factories`, `OrdinaryResources`, `ARExpr`, `FARResources`, `Beam`, `Torpedo`, `Bomb`, `Capacitors`, `Power`, `Class`, `HighestWarp`, `Speed`. Catalog statistics are explicit inputs; normal game arithmetic domains apply. |
| M03 §4.6: research and visibility | `TechCosts`, `ResearchCost`, `ResearchRemainder`, `FScoreVisible`. Current total technology and spent progress are inputs for each projected level; AI alliances remain an ownership note, not an invented multiplier. |
| M03 §4.7 / §4.9 / §5 | Generation is M05 below; UI formulas are `RowTop`, `FRowHit`, `RosterHeight`, `DigitCounts`, `Spinner`, `CheckboxIndex`; recovery is `CancelWizard`, `BeginFinish`, `Complete`. Saved races and consumed RNG are retained on cancel. |
| M04 identity, habitat, attributes, traits and credentials | [M04 race model](dafny/M04-race-model.dfy): `Axis`, `Race`, `FShape`, `FIdentity`, `FAxis`, `FPricing`, `FRace`, `MinAttr`, `MaxAttr`, `Trait`, `Primary`, `Temperature`, `GravityHundredths`, `Credential`, `FCredentialInput`. Names use S01 UTF-8 byte contracts. |
| M04 Rust/SQL sketches and caches | Former duplicate struct/column examples now reference `RaceVersion`, `FVersion`, `HabitatRow`, `FHabitatRows`, `FSamePriceInputs`, `FRepriceRequired`; metadata independence is proved by `MetadataIndependent`. Database storage/index/cascade mechanisms remain implementation guidance. |
| M04 correction and workflow | `NormalizeAxis`, `Bounds`, `NormalizeInnateAxis`, `Custom`, `DialogOrder`, `Navigate`, `Finish`, `Cancel`. Ordinary zero width is representable as observed normalization output but excluded from safe pricing. `AxisNormalization` proves normalization validity/idempotence; `HackerSticky` preserves prior detection. |
| M04 desirability and innate integration | [M04 pricing](dafny/M04-race-pricing.dfy): `AxisTotal`, `Habitat`, `FDesire`, `Terraform`, `AxisSamples`, `Sample`, `Residual`, `SamplePoint`, `Term`, `RadiationSum`, `Radiation`, `TemperatureSum`, `GravitySum`, `StageExpr`, `InnateExpr`, `FInnate`. Axes, signed residuals, sample duplicates and integer/float boundaries retain their specified order. |
| M04 complete advantage formula and handoff | `GrowthCredit`, `GrowthScale`, `Factory`, `Mines`, `TraitPrices`, `TraitSum`, `TraitCount`, `Research`, `PointsInternal`, `Balance`, `FPrice`. `PricingVectors` proves the golden integer subtotal 1623 and signed balance 541 **given innate 1470287**; it does not prove that full floating integration returns that innate value. The native race unit test checks the end-to-end golden calculation. |
| M05 §2–3: context, stages, desktop/batch/tutorial | [M05 creation](dafny/M05-creation.dfy): `Context`, `FContext`, `GenerationOrder`, `StagePlan`, `FCompletedStages`, `FStageOrder`, `AssignmentEvents`, `FleetEvents`, `TutorialSetup`. Epoch, clock, independent file seeds and catalog/text version are explicit inputs; detailed tutorial names/race templates remain pinned catalog data. |
| M05 §4.1: geometry | [M05 geometry](dafny/M05-geometry.dfy): `Diameter`, `Target`, `Candidates`, `Coordinate`, `Compare`, `StartSort`, `SortStep`, `FSortTrace`, `PrunePair`, `RandomRemoval`, `FRemovalTrace`, `FCompact`, `FNearestPoint`, `ClumpAxis`, `FClumpStep`. Sort models the current first-pivot partition actions and equal-key behavior; it is not replaced by an arbitrary sorted permutation. |
| M05 §4.2: names and unowned worlds | `FNameDraw`; [M05 initial state](dafny/M05-initial-state.dfy): `Artifact`, `Environment`, `TutorialEnvironment`, `Concentration`, `OneOverride`, `Overrides`, `Minerals`, `UnownedPlanet`, `Stock`, `Stocks`, `ClearPlanetZero`. Live planet-0 inputs are supplied anew to each home, not frozen at first initialization. |
| M05 §4.3: home placement | `HomeSpacing`, `HomeSquare`, `SquareDistance`, `FHome`, `FFirstStep`, `LaterDraw`, `LaterScan`, `Relax`, `FRestartSpacing`; `HomeSwap` and `AssignmentEvents` preserve interleaving with random-race calls. Search relations represent source steps; no total retry bound or termination theorem is claimed. |
| M05 §4.4–4.5: technology, homes and bonuses | `Technology`, `StartingTech`, `SurfaceLowest`, `FirstLowest`, `SurfaceBonus`, `ConcentrationBonus`, `HomeResources`, `HomeAxis`, `HomeEnvironment`, `Bases`. `SurfaceConservation` proves the total mineral bonus for nonnegative points, including tie allocation. |
| M05 §4.6: designs and fleets | `ShipTemplates`, `FSecondEligible`, `UpgradeCandidates`, `FirstAvailable`, `Fuel`, `StartupFleet`; `FleetEvents` records fuel before upgrades. A repeated selected template denotes shared design with separate one-ship fleets. S06 checks published design/reference closure. |
| M05 §4.7: second world | `FSecondWorld`, `Reservoir`, `FNearestUnowned`, `PopulationSplit`, `RerollFallback`, `RerollEnvironment`, `FSecondRerolls`, `SecondWorld`, `SecondStocks`; same M04 desirability relation. The hundredth reroll falls back even when it succeeds. |
| M05 §4.8: wormholes | `PairCount`, `ObstacleMask`, `Masks`, `WormholeScore`, `BetterPosition`, `WormTrial`; S06 `WormholePairs` checks reciprocal partner IDs. Obstacles exclude the current endpoint; only an already-linked partner is classified as partner. |
| M05 §4.9 / §5: completion and failures | `SingleScan`, `Relation`, `PlanAttack`, `TamperingRecipients`, `GameIdentity`, `Salt`, `GeneratorReturn`, `BatchReturn`, `FPublished`. Companion-write outcomes are deliberately absent from `GeneratorReturn`, reflecting their ignored return values; publication requires every complete output. `PublicationNeedsEveryPlayer` proves that requirement. |
| S01 all five detailed documents | Existing [S01 entry point](S01-game-file-specification.dfy), contracts, codecs, records, files, orders, external formats and witnesses remain canonical. This migration reuses them; their documented unsupported/raw-reader/transaction boundaries are unchanged. `CryptConsumption` has additional arithmetic proof steps and `CodecVectors` isolates assertions, with the same contracts. |
| M06 player session and turn orders | [M06 workflow model](dafny/M06-session.dfy): selection, credentials, pending log/dirty state, guarded and direct save, submission intent, export flags, import classification, new-turn detection and wait choices. The [coverage inventory](M06-player-session-and-turn-orders.md#6-coverage-inventory-and-module-interfaces) identifies reused S01/S06 rules and external obligations. |
| M07 galaxy map and navigation | [M07 navigation model](dafny/M07-navigation.dfy): signed zoom conversions, viewport/reveal, wheel/pan, ordered hit testing, selection/search precedence, gesture dispatch, filters, scalar displays and snapshot identity. The [coverage inventory](M07-galaxy-map-and-navigation.md#7-coverage-inventory-and-external-obligations) distinguishes source adapters, renderer/platform obligations and M08/M10 edits. |
| S06 §2–5, §7–8.1 | Existing [shared-state model](dafny/S06-shared-state.dfy): identities, sentinels, cargo, research, relations, hull slots, plans, production, waypoints, planets, fleets, topology, things, creation homes, order envelopes and RNG stack. |
| S06 §3 / §4 / §6 / §8.2 remaining groups | [S06 extensions](dafny/S06-state-extensions.dfy): `Configuration`, `Player`, `Status`, `Counts`, `PlanMetadata`, `DerivedDesign`, `FProductionTemplate`, `DesignView`, `Lifecycle`, `PowerState`, `FleetView`, `Damage`, `MotionState`, `PlanetExtras`, `ScoreKey`, `Score`, `CatalogEvent`, `FEvent`, `EncodedResult`, `SessionState`, `Lifetime`, `Held`, `CacheState`. Raw result payloads reuse S01 instead of duplicating its layouts. Cache invalidation/session destruction are explicitly proposed logical boundaries, not complete models of every C global. |
| Acceptance sections across M03/M04/M05/S01/S06 | Definitions cover the specified cases; named `*Vectors` lemmas exercise selected positive/negative cases and boundary values. General lemmas cover selected invariants. The full prose conformance matrices remain broader than the witnesses; they are not advertised as exhaustive implementation tests. |

### Exact floating expressions and remaining boundaries

`S04Arithmetic.Expr` preserves each binary64 operation. `Evaluation` supplies
its result bits and child results; `FEvaluate` checks exact rational midpoint
conditions, ties-to-even and square-root rounding at every node. This avoids
pretending mathematical reals reproduce a sequence of binary64 roundings.
The represented domain is finite values below maximum finite magnitude; NaN,
infinity, overflow and signed-zero identity are excluded. Game formulas lie
inside this domain. The model checks a supplied trace; it does not implement a
fast binary64 evaluator or prove that every expression has a trace. Golden
integration, all trait-mask/research combinations and C-to-model equivalence
remain conformance/proof work, not silently satisfied by a definition.

The following are specific boundaries, not missing replacement rules:

- M04 zero-width habitat division is undefined in C. `FAxis(...,false)` retains
  observed normalization; safe pricing requires `FAxis(...,true)` and does not
  manufacture a compatible quotient. Corrupt RNG states and arithmetic outside
  the documented normal game domains likewise have no new compatibility policy.
- Random-race construction, full part legality/statistics, catalog/text expansion,
  visibility projection and complete M11/M14/M18/M20 algorithms were only bounded
  dependencies in the existing specs. Calls, required data and ordering are
  represented; their still-unspecified internals are not invented. M05 §2.3 pins
  the exact source/data dependencies, and `Context` records their identity.
- Home placement has no source retry ceiling. Small-step definitions are available
  without asserting total termination. Composition of every generation step into
  a full state-invariant proof remains open; stage order alone is not that proof.
- Filesystem availability, clocks, library reservation/finalization and UI event
  delivery are external observations. Their success/failure predicates are modeled;
  no logical predicate makes I/O atomic or certifies the actual filesystem.
- S06 raw/typed serialization refinement, field-level history merging, battle
  legality and arbitrary mid-turn resume require their owners' additional
  specifications. Knowledge variants retain absence rather than guessing values.
- Protected legacy race admission conflicts with root guidance and ADR 0002;
  unsigned/signed observation-turn extension remains an open policy. Neither is
  resolved by this migration. Source provenance, ownership and open questions
  remain prose because they are evidence/decisions, not executable game rules.

### Reproducing the checks

Use [tests/spec/verify.py](../tests/spec/verify.py) with Dafny 4.11.0:

```sh
python3 tests/spec/verify.py
```

It checks local Markdown links and anchors, exact path casing, module filename
prefixes, trailing whitespace and every Dafny include, then verifies every Dafny
file with `--verify-included-files` and audits every file. `--links-only` performs
the structural checks without Dafny. No runtime source, file layout, gameplay
baseline or generated game dependency is changed.

### Migration verification results

| Check, 2026-10-24 | Result |
| --- | --- |
| `python3 tests/spec/verify.py`, Dafny 4.11.0, all 23 Dafny files and include closure | **765 verified, 0 errors; auditor 0 findings.** |
| Local links/includes, anchors, casing and module prefixes | **582 checked, 0 errors.** |
| `cmake --build dist/m05-macos-sdl-debug -j 8` | Passed; no compiler warnings. Existing build uses the `macos-sdl-debug` preset. |
| CTest with the repository's dummy SDL environment, `^sdl_test_\|win32_native_tutorial_.*dummy$` | **18/18 passed**: 17 unit suites and the available tutorial smoke checkpoint. This remains partial tutorial coverage. |
| Full `make regression`, `REGRESSION_BUILD=dist/m05-regression-build`, work/report prefix `dist/scaffold/regression/dafny-migration` | All nine scenarios through turn 150: **519 matches**, including 21 unused-storage warning matches; **0 differences/errors, 0 reference inputs skipped**. Baseline unchanged. |
| Diff/runner checks | `git diff --check`, spec whitespace checks and Python syntax compilation passed. No C source, on-disk layout or baseline changes. Host-only checks were not required. |

Initial collection-wide proof runs timed out in the existing `CryptConsumption`
and `CodecVectors` lemmas. Explicit quotient arithmetic and isolated assertions
resolved those timeouts without changing the contracts or admitting assumptions.
The final command above passes. The prior M05 New Game workflow timeout remains
recorded in that document; this migration does not claim that additional test
passed or that the entire acceptance matrix was executed.

Native checks reused the already-configured separate build directories because
the default caches retain the repository's old path. Regression used the writable
`dist/go-cache`. These are verification environment choices, not game changes.

M07's later [verification results](M07-galaxy-map-and-navigation.md#8-acceptance-criteria-and-verification)
record the expanded 27-file collection (931 verified, zero errors/audit findings),
19 passing runtime suites/checkpoints and the unchanged full native regression.
The migration table above remains the historical result of its original batch.

## Story expansion verification

On 2026-10-24, the seven existing entry specifications and six technical
companions gained the structure above: **28 stories, 66 acceptance criteria,
20 interaction requirements and 34 quality requirements**. Every entry story
has acceptance criteria; requirement IDs are unique across the collection.
The root/spec agent rules and two race reference documents now point to the
same ownership and traceability conventions. Overview-only modules retain their
earlier scope.

This pass adds intent, workflow and evidence links around existing rules. All
27 Dafny files remain unchanged. Proposed interfaces/accessibility outcomes and
Open budgets remain separate work; these checks do not establish new UI coverage,
performance guarantees or C/model equivalence. Source provenance remains
`e2eae18cff0f990db20415c1c2262aed2022f95b`.

| Check | Result |
| --- | --- |
| `python3 tests/spec/verify.py --time-limit 120`, Dafny 4.11.0 | Full include closure: **931 verified, 0 errors; auditor 0 findings**. |
| Final `python3 tests/spec/verify.py --links-only` | **830 local links/includes checked, 0 errors**, including anchors, casing, module prefixes and whitespace. New root/reference links also resolve. |
| Requirement review | All 28 stories mapped to criteria; all 148 new requirement/story IDs unique. Entries and companions account for interaction, quality, dependencies and explicit gaps. |
| `cmake --preset macos-sdl-debug -B dist/m05-macos-sdl-debug` and `cmake --build dist/m05-macos-sdl-debug` | Passed; no compiler warnings. |
| Dummy-SDL CTest unit suites, GUI workflow and tutorial smoke | **19/19 passed**: 17 unit suites, `win32_client_newgame_dummy` (102.99 seconds), and `win32_native_tutorial_smoke_dummy`. Tutorial coverage remains partial. |
| Full `make regression` | All nine scenarios through turn 150: **519 matches**, including 21 unused-storage warning matches; **0 differences/errors, 0 reference inputs skipped**. |
| Diff review / `git diff --check` | Passed; this expansion changes documentation only. Runtime source, formal definitions and regression baselines are unchanged. Host-only checks were not required. |

CTest ran in `dist/m05-macos-sdl-debug` with `--output-on-failure --timeout 300`
and filter `^sdl_test_|win32_native_tutorial_.*dummy$|^win32_client_newgame_dummy$`.
Its environment was `SDL_VIDEODRIVER=dummy`, `SDL_AUDIODRIVER=dummy`,
`STARS_SDL_RENDERER=software`, `STARS_FILE_DIALOG_BACKEND=compat`. These are the
unit/tutorial filters used by the Make targets plus the GUI workflow.
Regression used `GOCACHE=$PWD/dist/go-cache`,
`REGRESSION_BUILD=dist/m05-regression-build`,
`REGRESSION_WORK=dist/scaffold/regression/stories-native` and
`REGRESSION_REPORT=dist/scaffold/regression/stories-comparison.json`.
Run logs use the `dist/stories-` prefix. Earlier verification tables in this
collection remain the historical results of their original batches.

## M01 and M02 verification

M01 Game Library and M02 Race Library were added on 2026-10-24 against
`e2eae18cff0f990db20415c1c2262aed2022f95b`. They use the updated outline and add
15 stories, 29 acceptance criteria, eight interaction requirements and ten quality
requirements. Their model entry points include the existing session, race and
persistence definitions; shared library-name predicates live once under S01.
The earlier migration and story-expansion tables remain historical results.

| Coverage | Canonical definitions / boundaries |
| --- | --- |
| Game library | [M01 model](dafny/M01-library.dfy): listing gates, Continue ranking, import-target count, M06 turn classification, host-order matching, metadata preservation, copy collisions, rename/delete/export outcomes. Native I/O and UI behavior remain external obligations in the module's coverage inventory. |
| Race library | [M02 model](dafny/M02-library.dfy): complete-design comparison projection, version scan, reuse/update/new-version choice, destination binding, wrapper restoration and import/edit result. Native record extraction, Unicode comparison, credentials and filesystem results remain explicit inputs. |
| Shared names | [S01 library names](dafny/S01-library-names.dfy): portable components/reserved stems, game/race path budgets, digit domains, sanitization core, safe UTF-8 cuts and availability. Non-ASCII comparison keys require the native adapter; no normalization policy is invented. |

| Check, 2026-10-24 | Result |
| --- | --- |
| `python3 tests/spec/verify.py --time-limit 120`, Dafny 4.11.0 | All **32 Dafny files** and include closure: **1145 verified, 0 errors; auditor 0 findings**. |
| Final `python3 tests/spec/verify.py --links-only` | **903 local links/includes checked, 0 errors**, including anchors, casing, prefixes and whitespace. |
| New models alone: `dafny verify spec/dafny/M01-library.dfy spec/dafny/M02-library.dfy spec/dafny/S01-library-names.dfy --cores 4 --verification-time-limit 20` | **214 verified, 0 errors**; the full run above also verifies their existing dependencies. |
| Requirement inventory | All **15 new stories** have acceptance criteria; **62 new IDs** are unique across the collection. Each entry covers interaction/quality, ownership, evidence and open decisions. |
| `cmake --preset macos-sdl-debug -B dist/m05-macos-sdl-debug` and `cmake --build dist/m05-macos-sdl-debug` | Passed; no compiler warnings. |
| Dummy-SDL CTest unit, library, GUI workflow and tutorial selection | **26/26 passed**: 17 unit suites, Continue, New Game, Game Export, Game Library Safety, Game Library Delete, Race Library, Host Library, File Workflow and tutorial smoke. Total 237.83 seconds; tutorial remains partial. |
| Full `make regression` | All nine scenarios through turn 150: **519 matches**, including 21 unused-storage warning matches; **0 differences/errors, 0 reference inputs skipped**. |
| Diff review / `git diff --check` | Passed. Runtime source, record layouts and regression baselines unchanged; no host-only checks required. |

CTest used `--test-dir dist/m05-macos-sdl-debug --output-on-failure --timeout 300`
and filter
`^sdl_test_|win32_native_tutorial_.*dummy$|^win32_client_(newgame|continue|game_export|game_library_safety|game_library_delete|race_library|host_library|fileworkflow)_dummy$`.
The environment was `SDL_VIDEODRIVER=dummy`, `SDL_AUDIODRIVER=dummy`,
`STARS_SDL_RENDERER=software`, `STARS_FILE_DIALOG_BACKEND=compat`. This includes
the unit/tutorial filters used by the Make targets plus relevant library workflows.
Regression used `GOCACHE=$PWD/dist/go-cache`,
`REGRESSION_BUILD=dist/m05-regression-build`,
`REGRESSION_WORK=dist/scaffold/regression/m01-m02-native` and
`REGRESSION_REPORT=dist/scaffold/regression/m01-m02-comparison.json`.
Final logs are `dist/m01-m02-spec-final.log`, `dist/m01-m02-model-final.log`,
`dist/m01-m02-runtime-checks.log`, `dist/m01-m02-regression.log` and the
`dist/m01-m02-configure.log` / `dist/m01-m02-build.log` pair.

Initial model drafts had a reserved-identifier parse error and unproved recursive/
quantifier witnesses. Renaming the parameter, explicit unfolding/membership
steps and isolating witness assertions resolved them without weakening contracts.
An intermediate collection run also timed out in the unchanged
`SharedGameState.AcceptanceVectors` at 120 seconds; the final complete run above
passed with the same limit and no changes to that existing model. No assumptions,
axioms or disabled verification were introduced.

The protected-legacy-race policy conflict remains Open; documenting current
conversion does not override root instructions. Native comparison, I/O,
accessibility and timing boundaries remain explicit; passed tests/proofs do not
establish every acceptance criterion or a C/model refinement theorem.

## M08–M11 verification

M08 Planet and Colony Management, M09 Production Management, M10 Fleet Operations
and M11 Ship and Starbase Design were added on 2026-10-24 against
`e2eae18cff0f990db20415c1c2262aed2022f95b`. Each uses the updated requirements
outline, stable story/acceptance/interaction/quality IDs, source-backed workflows,
canonical Dafny rules, a coverage inventory and named owners for open decisions.
There are 22 new stories, 38 acceptance criteria, 12 interaction requirements and
16 quality requirements (88 unique new IDs). Existing user work is retained.

| Coverage | Canonical definitions / external obligations |
| --- | --- |
| M08 colony | [Model](dafny/M08-colony.dfy): capacity display, installation/defense limits, terraform endpoints, selection/base predicates, driver aggregation and scroll offsets. Reuses M04 habitat and M03 resource/capacity models; growth/mining/catalog/pane adapters remain explicitly inventoried. |
| M09 production | [Model](dafny/M09-production.dfy): editing limits, swap/removal/finish, research reservation, paid-cost differences, progress percentages, base credits and result statuses. Catalog cost iteration, templates, forecasts and full host effects remain external obligations. |
| M10 fleet | [Model](dafny/M10-fleet.dfy): split capacity, constrained cargo transfer, two-fleet stack overflow, no-heal, waypoint removal/recycle and integer fuel/range projections. S06 owns task payloads; packed damage, float branches and full task execution remain inventoried. |
| M11 design | [Model](dafny/M11-design.dfy): tech status/buildability, roster/slot limits, edit outcome, scalar cost discounts and armor/aggregate/narrowing calculations. S03 catalog extraction, native controls and deletion cascades remain explicit. |

| Check, 2026-10-24 | Result |
| --- | --- |
| `python3 tests/spec/verify.py --time-limit 120`, Dafny 4.11.0 | **40 Dafny files** and complete include closure: **1445 verified, 0 errors; auditor 0 findings**. |
| Focused new-model verification, four M08–M11 files, `--cores 4 --verification-time-limit 60` | **300 verified, 0 errors**. |
| `python3 tests/spec/verify.py --links-only` | **1018 local links/includes, 0 errors**; exact casing, anchors, include paths, prefixes and whitespace checked. |
| Requirement inventory | **88 unique new IDs**, all **22 stories** mapped to acceptance criteria; all existing explicit anchors remain unique. |
| `cmake --preset macos-sdl-debug -B dist/m05-macos-sdl-debug`, then `cmake --build dist/m05-macos-sdl-debug` | Passed; no compiler warnings. |
| Dummy-SDL CTest unit / Continue / designer / tutorial selection | **20/20 passed** in 28.50 seconds: 17 unit suites and three client workflows. Tutorial coverage remains partial. |
| Explicit fleet branch of `run_client_continue.py` | Passed cargo accept/cancel/reverse, waypoint task/reselection, map append/drag/delete, repeat, merge and split/cancel workflows. |
| Full native `make regression` | Nine scenarios through turn 150: **519 matches**, including 21 unused-storage warning matches; **0 differences/errors**, 0 reference inputs skipped. |
| Diff / source review | No runtime, catalog, record-layout or baseline changes; `git diff --check` passed. Existing unrelated specification work retained. Host-only checks not required for this documentation batch. |

CTest used `--test-dir dist/m05-macos-sdl-debug --output-on-failure --timeout 300`
and filter `^sdl_test_|win32_native_tutorial_.*dummy$|^win32_client_(continue|designer)_dummy$`.
The explicit fleet run used `STARS_CLIENT_WORKFLOW=fleet python3
tests/compat/run_client_continue.py` with the absolute path to
`dist/m05-macos-sdl-debug/bin/stars-compat-client-startup`.
Both used `SDL_VIDEODRIVER=dummy`, `SDL_AUDIODRIVER=dummy`,
`STARS_SDL_RENDERER=software`, `STARS_FILE_DIALOG_BACKEND=compat`.
Regression used `GOCACHE=$PWD/dist/go-cache`,
`REGRESSION_BUILD=dist/m05-regression-build`,
`REGRESSION_WORK=dist/scaffold/regression/m08-m11-native` and
`REGRESSION_REPORT=dist/scaffold/regression/m08-m11-comparison.json`.
Logs: `dist/m08-m11-spec-final.log`, `dist/m08-m11-model-third.log`,
`dist/m08-m11-runtime-checks.log`, `dist/m08-m11-fleet-workflow.log`,
`dist/m08-m11-regression.log`, `dist/m08-m11-configure.log` and
`dist/m08-m11-build.log`.

Initial drafts failed on a reserved parameter name, explicit membership/witness
proofs and range-division facts; some witness obligations timed out. Renaming,
explicit proof steps and the final 60-second focused limit resolved them.
A failed strict terraforming-bound claim exposed a real source distinction:
clamping at 1/99 can produce a present endpoint equal to the current environment.
The corrected non-strict property and Markdown now preserve that behavior.
No assumptions, axioms, disabled verification or runtime fixes were introduced.

Model verification establishes properties of these definitions, not C refinement
or complete UI coverage. The per-module inventories retain the full-host,
catalog, numerical-adapter and accessibility gaps. M11 also records the observed
uninitialized `cExcess` path as an unresolved source ambiguity rather than
assigning deterministic behavior to it.

## Remaining specifications verification

The 2026-10-24 completion batch adds M12–M20 and S02–S05: thirteen detailed
entries, thirteen canonical model files and thirteen include entry points, plus
the S03 versioned catalog companion. It preserves existing stories/anchors and
canonical shared definitions. The collection now covers all 26 first-level IDs.
M08–M11 detailed rows have been moved from the shared-support map to the detailed
index; their player-turn map entries remain in place.

Formal coverage is deliberately bounded. Research allocation, relation policy,
battle-plan/replay transitions, message selection, tutorial page steps, host
readiness, phase sequencing, mining/growth arithmetic, AI dispatch/traversal,
preferences, catalog identity/text mappings and platform/desktop projections have
named definitions and witnesses. Full simulation/AI algorithms, external I/O,
control callbacks, native rendering and accessibility remain explicitly owned
source/test/refinement boundaries in the entries. Document coverage is not a
claim that every native branch has a complete executable formal counterpart.

| Check | Actual result |
| --- | --- |
| `python3 tests/spec/verify.py --time-limit 120`, Dafny 4.11.0 | **66 Dafny files**, complete include closure: **1848 verified, 0 errors; audit 0 findings**. |
| `python3 tests/spec/verify.py --links-only` | **1582 local links/includes, 0 errors** after integration; exact casing, anchors, include paths and whitespace checked. |
| Requirement / module inventory | **51 new stories, 90 acceptance criteria, 26 interaction requirements and 39 quality requirements** (206 unique IDs, including two catalog-companion criteria). Every story maps to acceptance; no duplicate explicit anchors. All 26 module IDs appear exactly once in the detailed index. |
| Catalog identity | All **67** listed SHA-256 source/generator/schema references match current files. This establishes identity, not semantic correctness. |
| `cmake --preset macos-sdl-debug -B dist/m05-macos-sdl-debug`, then `cmake --build dist/m05-macos-sdl-debug -j 8` | Passed; no compiler warnings. |
| Dummy-SDL CTest unit / compatibility / host-library / tutorial selection | **31/31 passed** in 19.83 seconds, including all 17 unit suites and the available tutorial smoke checkpoint. Real display/accessibility and full 80-page tutorial coverage remain open. |
| Full native `make regression` | Nine scenarios through turn 150: **519 matches**, including 21 unused-storage warning matches; **0 differences/errors**, 0 reference inputs skipped. Baseline unchanged. |
| Source / diff review | Only specification files changed in this batch. Runtime code, catalogs, record layouts and regression baselines unchanged; existing unrelated work preserved. `git diff --check` passed. Host-only runtime checks were not required. |

CTest used `--test-dir dist/m05-macos-sdl-debug --output-on-failure --timeout 300`
and filter `^sdl_test_|win32_native_tutorial_.*dummy$|^win32_(utf8_.*|runtime|controls|messagebox|client_host_library_dummy)$|^test_redirect_contract$|^test_help_clean$|^macos_bundle_unit$`,
with `SDL_VIDEODRIVER=dummy`, `SDL_AUDIODRIVER=dummy`,
`STARS_SDL_RENDERER=software`, `STARS_FILE_DIALOG_BACKEND=compat`.
These directly run the unit/tutorial suites selected by `make test-unit` and
`make tutorial` against the existing validated preset build directory, with
additional relevant compatibility/host-library tests.

Regression used `GOCACHE=$PWD/dist/go-cache`,
`REGRESSION_BUILD=dist/m05-regression-build`,
`REGRESSION_WORK=dist/scaffold/regression/remaining-native` and
`REGRESSION_REPORT=dist/scaffold/regression/remaining-comparison.json`.
Logs: `dist/remaining-spec-final-reviewed.log`, `dist/remaining-links-final.log`,
`dist/remaining-configure.log`, `dist/remaining-build.log`,
`dist/remaining-runtime-checks.log` and `dist/remaining-regression.log`.

Initial model drafts failed on a reserved identifier, a generic equality
constraint, a recursive termination measure and witness proofs/timeouts. Explicit
constraints, decreasing measures, sequence witnesses and arithmetic proof steps
resolved them; no assumptions, axioms or disabled verification were introduced.
Final source review also corrected the M16 help boundary to packaged HTML export
and launch: the older runtime HLP description in the historical help notes is
not current behavior. The models preserve the source's partial-write, close/commit
and cleanup limitations rather than asserting stronger recovery guarantees.

## Completeness review and completion backlog

Reviewed 2026-10-24 against `e2eae18cff0f990db20415c1c2262aed2022f95b` and the
current working specification collection. **Document presence is complete;
rule-level formalization and implementation qualification are not.** The active
completion goal follows this register. The previous “remaining specifications”
batch completed the first-level entries, not every rule or external obligation.

The review covers all 26 entry documents, seven companions, their ownership and
requirement inventories, canonical model boundaries, and dated verification.
Source checks targeted error-prone transitions, storage widths, shared rules and
contradictory claims. It is not an exhaustive line-by-line C audit or a new claim
of C/Dafny refinement. The full verifier checks every formal file; the source
checks do not establish every source/model correspondence.

### Findings and immediate corrections

| ID / priority | Finding and evidence | Disposition |
| --- | --- | --- |
| <a id="gap-01"></a>GAP-01 / High | M04’s numbered acceptance summary said normalization preserves positive odd widths, contradicting `BoundsCheckPlayer`, `NormalizeAxis` and the pricing companion’s zero-width warning. | **Corrected.** [M04-AC07](M04-race-design.md#m04-ac07) and `NormalizationWidthVectors` distinguish observed zero/even widths from the proposed safe-pricing rejection. No runtime normalization changed. |
| <a id="gap-02"></a>GAP-02 / High | S02 modeled clamping parsed INI integers before their source storage narrowing. `WindowLayout` is uint16_t but compared as int16_t; pane sizes and `vcBackupDirs` are int16_t. Raw 65636 gives pane 100, and retention 65538 gives 2. The old scalar clamps gave different results. | **Corrected.** `ProfileI16`, `PaneSize`, `Layout`, `ConfiguredBackups` and M17 forwarding now preserve the narrowing. [S02](S02-application-preferences-and-window-layout.md) and [M17](M17-host-game-management.md) distinguish parsed integers from the normalized range test. |
| <a id="gap-03"></a>GAP-03 / Medium | M12–M20/S02–S05 interaction requirements mixed source observations, normative feedback expectations and Open accessibility work under absent or mixed status labels. This could make desired behavior look delivered. | **Corrected labeling.** IR01/IR02 are explicitly Proposed; §3 remains the source behavior, and Open qualification stays explicit. IDs are preserved. No accessibility support is newly asserted. |
| <a id="gap-04"></a>GAP-04 / Medium | `tests/spec/verify.py` checked links and proofs but did not enforce unique requirement IDs, story references or acceptance coverage. The prior inventory was a one-off check. | **Corrected.** The verifier now checks module ownership, duplicate IDs, undefined stories, criteria without stories and stories without criteria; nine positive/negative checker tests cover these failures. This checks trace structure, not semantic completeness. |
| <a id="gap-05"></a>GAP-05 / Medium | M16 correctly described packaged HTML, while its linked help notes still described loading the HLP archive and directly launching a URL at runtime. | **Corrected.** [Help notes](../docs/WIN32-HELP.md) now match `WinHelpA`/`PszHelpDocument`; original archive analysis remains historical build/reference evidence. |
| <a id="gap-06"></a>GAP-06 / High | Many E-boundaries contain ordinary representable algorithms, not intrinsically external effects. Examples: research passes, mine depletion, order resolution, all AI heuristics and tutorial task predicates. Names and source hashes do not replace these contracts. | **Open specification work.** Close the module work below with checked state/algorithm definitions, or a precise justified external boundary. Do not mark a whole family complete because one scalar witness passes. |
| <a id="gap-07"></a>GAP-07 / High | Most source-to-model adapters are supplied booleans, arrays, projections or floating traces. Proofs establish consequences of those supplied values, not that the C code supplies the right values. | **Open adapter/evidence work.** Name each extraction function, native width/domain, pre/post state and failure mapping. Add focused differential/integration cases where representable; keep actual OS effects separately qualified. |
| <a id="gap-08"></a>GAP-08 / High | Root instructions reject protected legacy races; accepted ADR 0002 and observed code describe replacement-password conversion. The disagreement spans M02/M04/S01/S06. | **Open policy alignment.** Root instructions govern this work; do not advertise the observed conversion as the supported root contract. The repository owner must authorize any policy/code/ADR alignment. This review makes no import behavior change. |
| <a id="gap-09"></a>GAP-09 / High | Publication, submission, host receipt and recovery remain different outcomes. Direct saves/generation/AI writes ignore some results; staged library operations do not make the whole application crash-atomic. | **Open state/evidence work; stronger guarantees are policy work.** [M01 publication/recovery](M01-import-publication.md) now specifies first/later turns, conflict replacement, separate copy and admitted-family publication, including query-failure exceptions, partial cleanup, restore retries and pending clearing. M02/M05/M06/M17/M18/S01 still require the shared wider outcome matrix with exact surviving files, flags and retry assumptions. Stronger transactions remain policy work. |
| <a id="gap-10"></a>GAP-10 / Medium | Performance, supported displays, keyboard equivalence, accessibility and native backend coverage have no complete acceptance matrix or agreed numeric targets. Dummy-SDL tests and “API present” counts cannot establish them. | **Open qualification/target work.** S05 plus each UI owner must identify workload/platform/input cases, measurement method and evidence. Product choices stay Open until agreed; do not invent budgets or conformance claims. |
| <a id="gap-11"></a>GAP-11 / Medium | Canonical gameplay functions remain in `M03-scoring.dfy` because they began as wizard consumers. Moving/copying them into later modules risks divergent definitions. | **Open trace integration, not a requested refactor.** Retain existing definitions and document the consumer/owner map. If moved later, update all includes/links atomically and preserve IDs; do not create a second formula. |
| <a id="gap-12"></a>GAP-12 / Medium | Older test counts and failures remain embedded in module documents; a later passing full run does not prove every missing acceptance case or erase earlier failed evidence. | **Ongoing evidence discipline.** Keep dated records and link the latest collection run. Separate model proof, native fixture, GUI smoke and manual/native qualification in each new acceptance row. |
| <a id="gap-13"></a>GAP-13 / Medium | M09 said a new Add inserted before the selected queue row; `ProdCommandHandler` tests that row and its successor, then inserts after it. | **Corrected.** `Destination`, `Locate`, `Add` and M09-AC19 cover selected/header/absent selection with source fixtures. |
| <a id="gap-14"></a>GAP-14 / High | M09's generic sanitization description hid overlapping memcpy and a diverging physical cursor in `InitProduction`; a faithful model cannot promise a safely filtered/clamped queue. | **Specification corrected; native repair remains open.** `Sanitize`, M09-AC18 and guarded source fixtures preserve the defined case and stop before undefined overlap. Known-bug/roadmap entries track UI reproduction, original comparison and a separately scoped repair. |
| <a id="gap-15"></a>GAP-15 / High | Native queue-growth capacity is an eight-bit header: write-back of 254/255 rows requests 256/257 physical rows but records maximum 0/1; a fresh selected read can allocate from that narrowed header before copying all active rows. | **Specification/native boundary recorded; runtime repair open.** S06-AC12, `Transfer` and guarded source write/read fixtures preserve partial state and allocator spare extent; exact-size heap fixtures stop before out-of-allocation copying. Native alignment is checked; host partial-prepend fixtures also confirm 255-row active/maximum header wrapping and missing-spare boundaries. Free-block reuse and UI/real-helper reachability remain open. [Known-bug evidence](../docs/KNOWN-BUGS.md#planet-queue-capacity-wrap--located-on-main) tracks UI reachability, original comparison and a separate repair. |
| <a id="gap-16"></a>GAP-16 / High | Forecast copying was being conflated with global purity: M19 said forecasts omit random increments, but the estimator inherits generation and RNG state; AR apply=true mining marks live remote fleets, and actual builder alchemy can emit messages. | **Specification corrected; native effects recorded.** M09-AC47/M19-AC08, checked controller/effect predicates and actual estimator/mining/builder/RNG fixtures preserve current behavior. [Known-bug evidence](../docs/KNOWN-BUGS.md#forecast-helper-side-effects--located-on-main) tracks full UI/AI qualification, historical comparison and any separately scoped behavior decision. |
| <a id="gap-17"></a>GAP-17 / High | Direct Genesis application zeroes planet mineral fields, but ordinary Produce later publishes its separate working mineral stocks over them. The resource-pointer parameter is not used by FBuildObject to clear that working stock. | **Observed outcome recorded; intended behavior open.** M19-AC13/M09-AC63, `Rebirth`/`FinalStock` and actual Produce/builder/application/RNG fixtures preserve this distinction. [Known-bug evidence](../docs/KNOWN-BUGS.md#genesis-stock-write-back--located-on-main) leaves full game/original comparison and any intentional behavior decision separate; no runtime change. |
| <a id="gap-18"></a>GAP-18 / Medium | M12 described switching to lowest after level 26 without distinguishing local `iTechNext=7` from packed player policy. Source lowest redirection writes current field but leaves packed next unchanged; explicit redirection writes packed next six. | **Clarified.** M12-AC20, checked `Redirect` and all-256-policy native fixtures preserve the local/persisted distinction. No runtime policy or UI behavior change; full caller/liveness qualification remains open. |
| <a id="gap-19"></a>GAP-19 / Medium | M11 summarized category/item validation as the first lookup step and omitted prior-pointer effects and nonuniform headless restrictions. Native FLookupPart reads primary first in player context; a valid denied row still assigns its pointer, and shield/armor/beam primary gates also run headless. | **Clarified.** M11-AC12–15 and the [native availability companion](M11-part-availability.md) preserve call order, pointer prefixes and headless distinctions, with checked decisions and exact actual-source/frame fixtures. No runtime behavior or validation policy changed; full caller/native memory refinement remains open. |
| <a id="gap-20"></a>GAP-20 / Medium | A TRUE availability check does not establish the base notification's later hull-read domain: FCanBuildShdef narrows its item to a byte, while LphuldefFromId indexes with the full signed hull. Raw 288 can pass the former and reach base index 256 in the latter. | **Boundary specified.** M19-AC32, Gate/HullMetadata/DecodedHullSafe and the [eligibility companion](M19-construction-buildability.md) distinguish the accesses. Current FReadShDef rejects hulls above 36; normal-file reachability is not claimed. S01/M11/M19/S06 retain complete writer/admission/lifetime qualification. No runtime validation or behavior change is made. |
| <a id="gap-21"></a>GAP-21 / Medium | RACESTATE restores the 16-byte filename cache but does not capture the 256-byte UI suggestion. A restored cache can share the suggestion's first fifteen bytes, causing the extracted FSaveRace cache-pointer selector to choose the newer full name. | **Native state boundary clarified; UI/policy qualification open.** M02-AC16/17, native Restore/Suggested and exact source fixtures preserve this outcome. M02/M04/S05 must qualify normal wizard/legacy-save reachability and decide whether cancellation should restore that extra UI state. No runtime restoration behavior changes here. |
| <a id="gap-22"></a>GAP-22 / Medium | SuggestRaceFilename checks availability after appending a numbered suffix but does not recheck the portable-name/root budget. A portable 239-byte input with a 236-byte ASCII stem and 14-byte root can grow to a rejected 241-byte `.r999` suggestion. Duplicate performs no second portable/path validation; enumeration rejects that name if written. | **Observed budget boundary specified; controller/UI qualification and behavior decision open.** M02-AC29, NameSearchHasNoFinalBudget and the [allocation companion](M02-race-file-allocation.md) retain the actual algorithm. Native Suggest/Portable tests establish the boundary; the action fixture now covers ordinary Duplicate completion under scripted parser/copy replies. M02/S04/S05 own callee/normal GUI qualification and a separate intended correction. No runtime guard is added. |
| <a id="gap-23"></a>GAP-23 / Medium | An explicit race-export target can equal the next generated temporary name. If absent, exclusive copy writes that final path before revalidation; failed validation plus failed removal returns FALSE while retaining the copied destination. | **Literal identity outcome specified; intended behavior and UI qualification open.** M02-AC34, ReplacementSelf and FailedAliasValidationRetainsFile plus the [action fixture](../tests/spec/check_race_actions_source.py) cover the complete ordinary outer controller under scripted services. M02/S04/S05 own normal picker reachability, native alias/OS qualification and any separate behavior correction. Local replacement-failure preservation is not a universal entry-to-exit destination guarantee. |
| <a id="gap-24"></a>GAP-24 / Medium | Successful current-format import binds path/name without clearing fNewLibraryRace; successful legacy save clears it. The raw flag can therefore differ for the same successful import outcome and later current-save planning reads this flag. | **Observed flag boundary specified; caller/UI reachability and intent open.** M02-AC43, NewFlagOnCurrentSuccess and native import/save/wrapper fixtures retain -7/0/1 cases. M02/M04/S05 own normal entry-state qualification and the intended later-save rule. No runtime reset or legacy policy change is added. |
| <a id="gap-25"></a>GAP-25 / Closed specification mismatch | M04Race.Finish returned Stay for read-only Finish, while RaceCreationWizard goes to cancellation/failure before pricing. | **Corrected in specification only.** Finish now returns Failure; M04-AC08, ReadOnlyFinish and actual source-controller cases cover every page/mode. No runtime or baseline behavior changes. |
| <a id="gap-26"></a>GAP-26 / Medium | RaceCreationWizard assigns global vrgrcRCW to an uninitialized local 17-RECT array and does not clear/restore the pointer. Initialized-hit/paint order, nested wizard callbacks and post-return access are not qualified. | **Lifetime boundary specified; native reachability/initialization qualification open.** M04-AC13, Begin/Complete and ReturnExpiresFrame record local lifetime without claiming valid post-return pointer use. M04/S05/S06 own access-site/lifetime mapping, initialized-slot event fixtures and nested/modal teardown cases. No actual use-after-return or runtime repair is asserted. |
| <a id="gap-27"></a>GAP-27 / Medium | IrcRaceDlgHitTest on editable habitat growth rectangles 15/16 reads rgEnvVarMax[3], outside PLAYER's three-element array. Habitat initialization exposes 17 hit rectangles; the helper does not distinguish growth before the immunity read. | **Undefined array boundary reproduced; native event qualification and repair open.** The [controller companion](M04-wizard-controller.md#habitat-growth-hit-test-boundary) and [actual helper fixture](../tests/spec/check_race_hit_source.py) cover 104 defined cases and require both UBSan array diagnostics. No post-access result is modeled. M04/S05/S06 own real geometry/event reproduction and a separate focused repair, native checks and historical comparison. No runtime or baseline change here. |

### Module-by-module completion inventory

Classes: **D** missing specification/model detail; **A** source/model adapter;
**V** runtime or platform validation; **P** product/policy decision. A row can
have more than one class. A V or P item does not authorize a runtime change.
Each row names the primary logical owner; slash-separated modules are handoff
owners, not unnamed blockers. Companion coverage is included with its entry.

| Owner / priority | Remaining work and existing boundary | Evidence required to close the specification work |
| --- | --- | --- |
| [M01](M01-game-library.md) / High | D/A/V: complete family lexing, enumeration/admission and byte-to-inspection/probe field mapping; host-orders, duplicate/rename/delete/export controller completion; native path/name comparison, source mutation and recursive copy/rename/delete helper refinement. [Import publication](M01-import-publication.md) now supplies controller programs and source-fixture outcomes; full native I/O/alias/crash qualification remains open. P: complete export versus supported-family export and missing last-played display. | One action/outcome matrix with surviving files/metadata and exact validation inputs; alias/conflict/failure cases; separate ADR/product decisions. Preserve M06 pending-state handoffs. |
| [M02](M02-race-library.md) / High | D/A/V: complete six-page paint/helper producers and native parser/path/Unicode/alias/lifetime/UI qualification. [Page commands](M04-page-commands.md) now specify all six WM_COMMAND arms and partial commit/cancellation. [Shared naming dialog](M01-library-name-dialog.md) now specifies init/command/retry/output control, and [wizard dispatch](M04-wizard-controller.md) specifies top-level Finish/recovery/sequence behavior. [Import contracts](M02-race-import.md) now compose source/name choice, current-copy/legacy save and import/load/edit wrappers with captured/global/file frames and native source fixtures. [File actions](M02-race-file-actions.md) now supply ordinary Duplicate/Export/Delete controllers, allocator composition, cleanup/notification and captured-byte/file frames with native source fixtures. [Native state/version/save](M02-native-state-and-save.md) specifies complete comparison/captured-byte projections, filename-prefix/selection, silent-reader frames, canonical scan/plan composition and staged-save normal/escaping/close/cleanup outcomes. [Allocation](M02-race-file-allocation.md) now specifies bounded stage/export naming, counter wrap, copy receipts and first-free suggestions with actual source fixtures. P: GAP-08, GAP-22 final duplicate-name budgets, GAP-23 target/temp identity, GAP-24 current-import new flag, dotted-stem duplication and corrupt-entry display. | Source-to-model field mapping and version/credential/failure cases, including version 999 and filename/name disagreement. Explicit effective policy plus observed deviation. |
| [M03](M03-new-game-wizard.md) / Medium | A/V: full widget-to-normalized setup mapping, batch/wizard distinctions, RNG/catalog inputs and failure matrix. P: deliberate changes to substitution, overflow, tie or retry behavior. | Trace each setup control/normalization input to its definition and source domain; vectors for boundary lattice values and controlled RNG/clock handoff to M05. Existing scoring/victory definitions remain canonical. |
| [M04](M04-race-design.md) and companions / High | D/A: all paint arms and complete drawing/pricing mutation, general catalog/format/native extraction and native cancellation frames. [Text producers](S03-player-labels-and-race-titles.md) now specify player/plural/article/fallback/grammar and pinned title/default catalog bindings, with actual decoder/formatter fixtures. [Mouse/message contracts](M04-pointer-and-message-interactions.md) now specify hit testing, portrait/habitat/economy tracking, cursor and color/erase arms. [Initialization](M04-page-initialization.md) now specifies all six entry arms, preset/control requests and habitat geometry. [Command contracts](M04-page-commands.md) now specify all six WM_COMMAND arms, preset/AR transitions and page-1 ordinary partial/successful commit. V: pricing differential coverage and GAP-26/27 rectangle-lifetime/growth-hit qualification. P: proposed Rust/SQL interface and GAP-08. | Complete page/event/guard/effect/return table with checked transitions and M02/M03 snapshot handoffs. Zero/even-width and read-only Finish specification mismatches are fixed; top-level dispatch/sequence now has checked contracts, while page-control/native behavior remains open. SQL implementation is not required merely to document its proposal. |
| [M05](M05-universe-creation.md) / High | D/A: random-race/starting-design helper closure and exact tutorial context; V: controlled creation/output/failure matrix. P: bounded placement cancellation and stronger publication. | Account for each helper/catalog/RNG call with pre/post state and draw order; validate initial decoded file sets and failure remnants. Preserve the observed absence of a total retry bound. |
| [M06](M06-player-session-and-turn-orders.md) / High | A/V: direct-open/close/reload versus guarded departure, late editor flush, ignored saves, waiting/mark/reload races and session invalidation. Post-open marker/fallback decisions now have an explicit M06 contract, with M07 restoration dispatch. P: durable submission/host handshake. | Cross-entry outcome matrix separating memory intent, successful orders/history, export and receipt; precise failure traces. No file flag described as a lock. |
| [M07](M07-galaxy-map-and-navigation.md) / Medium | D/A/V: full SelectAdj*/ChangeMainObjSel editor/cache effects, title/tutorial callbacks and reentrancy; native hit/capture/paint interleavings, trig/hypot/raster paths, 12-entry menu capacity, all views/disclosure and preference teardown. Restoration dispatch, fallback ordering, the post-open wrapper and construction-capable starbase target selection are now modeled. | Bind coordinate/selection projections to source inputs and test boundary/capture cases; keep exact pixels as rendering obligations with platform/display fixtures. |
| [M08](M08-planet-and-colony-management.md) / High | D/A: E08-2 full caller/forecast/phase and floating-trace refinement (catalog/bounds/habitat/best-axis/optimum/remote/application controllers now specified with actual source fixtures), E08-3 destination/upstream permission/disclosure and full caller qualification (stargate/scanner producers and signed terraforming totals now specified with actual lookup and production/application handoffs). The complete native mass-driver visibility/slot/output controller now has checked contracts, scalar refinement and actual construction/packet/Produce fixtures. Native capacity/resources/installations, growth/carry/writeback and numeric handoffs now have checked contracts and source fixtures; full lookup/habitat/helper frames and floating trace production remain open. V: pane lifecycle/focus/layout. The complete numeric mining/depletion/AR remote loop is now specified under M19, with M10 mining capacity. | Reuse M19 growth/mining and native terraforming controllers; qualify full caller copy/restore, field/alias/float adapters, foreign/unknown/owned views and base abilities. Do not duplicate resource/capacity formulas. |
| [M09](M09-production-management.md) / High | D/A: E09-1 getter/catalog/pointer refinement and full log/heap/selection/tutorial/modal callback effects (S06 now specifies native read/write queue-copy effects; lifecycle call order, accept/cancel/direct Clear, Next/Back, recovery, initial selection, catalogue assembly, sanitization boundaries, commands and presets now modeled), E09-2 M11/S03 quote and native caller extraction (full cost vectors, slot/native-width/trait/quantity caller arithmetic now modeled), E09-3 helper/pointer/caller-context refinement (full forecast controller, copy, native budget/query, partial/row/status/output/cleanup rules and M08 native economy handoffs now modeled; GAP-16 global effects recorded), E09-4 retrying-alchemy termination and full helper/pointer/callback/object/annual-state refinement for Produce/FBuildObject; applied construction families and queue invalidation now have M19/M10/M09 contracts; builder output handoffs, host budgets/validation and queue removal/prepend/application routing are now modeled; M19 now composes the real M11 build-check outcome through construction eligibility/metadata, with actual application and post-charge Produce integration cases. | Checked queue/economy state transitions, progress/cost conservation where the source guarantees it, and invalidation/partial-construction cases. Distinguish simulation from forecast assumptions. |
| [M10](M10-fleet-operations.md) / High | D/A: complete waypoint/task defaults, target selection and native modal/lifetime behavior; packed damage beyond the now specified construction-stack algorithm, merge/split allocation effects, fuel/gate/movement and host task execution (E10-1–4). Fleet mining capacity and M19 numeric mining/depletion effects are now specified; complete remote-task phase/target/cancellation behavior remains open. Preset application/customization and a bounded quantity/action refresh adapter now have checked contracts. | Task-by-mode prerequisites/outcomes with exact RNG/rounding, reference/cargo changes and partial effects; typed packed-damage adapter and focused movement/task vectors. |
| [M11](M11-ship-and-starbase-design.md) / High | D/A: E11-1 full native memory/alias/lifetime/caller-context extraction and browser/knowledge/annual integration (the complete native availability table, FCanBuildShdef controller/ordinary restoration and lookup-derived M12 scan now have checked contracts and actual helper/catalog composition fixtures; M19 now binds the outcome into construction guard/roster/metadata contracts with actual FBuildObject/Produce integration cases); E11-2 deletion cascade; E11-4 full derived stats/caches and pricing order. V: native slot gestures. | Catalog-to-design mappings and all-stat/cost vectors, copy restrictions and populated deletion/decline outcomes. Reuse S03 IDs and existing M03 numerical functions. |
| [M12](M12-research-and-technology.md) / High | D/A: E12-1 native quote/preview memory/alias/array/helper/caller-state refinement and real browser/trader qualification. E12-3 now has complete host/equipment operational transitions: player/redo/native spending/policy, breakthroughs, lookup/notice ordering, shared grants and recursion. D: catalog-bounded scan completion and normal-level host rank descent now have checked contracts; prove successful Done completion from native-derived arithmetic/policy/storage/returning-service frames. A/V: actual host row/context extraction for the now specified M11 availability table/M12 complete-scan adapter, plus full native message/global/heap/phase/caller-state refinement and saved-game/UI/platform qualification. Browser/category/trader adapters remain E12-2. | Native-domain successful Done theorem and actual helper/catalog/state extraction; complete phase/trader/browser integration and qualification. Model termination at Pending/Escaped/Undefined and native converging cases do not prove universal successful C completion. |
| [M13](M13-player-relations.md) / Medium | A/D: E13-1 live dialog/log state; E13-2 exact directed gate/mine/terraform/transfer prerequisites beyond attack intent. | Per-consumer relation direction table plus checked eligibility predicates and asymmetric witnesses. Selection/mutation may be live even when closing through IDCANCEL. |
| [M14](M14-battle-planning-and-review.md) / High | D/A: E14-1 full copy/rename/delete/reference state; E14-2 packed record traversal, reconstruction, corrupt indices and replay lifetime. | Plan-state transitions including copy-then-cancel and used-plan delete/decline; bounded record-to-token adapter and replay cleanup cases. M19 remains combat owner. |
| [M15](M15-messages-and-reports.md) / Medium | D/A: E15-1 packed-message/ID/filter mapping, goto/title, suppression and draft lifecycle; both traversal directions and eligible-neighbor properties are now modeled; E15-2 every report column/comparator/cache/knowledge projection. | Retain proved traversal properties and source fixtures; close packed-buffer/native-domain mapping, per-column value/sort rules, draft failure cases and viewpoint-specific exports; qsort16 ties remain S04-owned. |
| [M16](M16-tutorial-and-help.md) / High | D: E16-1 all 80 task predicates/hints/auto-actions and turn gates; A: HTML export/context/asset pipeline E16-2; V: full tutorial/native browser. | Page/task dependency inventory with checked predicates and outcomes; an explicit 80-page checkpoint plan. Passing `Advance` with a supplied boolean does not specify when a task is true. |
| [M17](M17-host-game-management.md) / High | D/A: E17-1 complete CLI/INI grammar, return codes and log classification; E17-2 backup/host UI failure sequencing. P: concurrency/cancellation. | Entry/option/error matrix, raw parsing versus narrowing contract, host-readiness/file identity adapter and recovery traces. S02 narrowing correction is integrated; full parsing is still open. |
| [M18](M18-turn-generation.md) / High | D/A: E18-1 replay/normalization/follow flags, exact phase effects/RNG and cleanup; E18-2 publication outcomes. | A year state machine with failure edges and phase-local state contracts, not only the successful schedule cursor. Use gameplay-owned effects and S01 output outcomes. |
| [M19](M19-game-rules-and-resolution.md) / High | D: E19-1–6 full economy-phase/caller-state integration; movement/gates/mines; orders/invasion; combat; objects/events; knowledge/score integration. Native colony growth/carry/writeback, capacity/resources/installations and complete numeric mining/depletion/ordered AR remote effects now have checked contracts; caller/helper/state refinement remains open. | Decompose these families into focused companions with complete state/input/RNG/width contracts and positive/negative witnesses. Reuse M08–M13/M03 rules. Hashes/function names and scalar projections are not algorithm completion. |
| [M20](M20-ai-players.md) / High | D: E20-1 strategy thresholds, role/target/design/queue/research choices and branch RNG; A/V: E20-2 history/order/persistence/cleanup. P: fairness/strength/visibility expectations. | Per-strategy decision tables and checked transitions, complete view/history input contract and deterministic decision fixtures. Mode dispatch is not a specification of the seven strategies. |
| [S01](S01-game-file-formats.md) and four companions / High | A/V: decoder/writer/replay refinement, malformed record/count/reference boundaries and partial writes (S01-D02–06). P: D01 legacy policy, stronger decoding/publication; D07 unsupported tags. | Per-entry/per-record accepted/rejected/error-effect matrix and byte/state differential fixtures. Keep proposed strict predicates separate from actual reader behavior; no undocumented semantics for unsupported tags. |
| [S02](S02-application-preferences-and-window-layout.md) / Medium | D/A: E02-1 outer loader/writer/error/path and source-adapter refinement; E02-2 selection/identity formatting, full live selection/editor/title/tutorial callback effects, full M09/M10 editor integration beyond checked preset/quantity commands and conditional tile round trip. M07 now models restoration dispatch and callback returns; M15 models bidirectional traversal. The [contracts companion](S02-preference-contracts.md) now covers active keys, desktop/host grammar, buffer/save control, tile/placement parsing, saved selection/GameID decoding, MRU partial updates and preset codecs. V/P: multi-process/display policy. | Retain the checked decode/codec contracts and native fixtures; close outer-call effects, byte/locale/path mapping and full live callback effects with M06/M07/M15/M16, M09/M10 editor integration beyond their checked preset/quantity commands, and the conditional tile round trip. Preserve entry-specific desktop/host parsing and signed-overflow boundaries. |
| [S03](S03-static-game-data-and-text.md) and inventory / Medium | [Player-label/title contracts](S03-player-labels-and-race-titles.md) now specify ordinary naming, conditional string reads, scratch effects and fifteen generated catalog bindings. D/A: E03-1 specialized generator references/schema checks, E03-2 text/rank/chunk/range/placeholder checks; V: compiled consumer equivalence. | Generator input/output/error contracts and negative fixtures; field units/ranges linked to actual consumers. Exact values are pinned by 67 hashes; that is provenance, not semantic validation. |
| [S04](S04-platform-services.md) / High | D/A: E04-1 filesystem modes/result mapping, E04-2 full conversion/format/prefix behavior, E04-3 qsort16 and native-width/clock adapters. V: actual OS effects. | Exact caller input domains, error/output/ownership semantics and checked pure algorithms; platform fixtures for filesystem, clock and float effects. Avoid extrapolating mathematical ints to every native input. |
| [S05](S05-sdl-desktop-implementation.md) / High | D/A: E05-1 control/modal/event state, E05-2 lifetime/reentrancy/teardown; V/P: E05-3 rendering/native APIs/accessibility/display matrix. | Bounded control/window/timer state contracts with failure edges; supported API/flag behavior and targeted lifecycle fixtures; separately recorded native/visual/assistive-technology qualification. |
| [S06](S06-shared-game-state.md) / Medium | A/D: full field/projection/cache extraction, knowledge merging, packed/transient state and phase lifetimes. Native planet/fleet lookup control and planet read/write prefix/queue copying now have checked S06 contracts and native struct fixtures; full array invariants, allocator/log/tutorial effects, pointer lifetime and complete semantic field mapping remain open. P: strict closure, duration overflow, mid-turn restart and legacy policy. | Logical↔C↔S01 field/identity/unit matrix with missing adapters named; field-level knowledge provenance and native lifecycle cases. Proposed full-host invariants must not become silently enforced runtime checks. |

### Execution order and definition of done

1. **Source accuracy and trace discipline:** preserve the completed accuracy
   corrections/boundaries (GAP-01–05 and GAP-13–21) and validate the entire inventory. Preserve all existing IDs/anchors and dated
   evidence. Add explicit boundary domains wherever a source narrowing or
   lookup is currently hidden in a mathematical argument.
2. **Shared inputs and workflow contracts:** S01/S02/S03/S04/S06 adapters,
   M04/M01/M02/M06 workflows, and M17/M18 failure/result state. This supplies
   trustworthy inputs and ownership for the later rule models.
3. **Gameplay rule closure:** M08–M15 with M19 companions, beginning with economy,
   research and orders, then movement/hazards, combat and objects/knowledge.
   Every representable rule needs its checked definition and boundary witnesses.
4. **AI and tutorial closure:** M20’s seven strategies and M16’s 80 task checks,
   reusing the completed gameplay contracts and recording exact RNG/phase inputs.
5. **Qualification and decisions:** complete achievable fixture/adapter checks;
   present concrete unresolved policy/target choices and native/assistive-tech
   qualification work to their owners. Neither elapsed time nor a model proof
   supplies a decision or a manual/native test result.

A D/A item closes only when its declared rule/adapter inventory has no unnamed
holes, its normal/alternative/cancel/failure outcomes are source-backed, its
representable definitions and useful witnesses verify, and its acceptance rows
identify both implementation evidence and the actual test/proof or explicit
remaining V/P obligation. A blanket “native adapter” label is not closure for an
ordinary unmodeled algorithm. Existing safe-domain restrictions must identify
which native values are outside the domain and what remains unknown there.

The goal remains **active** while actionable specification/model items above
remain. V/P work is reported separately; product decisions and runtime changes
are not silently approved by this documentation task. Completion must state
exactly which obligations are finished and which externally owned qualification
or decision boundaries remain. Passing all model obligations is necessary but
insufficient to call the full behavior specification complete.

### Review verification

Recorded 2026-10-24 for this review batch:

- `python3 tests/spec/verify.py --time-limit 120`: Dafny 4.11.0, all 66
  formal files with included-file verification, **1885 verified, 0 errors**;
  audit **0 findings**. The same run checked **1616 local links/includes** and
  **507 requirement IDs** (116 stories, 226 acceptance, 66 interaction and 99
  quality requirements), with zero errors. Log: `dist/spec-review-full.log`.
- `python3 -m unittest discover -s tests/spec -p 'test_*.py'`: **9 passed**.
  Every module appears exactly once in the completion inventory; comparison
  with the pre-review snapshot found no removed explicit requirement anchors.
- The focused M04/S02/M17 run finished with **99 verified, 0 errors**.
  Earlier attempts timed out or failed on concrete unfolding obligations;
  explicit intermediate arithmetic facts and a checked shared lemma resolved
  them without assumptions or weakened contracts. Log:
  `dist/spec-review-model-fourth.log`.
- A temporary C11 witness compiled the actual layout type and layout/pane
  normalization expressions extracted from `enums.h`/`init.c`. It confirmed
  the signed-width cases, including 32768 → layout 0 and 65636 → pane 100.
  This is expression evidence, not a complete INI/parser/GUI integration test.
- `cmake --preset macos-sdl-debug -B dist/m05-macos-sdl-debug` and
  `cmake --build dist/m05-macos-sdl-debug -j 8`: passed, with no new warnings.
  The selected dummy-SDL unit, compatibility and tutorial-smoke suite passed
  **31/31**. Logs: `dist/spec-review-configure.log`,
  `dist/spec-review-build.log`, `dist/spec-review-runtime.log`.
- Full native `make regression`, using `dist/m05-regression-build` and the
  `spec-review-native` work directory: **519 matches, 21 unused-storage
  warnings, 0 differences/errors, 0 skipped reference inputs**. Report:
  `dist/scaffold/regression/spec-review-comparison.json`. No baseline changed.
- `git diff --check`: passed. No game/runtime source changed; host-specific
  game-code checks were not required for this documentation/model batch.

These results establish the checked collection's internal consistency and the
recorded runtime regression outcomes. They do not close GAP-06–12 or the
module-specific missing algorithms, adapters, policy decisions and native
qualification listed above.


## S02 shared-contract continuation

Recorded 2026-10-24 at the same inspected source revision. The previous goal
turn made concrete progress by correcting inconsistencies and creating the
completion register. This continuation closes further D/A portions of S02;
it does not close the whole module or the collection goal.

The [new companion](S02-preference-contracts.md) inventories every active
preference key, source default, storage width, normalization and write condition,
including library and headless consumers. A source-selection check accounted for
all 46 statically selected reader/writer key names; dynamic placement, fonts,
MRU and preset families were separately checked. Eight technical companions now
support the 26 entry documents. All prior explicit requirement anchors remain.

New checked definitions cover desktop line parsing, first-section/first-key
lookup, quoting/defaults, uint32 integer parsing, ordered key/section mutations,
tile swapping and trailing-column updates, placement parsing, writer tile
encoding, and additional scalar width rules. Six new criteria trace these to
the existing S02 stories. `PrefixPreserved` proves consumed tiles cannot be
changed by subsequent parsing; it is not a full round-trip theorem.

Source checks corrected or clarified four misleading generalizations: malformed
profile reads return defaults while retaining the load error; tile case means
expansion and omitted tiles can reorder through swaps; bit assignments differ
from nonzero tests; four-bit graph storage precedes its clamp. The Wait writer’s
retained numeric suffix is also recorded as observed behavior, without changing
runtime behavior. The companion explicitly lists the still-missing representable
contracts and native qualification; GAP-06/07 remain open across the collection.

Verification:

- `python3 tests/spec/verify.py --time-limit 120`, Dafny 4.11.0, all **68 formal
  files** and their includes: **2027 verified, 0 errors; audit 0 findings**.
  Log: `dist/spec-s02-full.log`. Early focused attempts exposed one incorrect
  negative placement witness and insufficient unfolding; the valid embedded-minus
  outcome was corrected and explicit checked intermediate facts resolved the
  proofs. Final focused run: **142 verified, 0 errors**, with no assumptions.
- Requirements: **513 IDs**, comprising 116 stories, 232 acceptance, 66
  interaction and 99 quality requirements, with zero trace errors. Nine verifier
  unit tests pass. Final link/anchor check passes; the full-run log's link count
  precedes the final README/companion cross-links.
- `python3 tests/spec/check_s02_source.py`: **21 C witnesses pass**, compiled with
  C11 and warnings as errors. The harness extracts current functions and stubs
  profile input/storage, exercising actual tile/placement/integer code against
  the model's concrete cases. It does not claim full GUI/OS refinement.
- SDL Debug configure/build passed without new warnings. The selected runtime
  suite initially passed **30/31**, with `win32_client_host_library_dummy` timing
  out after a startup alert. Its isolated retry passed in **5.69 seconds**; no
  source or fixture was changed for the retry. Keep the initial timeout visible:
  `dist/spec-s02-runtime.log` and `dist/spec-s02-host-library-retry.log`.
- Full native regression: **519 matches, 21 unused-storage warnings, 0
  differences/errors, 0 skipped reference inputs**. Report:
  `dist/scaffold/regression/spec-s02-comparison.json`. Baselines unchanged.
- `git diff --check` passes. No runtime/game source changed, so no behavior
  changelog or host-specific game-code regression was required.

The goal stays active. Next shared-input work should close the companion's
remaining profile buffer/save and entry-specific parsing contracts before
claiming S02 complete, then continue the cross-module order above.

## S02 profile I/O and host grammar continuation

Recorded 2026-10-24 at `e2eae18cff0f990db20415c1c2262aed2022f95b`.
The preceding goal turn made concrete progress on S02's inventory and basic
contracts. This continuation closes additional representable rules from its
backlog, without changing the collection goal or declaring native refinement.

The [profile I/O contracts](S02-preference-contracts.md#profile-buffers-validation-and-save-outcomes)
now define scalar and enumeration buffer mutation/counts, accepted write
arguments, raw-row serialization, and ordered save failure/cleanup outcomes.
The [host grammar](S02-preference-contracts.md#headless-profile-grammar) has
separate checked line/traversal rules and a native-long conversion domain.
Shared byte-level UTF-8 prefix definitions belong to S04 and are reused by S02;
M17 consumes S02's headless grammar. Five new criteria retain story ownership at
S02. Existing requirement IDs and anchors remain intact.

The source makes distinctions now captured explicitly: a truncated multibyte
list may report capacity-2 despite writing only NULs; a BOM-write failure does
not explicitly set the last-error value; cleanup can fail; and the host's
malformed section header preserves its previous section state. Model-defined
`OutsideLongDomain` denotes an undefined conversion input, not a runtime error
or promised fallback. No stronger durability or import policy is introduced.

Verification:

- Focused verification of all four touched model files: **810 verified, 0
  errors** (`dist/spec-s02-io-focused14.log`). Earlier attempts failed or timed
  out while unfolding UTF-8/parser recursion and proving sequence identities.
  Checked prefix/search lemmas, explicit slice/concatenation facts and
  line/traversal composition resolved them. All algorithm bodies remain checked;
  opacity controls proof expansion, not verification or trust.
- `python3 tests/spec/verify.py --time-limit 120`, Dafny 4.11.0: all **71
  formal files** and their include closure, **2764 verified, 0 errors; audit
  0 findings**. Log: `dist/spec-s02-io-full.log`. The final documentation check
  covers **1663 local links/includes** with zero errors; its count includes
  cross-links added after the full run began.
- The requirement checker accounts for **518 IDs**: 116 stories, 237 acceptance,
  66 interaction and 99 quality requirements. Nine checker unit tests pass;
  links/anchors and `git diff --check` pass.
- Source-extracted C witnesses: **21 existing layout/integer cases**, **25 new
  buffer/host/UTF-8 cases**, and **46 new save-control cases**, all passed with
  C11 and warnings as errors. Commands: `python3 tests/spec/check_s02_source.py`,
  `python3 tests/spec/check_s02_io_source.py`, and
  `python3 tests/spec/check_s02_save_source.py`. The latter injects POSIX save
  failures into the actual source body with syscall doubles; it does not claim
  real OS-failure or crash qualification.
- Later C04 continuation: the [save-control runner](../tests/spec/check_s02_save_source.py)
  now links the [production serializer](../compat/win32/src/profile_save.cpp)
  through typed I/O effects. The current C03 parent receipt's 42 save assertions
  and original domains/exclusions are preserved in the [linked fixture](../tests/compat/profile_save.cpp).
  See the [C04 migration receipt](../docs/CPP-C-IMPLEMENTATION.md#c04-first-effect-family-and-c05-integration).
  This updates test integration, not the historical proof counts or canonical rules.
- SDL Debug configure/build passed without new warnings. All **31/31** selected
  unit/compatibility/tutorial tests passed, including the host-library test that
  timed out in the previous batch. Logs: `dist/spec-s02-io-configure.log`,
  `dist/spec-s02-io-build.log`, `dist/spec-s02-io-runtime.log`.
- Full native regression: **519 matches, 21 unused-storage warnings, 0
  differences/errors, 0 skipped inputs**. Report:
  `dist/scaffold/regression/spec-s02-io-comparison.json`. Baselines unchanged.
  No game/runtime source changed; host-specific game-code checks were not needed.

S02 remains open for selection/GameID/MRU/template state models, the conditional
tile round-trip theorem, outer loader/writer/error/path effects and complete
source-adapter mapping. Real filesystem faults, concurrency, displays and
accessibility retain their V/P ownership. The goal stays active across these and
the other modules' gap-register items.

### S02 saved-state and preset continuation

Recorded 2026-10-24 against `e2eae18cff0f990db20415c1c2262aed2022f95b`.
The [S02 companion](S02-preference-contracts.md#selection-identity-and-recent-file-state)
now specifies saved selection and GameID parsing, original-File1 startup versus
MRU compaction, insertion, deletion and partial per-key failures. The
[preset contracts](S02-preference-contracts.md#transport-and-production-preset-codecs)
add transport and production codecs, packed-row normalization, forced P1 defaults
and conditional round-trip proofs. Six added criteria retain S02 story ownership.
M01/M07/M09/M10 link to these canonical rules instead of duplicating them.

The review distinguishes five persisted production slots from the four offered
and snapshotted by the M09 UI. It also preserves disabled-selection field
retention, uppercase GameID behavior and signed-overflow limits, ignored MRU-add
write errors, partial MRU-deletion memory changes, and rejection of the two-byte
empty production preset. No runtime behavior or baseline was changed.

Verification:

- Focused verification of the two new models passed **153 obligations, 0
  errors** (`dist/spec-s02-state-focused8.log`). Earlier attempts exposed
  missing sequence/folding proof facts and one incorrectly chosen rejection
  witness; the checked proofs and witness were corrected. The final full run
  additionally includes the separate startup File1 input and its postconditions.
- Final `python3 tests/spec/verify.py --time-limit 120`, Dafny 4.11.0:
  all **73 formal files** with included-file verification, **2921 verified,
  0 errors; audit 0 findings** (`dist/spec-s02-state-full.log`). The prior full
  run passed 2917 before the separate startup-read correction; it is retained
  as `dist/spec-s02-state-before-startup-full.log`. The final run verifies the
  corrected inputs and postconditions; no assumptions or disabled proofs were
  introduced.
- **1680 local links/includes** and **524 requirement IDs** (116 stories, 243
  acceptance, 66 interaction and 99 quality) pass. All prior explicit requirement
  anchors are retained; all nine requirement-checker unit tests pass.
- The existing three source scripts pass **21 layout/integer**, **25 I/O/host**
  and **46 save-control** checks. New `check_s02_state_source.py` passes **42**
  assertions, including distinct results for the two File1 reads and every
  deletion-write failure stage. `check_s02_presets_source.py` passes **131086**
  assertions, including reader/writer checks over all **65536** packed production
  words. Both new scripts compile source-extracted C11 with warnings as errors
  and undefined-behavior instrumentation; out-of-domain signed arithmetic is
  deliberately not executed. Operation doubles do not qualify actual OS faults.
- SDL Debug configure/build passed without warnings in the build log; the
  selected dummy-SDL unit/compatibility/tutorial suite passed **31/31**. Logs:
  `dist/spec-s02-state-configure.log`, `dist/spec-s02-state-build.log`, and
  `dist/spec-s02-state-runtime.log`.
- Full native regression passed: **519 matches, 21 unused-storage warnings,
  0 differences/errors, 0 skipped reference inputs**. Report:
  `dist/scaffold/regression/spec-s02-state-comparison.json`. Baselines unchanged.
- `git diff --check` passes. No game/runtime source changed; host-specific
  game-code checks were not required.

Remaining S02 work is explicit: selection/identity formatting, the live
RestoreSelection/message/tutorial callbacks, M09/M10 preset application/edit/cancel,
conditional tile round trip, outer loader/writer/error/path behavior and complete
source-adapter mapping. Real filesystem faults, concurrency, displays and
accessibility retain V/P ownership. The all-module completion goal remains active.

### Selection restoration and message navigation continuation

Recorded 2026-10-24 against `e2eae18cff0f990db20415c1c2262aed2022f95b`.
[M07 restoration](M07-galaxy-map-and-navigation.md#61-restoring-selection-after-opening-a-turn)
now has a checked controller with explicit post-callback state, home/planet/fleet
fallback precedence, null-fleet termination and scanner-window command wrapper.
M06 specifies the caller's saved-marker decision and possible additional fallback.
[M15](M15-messages-and-reports.md#message-creation-filtering-and-navigation)
adds reverse traversal, end/sentinel handling, nearest-eligible proofs and both
neighbor round trips. [S06](S06-shared-game-state.md#native-lookup-adapters)
owns native planet/fleet binary lookup, packed-ID masking and count-prefix bounds.
Seven new criteria retain their module/story ownership. S02 and M16 link the
storage/tutorial handoffs to these contracts.

The review preserves distinctions that a generic restore-success boolean would
hide: a found fleet pointer need not become the main selection; the original
unmasked fleet ID is passed to the selection callback; a cleared saved marker is
not an acknowledgment of UI success; callback state can affect later fallback;
and FOpenGame may select again when the original marker was zero. Native null
probes, message-ID decoding failure and the final int16 end-cursor increment are
explicit domain limits, not invented fallback results. No runtime behavior changed.

Verification:

- Focused verification of all four changed model modules passed **268 verified,
  0 errors** (`dist/spec-restore-focused6.log`). Earlier verification exposed
  a missing result-bound precondition in the inverse-navigation theorem;
  checked postconditions on the canonical traversal functions resolved it.
  No assumptions, axioms or disabled verification were introduced.
- `python3 tests/spec/verify.py --time-limit 120`, Dafny 4.11.0: all **75
  formal files** and their include closure, **3159 verified, 0 errors; audit
  0 findings**. Log: `dist/spec-restore-full.log`.
- **1711 local links/includes** and **531 requirement IDs** pass: 116 stories,
  250 acceptance, 66 interaction and 99 quality requirements. All prior explicit
  requirement anchors are retained, and all **9** checker unit tests pass.
- `python3 tests/spec/check_restoration_source.py`: **6512 assertions passed**.
  The harness extracts actual C bodies for message navigation, lookup, fallback,
  restoration, post-open dispatch and the FOpenGame tail, using projected native
  structs and logged UI doubles. It compiles with C11, warnings as errors and
  fatal undefined-behavior instrumentation. It includes exhaustive small-view
  filter/cursor cases and explicit callback mutations; it does not establish
  actual callback effects, corrupt-pointer safety or whole-program refinement.
- SDL Debug configure/build passed with no warnings in the build log. Selected
  dummy-SDL unit/compatibility/tutorial tests passed **31/31**. Logs:
  `dist/spec-restore-configure.log`, `dist/spec-restore-build.log`,
  `dist/spec-restore-runtime.log`.
- Full native regression: **519 matches, 21 unused-storage warnings, 0
  differences/errors, 0 skipped inputs**. Report:
  `dist/scaffold/regression/spec-restore-comparison.json`. Baselines unchanged.
- `git diff --check` and new-file whitespace checks pass. No game/runtime source
  changed; host-specific game-code checks were not required.

Remaining work: full selection/editor/cache callback effects, message title/goto
and packed-buffer adapters, tutorial predicates/auto-actions, pointer lifetimes,
reentrancy and native rendering/focus qualification. The module completion rows
retain those obligations alongside the wider gameplay and platform backlog.
The all-module goal remains active.

### Production preset command continuation

Recorded 2026-10-24 against `e2eae18cff0f990db20415c1c2262aed2022f95b`.
[M09 production presets](M09-production-management.md#32-templates-and-automatic-policies)
now have a checked command model reusing the S01 order fields and S02 packed
codec. It covers availability release with native ten-bit wrap/unlimited state,
ordered replacement and trait filtering, full allocation versus eight-bit
headers, command Clear, payload copy/rename/delete, nested and outer cancellation,
label projection and allocation-escape cleanup. Six added acceptance criteria
preserve story ownership; S02 links the command/storage boundary.

Source review records behavior a generic successful import or transactional
Cancel would hide: import returns availability without reserving retained/new
rows again; it bypasses Add's row limit; 256 rows narrow the active header to
zero; cancelled customization leaves dirty state; rename cancellation also
marks dirty; and allocation cleanup does not restore research. These are source
observations, not approved gameplay changes. Missing matches at a full physical
catalog are undefined access boundaries, not invented error returns.

Verification:

- Focused M09 verification passed **242 verified, 0 errors** in
  `dist/spec-production-focused6.log`. Earlier failures exposed an inferred
  packed-word bound, a sequence-membership witness and insufficient recursive
  unfolding. A checked packing function, explicit witness and bounded proof
  unfolding resolved them; no assumptions, axioms or disabled verification.
- `python3 tests/spec/verify.py --time-limit 120`, Dafny 4.11.0: all **76 formal
  files** with include closure, **3401 verified, 0 errors; audit 0 findings**.
  Log: `dist/spec-production-full.log`.
- **1721 local links/includes**, **537 requirement IDs** (116 stories, 256
  acceptance, 66 interaction and 99 quality) and all **9** checker unit tests
  pass. Comparison with the pre-batch snapshot found no removed explicit anchors.
- `python3 tests/spec/check_production_presets_source.py`: **13,139 assertions
  passed**, including all **3,328** native count/preset-count pairs. Extracted
  source and structs compile as C11 with warnings as errors and fatal
  undefined-behavior instrumentation. GUI/localization and allocation doubles
  provide controlled outcomes; this is not full C/model refinement or native
  rendering/heap-failure qualification.
- SDL Debug configure/build passed without build warnings. Selected SDL unit,
  compatibility and tutorial-smoke tests passed **31/31**. Logs:
  `dist/spec-production-configure.log`, `dist/spec-production-build.log`,
  `dist/spec-production-runtime.log`.
- Full native regression: **519 matches, 21 unused-storage warnings, 0
  differences/errors, 0 skipped reference inputs**. Report:
  `dist/scaffold/regression/spec-production-comparison.json`. Baselines unchanged.
- `git diff --check` and changed/new-file whitespace checks pass. No game/runtime
  source changed; host-specific game-code checks were not required.

Remaining work includes complete initialization/sanitization/Add/Remove/Next
and modal lifecycle, production costs/forecast/host execution, M10 transport
presets and all other open module rows. The all-module goal remains active.

### Transport preset and quantity control continuation

Recorded 2026-10-24 against `e2eae18cff0f990db20415c1c2262aed2022f95b`.
[M10 transport commands](M10-fleet-operations.md#311-applying-transport-presets)
now specify built-in versus custom payload changes, menu indexing/bounds,
copy/rename/delete, nested and outer cancellation, cargo/action selection,
int16 amount parsing/clamping, and the bounded single-line notification path.
Seven added criteria trace these contracts to player route/task stories. S02
retains storage ownership and S06 retains typed action meaning; M10 preserves
raw nibble/quantity state separately from proposed validated task inputs.

The initial preset write and post-refresh state are different observations:
built-ins can preserve quantities, then the selected control synchronously
normalizes one of them, including disabled quantity controls. Write-back return
values do not establish file publication. Invalid supplied popup ranks can scan
past the four slots; the model marks undefined access, not a native error result.
Source review also corrected the M09/M10 outer modal result adapter: both callers
store DialogBox's return in int16_t before deciding rollback. Shared helpers and
both native fixture wrappers now retain that width; normal 0/1 results are
unchanged. No runtime/game behavior changed.

Verification:

- Focused M09/M10 models: **496 verified, 0 errors** in
  `dist/spec-transport-focused4.log`. Early attempts exposed lambda index domains,
  nested sequence predicate unfolding and missing explicit power-of-two facts;
  indexed constraints and checked intermediate facts resolved them. No assumptions,
  axioms or disabled verification were added.
- `python3 tests/spec/verify.py --time-limit 120`, Dafny 4.11.0: all **77 formal
  files** with include closure, **3655 verified, 0 errors; audit 0 findings**.
  Log: `dist/spec-transport-full.log`.
- Final link/trace pass: **1737 local links/includes**, **544 requirement IDs**
  (116 stories, 263 acceptance, 66 interaction and 99 quality), zero errors.
  All **9** checker unit tests pass; all prior explicit anchors are retained.
  Log: `dist/spec-transport-links.log`. The formal run's earlier link pass counted
  1736 before the final Markdown link to the shared M09 correction was added.
- `python3 tests/spec/check_transport_presets_source.py`: **131,593 assertions
  passed**. The harness extracts native structures and actual command/refresh/
  label code, with an entry-count wrapper around the refresh function. It checks
  all 65,536 int16 patterns under both limits, all valid-slot and active-cargo
  masks, unchanged ORDER header bytes, callback write snapshots and cancellation.
  Doubles supply GUI/localization/write-back outcomes; actual window lifetime,
  arbitrary callbacks and native rendering remain separate qualification.
- `python3 tests/spec/check_production_presets_source.py`: **13,140 assertions
  passed** after correcting its wrapper's modal result to native int16 storage
  and adding raw 65536 rollback. Both harnesses compile with C11, warnings as
  errors and fatal undefined-behavior instrumentation. Early harness failures
  exposed a const-qualification mismatch in a double and an incorrect expected
  seven-byte parse remainder; they were corrected without changing game code.
- SDL Debug configure/build passed without build warnings; selected SDL unit,
  compatibility and tutorial-smoke checks passed **31/31**. Logs:
  `dist/spec-transport-configure.log`, `dist/spec-transport-build.log`,
  `dist/spec-transport-runtime.log`.
- Full native regression: **519 matches, 21 unused-storage warnings, 0
  differences/errors, 0 skipped reference inputs**. Report:
  `dist/scaffold/regression/spec-transport-comparison.json`. Baselines unchanged.
- `git diff --check` and changed/new-file whitespace checks pass. No game/runtime
  source changed; host-specific game-code checks were not required.

Remaining work includes full waypoint/task-default and target-selection control
state, arbitrary UI reentrancy/lifetime, every host transport mode and the broader
movement/damage/economy/AI/tutorial backlog. The all-module goal remains active.

### Production editor initialization and command continuation

Recorded 2026-10-24 against `e2eae18cff0f990db20415c1c2262aed2022f95b`.
[M09 editor initialization and commands](M09-production-management.md#311-initial-queue-and-source-catalogue)
now have checked catalogue assembly/width rules, copied-queue sanitation,
Add/Remove selection and availability effects, realloc escape and complete-row
reorder. Eight criteria retain player-story ownership and identify native helper,
allocation and UI boundaries. Existing shared row, identity, modifier and preset
contracts remain canonical.

The review corrected an incorrect insertion description: Add checks the selected
row and its successor, then inserts after the selection, with separate header
and absent-selection behavior. Sanitization is not a safe generic filter: the
source has an overlapping memcpy boundary and a defined one-following-row case
where the moved row escapes its intended clamp. The known-bug/roadmap records
now distinguish this source evidence from a real UI trigger, original-binary
comparison or an approved runtime repair. Model fault markers do not invent
native error handling. No game/runtime source changed.

Verification:

- Focused editor model: **276 verified, 0 errors** in
  `dist/spec-editor-focused2.log`; the initial draft also verified before the
  additional boundary witnesses were added. No assumptions, axioms or disabled
  verification were introduced.
- `python3 tests/spec/verify.py --time-limit 120`, Dafny 4.11.0: all **78 formal
  files** and include closure, **3931 verified, 0 errors; audit 0 findings**.
  Log: `dist/spec-editor-full.log`.
- Final links/trace pass: **1749 local links/includes**, **552 requirement IDs**
  (116 stories, 271 acceptance, 66 interaction and 99 quality), zero errors.
  All **9** checker unit tests pass. Prior explicit anchors are retained;
  the new known-bug/roadmap cross-links and changed/new-file whitespace were
  checked. Log: `dist/spec-editor-links.log`. The full run's earlier Markdown
  pass counted 1747 links before two canonical-model navigation links were added.
- `python3 tests/spec/check_production_editor_source.py`: **19,415 assertions
  passed**. It extracts current initializer/command/allocation bodies and native
  queue/catalog record layouts, with explicit planet/design/helper and GUI
  projections. Cases cover every source count 1–1023 and modifier combination
  for Add, every row count 0–1023 and modifier combination for Remove, catalogue
  order/traits/widths, insertion/merge/refusal, allocation escape and preservation
  of progress/unused bits. Address/undefined-behavior instrumentation and compiler
  warnings are fatal. A guarded memcpy detects overlapping ranges and escapes
  before libc; it verifies the reached boundary, not undefined post-copy results.
  The defined cursor example uses an unavailable ship-design row followed by a
  factory row, without requiring an invalid object-kind encoding.
- SDL Debug configure/build passed without build warnings. Selected SDL unit,
  compatibility and tutorial-smoke checks passed **31/31**. Logs:
  `dist/spec-editor-configure.log`, `dist/spec-editor-build.log`,
  `dist/spec-editor-runtime.log`.
- Full native regression: **519 matches, 21 unused-storage warnings, 0
  differences/errors, 0 skipped reference inputs**. Report:
  `dist/scaffold/regression/spec-editor-comparison.json`. Baselines unchanged.
- `git diff --check` passes. No game/runtime sources or baseline files changed;
  host-specific game-code checks were not required.

Remaining M09 work includes native getter/catalog refinement, Next/Back and
global/modal lifecycle, full costs/forecast/production execution and their wider
module dependencies. Native sanitization repair remains a separate open task;
no safe postcondition is asserted past undefined overlap. The all-module goal
remains active.


### Production lifecycle and starbase navigation continuation

Continued on the existing reviewed revision and working collection. The
[M09 lifecycle controller](dafny/M09-lifecycle.dfy) now specifies native
completion, modal return narrowing, direct Clear, close commands, Next/Back,
allocation recovery and initial queue selection. [M07 adjacent selection](dafny/M07-adjacent.dfy)
owns construction-capable starbase target choice in actual planet-array order.
The module entries preserve IDs and add M09-AC25–30 and M07-AC12.

Callbacks are explicit boundaries: ignored lookup returns do not establish a
successful write; reload uses post-write identity, AI refresh uses post-reload
state, and tutorial can mutate state after final cleanup. Direct Clear consumes
the existing working global and depends on normal entry ownership. Recovery
after transfer or navigation does not undo earlier effects. No runtime behavior
or baseline was changed.

Verification and limits for this continuation:

- Focused verification: **88 verified, 0 errors** in
  `dist/spec-lifecycle-focused3.log`. The draft had one type-resolution error,
  then three unproved witness assertions; correcting the row argument and
  supplying explicit recursive/width unfolding resolved them. The final run
  also verifies the stronger last-nonautomatic-row postconditions. No assumptions,
  axioms or disabled verification were introduced.
- `python3 tests/spec/verify.py --time-limit 120`, Dafny 4.11.0: all **80 formal
  files** and include closure, **4019 verified, 0 errors; audit 0 findings**.
  Log: `dist/spec-lifecycle-full.log`.
- Final links/trace: **1765 local links/includes**, **559 requirement IDs**
  (116 stories, 278 acceptance, 66 interaction and 99 quality), zero errors.
  All **9** checker unit tests pass. Prior explicit anchors are retained.
  Log: `dist/spec-lifecycle-links.log`. The full run counted 1762 links before
  the final source/test/verification navigation links were added.
- [Native lifecycle fixtures](../tests/spec/check_production_lifecycle_source.py):
  **16,243 assertions passed**, including 15,360 starbase selector cases,
  all 511 automatic/manual queue patterns through eight rows, a fleet-row case,
  accept/cancel/null/empty/nonempty/AI/tutorial/player combinations, separate
  write/read result combinations, modal narrowing, direct Clear entry states,
  multi-colony cancellation, five injected escape stages and callback mutation.
  Actual C function/command bodies and native queue headers are extracted;
  initialization, allocator, lookup, selection, modal and GUI effects are bounded
  doubles. Freed-allocation identity is checked by the allocator double. The
  title formatter is a bounded double; the first harness compile rejected its
  deprecated `sprintf` mapping, which was replaced without changing runtime code.
  C11 warnings and address/undefined-behavior instrumentation are fatal.
  Log: `dist/spec-lifecycle-native.log`.
- SDL Debug configure/build passed without warnings; selected SDL unit,
  compatibility and tutorial-smoke checks passed **31/31**. Logs:
  `dist/spec-lifecycle-configure.log`, `dist/spec-lifecycle-build.log`,
  `dist/spec-lifecycle-runtime.log`.
- Full native regression: **519 matches, 21 unused-storage warnings, 0
  differences/errors, 0 skipped reference inputs**. Report:
  `dist/scaffold/regression/spec-lifecycle-comparison.json`; baseline unchanged.
- Changed-file whitespace, preserved explicit anchors and `git diff --check`
  passed. No game/runtime sources or baseline files changed; host-specific
  game-code checks were not required.

Remaining work includes real `FLookupPlanet`/`FDupPlanet` ownership and failure
refinement, full selection/tutorial/modal reentrancy, getter/catalog adapters,
localized title-buffer behavior and full cost/forecast/host production rules.
The source-extracted fixtures use explicit callback doubles and do not establish
whole-application persistence or platform qualification. The all-module goal
remains active.


### Planet editor copy and write-back continuation

Recorded 2026-10-25 at `e3222eb1fbdb01d68067231b5d5ef14a71759b30`, preserving the
committed handbook and traceability work. The [S06 planet editor companion](S06-planet-editor-contracts.md)
and [canonical copy model](dafny/S06-planet-editor.dfy) now distinguish selected
queue copies, borrowed reads, 52-byte prefix writes, different read/write
allocation sizes, eight-bit capacity wrap, partial allocation escapes and native
entry dispatch. S06-AC09–14 trace these behaviors to existing support stories;
M09 links the shared owner instead of duplicating its queue-copy contract.

The [native source fixtures](../tests/spec/check_planet_editor_source.py) execute
actual lookup, duplication, list-allocation and M09 finish bodies with actual
packed structs. Report/log/heap/tutorial effects remain controlled doubles.
Actual write-back followed by a fresh selected read confirms the capacity-wrap
mechanism (GAP-15), now tracked in the known-bug list and roadmap; UI
reachability, original comparison and a separate repair remain open.
Overlap and out-of-allocation guards stop before undefined native operations;
those markers are specification boundaries, not runtime error returns.

Verification for this continuation:

- Focused final model: **197 verified, 0 errors** in
  `dist/spec-planet-copy-focused6.log`. The first draft used a reserved parameter
  name; after that fix, one byte-subset arithmetic obligation required explicit
  integer operands. The initial 152-obligation model passed, then native heap
  review strengthened it to retain spare/unshrunk usable storage; a checked
  abort transition prevents continuation into tutorial after queue failure. No assumptions,
  axioms or disabled verification were introduced.
- `python3 tests/spec/verify.py --time-limit 120`, Dafny 4.11.0: all **81 formal
  files** and include closure, **4216 verified, 0 errors; audit 0 findings**.
  Log: `dist/spec-planet-copy-full-final2.log`. Earlier complete passes of 4171
  and 4215 obligations are retained in `dist/spec-planet-copy-full.log` and
  `dist/spec-planet-copy-full-final.log` as superseded evidence before the spare
  extent and checked abort refinements respectively.
- [Source fixture](../tests/spec/check_planet_editor_source.py): **17,432,153
  assertions passed**. The principal matrix has **131,072** source-count /
  destination-capacity / read-write combinations and checks every active word,
  source preservation, inactive tails, exact prefixes and request/header results.
  Additional cases cover borrowed and null/empty queues, entry dispatch,
  allocation/report/log/tutorial escapes, self/alias guards, capacity wrap and
  the actual M09 finish → lookup → duplication chain. Packed structs, native enum
  constants, list wrappers and heap-alignment helpers are extracted from source.
  On this platform `max_align_t` alignment is 8; fresh 0/1-row requests yield
  payloads of 6/14 bytes. Free-block selection and actual heap growth remain
  explicit S04 adapters. C11 warnings, ASan and UBSan are fatal. Log:
  `dist/spec-planet-copy-native.log`.
- Final link/trace check: **2084 local links/includes**, **565 requirement IDs**
  (116 stories, 284 acceptance, 66 interaction, 99 quality), zero errors. All
  **9** specification-checker unit tests pass. Log: `dist/spec-planet-copy-links.log`.
  The full formal run counted 2081 links before the final loader/forecast
  cross-references were added.
- **25 handbook tests passed**. The final handbook build contains **161
  rendered pages**, 26 modules, 565 requirement IDs and 81 formal files, with
  zero rendered-link/asset errors. Logs: `dist/spec-planet-copy-handbook-tests.log`
  and `dist/spec-planet-copy-handbook-build.log`. No diagram bodies changed.
- SDL Debug configure/build passed without warnings; selected SDL unit,
  compatibility and tutorial-smoke checks passed **31/31**. Logs:
  `dist/spec-planet-copy-configure.log`, `dist/spec-planet-copy-build.log`,
  `dist/spec-planet-copy-runtime.log`.
- Full native regression: **519 matches, 21 unused-storage warnings, 0
  differences/errors, 0 reference inputs skipped**. Report:
  `dist/scaffold/regression/spec-planet-copy-comparison.json`. Baselines unchanged.
- Changed-file whitespace, prior explicit anchors and `git diff --check` pass.
  No game/runtime source or baseline changes; host-specific game-code checks
  were not required.

Remaining work includes full log generation/publication and allocator internals,
full native heap extent/free-block selection, caller identity/array/lifetime invariants, arbitrary callback reentrancy and the
wider gameplay, AI, tutorial, platform and policy backlog. Passing these copy
contracts does not close the full S06 or M09 module. The goal remains active.


### Production cost vector continuation

Recorded 2026-10-25 at `e3222eb1fbdb01d68067231b5d5ef14a71759b30`, retaining
all earlier work. The [M09 cost companion](M09-production-costs.md) and
[canonical native vector model](dafny/M09-costs.dfy) specify the full numeric
`GetProductionCosts` caller: stored ship/base totals, different/same-hull upgrade,
ordered occupied-slot credits, signed and unsigned narrowing, trait/tutorial
branches, explicit colony/pricing contexts, scanner mapping, quantity and the
unhandled-item incoming-buffer path. M09-AC31–38 retain story ownership and link
source/model evidence; M11 keeps quote derivation and its undefined pricing path.

Verification and limits for this continuation:

- Focused final model: **277 verified, 0 errors** in
  `dist/spec-cost-focused7.log`. Draft fixes resolved a reserved identifier,
  nested sequence/slot membership facts, one unsigned-subset multiplication,
  vector extensionality and a draft extra parenthesis. General checked credit
  equivalence, nonincreasing ordered-slot credits, quantity identity/zero and
  native-width witnesses remain intact. No assumptions, axioms or disabled
  verification were introduced.
- `python3 tests/spec/verify.py --time-limit 120`, Dafny 4.11.0: all **82 formal
  files** and include closure, **4493 verified, 0 errors; audit 0 findings**.
  Log: `dist/spec-cost-full.log`.
- [Native source fixtures](../tests/spec/check_production_cost_source.py):
  **1,495,512 fixture assertions passed**. They compare four output words against an explicit
  arithmetic oracle, check lookup item/category, quote order/pricing player and
  distinct player rosters, and use actual cost/race/stored-hull bodies plus packed
  cost-bearing structs. Part lookup/pricing and hull-catalog access are bounded
  quote providers. The bare-hull request's unused item field is not read by the
  fixture. Planetary indexes derive from source YAML order; the fixture does
  not require an existing generated build header. The sole body annotation is
  a void-use of the stored-hull getter's unused player argument. C11 warnings,
  ASan/UBSan findings and mismatches are fatal. Log: `dist/spec-cost-native.log`.
- Final link/trace check: **2117 local links/includes**, **573 requirement IDs**
  (116 stories, 292 acceptance, 66 interaction and 99 quality), zero errors.
  All **9** specification-checker unit tests pass; prior explicit anchors are
  preserved. Log: `dist/spec-cost-links.log`. The full run counted 2115 links
  before the final source/evidence links were added.
- **25 handbook tests passed**; build has **163 rendered pages**, 26 modules,
  573 requirement IDs and 82 formal files, with zero rendered-link/asset errors.
  Logs: `dist/spec-cost-handbook-tests.log`, `dist/spec-cost-handbook-build.log`.
  Diagram bodies were unchanged.
- SDL Debug configure/build passed without warnings; selected SDL unit,
  compatibility and tutorial-smoke checks passed **31/31**. Logs:
  `dist/spec-cost-configure.log`, `dist/spec-cost-build.log`,
  `dist/spec-cost-runtime.log`.
- Full native regression: **519 matches, 21 unused-storage warnings, 0
  differences/errors, 0 skipped reference inputs**. Report:
  `dist/scaffold/regression/spec-cost-comparison.json`; baselines unchanged.
- `git diff --check` passed. No game/runtime code or baseline changed;
  host-specific game-code checks were not required.

Remaining work includes actual M11/S03 quote/pointer/helper extraction, the
99-year forecast and complete host production/alchemy/construction transitions,
alongside the wider all-module backlog. Passing the vector caller contracts does
not establish full pricing-helper refinement or forecast/execution correctness.
The completion goal remains active.


### Production forecast controller continuation

Recorded 2026-10-25 at `e3222eb1fbdb01d68067231b5d5ef14a71759b30`, preserving
the existing work. The [forecast companion](M09-production-forecast.md) and
[canonical controller](dafny/M09-forecast.dfy) specify copy selection/preparation,
native signed budget/query arithmetic, partial/row order and alchemy, selected
first/last outcomes, post-builder installation changes, signed status stopping,
99-pass horizon, aliased outputs and ordinary cleanup. Shared M09 status/message
adapters and M19 mining draw/remote-mark predicates retain canonical ownership.
M09-AC39–47 and M19-AC08 trace these rules to existing stories and native evidence.

Source review corrects the blanket no-random-forecast claim and item-3 wording:
automatic alchemy is not manual item 11. GAP-16 records generation-active native
RNG/message effects and AR live-fleet marking. The estimator's copied input can
remain unchanged while global state changes. M08 resource **return width** and
M12 temporary-percentage restoration now link the exact caller/effect boundary.
No runtime behavior or regression baseline changed.

Verification and limits for this continuation:

- Focused forecast/shared-production/mining models: **614 verified, 0 errors**
  in `dist/spec-forecast-focused10.log`. Draft corrections added explicit row-fit
  facts, native active-year preconditions and output-alias updates. A timeout in
  the old mining witness was resolved by keeping new mining vectors separate;
  an indentation warning was fixed with explicit expression grouping. No
  assumptions, axioms or disabled verification were introduced.
- `python3 tests/spec/verify.py --time-limit 120`, Dafny 4.11.0: all **83 formal
  files** and include closure, **4973 verified, 0 errors; audit 0 findings**.
  Log: `dist/spec-forecast-full.log`.
- [Native source fixtures](../tests/spec/check_production_forecast_source.py):
  **272,841 fixture assertions passed**. They execute actual estimator/builder/
  mining/RNG/race/list bodies with bounded pricing, capacity, resource, growth
  and heap services, native packed planet/queue structs and projected fleet/order
  fields. Scripted responses isolate all 65,536 status words and are not asserted
  to be real builder outputs. Separate actual-body cases verify partial ordering,
  progress 59/unused-bit preservation, RNG/fleet/message effects, ordinary cleanup
  and escaping initialization. All negative request words, signed percentage
  values, year/terminal combinations, 255 rows and output aliasing are covered.
  The actual-mining eligibility matrix has 9,216 combinations. C11 warnings,
  ASan/UBSan findings and invalid copied extents are fatal/guarded respectively.
  Draft fixes supplied a missing sentinel/type, reset the per-call test clock and
  corrected the expected partial percentage from 50 to 59. Log:
  `dist/spec-forecast-native.log`. No native repair was made.
- Final link/trace checks: **2157 local links/includes**, **583 requirement IDs**
  (116 stories, 302 acceptance criteria, 66 interaction and 99 quality
  requirements), zero errors. All **9** specification-checker unit tests pass;
  prior explicit anchors are retained.
- **25 handbook tests passed**; build has **165 rendered pages**, 26 modules,
  583 requirement IDs and 83 formal files, with zero rendered-link/asset errors.
  Logs: `dist/spec-forecast-handbook-tests.log`, `dist/spec-forecast-handbook-build.log`.
  No diagram bodies changed.
- SDL Debug configure/build passed without warnings; selected SDL unit,
  compatibility and tutorial-smoke checks passed **31/31**. Logs:
  `dist/spec-forecast-configure.log`, `dist/spec-forecast-build.log`,
  `dist/spec-forecast-runtime.log`.
- Full native regression: **519 matches, 21 unused-storage warnings, 0
  differences/errors, 0 skipped reference inputs**. Report:
  `dist/scaffold/regression/spec-forecast-comparison.json`; baselines unchanged.
- `git diff --check` passed. No game/runtime sources or baseline changed;
  host-specific game-code checks were not required.

Remaining work includes the full CBuild numeric/alchemy/cap/partial algorithm,
actual mining/depletion/resources/growth and remote-fleet/heap/pointer adapters,
full UI/AI generation-context qualification and display behavior, plus the
wider all-module backlog. Actual helper source fixtures do not replace missing
formal helper definitions. The completion goal remains active.

### Production builder continuation

Recorded 2026-10-25 at `e3222eb1fbdb01d68067231b5d5ef14a71759b30`, preserving
existing work. The [builder companion](M09-production-builder.md) and
[canonical step relation](dafny/M09-builder.dfy) close the formerly unspecified
numeric/control phases of `CBuildProdItem`: automatic caps, initial paid amounts,
affordability, ordered limiting progress and cumulative flags, payment, alchemy
retries/partial creation, completion and caller/partial/status/message publication.
M09-AC48–54 trace them to existing stories. Shared row, cost, rounding, status and
message ownership is retained. Non-alchemy execution has a decreasing-rank
termination proof. Retrying-alchemy termination, native helper/pointer/frame
refinement and outer queue/object application remain named E09-4 work.

Verification and limits for this continuation:

- Focused builder verification: **1296 verified, 0 errors** in
  `dist/spec-builder-focused4.log`. The draft first exposed missing phase-specific
  index/deficit facts and constant-unfolding needs; a large verification condition
  timed out. Explicit invariants, shared signed-conversion bounds and isolated
  assertions resolved these issues. `NoAlchemyStepProgress` and
  `FinishNoAlchemy` prove termination to ordinary or undefined-boundary outcomes
  with alchemy disabled. Retrying alchemy has no new total-termination claim.
  No assumptions, axioms or disabled verification were introduced.
- [Native builder comparison](../tests/spec/check_production_builder_source.py):
  **68,307 defined cases, 751,377 assertions passed** against the actual C body.
  All 128 packed progress values, eight statuses, automatic cap/helper order,
  distinct installation fields, short-circuit terraform queries, raw row/partial
  bits, optional outputs, preview, large-deficit signed-16 conversion counter
  wrapping and conversion messages are covered with
  deterministic providers. Before C execution, **232** uninitialized-deficit
  and **45** signed-overflow vectors are excluded as undefined. Those exclusions
  are not successful native outcomes. There were no trace-budget exclusions.
  C11 warnings, ASan and UBSan findings are fatal. Log:
  `dist/spec-builder-native3.log`; providers and the finite interpreter are not
  universal helper/source/model refinement proofs.
- `python3 tests/spec/verify.py --time-limit 120`, Dafny 4.11.0: all **84
  formal files** and include closure, **6269 verified, 0 errors; audit 0
  findings**. Log: `dist/spec-builder-full.log`. Its initial link pass preceded
  the completed evidence text; the final link totals are recorded below.
- All **9** specification-checker unit tests pass. Link/trace checks find
  **2183 local links/includes**, **590 requirement IDs** (116 stories, 309
  acceptance criteria, 66 interaction and 99 quality requirements), zero errors.
  Prior explicit anchors are preserved.
- **25 handbook tests passed**; build has **167 rendered pages**, 26 modules,
  590 requirement IDs and 84 formal files, zero rendered-link/asset errors. Logs:
  `dist/spec-builder-handbook-tests.log`, `dist/spec-builder-handbook-build.log`.
- SDL Debug configure/build passed without new warnings; selected SDL unit,
  compatibility and tutorial-smoke checks passed **31/31**. Logs:
  `dist/spec-builder-configure.log`, `dist/spec-builder-build.log`,
  `dist/spec-builder-runtime.log`.
- Full native regression: **519 matches, 21 unused-storage warnings, 0
  differences/errors, 0 skipped reference inputs**. Report:
  `dist/scaffold/regression/spec-builder-comparison.json`; baselines unchanged.
- `git diff --check` passed. No runtime/game sources changed; host-specific
  game-code checks were not required.

Next work includes retrying-alchemy termination, binding builder outputs to the
forecast controller, native helper/pointer/frame and quote refinement, and outer
`Produce`/`FBuildObject` queue/application/failure effects. The full module
completion backlog and goal remain active. No runtime/game source or baseline
changes are part of this batch.

### Host production handoff continuation

Recorded 2026-10-25 at `e3222eb1fbdb01d68067231b5d5ef14a71759b30`, preserving
existing work. The [execution companion](M09-production-execution.md) links
canonical builder traces to forecast and host observations, and specifies native
host entry/extra-resource/reservation, manual validation, carry/removal/prepend,
post-application routing and completion-notice rules. M09-AC55–61 trace them to
existing stories. S06 remains physical queue/list owner; M18/M19 retain year,
economy and object-effect ownership. GAP-15 now includes host prepend wrapping.
No runtime behavior or baseline changes are part of this batch.

Verification and limits for this continuation:

- Focused handoff/host verification: **187 verified, 0 errors** in
  `dist/spec-host-production-focused5.log`. Draft repairs resolved a datatype
  field-type collision, made signed-32 resource projection facts explicit,
  expanded packed-word quotient/remainder proofs after a timeout, and grouped a
  case expression to remove an indentation warning. All definitions have checked
  bodies; no assumptions, axioms or disabled verification were introduced.
- [Actual host source fixtures](../tests/spec/check_host_production_source.py):
  **1,362,708 assertions passed**, C11 warnings and ASan/UBSan findings fatal.
  Actual Produce/CBuildProdItem/list helpers execute with native packed
  planet/row/list layouts and bounded economy/object/heap/phase providers.
  Scripted status/partial/application combinations cover all 65,536 status words;
  these do not certify builder reachability. Actual-builder cases verify retained
  charges after controlled object rejection, manual count clearing versus
  automatic retention, partial progress and resize escape after charging.
  The corpus includes manual item/category/guard and extreme signed-cap matrices,
  notices before reduction, every signed-16 gross with seven extras (four
  zero-denominator inputs excluded before C), captured query mutations,
  single/consecutive alchemy, all prepend sizes 1..255 and guarded missing-spare
  copies. Final population/research/random/scoring spies check caller order,
  not full helper implementations. Log: `dist/spec-host-production-native4.log`.
- `python3 tests/spec/verify.py --time-limit 120`, Dafny 4.11.0: all **86
  formal files** and include closure, **6456 verified, 0 errors; audit 0
  findings**. Log: `dist/spec-host-production-full.log`. Its initial link pass
  preceded the completed evidence record; final link totals follow below.
- All **9** specification-checker tests pass. Final link/trace checks find
  **2219 local links/includes**, **597 requirement IDs** (116 stories, 316
  acceptance criteria, 66 interaction and 99 quality requirements), zero errors.
  Prior explicit anchors and stable requirement IDs are retained.
- **25 handbook tests passed**; build has **170 rendered pages**, 26 modules,
  597 requirement IDs and 86 formal files, zero rendered-link/asset errors. Logs:
  `dist/spec-host-production-handbook-tests.log`,
  `dist/spec-host-production-handbook-build.log`.
- SDL Debug configure/build passed without new warnings; selected SDL unit,
  compatibility and tutorial-smoke checks passed **31/31**. Logs:
  `dist/spec-host-production-configure.log`, `dist/spec-host-production-build.log`,
  `dist/spec-host-production-runtime.log`.
- Full native regression: **519 matches, 21 unused-storage warnings, 0
  differences/errors, 0 skipped reference inputs**. Report:
  `dist/scaffold/regression/spec-host-production-comparison.json`; baselines
  unchanged. No runtime/game source changed; host-specific game-code checks
  were not required.
- `git diff --check` and final explicit-anchor/whitespace checks passed.
  Source/runtime and baseline paths have no changes.

Remaining callback/row-pointer/helper/object/annual frames and retrying-alchemy
termination remain explicit. Actual FBuildObject family/fleet-capacity and
whole-host creation/failure fixtures remain open; the object-rejection provider
is not proof of those outcomes. Passing a supplied result does not establish
its real source production or universal host refinement. The wider module
completion backlog and goal remain active.

### Construction application continuation

Recorded 2026-10-25 at `e3222eb1fbdb01d68067231b5d5ef14a71759b30`, preserving
existing work. The [construction-effects companion](M19-construction-effects.md)
and canonical M19/M10/M09 models specify FBuildObject family rules, construction
stack/counter/damage behavior, physical queue invalidation and final working-stock
publication. New M19-AC09–16, M10-AC18–20 and M09-AC62–64 trace these rules to
existing stories. Actual application helpers replace supplied outcomes in focused
source fixtures. GAP-17 records Genesis direct clearing versus host write-back.
No runtime behavior or baseline change is made.

Verification and limits for this continuation:

- Focused construction, invalidation, damage and host-stock models: **422
  verified, 0 errors**, `dist/spec-object-effects-focused7.log`. Draft corrections
  renamed reserved identifiers, supplied explicit signed-word/sequence bounds,
  added weight bounds, exposed updated-environment membership facts and split
  a stock-publication witness into its component equalities. No assumptions,
  axioms or disabled verification were introduced.
- [Actual application source fixtures](../tests/spec/check_construction_effects_source.py):
  **1,897,437 assertions passed**. They compile actual FBuildObject, CreateShip,
  queue invalidators, LpthNew, CBuildProdItem, Produce, race accessors and RNG
  against common.h and freshly generated data/text IDs. Economy, availability,
  catalog, routing, messages, low-level heap and selection services remain
  providers. The corpus covers all signed-16 installation maxima, all packed
  damage words at a declared context, all signed-byte environments/valid axes,
  negative terraform iteration, native RNG Genesis, all generic scanner bytes,
  packet trait/warp/quantity/target and native ID exhaustion, base replacement,
  actual host clipping, and the exact 512-fleet failure with a full owned roster.
  Whole saved-game/helper/native-state refinement remains open. Defined numeric
  traces only are executed; an axis guard stops before invalid indexing. GNU C11
  uses the existing project unused-parameter/pointer-sign warning exclusions;
  all other warnings and ASan/UBSan findings are fatal. A draft scanner fixture
  checked the wrong message-parameter position; correcting that expectation
  resolved the failure without a runtime change. Log:
  `dist/spec-object-effects-native4.log`.
- `python3 tests/spec/verify.py --time-limit 120`, Dafny 4.11.0: all **89
  formal files** and include closure, **6700 verified, 0 errors; audit 0
  findings**. Log: `dist/spec-object-effects-full.log`. Its initial link pass
  preceded the final evidence text; current link totals are below.
- All **9** specification-checker tests pass. Final link/trace checks find
  **2277 local links/includes**, **611 requirement IDs** (116 stories, 330
  acceptance criteria, 66 interaction and 99 quality requirements), zero errors.
  Existing explicit anchors and stable requirement IDs are preserved.
- **26 handbook tests passed**. Cross-module story/criterion edges exposed six
  broken local trace anchors in the first rendered build. The trace renderer
  now links to each requirement's owning module and a new bidirectional
  cross-module regression test preserves local links as well. The resulting
  handbook has **174 rendered pages**, 26 modules, 611 requirement IDs and 89
  formal files, zero rendered-link/asset errors. Logs:
  `dist/spec-object-effects-handbook-tests.log`,
  `dist/spec-object-effects-handbook-build.log`. No diagram bodies changed.
- SDL Debug configure/build passed without new warnings; selected SDL unit,
  compatibility and tutorial-smoke checks passed **31/31**. Logs:
  `dist/spec-object-effects-configure.log`, `dist/spec-object-effects-build.log`,
  `dist/spec-object-effects-runtime.log`.
- Full native regression: **519 matches, 21 unused-storage warnings, 0
  differences/errors, 0 skipped reference inputs**. Report:
  `dist/scaffold/regression/spec-object-effects-comparison.json`; baselines
  unchanged. No runtime/game source changed; host-specific game-code checks
  were not required.
- `git diff --check` and explicit-anchor/whitespace checks passed. Source/runtime
  and baseline paths have no changes; the changed tooling is the trace renderer
  with its focused documentation regression test.

Complete native-state/source refinement, helper/callback/allocator/selection
frames, saved-game qualification, original comparison and intended behavior
decisions remain explicit. Movement, combat, economic producers, retrying-alchemy
termination and the wider module backlog remain open; the completion goal is active.


### Native colony economy continuation

Reviewed 2026-10-25 at `e3222eb1fbdb01d68067231b5d5ef14a71759b30`.
The [M08 economy companion](M08-colony-economy.md) now specifies actual native
capacity/installation/resource/display arithmetic, stored versus passed planet
snapshots and owner versus query-player contexts. M19 owns native growth/carry
and optional numeric writeback; M08 handoffs connect it to next-year installation
queries and the existing M09 forecast/host budgets. Existing M03 expressions,
M08 scalar installation rules, M19 normalized carry and S04 finite binary64
evaluation remain canonical and are reused rather than copied.

- Focused Dafny verification: **282 verified, 0 errors**, after adding explicit
  carry/word proof steps to resolve the draft hostile-growth vector timeout.
  No assumptions, axioms or disabled verification were introduced. Log:
  `dist/spec-native-colony-focused7.log`.
- Actual source differential: **239344 defined cases, 1331470 assertions**,
  native records/fresh generated IDs and actual population table, ASan/UBSan,
  source race/habitat/capacity/installation/resource/display/growth bodies. Declared snapshot and raw-helper
  providers are distinguished from actual calculations. Growth-call counters
  check conditional next-year evaluation; extreme signed habitat cases cover
  negation/narrowing. Final AR floating bits are compared at declared contexts;
  this is not universal binary64/platform
  refinement. **3609** undefined-domain candidates were excluded before C
  execution (arithmetic, division, root/conversion and table boundaries). Log:
  `dist/spec-native-colony-source4.log`.
- Added M08-AC09–16, M19-AC17–20 and M09-AC65; corrected research preview wording
  to preserve local restoration without promising global helper purity.
- Full `python3 tests/spec/verify.py --time-limit 120`, Dafny 4.11.0:
  **92 formal files**, complete include closure, **6982 verified, 0 errors;
  audit 0 findings**. Log: `dist/spec-native-colony-full.log`. Final link/trace
  checks after the evidence edits pass **2337 local links/includes**,
  **624 requirement IDs** (116 stories, 343 acceptance criteria, 66 interaction
  and 99 quality requirements), zero errors. All **9** checker tests pass.
- **26 handbook tests pass**. The handbook builds **178 rendered pages**,
  26 modules, 624 requirements and 92 formal files with zero rendered-link/asset
  errors, including the final evidence text (**30962 local links/assets**).
  Logs: `dist/spec-native-colony-handbook-tests.log`,
  `dist/spec-native-colony-handbook-final.log`. No diagram bodies changed.
- SDL Debug configure/build passed without new warnings; SDL unit, compatibility
  and tutorial-smoke checks passed **31/31**. Logs:
  `dist/spec-native-colony-configure.log`, `dist/spec-native-colony-build.log`,
  `dist/spec-native-colony-runtime.log`. These are the configured unit/tutorial
  test families; full 80-page/native UI qualification is still open.
- Full native regression: **519 matches, 21 unused-storage warnings, 0
  differences/errors, 0 reference inputs skipped**. Report:
  `dist/scaffold/regression/spec-native-colony-comparison.json`. No game source
  changed, so host-specific game-code checks were not required. Baselines
  remain unchanged.
- `git diff --check`, new-file whitespace and old explicit-anchor checks pass;
  all 153 pre-batch tracked/snapshotted files remain present. Runtime/source
  and baseline paths have no changes.

Remaining work includes full lookup/copy/lifetime/habitat/catalog/helper frames,
floating trace production, saved-game/UI qualification, mining/depletion and
research/turn algorithms. Typed malformed inputs are evidence of native widths,
not newly supported game settings or implemented error returns. No game source,
on-disk layout or regression baseline changed. The completion goal stays active.


### Mining and depletion continuation

Reviewed 2026-10-25 at `e3222eb1fbdb01d68067231b5d5ef14a71759b30`.
The [mining/depletion companion](M19-mining-and-depletion.md) adds the complete
three-mineral numeric loop, byte concentration/level updates, source rounding
and RNG order, stock publication, ordered AR remote quantities/marking and
first-order/first-null short circuits. M10 now specifies all sixteen design
counts and mining slot/part arithmetic through the native 4000 upper cap.
M19 retains the existing concentration/yield/depletion-band definitions and
valid-domain eligibility rules; checked refinements connect the native models.

- Focused Dafny verification: **146 verified, 0 errors**. Proofs include byte
  depletion monotonicity and a max(0,initial concentration-1) iteration bound,
  non-applying ore/mark frames and non-generating draw-cursor preservation,
  normal-domain yield/eligibility agreement and operating-mine composition.
  Explicit power-of-two facts resolved a draft capacity-vector timeout.
  Correcting a zero-left depletion expectation and adding an explicit signed
  widening/indexed draw invariant resolved draft failures. No assumptions,
  axioms or disabled verification were introduced. Log: `dist/spec-mining-focused5.log`.
- Actual native differential: **221240 defined cases, 4686320 assertions**,
  actual common.h records, fresh generated IDs and source mining, fleet capacity,
  operating-mines, race and RNG bodies. Operating-limit and part-pointer/ability
  providers remain explicit. All 65536 concentration/level byte pairs run at
  three mine counts; signed productivity/counts, generation/application, all scripted
  draw values and seeded native RNG, waypoint guards/holes, slot/capacity arithmetic
  and reversed remote order are covered. **6574** invalid-domain candidates were
  excluded before C execution; no runtime error return is inferred from that
  filtering. ASan/UBSan findings and all compiler warnings are fatal. Draft harness
  declarations and flexible-array order storage were corrected after compile/ASan
  failures; no game source changed. Log: `dist/spec-mining-source5.log`.
- Added M19-AC21–28, M10-AC21–22, M08-AC17 and M09-AC66 with owning stories,
  source/model/evidence scope and remaining obligations.
- Full `python3 tests/spec/verify.py --time-limit 120`, Dafny 4.11.0:
  **94 formal files**, complete include closure, **7128 verified, 0 errors;
  audit 0 findings**. Log: `dist/spec-mining-full.log`.
- Final link/trace checks: **2393 local links/includes**, **636 requirement IDs**
  (116 stories, 355 acceptance criteria, 66 interaction and 99 quality
  requirements), zero errors. All **9** specification-checker tests pass.
- **26 handbook tests pass**; the handbook builds **181 rendered pages**,
  26 modules, 636 requirements and 94 formal files with zero errors across
  **31764 rendered local links/assets**, including final evidence text.
  Logs: `dist/spec-mining-handbook-tests.log`,
  `dist/spec-mining-handbook-final.log`.
  No diagram bodies changed.
- SDL Debug configure/build passed without new warnings; SDL unit, compatibility
  and tutorial-smoke checks passed **31/31**. Logs:
  `dist/spec-mining-configure.log`, `dist/spec-mining-build.log`,
  `dist/spec-mining-runtime.log`. Full 80-page/native UI qualification stays open.
- Full native regression: **519 matches, 21 unused-storage warnings, 0
  differences/errors, 0 reference inputs skipped**. Report:
  `dist/scaffold/regression/spec-mining-comparison.json`. No runtime/game source
  changed; host-specific game-code checks were not required. Baselines unchanged.
- Existing anchors/IDs and all **154** pre-batch snapshotted files are preserved;
  new-file whitespace and `git diff --check` pass. Runtime/baseline paths have no
  changes.


Full native helper/catalog/pointer/alias frames, RNG extraction across actual
callers, saved-game/UI/platform qualification, remote task prerequisites and
phase/cancellation state, and whole annual economic/research integration remain
open. No game source, record format or baseline changed; the completion goal
remains active.


### Research quote and preview continuation

Reviewed 2026-10-25 at `e3222eb1fbdb01d68067231b5d5ef14a71759b30`.
Verification completed 2026-10-26. The [M12 native quote/preview companion](M12-development-and-budget-quotes.md)
adds native single-level cost arithmetic and its checked refinement to the
existing M03 cost/table, ordered six-field development quotes with unsigned
stored spending and temporary-level restoration, and the full budget-preview
controller over explicit returning/escaping helper states. Allocation narrowing,
unconditional gross query, post-query queue selection, captured array bounds,
signed-word budget bounds and current-player restoration are specified.

- Focused Dafny verification: **173 verified, 0 errors**. A draft normal-domain
  cast needed a proved nonnegative-total postcondition. A development-restoration
  timeout exposed missing inner status postconditions; proving that inner loops
  cannot produce Unreachable resolved it without weakening the outer guarantee.
  Parentheses corrected a rejected indentation warning. No assumptions, axioms
  or disabled verification were introduced. Log: `dist/spec-research-focused4.log`.
- Native source fixtures: **120992 defined cases, 2654102 assertions**, actual
  common.h records, fresh IDs, actual table/race/cost/development/preview bodies.
  Preview resource/estimator post-state providers are explicit, including
  controlled longjmp nonreturns; no result value is asserted after escape.
  Traces check field/level/total/cost and callback order/context, final levels,
  unchanged spending/choices, allocation/owner/queue/player/count effects. All
  signed-byte cost choices, table indices at declared level contexts, ordered
  quotes, raw spending, slow/negation/addition boundaries and maximum-count
  signed-word preview cases are covered. **2827** invalid-domain candidates
  were excluded before C, not recovered by the source. Native declarations
  corrected a draft provider signature; existing source pointer-sign/unused-local
  warnings have the project's exclusions, other C11/ASan/UBSan findings are fatal.
  Log: `dist/spec-research-source4.log`.
- Added M12-AC07–16 and M09-AC67, retaining the stable stories/IDs. M12-AC03 now
  explicitly requires stable identity for original-player restoration and links
  the broader observed current-player contract. E12-3's next work explicitly
  inventories dead/alive handling, Generalized Research redo state, packed policy
  and low-byte comparisons, repeated advances, equipment lookup/announcement,
  shared totals and recursive nonpool state.
- Full `python3 tests/spec/verify.py --time-limit 120`, Dafny 4.11.0:
  **96 formal files**, complete include closure, **7301 verified, 0 errors;
  audit 0 findings**. Log: `dist/spec-research-full.log`.
- Link/trace checks pass **2439 local links/includes**, **647 requirement IDs**
  (116 stories, 366 acceptance criteria, 66 interaction and 99 quality
  requirements), zero errors. All **9** checker tests pass.
- **26 handbook tests pass**. The handbook builds **184 rendered pages**,
  26 modules, 647 requirements and 96 formal files, zero rendered-link/asset
  errors, including final evidence text. Logs: `dist/spec-research-handbook-tests.log`,
  `dist/spec-research-handbook-final.log`. No diagram bodies changed.
- SDL Debug configure/build passed without new warnings; selected SDL unit,
  compatibility and tutorial-smoke checks passed **31/31**. Logs:
  `dist/spec-research-configure.log`, `dist/spec-research-build.log`,
  `dist/spec-research-runtime.log`. Full 80-page/native UI qualification stays open.
- Full native regression: **519 matches, 21 unused-storage warnings, 0
  differences/errors, 0 reference inputs skipped**. Report:
  `dist/scaffold/regression/spec-research-comparison.json`. No runtime/game
  source changed; host-specific game-code checks were not required. Baselines
  unchanged.
- `git diff --check`, new-file whitespace and old explicit-anchor checks pass;
  all **158** pre-batch snapshotted files remain present. Runtime/baseline paths
  have no changes.

Remaining work includes complete native memory/alias/array/helper/caller-state
refinement, real browser/trader and UI/platform qualification, and E12-3's full
host spending/advancement/equipment/shared-research/recursive algorithms. These
quote/controller proofs do not close host advancement or global helper purity,
approve a gameplay change or establish unagreed accessibility/performance targets.
No game source, file layout or baseline changed; the completion goal stays active.


### Host research resolution continuation

Reviewed 2026-10-26 at `e3222eb1fbdb01d68067231b5d5ef14a71759b30`.
The [host research companion](M12-host-research-resolution.md) adds the complete
UpdateResearchStatus operational transition machine and equipment scan. It reuses
M12 native costs and canonical scalar eligibility/pool/lowest rules through
checked refinements, retaining native storage, signed intermediates, local versus
packed policy, redo/distribution, all announcement branches, shared totals/alive
divisor and recursive nonpool control. GAP-18 clarifies local next seven versus
persisted policy. Added M12-AC17–26, M19-AC29 and M18-AC08.

- Final focused Dafny verification: **1677 verified, 0 errors** after strengthening
  local policy fields to native signed-16. Log:
  `dist/spec-research-host-focused7.log`. Initial type/shape/precondition and
  transition proof failures were resolved by explicit control/alive/field bounds,
  indexed projections and separated proof obligations. Checked properties include
  structural/control preservation, normal 0..26 level preservation over represented
  prefixes, pool/lowest refinements and terminal stability. No assumptions, axioms
  or disabled verification were introduced.
- Actual native host differential: **11045 cases, 2772295 assertions**, complete
  actual host body, native common.h records/fresh IDs, actual cost/race bodies and
  actual two-argument message forwarding. Part availability/requirements and
  message results are explicit returning providers; selected lookup longjmps
  preserve nonreturn prefixes. All 256 packed policies, signed/unsigned/slow
  boundaries, dead/crippled and zero/negative/1/2/4/16-player contexts, general
  redo and shared recursion, status/notice/TT groups and byte/signed-16 item
  wrapping are covered. Ordered event fingerprints/counters and all numeric
  player state are checked; remaining PLAYER/GAME bytes are unchanged. Stateful
  long-iterator providers are width fixtures, not real-catalog reachability.
  **1693** undefined-domain candidates are excluded before C, not recovered by
  the source. Existing pointer-sign/unused-local source exclusions apply; other
  C11 warnings and ASan/UBSan findings are fatal. Log:
  `dist/spec-research-host-source6.log`.
- Actual catalog terminator probe: **991743 assertions pass** over the full
  actual lookup/TechStatus/hidden-part bodies and fresh generated data tables.
  All sixteen counts terminate before byte wrap; all headless byte IDs and all
  invalid IDs across signed-byte primary traits are checked. A draft probe
  incorrectly expected every valid headless entry to be Available; the corrected
  terminator assertion preserves Restricted and other non-Invalid statuses.
  This is native current-catalog evidence, not full availability or dynamic-trace
  refinement. Log: `dist/spec-research-host-source6.log`.
- Full `python3 tests/spec/verify.py --time-limit 120`, Dafny 4.11.0: all
  **98 formal files** and include closure, **8978 verified, 0 errors; audit
  0 findings**. Log: `dist/spec-research-host-full.log`.
- Final link/trace checks pass **2497 local links/includes**, **659 requirement IDs**
  (116 stories, 378 acceptance criteria, 66 interaction and 99 quality
  requirements), zero errors. All **9** specification-checker tests pass.
- **26 handbook tests pass**; the handbook builds **187 rendered pages**,
  26 modules, 659 requirements and 98 formal files with zero errors across
  **33354 rendered local links/assets**, including final evidence text.
  Logs: `dist/spec-research-host-handbook-tests.log`,
  `dist/spec-research-host-handbook-final.log`. No diagram bodies changed.
- SDL Debug configure/build passed without new warnings; selected SDL unit,
  compatibility and tutorial-smoke checks passed **31/31**. Logs:
  `dist/spec-research-host-configure.log`, `dist/spec-research-host-build.log`,
  `dist/spec-research-host-runtime.log`. Full 80-page/native UI qualification stays open.
- Full native regression: **519 matches, 21 unused-storage warnings, 0
  differences/errors, 0 reference inputs skipped**. Report:
  `dist/scaffold/regression/spec-research-host-comparison.json`. No game source
  changed; host-specific game-code checks were not required. Baselines unchanged.
- `git diff --check`, new-file whitespace and old explicit-anchor checks pass;
  all **162** pre-batch snapshotted files remain present. Runtime/baseline paths
  have no changes.

Total native host termination over declared supported level/policy/catalog/service
frames remains open. A finite Run ending Running/Pending is not success; the
lookup trace consumer does not invent Invalid results. Actual helper/message/heap/
pointer/catalog/global/caller frames, phase integration and saved-game/GUI/AI/
platform qualification remain explicit. No game source, on-disk layout or native
baseline changed; the completion goal stays active.


### Research catalog and control termination continuation

Reviewed 2026-10-26 at `e3222eb1fbdb01d68067231b5d5ef14a71759b30`.
The [host research companion](M12-host-research-resolution.md#catalog-bound-and-host-control-progress)
now links S03 catalog bounds, canonical equipment scan completion and host
control rank/termination. Added M12-AC27–28 and S03-AC09, retaining owning stories.

- [S03 research limits](dafny/S03-research-catalog.dfy) uses all sixteen native
  extents and proves `Calls(Limits(),0)==255` (counts plus terminating calls).
  [M12 scan proof](dafny/M12-catalog-termination.dfy) proves Done and exact cursor
  consumption for `Complete` returning traces. It reuses canonical `AfterLookup`
  message calculation, preserving iterator/notice behavior.
- [Host control progress](dafny/M12-host-termination.dfy) proves strict natural
  rank descent for shaped Running states with normal levels 0..26. Components
  cover bounded advancement, original pool flag, main/shared stages, remaining
  players, one-time distribution, redo credit and field/phase traversal.
  `Finish` terminates without supplied fuel; `SufficientFuel` proves agreement
  with prefix `Run`. The canonical Step summary proves an exact +1 level sum
  only on completed Check-to-Announce transitions and otherwise preserves it.
  `Shape` adds source control invariants; terminal shape checking preserves the
  native general-state write before an undefined arithmetic boundary.
- Focused host progress verification: **966 verified, 0 errors**, Dafny 4.11.0.
  Log: `dist/spec-research-liveness-rank4.log`. The initial import errors and
  recursive catalog-sum unfolding were fixed. The first combined run reached
  **2295 verified, 0 errors, 1 timeout** at rank descent; per-phase assertions,
  a separate arithmetic ordering lemma and an opaque recursive level sum resolved
  it. No assumptions, axioms or disabled verification were introduced.
- Full `python3 tests/spec/verify.py --time-limit 120`, Dafny 4.11.0:
  **101 formal files**, complete include closure, **10492 verified, 0 errors;
  audit 0 findings**. Log: `dist/spec-research-liveness-full.log`.
- Actual native host differential still passes **11045 cases, 2772295
  assertions**. The actual lookup/generated-table probe now also binds every
  formal count to its fresh enum, passing **991759 assertions**. The existing
  **1693** undefined-domain candidates remain excluded before C execution;
  this is not source error recovery. ASan/UBSan and non-excluded warnings are
  fatal. Log: `dist/spec-research-liveness-source2.log`.
- SDL Debug configure/build passes without new warnings; selected SDL unit,
  compatibility, host-library and tutorial-smoke checks pass **31/31**. Logs:
  `dist/spec-research-liveness-configure.log`, `dist/spec-research-liveness-build.log`,
  `dist/spec-research-liveness-runtime.log`. The full 80-page tutorial and real
  platform/accessibility qualification stay open.
- Full native regression: **519 matches, 21 unused-storage warnings, 0
  differences/errors, 0 reference inputs skipped**. Report:
  `dist/scaffold/regression/spec-research-liveness-comparison.json`.
  Runtime/game/catalog/layout and baseline files are unchanged; host-specific
  checks for game-code changes were not required.
- All **9** specification-checker and **26** handbook tests pass. Final
  integrated link/trace checks cover **2525 links/includes**, **662 requirement
  IDs** (116 stories, 381 criteria, 66 interaction, 99 quality), zero errors.
  Handbook builds **190 pages**, **101 formal files** and 26 modules, with zero
  errors across **33822 rendered local links/assets**. A rendered-HTML check
  confirms all three new criteria anchors occur inside acceptance-table rows.
  Logs: `dist/spec-research-liveness-handbook-tests.log`,
  `dist/spec-research-liveness-handbook-final.log`.
- Final diff/anchor review preserves all **166** pre-batch files and their
  explicit anchors. `git diff --check` and new-file whitespace checks pass;
  runtime and baseline paths have no changes.

Remaining E12-3 work is successful Done completion under native-derived
arithmetic/index/policy/storage/service frames and complete actual
helper/message/heap/pointer/global/caller extraction and annual integration.
The catalog theorem is conditional on Complete; the host rank may end at
Pending/Escaped/Undefined. Neither is universal successful C completion.
Saved-game, browser/trader/UI/AI/platform qualification and unapproved policy or
quality targets retain their named owners. The full collection goal stays active.


### Native part availability continuation

Reviewed 2026-10-26 at `e3222eb1fbdb01d68067231b5d5ef14a71759b30`.
The [M11 availability companion](M11-part-availability.md) covers the entire
FLookupPart/TechStatus/FShouldPartBeHidden decision algorithms, all sixteen
category policies, source short-circuit getter order, prior/row pointer effects,
raw widths and headless differences. S03 owns 116 versioned named identities
and shared masks/extents. M12 adds a catalog/context-to-reply adapter with
proved complete 255-call announcement scans. Added M11-AC11–15, M12-AC29,
S03-AC10 and the GAP-19 clarification, preserving existing stories/IDs.

- Focused final verification, Dafny 4.11.0: **263 verified, 0 errors**.
  Log: `dist/spec-availability-focused4.log`. Initial draft `reads` used a
  reserved name; const semicolons and quantified indentation produced warnings.
  Those were corrected rather than allowing warnings. The catalog adapter
  derives Complete from LookupRow, preserves the reply prefix and invokes the
  existing canonical Scan; it does not assume an arbitrary complete reply list.
  No assumptions, axioms or disabled verification were introduced.
- Full `python3 tests/spec/verify.py --time-limit 120`, Dafny 4.11.0:
  **104 formal files**, complete include closure, **10871 verified, 0 errors;
  audit 0 findings**. Log: `dist/spec-availability-full.log`.
- Actual native differential: **2047760 cases, 32854889 assertions**, all **239**
  current generated rows and **116** bound model identities, plus native
  primary/trait/grant/mask protocols. Exact status and helper sequence, direct
  expected array pointer, PART/HS/quantity/padding, all PLAYER/GAME bytes,
  global identity and requirement-buffer frames are checked. Cases include
  primary/trait combinations, every uint16 mask, byte items, all 65536 trader
  words for all twelve special pairs, signed-byte status pairs, current/missing
  combinations and seeded raw contexts. Initial harness indentation failed
  warning-as-error compilation and was fixed. ASan/UBSan and other compiler
  warnings are fatal; only existing native pointer-sign declarations are
  excluded. Log: `dist/spec-availability-source3.log`.
- SDL Debug configure/build passes without new warnings. Logs:
  `dist/spec-availability-configure.log`, `dist/spec-availability-build.log`.
  SDL unit/tutorial/compatibility/host-library checks pass **31/31**;
  `dist/spec-availability-runtime.log`. Full 80-page tutorial, real display,
  accessibility and all-backend qualification remain open.
- Full native regression: **519 matches, 21 unused-storage warnings, 0
  differences/errors, 0 reference inputs skipped**. Report:
  `dist/scaffold/regression/spec-availability-comparison.json`.
  Runtime/game source, catalogs, saved layouts and baselines are unchanged;
  host-specific checks for game-code changes were not required.

- All **9** specification-checker and **26** handbook tests pass. Final
  link/trace checks cover **2584 local links/includes** and **669 IDs**
  (116 stories, 388 acceptance criteria, 66 interaction, 99 quality), zero errors.
  The handbook builds **194 pages**, 26 modules and 104 formal files with zero
  errors across **34736 rendered local links/assets**. Logs:
  `dist/spec-availability-handbook-tests.log`,
  `dist/spec-availability-handbook-final.log`.
- Rendered HTML confirms all seven new criteria anchors are in acceptance-table
  rows. All **169** pre-batch files and explicit anchors are preserved;
  `git diff --check` and new-file whitespace checks pass. Runtime and baseline
  paths have no changes.

The complete decision models and native source fixtures reduce E11-1/E12-2/E12-3
without claiming universal C memory/caller refinement. Native host context/row/
message/heap/frame integration and arithmetic-safe successful Done completion
remain open, together with build-check restoration, enemy knowledge, design
cascades/stats, UI/platform qualification and unapproved quality/policy decisions.
The full collection completion goal stays active.


### Design buildability continuation

Reviewed 2026-10-26 at `e3222eb1fbdb01d68067231b5d5ef14a71759b30`.
The [M11 design-check companion](M11-design-buildability.md) specifies the entire
FCanBuildShdef controller in util.c, composed with canonical native lookup,
trait/hidden/technology/catalog rules. Signed hull classification precedes byte
item assignment; hull first, skip empty slots, exact-status rejection, ordinary
global restoration, raw count/target boundaries and the seventeen-lookup bound
are explicit. Added M11-AC16–21. Construction, S06 and availability documents
link the producer without claiming complete higher-caller integration.

- Final focused verification, Dafny 4.11.0: **526 verified, 0 errors**.
  Log: `dist/spec-buildability-focused4.log`. The controller proves ordinary
  restoration, exact canonical CanBuild agreement for bounded calls, first hull
  probe and maximum lookup count. HullOnly, RejectBeforeExtent and EmptyOverrun
  distinguish availability from broader validation and late undefined reads.
  Draft statement-expression syntax and missing sequence-membership/requirement
  proof facts were corrected; no assumptions, axioms or disabled verification
  were introduced.
- Full `python3 tests/spec/verify.py --time-limit 120`, Dafny 4.11.0:
  **105 formal files**, complete include closure, **11226 verified, 0 errors;
  audit 0 findings**. Log: `dist/spec-buildability-full.log`.
- Actual composed source: **299402 defined cases, 78741602 assertions**. This
  executes the actual controller and actual availability/race/technology/hidden
  bodies with fresh tables, comparing request/query order, ordinary restoration,
  direct expected catalog pointers and SHDEF/PLAYER/GAME frames. It covers all
  hull words, saved global words, occupancy masks, all current rows/terminators
  at every slot, byte counts with early-rejection paths and seeded contexts.
  **65519** invalid-target and **239** reached out-of-array candidates are
  excluded before C; this is not a source error return or recovery. ASan/UBSan
  and non-excluded warnings are fatal. Log: `dist/spec-buildability-source1.log`.
- The native source builder is shared with the availability fixture. Its full
  prior matrix still passes **2047760 cases, 32854889 assertions** after that
  refactoring. Log: `dist/spec-buildability-availability.log`. The native hull
  probe deliberately does not read its uninitialized quantity or prior pointer.

- SDL Debug configure/build passes without new warnings. Logs:
  `dist/spec-buildability-configure.log`, `dist/spec-buildability-build.log`.
  Unit/tutorial/compatibility/host-library checks pass **31/31**;
  `dist/spec-buildability-runtime.log`. The full 80-page tutorial and real
  backend/accessibility/display qualification remain open.
- Full native regression: **519 matches, 21 unused-storage warnings, 0
  differences/errors, 0 reference inputs skipped**. Report:
  `dist/scaffold/regression/spec-buildability-comparison.json`. Runtime/game
  source, catalogs, saved layouts and baselines are unchanged; host-specific
  checks for game-code changes were not required.
- All **9** specification-checker and **26** handbook tests pass. Integrated
  link/trace checks cover **2632 local links/includes**, **675 IDs** (116 stories,
  394 acceptance criteria, 66 interaction, 99 quality), zero errors. Final
  handbook builds **196 pages**, 26 modules, 105 formal files, with zero errors
  across **35371 rendered local links/assets**. All six new criterion anchors
  render inside acceptance-table rows. Logs:
  `dist/spec-buildability-handbook-tests.log`,
  `dist/spec-buildability-handbook-final.log`. Final evidence text is included.
- Review preserves all **174** pre-batch files and explicit anchors;
  `git diff --check` and new-file whitespace checks pass. Runtime/baseline paths
  have no changes.

The remaining E11-1 work is full native memory/alias/lifetime and real
construction/annual/UI/knowledge context integration. The higher construction
fixture retains its availability provider; the new composed checker fixture
supplies stronger service evidence without proving that whole caller. No game
source, catalog/record layout or baseline changes are introduced; unapproved
policy/quality targets retain their owners and the full goal stays active.


### Construction eligibility integration continuation

Reviewed 2026-10-26 at `e3222eb1fbdb01d68067231b5d5ef14a71759b30`.
The [eligibility integration companion](M19-construction-buildability.md) binds
canonical M11 Check into FBuildObject's no-base/owner/roster/index/free/check
order and separate base notification metadata read. Ready remains distinct from
successful object application. Added M19-AC30–33 and GAP-20; M09-AC10 and M11/S06
handoffs gain actual composition evidence without new duplicate stories.

- Final focused verification: **273 verified, 0 errors**, Dafny 4.11.0.
  Log: `dist/spec-construction-buildability-focused4.log`. Exact checker outcome,
  ordinary global frame, free/no-base short circuits, Route refinement, metadata
  address and decoded-hull safety are proved. BaseNotice reuses the existing
  variant and word helpers. Initial shared-destructor types and a missing signed
  word/power witness were corrected. No assumptions, axioms or disabled
  verification were introduced.
- Full `python3 tests/spec/verify.py --time-limit 120 --progress Symbol`, Dafny
  4.11.0: **106 formal files**, complete include closure, **11499 verified,
  0 errors; audit 0 findings**. Log: `dist/spec-construction-buildability-full.log`.
- New actual eligibility integration: **179072 defined cases** plus a full
  post-charge Produce/fleet-limit case; **43707827 assertions pass**. Actual
  application/builder/checker/race/hidden/technology/catalog bodies and fresh
  tables are composed. Exact helper requests, query order, context, metadata
  pointers, notification payloads and expected full design/player/game/planet
  frames are compared. **98380** design-index, **96** owner, **3806** roster,
  **2686** metadata and **488** reached checker-slot candidates are excluded
  before C. These are domain filters, not source recovery. An initial unsigned
  expected cargo comparison was corrected to the actual signed-16 message
  argument; 65535 becomes -1. Log: `dist/spec-construction-buildability-source2.log`.
  Existing unused-resource-parameter/pointer-sign exclusions apply; all other
  warnings and ASan/UBSan findings are fatal.
- Original construction matrix still passes **1897437 assertions** after sharing
  its native source builder. Full availability matrix still passes **2047760
  cases, 32854889 assertions** after sharing its typed catalog selector. Logs:
  `dist/spec-construction-buildability-legacy.log`,
  `dist/spec-construction-buildability-availability.log`.
- `tests/spec/verify.py` adds optional `--progress None|Symbol|Batch`; default
  None retains quiet output. The full run uses Symbol for visible progress.
  Included-file verification, proof limits and mandatory audit are unchanged.

- SDL Debug configure/build passes without new warnings. Logs:
  `dist/spec-construction-buildability-configure.log`,
  `dist/spec-construction-buildability-build.log`. Unit/tutorial/compatibility/
  host-library checks pass **31/31**; `dist/spec-construction-buildability-runtime.log`.
  The full 80-page tutorial and actual backend/display/accessibility coverage stay open.
- Full native regression: **519 matches, 21 unused-storage warnings, 0
  differences/errors, 0 reference inputs skipped**. Report:
  `dist/scaffold/regression/spec-construction-buildability-comparison.json`.
  Runtime/game source and baselines are unchanged; host-only checks for game-code
  edits were not required.
- All **9** specification-checker and **26** handbook tests pass. Integrated
  link/trace checks cover **2672 links/includes** and **679 IDs** (116 stories,
  398 acceptance criteria, 66 interaction, 99 quality), zero errors. The handbook
  builds **198 pages**, 26 modules and 106 formal files with zero errors across
  **35974 rendered local links/assets**. Logs:
  `dist/spec-construction-buildability-handbook-tests.log`,
  `dist/spec-construction-buildability-handbook-final.log`. Final evidence text
  is included; rendered HTML confirms all four new criteria are acceptance-table rows.
- All **177** pre-batch files and explicit anchors are preserved;
  `git diff --check` and new-file whitespace checks pass. Runtime/baseline paths
  have no changes.

Remaining work includes complete native pointer/alias/lifetime/caller and annual
qualification, actual remaining economy/warp/message/allocator/fleet-registration/
fuel/routing producers and other module algorithms. The source loader hull
condition closes this one address bound, not all admission/writer reachability.
No game source, catalog/layout or baseline changes; policy and quality targets
retain their owners, and the full completion goal stays active.


### Mass-driver capability continuation

Reviewed 2026-10-26 at `e3222eb1fbdb01d68067231b5d5ef14a71759b30` with the
working collection. The [M08 companion](M08-mass-driver-capability.md) replaces
supplied driver results at this source boundary with a checked complete native
controller. M08-AC07 is strengthened, M08-AC18–19 and M19-AC34 add focused
acceptance coverage without renumbering prior IDs. `Scalar` reuses the existing
aggregation; `DesignFling` composes actual old/optional-new capability with the
existing construction fling rule. No runtime/catalog/layout/baseline change.

- Focused model verification: **169 verified, 0 errors**, including maximum/
  multiplicity and scalar-refinement properties. Log:
  `dist/spec-mass-driver-focused5.log`. Full included-file collection verification
  under Dafny 4.11.0: **11517 verified, 0 errors; mandatory audit 0 findings**, all
  **107 formal files**. Command: `python3 tests/spec/verify.py --time-limit 120
  --progress Symbol`; log: `dist/spec-mass-driver-full.log`. The progress option
  changes reporting only; verification/audit limits and trust checks are unchanged.
- Actual native `IWarpMAFromLppl` plus FBuildObject/Produce/packet integration:
  **488905 defined standalone cases, 55494 integration cases, 4392987 assertions
  passed**. All category words, owner/viewer words in declared contexts,
  item/quantity/count bytes, physical positions, tie order, null outputs and
  visibility are exercised. Native struct frames and global identity are compared.
  Fresh enum assertions bind model identities. ASan/UBSan and warnings-as-errors
  are enabled. Log: `dist/spec-mass-driver-source4.log`. Integration uses the actual
  eligibility/lookup/technology/hidden-part/race/hull helpers and fresh generated
  catalogs together with the actual driver helper. Controlled economy, messages,
  allocation/registration and routing remain explicit providers.
- Extracting the existing actual-eligibility harness into a reusable factory
  preserves its **179072 defined cases plus the full post-charge Produce/fleet-limit
  case, 43707827 assertions passed**. Log:
  `dist/spec-mass-driver-eligibility-reuse.log`.
- SDL Debug configure/build pass without new warnings. Unit/tutorial/compatibility/
  host-library checks pass **31/31**; the full native regression has **519 matches,
  21 unused-storage warnings, 0 differences/errors, 0 reference inputs skipped**.
  Logs: `dist/spec-mass-driver-configure.log`, `dist/spec-mass-driver-build.log`,
  `dist/spec-mass-driver-runtime.log`, `dist/spec-mass-driver-regression.log`.
  No runtime/game source or baseline is changed; game-code-only host checks are
  not required for this batch.
- All **9** spec-checker and **26** handbook tests pass. Link/trace checks cover
  **2703 local links/includes and 682 IDs** (116 stories, 401 acceptance, 66
  interaction, 99 quality), zero errors. The handbook builds **200 pages, 26
  modules and 107 formal files**, zero errors across **36546 rendered links/assets**.
  Logs: `dist/spec-mass-driver-checker-tests.log`,
  `dist/spec-mass-driver-handbook-tests.log`, `dist/spec-mass-driver-handbook-final.log`.
  The final build includes the evidence text. The new companion and all three
  new criteria render correctly; **180 pre-batch files and explicit anchors**
  remain preserved, and tracked/new-file whitespace checks plus `git diff --check`
  pass. Runtime/catalog/text/baseline paths have no changes.
- Pre-C undefined-domain exclusions are explicit: 66668 roster-owner,
  5946 missing-roster, 2235 physical base-slot and 3070 hull-slot reads. These
  are fixture filters, not added C validation or ordinary error returns.
- Output-pointer alias/lifetime, complete caller/helper preservation, admission,
  actual annual/UI/AI reachability, stargate/scanner/destination and terraforming
  choice remain named work. Other module algorithm gaps retain their existing
  owners; this focused closure does not complete E08-3 or the collection goal.


The M08 batch planned at that checkpoint was the complete terraforming selection
pipeline, addressed by the following continuation:
ordered all-axis/axis-specific catalog availability and costs in
`FCanTerraformLppl`, native endpoint/immune/help/harm handling, desirability
trial/restore and width-aware ranking in `IBestTerraform`, and
`IBestRemoteTerra` player substitution/restoration. It must account for helper
failure and reached invalid arithmetic/access prefixes, then connect the
result to the existing construction application; testing a supplied axis alone
will not close E08-2. Existing catalog/availability/habitat rules remain canonical.


### Terraforming selection continuation

Reviewed 2026-10-26 at `e3222eb1fbdb01d68067231b5d5ef14a71759b30` with the
working collection. [Native terraforming selection](M08-terraform-selection.md)
adds canonical ordered catalog/cost/bounds, native habitat, best-axis ranking,
optimum viewpoint, remote substitution and repeated construction handoffs.
M08-AC20–24 and M19-AC35–36 link their existing owning stories. No runtime,
format, layout, catalog, data or baseline change is made.

- Focused model verification: **452 verified, 0 errors** under Dafny 4.11.0,
  including native/safe-axis and endpoint refinements, ordinary environment/
  identity/remote restoration, first-tie selection and construction iteration
  conservation. Log: `dist/spec-terraform-focused9.log`. Final included-file
  collection verification under Dafny 4.11.0: **11817 verified, 0 errors;
  mandatory audit 0 findings**, all **109 formal files**. Command:
  `python3 tests/spec/verify.py --time-limit 120 --progress Symbol`;
  `dist/spec-terraform-full.log`. The final run includes the completed optimum
  read-prefix distinction and all current native/canonical refinements.
- Native pipeline: **1423925 defined cases, 303296026 assertions passed**;
  `dist/spec-terraform-source6.log`. Every one of the 1048576 availability
  subsets, every signed ability word, byte current/original pairs, all saved
  identity words in declared contexts, all signed helper deltas in a ranking
  context, raw habitat fields, asymmetric remote/optimum views and repeated
  construction are exercised. The source fixture composes
  actual lookup/technology/race/habitat with Can/Best/Remote/Optimum and construction,
  with independent controller-provider modes. It preserves exact call traces,
  partial outputs and complete native frame comparisons under ASan/UBSan.
  Fresh generated enum/mask/sentinel/count assertions bind the model identities.
  Pre-C exclusions: 65547 lookup-player reads, 2509 reached player reads and
  129 divisions; these do not become new C error returns. Initial fixture
  construction/optimum provider expectations incorrectly used the input help
  flag; corrected to each caller's actual forced TRUE before the final pass.
  Initial proof timeouts were repaired with checked arithmetic helper lemmas,
  without removing refinement claims or increasing the final collection limit.
- SDL Debug configure/build pass without new warnings; all **31** native unit/
  tutorial/compatibility/host-library checks pass. Full native regression:
  **519 matches, 21 unused-storage warnings, 0 differences/errors, 0 reference
  inputs skipped**. Logs: `dist/spec-terraform-configure.log`,
  `dist/spec-terraform-build.log`, `dist/spec-terraform-runtime.log`,
  `dist/spec-terraform-regression.log`. No game-code-specific host checks are
  required for this specification/test-only batch.
- All **9** specification-checker and **26** handbook tests pass. Link/trace checks cover **2745 local links/includes and 689 IDs**
  (116 stories, 408 acceptance, 66 interaction, 99 quality), zero errors. The
  handbook builds **203 pages, 26 modules and 109 formal files**, zero errors
  across **37345 rendered local links/assets**. Logs:
  `dist/spec-terraform-links-final.log`, `dist/spec-terraform-checker-tests.log`,
  `dist/spec-terraform-handbook-tests.log`, `dist/spec-terraform-handbook-final.log`.
  Final evidence is included in the final build. Existing
  IDs/anchors and all **183** pre-batch files are preserved; tracked/new-file
  whitespace and `git diff --check` pass. All seven new criteria render in
  exactly one acceptance row each. Runtime/catalog/text and frozen/native
  baseline paths have no changes.

The existing M04/M08/M11/S04 rules remain canonical; no proof of whole-application
C refinement, file admission or all-platform floating behavior is asserted.

Remaining work includes S04 complete native floating-trace production, S06
pointer/alias/lifetime and whole-caller frames, admitted race-field mapping,
full forecast/annual/remote-task integration, the native signed aggregation in
`IpctCanTerraformLppl` feeding production limits, and other module algorithms.
Unapproved policy and platform/quality targets keep their recorded owners.


### Colony capability producers continuation

Reviewed 2026-10-26 at `e3222eb1fbdb01d68067231b5d5ef14a71759b30` with the
working collection. [Colony capability contracts](M08-colony-capabilities.md)
add actual signed terraform-count, first-slot gate-range and descending scanner
selection, connected to existing M09 validation and M19 scanner installation.
M08-AC25–28/M09-AC68 and strengthened M19-AC15 preserve all earlier IDs.
No runtime, format, catalog, text, layout or baseline change is made.

- Focused verification: **387 verified, 0 errors**, including word/scalar sum
  correspondence, gate pointer/metadata and scanner result/trace bounds, actual
  validation query selection and planned scanner installation. Log:
  `dist/spec-colony-capabilities-focused4.log`. Final included-file collection
  verification under Dafny 4.11.0: **11878 verified, 0 errors; mandatory audit
  0 findings**, all **110 formal files**; `dist/spec-colony-capabilities-full2.log`.
  Command: `python3 tests/spec/verify.py --time-limit 120 --progress Symbol`.
  The first full run
  timed out on the pre-existing native habitat percentage bound (120 seconds).
  That bound is factored into a separately checked Euclidean-division lemma;
  no claim is dropped or final verification limit increased. The repair and
  complete rerun are recorded before final closure. The first focused repair
  verified the quotient bound but exposed a timeout on the accumulated square;
  that bound is likewise factored into a checked algebra lemma. Both retain
  the original native/safe-axis refinement and the 120-second limit. The repaired
  focused run passes **666 verified, 0 errors** across native habitat, capability,
  production and construction contracts;
  `dist/spec-colony-capabilities-focused6.log`. The final full rerun above
  includes both arithmetic lemmas and all capability/production/construction
  contracts; no assumptions, disabled verification or larger limit are used.
- Native capability integration: **1756689 cases plus four Produce cases,
  497240245 assertions passed**. Per-operation counts: 1253964 terraform totals,
  405480 gate, 15982 scanner and 81263 generic application. Log:
  `dist/spec-colony-capabilities-source4.log`. All availability subsets from the
  existing terraforming corpus, nine-item scanner subsets, all category words,
  raw ranges/identities/counts, real lookup/race/technology and complete native
  frame/message-time comparisons are included. Four Produce cases cover zero,
  positive clipping/application and raw negative-limit removal under declared
  returning economy/quote/message/heap providers. Branch-presence checks prevent
  a pass that skips a producer family.
- Pre-C native-domain exclusions: 65519 terraform lookup-player, 272 terraform
  habitat-player, 65931 capability lookup-player, 66460 gate design, 541 gate
  hull-slot, 476 gate roster-owner and 2269 gate roster-pointer reads. Separately,
  709 generic-application cases exceed the message provider's declared owner
  domain; these are not claimed native failure outcomes. No C recovery or new
  validation is invented.
- The test-only factory extraction preserves the existing terraform source
  fixture: **1423925 cases, 303296026 assertions passed**;
  `dist/spec-colony-capabilities-terraform-reuse.log`.
- An initial count oracle used the input help flag instead of Ipct's forced
  TRUE; corrected before the final pass. An initial fixture routed scanner
  requests through gate mode; it supplies no scanner evidence. Explicit routing
  and nonzero per-operation gates now cover all four families. The final run
  also observes the real lookup context and pre-install scanner at notification.
- SDL Debug configure/build pass without new warnings. Native unit/tutorial/
  compatibility/host-library checks pass **31/31**. Full native regression:
  **519 matches, 21 unused-storage warnings, 0 differences/errors, 0 reference
  inputs skipped**. Logs: `dist/spec-colony-capabilities-configure.log`,
  `dist/spec-colony-capabilities-build.log`,
  `dist/spec-colony-capabilities-runtime.log`,
  `dist/spec-colony-capabilities-regression.log`. No runtime game-code edits
  require additional host-only checks for this batch.
- All **9** specification-checker and **26** handbook tests pass. Link/trace
  checks cover **2781 local links/includes and 694 IDs** (116 stories, 413
  acceptance, 66 interaction, 99 quality), zero errors. The handbook builds
  **205 pages, 26 modules and 110 formal files**, zero errors across **37981
  rendered local links/assets**. Logs:
  `dist/spec-colony-capabilities-checker-tests.log`,
  `dist/spec-colony-capabilities-handbook-tests.log`,
  `dist/spec-colony-capabilities-links-final.log`,
  `dist/spec-colony-capabilities-handbook-final.log`.
- All **187** pre-batch files/explicit anchors are preserved. Tracked/new-file
  whitespace and `git diff --check` pass. All five new criteria render in exactly
  one acceptance row each. Runtime, catalog, text and native/frozen baseline
  paths have no changes. The final evidence build includes this record.


Remaining work retains explicit owners: native alias/lifetime/admitted-field
mapping, S04 float-trace production for other calculations, destination/travel/
construction/disclosure permission, complete messages and whole production/
annual integration, UI/platform qualification, and other module algorithms.
A passing capability helper is not complete M08/M09/M19 or simulation coverage.


Next shared-workflow batch: M01 import/update publication and recovery outcomes,
starting with `FImportSubsequentTurn`, `FImportLibraryGame`,
`FImportConflictingTurnAsCopy` and their staged-file helpers. Preserve the
accepted library decision and separate actual partial effects from stronger
publication guarantees (GAP-09). Source comparison found no changes in
libraryui.c/libraryzip.c/mdi.c/fileui.c between M01's inspected e2eae18 revision
and current e3222eb1; the open work is detailed rule/adapter/outcome closure,
not an inferred change in supported import policy. M08 destination/UI/native
qualification and all other module backlogs remain in scope.


<a id="m01-import-publication-and-recovery"></a>
## M01 import publication and recovery

Continuation inspected 2026-10-26 at `e3222eb1fbdb01d68067231b5d5ef14a71759b30`
with the working collection. The [M01 publication companion](M01-import-publication.md)
connects M01-US03/04/05 to M01-AC16–21 and qualifies M01-AC08/M06-AC06:
a native TRUE import can retain old orders when the orders-file query is FALSE.
S01 completed logical retirement remains distinct from source-specific outcomes.

`M01ImportPublication` supplies checked post-admission first/later-turn and family
publication programs, complete conflict/separate-copy controllers under explicit
helper receipts, projected frame and pending-clearing proofs, and failure/retry
witnesses. Partial cleanup, existing-stage collision removal, previous-copy
residue, restore failure/retry and success-with-cleanup-warning are explicit.
The [source fixture](../tests/spec/check_library_publication_source.py) compiles
extracted current C controllers with warnings as errors and compares their
operation/store/notice/return traces with an independent interpreter. Native I/O,
admission, recursive helper algorithms and UI/crash qualification remain open.

Verification for this continuation:

- Focused `dafny verify spec/dafny/M01-import-publication.dfy --cores 4
  --verification-time-limit 60 --progress Symbol`: **120 verified, zero errors**
  (`dist/spec-library-publication-focused7.log`). New long-path witnesses initially
  failed to discharge, and raising unfolding fuel produced three 60-second
  timeouts. The final model names the shared publication checkpoint and proves
  recovery outcomes from the corresponding source checkpoints; the complete
  controller reply traces remain separately checked. No assumptions, axioms,
  disabled verification, altered C behavior or raised final time limit are used.
- `python3 tests/spec/check_library_publication_source.py`: **61975**
  extracted-controller cases with **32/12/38/20/39** distinct traces for later,
  first-player, conflict replacement, separate copy and family publication.
  Includes successful-enumeration/bad-flag failures and metadata-player arguments
  for all sixteen slots and host-only -1, plus stage ID/year mismatch and
  multiyear-flag cases with a NULL generation output. Native global path-array
  capacities are bound to current declarations. A separate extracted-copy executable
  passes **48** cases: fifteen ordinary-stdio chunk/collision/missing-source cases,
  plus 33 injected read/write/close/cleanup result cases. These do not qualify
  actual OS fault delivery, helper recursion, path aliases or crash durability.
  Log: `dist/spec-library-publication-source8.log`.
- SDL Debug configure/build pass using `macos-sdl-debug` and
  `dist/m05-macos-sdl-debug`. The selected unit/runtime/tutorial/packaging suite
  passes **31/31**; logs: `dist/spec-library-publication-configure.log`,
  `dist/spec-library-publication-build.log`,
  `dist/spec-library-publication-runtime.log`.
- Full native regression passes: **519 matches**, **21 existing unused-storage
  warnings**, **zero differences/errors**, **zero skipped reference inputs**.
  Log: `dist/spec-library-publication-regression.log`; report:
  `dist/scaffold/regression/spec-library-publication-comparison.json`.
  Runtime code is unchanged; no additional host-only checks are required here.
- Specification-checker tests pass **9/9**; handbook tests pass **26/26**.
  Logs: `dist/spec-library-publication-checker-tests.log`,
  `dist/spec-library-publication-handbook-tests.log`.
- The first collection-wide run reported **11997 verified, zero errors and one
  120-second timeout**, in the existing M08 `AxisRefines` positive-result
  postcondition; audit did not run after that failed verification. Explicit case
  facts alone still left a 60-second timeout in the full-state equality. The
  proof now separates immune/outside/in-range cases and uses the already proved
  `WordPreserves`/`Signed32Preserves` identities with local implementation hiding,
  avoiding modular expansion inside the nonlinear calculation. The final focused
  habitat run passes **325 verified, zero errors**, at the unchanged 60-second
  limit (`dist/spec-library-publication-habitat-focused2.log`). All model
  definitions and the complete `AxisRefines` contract are unchanged; only its
  proof body changes. The complete rerun passes **12044 verified, zero errors**
  and **zero audit findings** across **111 formal files**, at the unchanged
  120-second collection limit. Command:
  `python3 tests/spec/verify.py --time-limit 120 --progress Symbol`;
  final log: `dist/spec-library-publication-full2.log`.
  Checks span 2026-10-26–27.
- Final link/trace checks pass: **2812** local links/includes, **700** requirement
  IDs (**116** stories, **419** acceptance, **66** interaction, **99** quality),
  zero errors (`dist/spec-library-publication-links-final.log`). The handbook
  builds **207 pages**, **26 modules**, **111 formal files** and checks **38590**
  local links/assets, zero errors (`dist/spec-library-publication-handbook-final.log`).
  All six new criteria render in exactly one acceptance row each.
- All **190** pre-batch files and their **732** explicit anchors remain present;
  existing declarations across all **110** pre-batch formal files are preserved.
  Whitespace, proof-body/contract preservation, native-report assertions and
  `git diff --check` pass (`dist/spec-library-publication-final-integrity.log`).
  Runtime, catalog, text and native/frozen baseline paths have no changes.

GAP-09 remains open across M02/M05/M06/M17/M18/S01; M01 still owns full
enumeration, native helper/path and UI qualification, export scope and
last-played display decisions. The goal remains active. The next workflow
contract batch is M02 stored-design equality/version/save-publication outcomes,
retaining the explicit GAP-08 policy conflict rather than silently resolving it.


<a id="m02-native-state-version-and-save-continuation"></a>
## M02 native state, version and save continuation

Inspected 2026-10-27 at `e3222eb1fbdb01d68067231b5d5ef14a71759b30`.
The [native M02 companion](M02-native-state-and-save.md) connects M02-US01/02/03
to M02-AC15–23 and strengthens comparison, snapshot and version-exhaustion
qualification. Existing IDs and the GAP-08 legacy-policy distinction remain.
`M02NativeRaceState` supplies exact comparison/capture/binding projections;
`M02VersionReader` supplies silent read/output/frame outcomes; `M02RaceSave`
supplies normal/escaping staged-save flow, checksum EOF, native candidate mutation
and canonical version-scan composition. Existing S04 prefix, S01 checksum/header
and M02 decision definitions remain canonical. No game/data/layout/format/baseline
change is made.

Verification (2026-10-27):

- Final focused M02 verification: **81 verified, zero errors**, using Dafny
  **4.11.0**, cores 4 and the unchanged 60-second limit
  (`dist/spec-race-workflows-focused-final2.log`). Initial proof attempts needed
  explicit comparison witnesses; checksum/EOF expansion then caused two
  60-second controller-frame timeouts. Local implementation hiding now uses the
  verified checksum/EOF contracts without weakening their definitions or frame
  postconditions. No assumptions, axioms or disabled verification are added.
- Final full include closure and mandatory audit:
  `python3 tests/spec/verify.py --time-limit 120 --progress Symbol` passes
  **12125 verified, zero errors**, **zero audit findings**, **114 formal files**
  (`dist/spec-race-workflows-full4.log`). An earlier attempt caught an accidental
  overwrite of the existing S04 prefix model; its model and owning entry were
  restored byte-for-byte from the pre-batch snapshot. A later intermediate pass
  preceded final hf/pointer-domain/EOF contracts, and another launch stopped on
  a Markdown anchor being added concurrently. Neither substitutes for the final
  complete run recorded here.
- `python3 tests/spec/check_race_workflows_source.py`: **98116** native
  comparison/snapshot cases, **196552 assertions**; **272** unterminated-name
  cases excluded before C comparison. All prefix bits, captured bytes, C truth
  flags, shared-prefix suggestion and pointer-selection cases pass. **77858**
  native UTF-8/cache/suggestion outputs agree with an independent maximal-valid-
  prefix oracle. Log: `dist/spec-race-workflows-source-final.log`.
- `python3 tests/spec/check_race_save_source.py`: **9920** actual controller/
  player-writer/checksum/close cases, **9** call traces. File bytes, candidate/
  global alias effects, full binding arrays, escaping writes, cleanup and
  logical/physical-close distinctions pass the independent oracle.
  Log: `dist/spec-race-workflows-save-source-final.log`.
- `python3 tests/spec/check_race_versions_source.py`: **5267** native reader/
  scan/current-save cases, **44526 assertions**, including all parser-return
  classes, ignored header-close failure, hf restoration boundary, case-sensitive
  equality versus case-insensitive families, occupancy/ties, 999 exhaustion,
  unreadable-target priority, both candidate pointer modes and authentication
  restoration. Log: `dist/spec-race-workflows-version-source-final.log`.
  All three fixtures use native headers, warnings as errors and address/undefined-
  behavior sanitizers. Parser, dialog, authentication, allocator and native I/O
  providers remain explicit; no full C refinement, crypto, OS fault delivery,
  Unicode-comparison or UI/crash qualification claim follows.
- SDL Debug configure/build pass with `macos-sdl-debug` in
  `dist/m05-macos-sdl-debug`; selected unit/runtime/tutorial/packaging tests pass
  **31/31**. Full native regression passes **519 matches**, **21 existing unused-
  storage warnings**, **zero differences/errors**, **zero skipped inputs**.
  Logs: `dist/spec-race-workflows-configure.log`, `dist/spec-race-workflows-build.log`,
  `dist/spec-race-workflows-runtime.log`, `dist/spec-race-workflows-regression.log`;
  report: `dist/scaffold/regression/spec-race-workflows-comparison.json`.
  No game-code change requires additional host-only checks in this batch.
- Specification-checker tests pass **9/9**, handbook tests **26/26**.
  Logs: `dist/spec-race-workflows-checker-tests.log`,
  `dist/spec-race-workflows-handbook-tests.log`.
- Final link/trace checks pass **2854** local links/includes and **709** IDs:
  **116** stories, **428** acceptance, **66** interaction and **99** quality,
  zero errors (`dist/spec-race-workflows-links-final.log`). The handbook builds
  **211 pages**, **26 modules**, **114 formal files** and validates **39533**
  local links/assets, zero errors (`dist/spec-race-workflows-handbook-final.log`).
  All nine new criteria render in exactly one acceptance row each.
- All **193** pre-batch files, **739** explicit anchors and existing declarations
  in all **111** pre-batch formal files are preserved. Whitespace, fixture syntax,
  native comparison-report assertions and `git diff --check` pass. Runtime,
  catalog, text and native/frozen baseline paths remain unchanged.
  Log: `dist/spec-race-workflows-integrity.log`.

GAP-21 records the uncaptured full filename suggestion; its normal UI reachability
and intended cancellation policy remain owned by M02/M04/S05. Passing controller
receipts do not establish missing parser/codec, allocator, native alias/path/OS/
concurrency/crash or GUI contracts. The goal remains active. The next M02 batch
is allocator and import/export/duplicate/delete controller completion, retaining
the existing snapshot/reader/save contracts and the explicit GAP-08 policy split.


## M02 race allocation continuation

Observed at `e3222eb1fbdb01d68067231b5d5ef14a71759b30` on 2026-10-27.
The [allocation companion](M02-race-file-allocation.md) and
[M02RaceAllocation](dafny/M02-race-allocation.dfy) specify stage/export temporary
names, exact bounded retries/counter advance, directory and copy effects,
first-free suggestions and the successful save-input projection. M02-AC24–29
trace these rules; GAP-22 keeps the unchecked duplicate-name budget explicit.
Runtime C, generated data/text and native/frozen baselines are unchanged.

Verification in this batch:

- Focused `dafny verify spec/dafny/M02-race-allocation.dfy --cores 4
  --verification-time-limit 60 --progress Symbol`: **31 verified, zero errors**.
  Log: `dist/spec-race-allocation-focused3.log`.
- `python3 tests/spec/check_race_allocation_source.py`: **2376** allocator/
  suggestion traces, **48** exclusive-copy cases and **16** nonexclusive-copy
  cases pass. Exact formatter/counter/call traces, zero-capacity/UTF-8/budget boundaries and enumeration admission,
  real-file overwrite/self-alias/open failures and scripted copy faults are
  checked. Log: `dist/spec-race-allocation-source-final.log`.
- SDL Debug configure/build pass using `macos-sdl-debug` and
  `dist/m05-macos-sdl-debug`. The selected SDL/unit/runtime/tutorial/packaging
  checks pass **31/31**. Full native regression passes **519 matches**, **21**
  existing unused-storage warnings, **zero differences/errors** and **zero
  skipped inputs**. Logs: `dist/spec-race-allocation-configure.log`,
  `dist/spec-race-allocation-build.log`, `dist/spec-race-allocation-runtime.log`,
  `dist/spec-race-allocation-regression.log`; report:
  `dist/scaffold/regression/spec-race-allocation-comparison.json`.
  No game-code change requires additional host-only checks.
- Specification-checker tests pass **9/9**, handbook tests **26/26**.
  Logs: `dist/spec-race-allocation-checker-tests.log`,
  `dist/spec-race-allocation-handbook-tests.log`.
- Integrity checks preserve all **201** pre-batch files, **750** explicit anchors
  and **2589** existing formal declarations. All six criteria render once in a
  four-column acceptance row. New-fixture syntax, whitespace, native comparison
  assertions and `git diff --check` pass. Log:
  `dist/spec-race-allocation-integrity.log`.
- `python3 tests/spec/verify.py --time-limit 120 --progress Symbol` passes
  the full include closure: **12156 verified, zero errors**; mandatory audit:
  **zero findings**. Log: `dist/spec-race-allocation-full.log`.
- Final local link/trace checks pass **2891** links/includes and **715** IDs:
  **116** stories, **434** acceptance, **66** interaction and **99** quality,
  zero errors (`dist/spec-race-allocation-links-final.log`). The handbook
  prepares **26 modules**, **115 formal files** and **213 rendered pages**, and
  validates **40184** local links/assets with zero errors. Log:
  `dist/spec-race-allocation-handbook-final.log`.

The source fixture supplies clock, format-error, directory, availability and
exclusive-copy outcomes for controller checks; real-stdio copy checks have their
own fault domain. Full OS/concurrency/path-alias/Unicode-compare/UI qualification
and outer import/export/duplicate/delete/wizard contracts remain owned work.
No policy conflict or runtime bug is resolved by these passing projections.


## M02 race actions continuation

Observed 2026-10-27 at `e3222eb1fbdb01d68067231b5d5ef14a71759b30`.
[Action contracts](M02-race-file-actions.md) and
[M02RaceActions](dafny/M02-race-actions.dfy) specify the ordinary Duplicate,
Export and Delete controllers with canonical allocation/copy/suggestion and
native-state composition. M02-AC30–36 trace captured-byte and surviving-file
outcomes, notification gates, source-before-comparison order and target/temp
identity. GAP-23 preserves the observed publication exception; GAP-22 remains.

- Focused `dafny verify spec/dafny/M02-race-actions.dfy --cores 4
  --verification-time-limit 60 --progress Symbol`: **20 verified, zero errors**.
  Log: `dist/spec-race-actions-focused3.log`.
- SDL Debug configure/build pass with `macos-sdl-debug` in
  `dist/m05-macos-sdl-debug`; selected unit/runtime/tutorial/packaging checks pass
  **31/31**. Full native regression passes **519 matches**, **21** existing unused-
  storage warnings, **zero differences/errors**, **zero skipped inputs**. Logs:
  `dist/spec-race-actions-configure.log`, `dist/spec-race-actions-build.log`,
  `dist/spec-race-actions-runtime.log`, `dist/spec-race-actions-regression.log`;
  report: `dist/scaffold/regression/spec-race-actions-comparison.json`.
  No game-code change requires additional host-only checks in this batch.
- Specification-checker tests pass **9/9**, handbook tests **26/26**. Logs:
  `dist/spec-race-actions-checker-tests.log`,
  `dist/spec-race-actions-handbook-tests.log`.
- Integrity checks preserve all **203** pre-batch files, **757** explicit anchors
  and **2594** existing formal declarations. All seven criteria render once in a
  four-column acceptance row. Fixture syntax, whitespace, native comparison
  assertions and `git diff --check` pass; runtime/data/text/native/frozen baseline
  paths remain unchanged. Log: `dist/spec-race-actions-integrity.log`.
- Initial full closure run: **12175 verified**, **zero errors**, **one timeout**
  in existing S06 `AcceptanceVectors` at 120 seconds; audit did not run. Log:
  `dist/spec-race-actions-full.log`. The witness now isolates its original
  assertions into independent checked obligations. Only that proof attribute
  changes; every assertion, contract and rule is unchanged. Focused S06 check:
  **358 verified, zero errors** at 60 seconds
  (`dist/spec-race-actions-s06-focused.log`).
- Final `python3 tests/spec/verify.py --time-limit 120 --progress Symbol`
  (Dafny 4.11.0) passes the full include closure: **12496 verified, zero errors**;
  mandatory audit: **zero findings**. Final command exits successfully.
  Log: `dist/spec-race-actions-full2.log`. The larger check count includes the
  isolated S06 obligations; it is not a count of newly specified runtime rules.
- Final link/trace checks pass **2929** local links/includes and **722** IDs:
  **116** stories, **441** acceptance, **66** interaction and **99** quality,
  zero errors (`dist/spec-race-actions-links-final.log`). The handbook prepares
  **26 modules**, **116 formal files** and **215 pages**, and validates **40828**
  local links/assets with zero errors
  (`dist/spec-race-actions-handbook-final.log`).
- `python3 tests/spec/check_race_actions_source.py` passes **4354** actual
  controller cases with independent full-array/file/call/version/counter oracle,
  native headers and address/undefined-behavior sanitizers. Log:
  `dist/spec-race-actions-source-final.log`. Initial fixture compile/link declarations
  and a self-alias copy provider were corrected without changing runtime C.

Remaining M02 D work is import/current-versus-legacy publication, ImportAs/
ImportAndEdit and full name-choice/wizard controllers. Native callee/alias/
lifetime/parser/UI/OS/concurrency/crash qualification remains A/V work; supplied
replies do not establish complete C refinement. This does not close the overall
26-module goal or resolve unapproved product policy.


## M02 race import continuation

Observed 2026-10-27 at `e3222eb1fbdb01d68067231b5d5ef14a71759b30`.
[Import contracts](M02-race-import.md) and
[M02RaceImport](dafny/M02-race-import.dfy) compose source/name choice, exact raw
hint/buffer effects, latched current/legacy dispatch, canonical stage/save,
post-publication validation/recovery and ImportAs/Load/Edit/ImportAndEdit frames.
M02-AC37–45 trace these rules; GAP-24 retains the observed new-flag distinction.
The GAP-08 legacy policy conflict remains Open; runtime C/data/text/record formats
and native/frozen baselines are unchanged.

- Focused `dafny verify spec/dafny/M02-race-import.dfy --cores 4
  --verification-time-limit 60 --progress Symbol`: **44 verified, zero errors**.
  Log: `dist/spec-race-import-focused-final.log`. Initial composition timeouts
  at a partial status destructor were resolved by checked exhaustive status
  matching; no cases or contracts were removed.
- `python3 tests/spec/check_race_import_source.py`: **3202** actual import/name-
  choice/allocation/save/load/edit/wrapper cases pass. Native headers, exact
  writer/checksum/close bodies, live nonlocal jumps and address/undefined-behavior
  sanitizers are used with explicit parser/dialog/BOF/I/O providers. Full arrays,
  naming-buffer tails, override/dialog state, versions/counters, file/directory
  remnants and logical/physical close distinctions match the independent oracle.
  Log: `dist/spec-race-import-source-final.log`.
- SDL Debug configure/build pass with `macos-sdl-debug` in
  `dist/m05-macos-sdl-debug`; selected unit/runtime/tutorial/packaging checks pass
  **31/31**. Full native regression passes **519 matches**, **21** existing unused-
  storage warnings, **zero differences/errors**, **zero skipped inputs**. Logs:
  `dist/spec-race-import-configure.log`, `dist/spec-race-import-build.log`,
  `dist/spec-race-import-runtime.log`, `dist/spec-race-import-regression.log`;
  report: `dist/scaffold/regression/spec-race-import-comparison.json`.
  No game-code change requires additional host-only checks.
- Specification-checker tests pass **9/9**, handbook tests **26/26**. Logs:
  `dist/spec-race-import-checker-tests.log`,
  `dist/spec-race-import-handbook-tests.log`.
- Integrity checks preserve all **206** pre-batch files, **765** explicit anchors
  and **2625** existing formal declarations. Each of nine new criteria renders in
  exactly one four-column row. Fixture syntax, whitespace, native comparison
  assertions and `git diff --check` pass. Log: `dist/spec-race-import-integrity.log`.
- Final `python3 tests/spec/verify.py --time-limit 120 --progress Symbol`
  (Dafny 4.11.0) passes the complete include closure: **12540 verified, zero
  errors**; mandatory audit: **zero findings**; command exits successfully.
  Log: `dist/spec-race-import-full.log`.
- Final link/trace checks pass **2970** local links/includes and **731** IDs:
  **116** stories, **450** acceptance, **66** interaction and **99** quality,
  zero errors (`dist/spec-race-import-links-final.log`). The handbook prepares
  **26 modules**, **117 formal files**, **217 pages** and validates **41515**
  local links/assets with zero errors
  (`dist/spec-race-import-handbook-final.log`).

Remaining M02 D work is the full race wizard and shared LibraryNameDlg event,
invalid-input retry and lifecycle behavior. Native decoder/credential/admission,
pointer/alias/lifetime, path/Unicode/OS/concurrency/crash and actual UI extraction
remain owned A/V obligations. Passing supplied receipts does not establish full
C/application refinement or complete the overall 26-module goal.


## Shared naming and wizard control continuation

Observed 2026-10-27 at `e3222eb1fbdb01d68067231b5d5ef14a71759b30`.
[Shared naming](M01-library-name-dialog.md) and
[wizard control](M04-wizard-controller.md) specify initialization/command/retry,
game/race suggestions, raw page-result dispatch, point/credential/save gates and
owned modal-sequence cleanup with explicit service/native-input boundaries.
M01-AC22–25, M02-AC46–48 and M04-AC08–14 trace these rules. GAP-25 corrects the
old read-only Finish model; GAP-26 retains native rectangle initialization and
lifetime work, and GAP-27 records the separately reproduced growth-hit array
boundary. Runtime C/data/text/record formats and native/frozen baselines
remain unchanged.

- Focused verification: 81 verified, zero errors. Source fixtures pass 342 shared
  naming cases and 988 actual wizard/hash cases with scripted services, native
  headers, warnings as errors and address/undefined-behavior sanitizers.
- The page-source review also corrects M04-AC01's timing: Custom's Random-bit
  clearing is in non-Cancel page commit, before password text validation, not in
  the Custom click handler. The existing native regression sends Custom then
  Next. Complete page semantics remain open.
- GAP-27 records a new native hit-test boundary: 104 defined helper cases pass,
  and both growth hits reproduce an array-bounds sanitizer diagnostic. No
  undefined return value is accepted as a specification result.
- macOS SDL Debug configure/build passed without new warnings; all 31 selected
  runtime/unit/packaging/tutorial checks passed. Full native regression reports
  519 matches, 21 existing unused-storage warnings, zero differences/errors and
  zero skipped reference inputs. Runtime C/data/text/format/baseline files are
  unchanged. No host-only checks are required for these specification edits.
- Full collection verification completed with exit 0: Dafny 4.11.0, 119 formal
  files, `python3 tests/spec/verify.py --time-limit 120 --progress Symbol`,
  **12,579 verified, zero errors; audit zero findings**. No proof assumptions,
  disabled verification or weakened contracts were introduced.
- Checker tests pass 9/9; handbook tests pass 26/26. Final link validation checks
  3,045 local links/includes and 745 requirement IDs (116 stories, 464 acceptance,
  66 interaction, 99 quality), with zero errors. Handbook validation checks
  221 rendered pages and 42,887 local links/assets, with zero errors.
- Preservation checks retain all 209 pre-batch specification/test files, 775
  explicit anchors and 2,665 formal declarations. All 14 new acceptance criteria
  render once in their intended four-column rows. Python syntax, whitespace and
  diff checks pass, including untracked specification/test sources.

Evidence: `dist/spec-race-wizard-full.log`, `dist/spec-race-wizard-focused-final.log`,
`dist/spec-race-wizard-name-source-final.log`,
`dist/spec-race-wizard-controller-source-final.log`,
`dist/spec-race-wizard-hit-source.log`, `dist/spec-race-wizard-configure.log`,
`dist/spec-race-wizard-build.log`, `dist/spec-race-wizard-runtime.log`,
`dist/spec-race-wizard-regression.log`,
`dist/scaffold/regression/spec-race-wizard-comparison.json`,
`dist/spec-race-wizard-checker-tests.log`, `dist/spec-race-wizard-handbook-tests.log`,
`dist/spec-race-wizard-links-final.log`, `dist/spec-race-wizard-handbook-final.log`
and `dist/spec-race-wizard-integrity.log`. Source fixtures use scripted service
receipts; these counts do not establish full native UI or C refinement.

Remaining D scope is full six-page/preset/AR/habitat/economy/research event,
commit and tracking behavior; native geometry/text/control/page/pricing/
credential/save/lifetime/reentrancy/display extraction remains A/V work with
M01/M02/M04/S01/S04/S05/S06. This does not close the full 26-module goal or
approve a new UI/legacy policy.


## Race page command continuation

Observed 2026-10-27 at `e3222eb1fbdb01d68067231b5d5ef14a71759b30`.
[Page-command contracts](M04-page-commands.md) cover every WM_COMMAND arm across
all six race pages, full preset/Custom/name/password/portrait/leftover commit,
AR resets, trait/immunity/research mutations, notification/raw-check narrowing,
callback-versus-modal returns and drawing post-state frames. M04-AC15–25 trace
these rules and ordinary rejection's partial state. No runtime source, file
format, catalog, policy or regression baseline changes.

- Focused proof: 26 verified, zero errors. The source fixture passes 3,344 cases
  with native headers/resource IDs, actual stat/trait/hash/password-rejection and
  BoundsCheckPlayer helpers, warnings as errors and address/undefined-behavior
  sanitizers. It compiles command arms in small dispatch wrappers; UI/catalog/
  geometry/drawing services remain scripted, not full native callbacks.
- The first full run was intentionally stopped (exit 143) after source review
  found a new-model/fixture argument-width mismatch. Immunity masks now narrow
  at DrawRace2's signed-16 boundary; DrawingMaskVectors and corrected fixture
  signatures check -255/-254/-252. Runtime C and baselines were unchanged.
- macOS SDL Debug configure/build passed with no new warnings. All 31 runtime/
  unit/packaging/tutorial checks passed. Full native regression reports 519
  matches, 21 existing unused-storage warnings, zero differences/errors and zero
  skipped reference inputs. No host-only checks are required for these edits.
- Checker tests pass 9/9 and handbook tests pass 26/26. Preservation checks keep
  all 216 pre-batch specification/test files, 792 explicit anchors and 2,725
  formal declarations; all 11 new criteria render in single four-column rows.
  Python syntax and whitespace/diff checks pass, including untracked sources.
- The corrected full collection verification reports **12,605 verified, zero
  errors**, and audit reports **zero findings**, with Dafny 4.11.0 over all 120
  formal files using `python3 tests/spec/verify.py --time-limit 120 --progress
  Symbol`. The process has finished. No proof assumptions, disabled verification
  or weakened contracts were introduced. The original stopped run is not a pass.
- Final validation checks 3,080 local links/includes and 756 requirement IDs
  (116 stories, 475 acceptance, 66 interaction, 99 quality), with zero errors.
  Handbook validation checks 223 rendered pages and 43,614 local links/assets,
  with zero errors.

Evidence: `dist/spec-race-pages-focused-final2.log`,
`dist/spec-race-pages-source-final.log`, `dist/spec-race-pages-full2.log`,
`dist/spec-race-pages-configure.log`, `dist/spec-race-pages-build.log`,
`dist/spec-race-pages-runtime.log`, `dist/spec-race-pages-regression.log`,
`dist/scaffold/regression/spec-race-pages-comparison.json`,
`dist/spec-race-pages-checker-tests.log`, `dist/spec-race-pages-handbook-tests.log`,
`dist/spec-race-pages-links-final.log`, `dist/spec-race-pages-handbook-final.log`
and `dist/spec-race-pages-integrity.log`. These checks establish the stated
command/proof/fixture scope; initialization and complete native UI/pricing
behavior are still unclosed work.

The explicit remaining inventory is initialization/title, all mouse/paint/
tracking and complete pricing/drawing/native extraction; see the companion's
owner/evidence table. GAP-26/27 and policy/runtime qualification remain open.
This closes command-detail work, not the full 26-module objective.


## Race page initialization continuation

Observed 2026-10-27 at `e3222eb1fbdb01d68067231b5d5ef14a71759b30`.
[Initialization contracts](M04-page-initialization.md) cover all six entry arms,
preset prefix matching/portrait restoration, tutorial bypass, default-name/
password boundaries, five leftover additions versus raw stat requests, exact
read-only requests, title ID/argument schedule and all 17 habitat rectangles.
M04-AC26–34 trace these rules. No runtime source, data/text, file format, baseline
or policy changes are included.

- Focused proof: 33 verified, zero errors and no warnings. The source fixture
  passes 1,126 initialization/title/helper cases with native layout and fresh IDs,
  complete player/password bytes, API request order and all 17 rectangles. The
  title's existing unused cch warning is locally suppressed without editing its
  source body; other warnings are errors. Native window/catalog/metric services
  remain scripted.
- macOS SDL Debug configure/build passed with no new warnings. All 31 runtime/
  unit/packaging/tutorial checks passed; full native regression reports 519
  matches, 21 existing unused-storage warnings, zero differences/errors and zero
  skipped reference inputs. Runtime C/data/text/format/baseline files are
  unchanged; host-only checks are not required for these specification edits.
- Checker tests pass 9/9; handbook tests pass 26/26. Preservation checks keep
  all 219 pre-batch specification/test files, 803 explicit anchors and 2,759
  formal declarations. All nine new criteria render once in four-column rows.
  Python syntax, whitespace and diff checks pass, including untracked sources.
- Full collection verification completed with exit 0: Dafny 4.11.0, 121 formal
  files, `python3 tests/spec/verify.py --time-limit 120 --progress Symbol`,
  **12,638 verified, zero errors; formal trust check zero findings**. No proof
  assumptions, disabled verification or weakened contracts were introduced.
- Final link/ID validation checks 3,116 local links/includes and 765 requirements
  (116 stories, 484 acceptance, 66 interaction, 99 quality), with zero errors.
  Handbook validation checks 225 rendered pages and 44,307 local links/assets,
  with zero errors.

Evidence: `dist/spec-race-init-focused3.log`,
`dist/spec-race-init-source-final.log`, `dist/spec-race-init-full.log`,
`dist/spec-race-init-configure.log`, `dist/spec-race-init-build.log`,
`dist/spec-race-init-runtime.log`, `dist/spec-race-init-regression.log`,
`dist/scaffold/regression/spec-race-init-comparison.json`,
`dist/spec-race-init-checker-tests.log`, `dist/spec-race-init-handbook-tests.log`,
`dist/spec-race-init-links-final.log`, `dist/spec-race-init-handbook-final.log`
and `dist/spec-race-init-integrity.log`. Source fixtures establish the stated
initialization/request/helper scope under supplied services; they do not prove
complete native dialog, display, mouse or painting behavior.

Remaining work is full mouse/paint/tracking, complete pricing/drawing and plural
producer rules, title catalog/format and native UI/state extraction, with the
existing explicit owners. This does not complete the full 26-module goal.


## Race mouse and message continuation

Observed 2026-10-27 at `e3222eb1fbdb01d68067231b5d5ef14a71759b30`.
[Pointer/message contracts](M04-pointer-and-message-interactions.md) specify all
parent mouse/cursor/color/erase arms, full hit testing and portrait/habitat/
economy tracking programs, signed-byte/coordinate/MulDiv boundaries, current
callback state and redraw/cleanup/escaping outcomes. M04-AC35–47 trace these
rules. Existing GAP-26/27 domains remain explicit. No runtime source, data/text,
file format, policy or baseline changes.

- Focused verification: 42 verified, zero errors. Source fixtures pass 2,261
  hit/track/mouse/cursor cases and 1,882 color/erase cases (4,143 total), using
  actual controller/helper bodies, native layout, warnings as errors and address/
  undefined-behavior checks. Event/capture/drawing/window services are scripted;
  known invalid source reads are not counted as ordinary C returns.
- macOS SDL Debug configure/build passed without new warnings. All 31 runtime/
  unit/packaging/tutorial checks passed. Full native regression reports 519
  matches, 21 existing unused-storage warnings, zero differences/errors and zero
  skipped reference inputs. Runtime C/data/text/format/baseline files are
  unchanged; no host-only checks are required for these specification edits.
- Checker tests pass 9/9 and handbook tests pass 26/26. Preservation checks keep
  all 222 pre-batch specification/test files, 812 anchors and 2,791 formal
  declarations. All 13 new criteria render once in four-column rows. Python
  syntax, whitespace and diff checks pass, including untracked sources.
- Full collection verification completed with exit 0: Dafny 4.11.0, 122 formal
  files, `python3 tests/spec/verify.py --time-limit 120 --progress Symbol`,
  **12,680 verified, zero errors; formal trust check zero findings**. No proof
  assumptions, disabled verification or weakened contracts were introduced.
- Final validation checks 3,157 local links/includes and 778 requirements
  (116 stories, 497 acceptance, 66 interaction, 99 quality), with zero errors.
  Handbook validation checks 227 rendered pages and 45,083 local links/assets,
  with zero errors.

Evidence: `dist/spec-race-tracking-focused-final2.log`,
`dist/spec-race-tracking-source-final.log`,
`dist/spec-race-tracking-appearance.log`, `dist/spec-race-tracking-full.log`,
`dist/spec-race-tracking-configure.log`, `dist/spec-race-tracking-build.log`,
`dist/spec-race-tracking-runtime.log`, `dist/spec-race-tracking-regression.log`,
`dist/scaffold/regression/spec-race-tracking-comparison.json`,
`dist/spec-race-tracking-checker-tests.log`,
`dist/spec-race-tracking-handbook-tests.log`,
`dist/spec-race-tracking-links-final.log`,
`dist/spec-race-tracking-handbook-final.log` and
`dist/spec-race-tracking-integrity.log`. These checks cover the stated controller/
helper/request scope; complete painting and native UI refinement remain open.

Remaining D/A/V scope is full WM_PAINT/DrawRace2/3/advantage drawing/invalidation,
plural and title/catalog/format producers, and native event/timer/capture/window/
state/lifetime extraction. This does not complete the 26-module objective.

## Player label and race title continuation

Reviewed 2026-10-27 at `e3222eb1fbdb01d68067231b5d5ef14a71759b30`.
[Player-label/title contracts](S03-player-labels-and-race-titles.md) now specify
name/plural/article/fallback/grammar, conditional native string domains and shared
scratch effects, all seven preset plurals, current title/default/grammar catalog
rows and the common single-integer format subset. S03-AC11–14, S04-AC08 and
M04-AC48–49 trace these additions. Full painting, generic formatting/catalog
extraction and native UI/state/lifetime qualification remain open. No runtime
source, data/text, file format, product policy or baseline changes.

- Focused verification of all three new formal files passes: **536 verified,
  zero errors**. General properties include exact trimming/prefix frames,
  literal/single-integer templates and every signed-16 ordinary wizard title.
  Full collection verification is running with
  `python3 tests/spec/verify.py --time-limit 120 --progress Symbol`; its result
  and formal trust check are not yet claimed as passed.
- The source fixture passes **1,033 defined cases**, binds all fifteen generated
  catalog rows and classifies 39 undefined-domain vectors before C execution.
  Complete PLAYER bytes, scratch prefixes/NULs/tails, lookup order and ignored
  text API failure are checked. This is helper evidence, not native display or
  whole-wizard qualification.
- macOS SDL Debug configure/build passes without new warnings. All **31 runtime
  checks** pass; full native regression reports **519 matches**, 21 existing
  unused-storage warnings, zero differences/errors and zero skipped references.
- Checker tests pass **9/9** and handbook tests **26/26**. Preservation checks
  retain 226 pre-batch files, 1,728 explicit/heading link targets and 2,850
  declarations. All seven new criteria render once in the proper table columns;
  Python syntax, whitespace and diff checks pass.

Evidence: `dist/spec-race-text-focused-final.log`,
`dist/spec-race-text-source-final.log`, `dist/spec-race-text-full.log`,
`dist/spec-race-text-configure.log`, `dist/spec-race-text-build.log`,
`dist/spec-race-text-runtime.log`, `dist/spec-race-text-regression.log`,
`dist/scaffold/regression/spec-race-text-comparison.json`,
`dist/spec-race-text-checker-tests.log`,
`dist/spec-race-text-handbook-tests.log`,
`dist/spec-race-text-links-final.log`,
`dist/spec-race-text-handbook-final.log` and
`dist/spec-race-text-integrity.log`. Full collection verification remains
pending; this continuation does not complete the 26-module objective.
