# Society Designer — standalone prototype

A disposable native Rust design study: can SCARS present the existing Stars!
RDL design model as an understandable society creation wizard? The player-facing
term is **Society**, as chosen for this prototype. Biology, shared identity,
technology and economy are parts of the design; culture alone would describe
only some of those choices.

## Run

This package is a member of the sibling SCARS Cargo workspace at
`cpp-demos/rust/Cargo.toml`. Reload that workspace in the IDE to see the
`society-prototype` binary target. From `cpp-demos/rust`:

```sh
cargo run --locked --release -p society-prototype
```

From the `sdl-stars` checkout:

```sh
./beyond/society-prototype/run.sh
```

Requires the sibling `cpp-demos/rust` checkout containing SCARS, Rust, the macOS
SDK and the build tools required by SCARS. The current native SCARS window
backend is macOS-only. This prototype follows its macOS 26 deployment target.
The first build compiles Skia from source and can take several minutes; subsequent
runs reuse the build. The launcher changes into the SCARS workspace so Cargo reads
its configuration, including `FORCE_SKIA_BUILD`. Cargo uses the SCARS workspace's
shared lockfile and target directory.

IDE Debug builds also optimise the SCARS `scars-ui` package (`opt-level = 2`)
while retaining debug symbols. The workspace already optimises Skia, the RDL
interpreter and this prototype. Optimised framework code may step less literally
in a debugger; other Rust packages retain their usual Debug settings.

The window offers three structurally different layouts without discarding choices:

- **Guided:** a step rail, the current page, and a live society dossier.
- **Focus:** top navigation, a compact balance and a wide editing surface.
- **Workbench:** top navigation with the RDL model beside the current page.

Colours follow the native light/dark appearance, including selected traits,
sigils and navigation. Use the titlebar appearance button to switch, or launch
with `--light` / `--dark`. CPU previews support the same flags (default: dark).

Start from any of the six fixed source presets, customize all fourteen additional
traits and ten primary traits, choose environment/immunity/growth, set economy
and research preferences, then review leftover-point allocation. Back and earlier
step navigation preserve edits. Cancel pauses the design and allows resuming.
Finish validates the complete design, including the latest point calculation.
A negative balance or invalid numeric draft prevents completion.

The basic and detailed sigil sheets in `../` are embedded and cropped at their
actual, uneven gutters into sixteen cells at runtime. Each icon and label form
one selectable tile: artwork, label and padding share the same click target and
keyboard focus stop. The selector uses descriptive names invented for this interface;
these are not asserted to be the original artwork's names. Sigils are cosmetic,
independent of the legacy portrait field. The original sheets are unchanged.

## Implementation and SCARS assessment

- [`src/ui.rs`](src/ui.rs) uses SCARS's existing `view!` DSL, typed bindings,
  retained wizard pages, numeric drafts, scrolling, images and native rendering.
  Numeric models and image resources live outside view construction, preserving
  draft lifetime and avoiding repeated decoding.
- Bindings borrow individual design fields through `State::with`, preserving
  dependency tracking without cloning the design and all its strings for every
  numeric/toggle read. Edit equality checks also borrow the current design.
- [`src/model.rs`](src/model.rs) loads the six designs and primary-trait descriptions
  from [`../rdl/examples/stars`](../rdl/examples/stars/README.md). It serializes
  edited choices using the existing parameterized-trait syntax and asks the RDL
  library to validate them and calculate points, growth and reference capacity.
  It does not duplicate the point formula in Rust.
- Pricing runs on one background worker. Requests coalesce, results carry revision
  IDs, and stale answers cannot replace the current balance. While a new result is
  pending, the previous balance is hidden and Finish cannot succeed. SCARS state
  stays on the UI thread. Polling stops when the current calculation completes.
- The RDL inspector exposes the actual generated definition. `Race society = {...}`
  remains the existing RDL schema spelling. Renaming that shared schema and all
  downstream references is outside this prototype. `import settings` resolves
  through the existing model directory, for example with RDL's `-I` option.
- SCARS already supplies stable wizard IDs, retained editing, all-step finish
  validation and mounted navigation that commits native composition before
  checking validity. It is a suitable foundation for this experiment. Its release
  audit still records unfinished interactive accessibility and foreground-example
  gates; building this prototype does not close those framework gates.
- The existing RDL pricing benchmark measured about 0.7 seconds for a cold query
  on this machine before implementation. Background work is necessary for a
  comfortable editor. This is an observation, not a performance guarantee.

## Boundaries

This is an in-memory prototype. Completing it creates a session result, not a
saved library entry or playable `.r1` file. Closing the process discards it.
There is no password UI, game integration, file import/export, random-design
search or new gameplay rule. The existing game and its regression baselines are
unchanged by this prototype.

