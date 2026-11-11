# Trellis in-memory prototype

An experimental Rust implementation of entities, reusable parts, typed attributes,
read-only contexts, staged transactions, validation, evaluation and `explain`.
This answers whether the core model works before introducing grammar or persistence.
It is a standalone, dependency-free Cargo package; it does not build SCARS or alter
the Stars! executable, file formats, turn generation or regression baselines.

## Run

From the repository root:

```sh
cargo run --offline --manifest-path beyond/trellis/prototype/Cargo.toml
```

The demo creates a JOAT default, clones its configuration, stages HE with growth 18
and OBRM, explains a preview, commits, and rejects an invalid growth-21 edit. It
prints the current value after rejection so preservation of the valid state is visible.

The relevant output is:

```text
carrying_capacity = 550000
  Base 1000000 from standard_capacity() [population]
  Multiply 0.5 from hyper_expansion() [primary_trait]
  Multiply 1.1 from only_basic_remote_mining() [lesser_traits]
  round once: half away from zero

effective_growth_percent = 36
  growth_rate = 0.36
    Base 0.18 from growth(percent=18) [growth]
    Multiply 2 from hyper_expansion() [primary_trait]
  fraction x 100, rounded to percent = 36
```

The default remains at 1,200,000 reference capacity and selected growth 15.
The edited race retains selected growth 18 after the invalid transaction fails.

## Core model

Definitions and instances are separate:

| Element | Representation |
| --- | --- |
| Definitions | Rust maps of entity, slot, part, parameter and attribute definitions; immutable after knowledge-base construction |
| Entity | ID, entity-type ID, global/game scope, stored input values and selected parts grouped by slot |
| Part instance | Part-definition ID and typed arguments, such as `growth(percent=18)`; no independent entity identity is required |
| Attribute | Declared value type with an input, numeric-composition or Rust-derived source |
| Context | Read predicate for global entities or global plus one game, optionally narrowed to explicit entity IDs |
| View | Scoped, immutable access to a knowledge base or a transaction's candidate state |
| Transaction | Private candidate entities validated as a batch before publication |
| Evaluator | Read-only calculation session with memoized results, dependency frames and explanations |

IDs are distinct Rust newtypes. Values are integers (`i64`), finite reals (`f64`),
booleans, text or entity references. These are prototype runtime representations,
not a persistence contract. Parts may supply several values: `growth(18)` supplies
both the selected integer percentage and the baseline fraction 0.18.

The examples in [src/stars.rs](src/stars.rs) define four part families and slots:
one population baseline, one primary trait, one growth setting and up to fourteen
lesser traits. The catalog contains only standard capacity, HE, JOAT, growth and
OBRM. It is a small reference-capacity/growth model, not a complete Stars! race or
colony model. Actual colony capacity requires environment and other game inputs.

## Application use

```rust
use trellis_prototype::{stars::*, *};

let mut knowledge = KnowledgeBase::new(definitions()).unwrap();
let context = Context::global();

let mut transaction = knowledge.transaction();
transaction.put(race(
    EntityId(1), Scope::Global, HYPER_EXPANSION, 18, true,
));
{
    let view = transaction.view(&context).unwrap();
    let mut evaluator = Evaluator::new(view);
    assert_eq!(
        evaluator.evaluate(EntityId(1), CAPACITY).unwrap().value,
        Value::Integer(550_000),
    );
}
transaction.commit().unwrap();

let mut evaluator = Evaluator::new(knowledge.view(&context));
assert_eq!(
    evaluator.evaluate(EntityId(1), EFFECTIVE_GROWTH).unwrap().value,
    Value::Integer(36),
);
let explanation = evaluator.explain(EntityId(1), EFFECTIVE_GROWTH).unwrap();
assert!(explanation.contains("growth(percent=18)"));
```

To clone, obtain a visible entity from a view, clone the ordinary Rust value,
assign a new entity ID and stage it in a transaction. To edit, clone the current
entity, change its input/slot data and stage it under the same ID. The demo in
[src/main.rs](src/main.rs) shows both preview and committed evaluation.

The context has no editing target, pending changes, undo state or mutation
operations. `Context::for_game(id)` includes global entities and that game's
entities. `restrict_to(ids)` intersects membership; applying it again cannot
broaden a previous explicit selection. Enumeration, direct reads and derived-rule
dependencies use the same view. Unknown and hidden entities return the same
unavailable-entity error.

Transactions exclusively borrow the in-memory knowledge base. `put` stages an
insert or replacement. `view` validates staged structure before exposing a preview;
`commit` validates the entire batch before changing any stored entity. Dropping
a transaction or rejecting it leaves the knowledge base untouched. This prototype
does not implement authorization, concurrent writers, delete, undo/redo, durable
revisions or multi-player projection. A context is a view, not a security capability.

## Validation

Rust checks definitions when creating the knowledge base, and candidate entities
before preview or commit. Errors contain an entity/slot/part/parameter path and a
message. The checks include:

- Declared attributes, numeric attribute types and consistent slot cardinalities.
- Known entity types, input attributes, slots, parts and parameters.
- Required stored inputs, exact input/argument types and finite real values.
- Part families, missing or extra parameters, and duplicate part instances.
- Native parameter rules, including selected growth in 1..=20 and nonempty names.
- Numeric contribution destinations and types; factors are real ratios.
- Exactly one base provider for every numeric-composed attribute.
- Rejection of direct assignments to composed or derived attributes.