Names have a visible 31-byte UTF-8 budget, matching the stored name capacity,
while the old wizard imposes a smaller editing limit. The prototype rejects an
oversize name rather than silently truncating it. Environmental inputs use exact
0–100 model coordinates, not approximate physical-unit conversions. AR-specific
factory/mine behavior is explained while its stored settings remain inspectable.
The review's economy summary follows population efficiency, factory output/cost/
operation, then mine output/cost/operation; research follows the six displayed
fields. The sigil index is recorded in an RDL comment, not a new model component.

The default preview is 1360×980. Use Focus for narrower windows. This is a layout
study, not a completed small-display or accessibility audit. RDL is inspectable,
not editable in the interface. Editing the DSL and projecting arbitrary definitions
back into controls would be a separate workflow.

## Inspect and verify

```sh
./beyond/society-prototype/run.sh --verify
./beyond/society-prototype/run.sh --verify-contrast
./beyond/society-prototype/run.sh --verify-sigils
./beyond/society-prototype/run.sh --smoke
./beyond/society-prototype/run.sh --render /tmp/society.png
./beyond/society-prototype/run.sh --render /tmp/society-research.png --page research
./beyond/society-prototype/run.sh --render /tmp/society-workbench.png --layout workbench
./beyond/society-prototype/run.sh --render /tmp/society-focus.png --layout focus --size 960x800
```

`--verify` checks all six preset prices against their source objects, string
escaping/UTF-8, invalid habitat, a mounted sigil action, stale-result rejection,
valid completion, overspending, and layout of all seven pages in all three modes.
`--render` uses SCARS's actual CPU renderer and does not need an open window.
`--smoke` opens the actual AppKit/Metal window and closes after two seconds.
`--verify-contrast` checks selected and unselected trait/sigil controls against
their rendered background colours at a minimum 4.5:1 text contrast, including
light → dark → light changes on the same mounted view.
`--verify-sigils` exercises artwork, padding, label and keyboard selection for all
sixteen tiles in both themes and both artwork styles. Use `--sigils --render FILE`
for a preview of the complete selector without the surrounding wizard.

To compare scrolling in Debug and Release, run from `cpp-demos/rust`:

```sh
cargo run --locked -p society-prototype -- --profile-scroll
cargo run --locked -p society-prototype -- --profile-scroll --profile-force-layout
cargo run --locked --release -p society-prototype -- --profile-scroll
```

`--profile-scroll` runs the real Guided identity and additional-traits pages at
1360×800, warms up for 20 iterations, and reports p50/p95 timings for 120 scroll
iterations. It models the current native path's input layout, pointer routing,
coalesced notification layout, one draw synchronization and CPU painting. Native
drawing synchronizes again only when a custom underlay callback can change the UI.
The normal path uses SCARS's `layout_if_needed`; `--profile-force-layout` disables
reuse for a comparison in the same binary. The workload asserts
that scrolling moves, does not rebuild the view tree and does not grow the image
cache after warmup. It does not measure AppKit notification posting, native cache
pruning, Metal submission or display pacing; these are CPU workload timings,
not an FPS measurement. The path should be updated if native scheduling changes.
Use `--profile-frames N` for longer samples and `--profile-max-ms N` for an optional
local p95 budget. Timing budgets are machine/load dependent and are not CI gates.

The original Debug configuration measured 16.0–25.8 ms p95 for the sigil page;
optimising `scars-ui` reduced that to about 6 ms. With framework geometry reuse,
a subsequent 240-iteration Debug comparison measured 3.3 ms p95 versus 6.1 ms
with forced layout in the same binary. The traits page measured 1.8 ms versus
3.7 ms. Input/draw synchronizations reused geometry; scrolling required one
measurement. Both retained zero view rebuilds and a stable image cache. These
are local observations, not performance guarantees or native frame-rate results.
The final Debug rerun reached 7.9 ms p95 for sigils and 3.4 ms for traits; timing
varied materially across runs. The deterministic framework regression counts
measurement calls: unchanged synchronizations add none, while scrolling updates
bounds and hit targets. This is the reliable check for redundant layout work.

Framework optimisation verification: 284 portable tests, 15 renderer tests and
six opt-in native component/accessibility suites passed, including composition,
virtual collections, element retirement and scroll coordinates. The rebuilt
Debug wizard passed its model, sigil, contrast and native-launch checks. Both
Debug and Release built without warnings; the game regression remained unchanged.

Verification for this implementation: the release build completed without
warnings; `--verify` passed; the native AppKit/Metal `--smoke` run exited
successfully with graphics access; all seven pages and the three layout modes
were rendered through SCARS. The existing RDL suite passed 189 tests. Repository
checks passed the macOS SDL Debug build, all 18 client unit-test suites, and the
SDL tutorial smoke checkpoint. Full native regression: **519 matches (25 with
warnings), zero differences/errors, zero skipped reference inputs**. No baseline
was regenerated. Host checks were not needed because no game-core code changed.