Part-instance identity preserves real argument bits, including signed zero. Rust
definitions are trusted code; their checks and expansion functions must be pure
and deterministic. This is not a sandbox for arbitrary Rust callbacks.

Validation is deliberately staged. Parameter and structural errors reject an edit
before publication. Resolved value constraints, derived-result types, arithmetic
errors and dependency cycles are checked when the attribute is evaluated. They do
not force every derived attribute during insertion. A later requirement mechanism
can explicitly force selected attributes during validation; this prototype does not
provide general cross-entity invariant propagation.

## Numeric evaluation

For an input attribute, return the validated stored value. For a numeric attribute,
expand its selected parts and calculate:

```text
(base + sum(additions)) * product(multipliers)
```

Part expansion is canonical: part-definition ID, typed arguments, slot ID, then
the Rust expansion function's effect order. It is independent of selection order.
Integer additions are checked `i64` operations. Real additions and the product of
real factors preserve their explicit binary64 operation order. Non-finite
intermediate results are errors. With no factors, integer composition stays integer.
With factors, an integer base/addition result converts to binary64, scales once,
rounds once half away from zero and checks the signed range before conversion.
The test suite includes the asymmetric upper `i64` conversion boundary.

This is an explicit prototype policy, not permission to reassociate game arithmetic
or replace existing host formulas. No game regression baseline is changed.

## Rust-derived attributes and explain

A derived rule is an ordinary Rust function receiving `&mut dyn Inputs` and an
entity ID. `Inputs::get` resolves another attribute through the scoped evaluator;
it records the dependency even when that result was memoized. `Inputs::note`
records an optional named internal calculation step. The example effective-growth
rule reads the resolved fraction, converts it to a bounded integer percentage,
and records that conversion.

The evaluator checks visibility before reading a result, detects cycles using the
active dependency path, and unwinds frames on errors. Successful evaluations own
their values and structured explanations. A caller can retain an explanation after
the underlying knowledge base changes without it becoming today's explanation.

Numeric explanations contain each contribution's part ID, slot ID, formatted
arguments, operation and value. Derived explanations contain the actual attribute
reads and explicit named steps. Arbitrary internal Rust arithmetic is not traced
automatically. [src/evaluate.rs](src/evaluate.rs) renders this structure as text;
a future UI can render the same public explanation data.

Memoization is limited to one evaluator session. That session immutably borrows its
source, so it cannot observe a concurrent edit through this interface. Repeated
reads share `Arc<Evaluation>` nodes. After committing changes, create a new session.
Dependency-aware cache reuse across edits remains future work; caching merely by
entity ID across commits would be incorrect. No pricing-performance improvement is
claimed from these tiny examples.

## Files and verification

| File | Responsibility |
| --- | --- |
| [src/model.rs](src/model.rs) | IDs, values, instances, definitions, callbacks and errors |
| [src/lib.rs](src/lib.rs) | Knowledge base, read-only contexts/views and staged transactions |
| [src/validate.rs](src/validate.rs) | Definition and entity validation |
| [src/evaluate.rs](src/evaluate.rs) | Composition, derived evaluation, session memo, cycle handling and explanations |
| [src/stars.rs](src/stars.rs) | Rust definitions for the small growth/capacity catalog |
| [src/main.rs](src/main.rs) | Runnable clone/edit/preview/commit/reject demonstration |
| [tests/core.rs](tests/core.rs) | Behavioral validation, numeric, view, transaction and explanation checks |

```sh
cargo test --offline --manifest-path beyond/trellis/prototype/Cargo.toml
cargo clippy --offline --manifest-path beyond/trellis/prototype/Cargo.toml --all-targets -- -D warnings
cargo fmt --manifest-path beyond/trellis/prototype/Cargo.toml --check
```

The README's Rust example also runs as a documentation test. Focused tests cover
the known HE/JOAT/OBRM results, growth bounds and type errors, failed batches and
discarded previews, global/game and explicit views, hidden derived dependencies,
source traces and memo reuse, canonical selection order, signed half ties, checked
integer and real overflow, cycle recovery and fresh results after committed edits.

There is no grammar, parser, REPL, storage engine, database dependency, serialization,
network access or full race-pricing implementation in this package. The package's
local `[workspace]` keeps it independent of the surrounding UI workspace.

## Verification receipt

On 2026-11-11, with Rust 1.99.1, the demo, 15 behavioral tests, one documentation
test, `cargo fmt --check`, Clippy with warnings denied, and 13 local documentation
links passed. The package builds and tests offline with no third-party crates.

The repository-required macOS SDL Debug build, `make test-unit`, `make regression`
and `make tutorial` were attempted. All stopped at the existing
`compat/win32/src/audio.cpp:36` expansion of `SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK`:
the installed SDL header's old-style cast is rejected by `-Werror,-Wold-style-cast`.
Those game test suites did not run to completion; no regression match or baseline
update is claimed. The prototype does not change game-core code, so host checks
were not required. This unrelated build issue is outside the prototype commit.
