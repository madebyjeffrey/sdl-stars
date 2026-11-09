# RDL

RDL lives in `beyond/rdl/`; `beyond/` groups additional projects and experiments.

An experimental Rust implementation of the fact language developed in this chat.
It has a lexer, recursive-descent/Pratt parser, schema and expression checker,
in-memory world, validation runtime, command-line interface, query-only REPL,
and stdio language server. The default `lsp` feature uses `serde_json` and `url`.
The core language builds without dependencies using `--no-default-features`.
All experimental sources, examples,
tests, and Cargo outputs stay in this directory. It is independent of the Stars!
client, host, game files, and regression baselines.

## Source-derived Stars! designs

[The Stars! library](examples/stars/README.md) includes the six predefined race
objects, the separate Random seed, fourteen LRTs, equipment/starting-template
facts and contextual rules for the property report. Start with:

```sh
cargo run --offline -- run examples/predefined.rdl
cargo run --offline -- repl examples/predefined.rdl
```

It preserves actual wizard settings and calculates habitat, population, economy,
fleet policies, equipment eligibility, initial state and wizard points from
explicit contexts. The original three-object demo below retains its illustrative
point formulas. See the library guide for units, coverage and native comparisons.

In that world, `humanoid.points` returns 25 remaining advantage points. To list
all fixed designs:

```ocaml
query { from r: Race select { name = r.name; points = r.points; } }
```

## Try it

From this directory, with Rust and Cargo installed:

For a fresh dependency cache, run `cargo build --locked` once before using the
offline commands below.

```sh
cargo run --offline -- check examples/races.rdl
cargo run --offline -- run examples/races.rdl
cargo run --offline -- query examples/races.rdl -- 'terrans.gravity.points'
cargo run --offline -- repl examples/races.rdl
```

The sample contains three objects and fourteen traits (ten PRT bundles, one
population baseline bundle, two identity bundles, and one secondary trait). Its point formulas are
illustrative, not implementations of Stars! race pricing. `run` prints:

```text
[{ name = "Terrans"; points = 129 }; { name = "Settlers"; points = 120 }]
```

In the REPL, enter an expression or a multiline query:

```ocaml
terrans.habitability_points

query {
  from r : race
  where r.PrimaryRacialTrait.name = "Hyper-Expansion"
  order by r.population descending
  select { name = r.name; points = r.habitability_points; }
}

List.sum (query {
  from t : secondary_racial_trait
  where Set.contains t terrans.secondary
  select t.point_cost
})
```

Complete input evaluates immediately. A blank line forces evaluation of pending
input, including reporting syntax errors. `:cancel` discards pending input.
`:types` lists the declared types; `:world` prints entity counts; `:reload`
re-reads the original files; `:help` lists commands; `:quit` exits. Reloading an
invalid file reports an error and retains the previous world. Declarations are
rejected in the REPL: edit the source files and reload instead. A piped REPL
returns a failure status if any submitted input or command failed.

Multiple files form one world, with forward references across file boundaries:

```sh
cargo run --offline -- check schema.rdl traits.rdl races.rdl
cargo run --offline -- repl schema.rdl traits.rdl races.rdl
```

The CLI loads these entry files and their transitive imports.

## Named imports and search paths

Import a module using its name, with an optional trailing semicolon:

```ocaml
import schema
import stars.traits
```

`schema` names `schema.rdl`; `stars.traits` names `stars/traits.rdl`. Names use
ASCII identifier segments separated by dots; filename case follows the filesystem.
Imports use module
names without file extensions or quoted filesystem paths. Declarations retain
their original, unqualified names in the shared world: types, traits, objects,
and `let` bindings are available to every loaded source.

For each import, the resolver searches in this order:

1. The importing file's directory.
2. The directories of the entry files, in input order.
3. Explicit `-I` / `--import-path` directories, in argument order.
4. Directories from `RDL_PATH`, in environment-list order.

The first matching module wins. Paths passed on the CLI or through `RDL_PATH`
are relative to the process working directory. `RDL_PATH` uses the platform's
normal path separator: `:` on macOS/Linux, `;` on Windows. Empty environment
segments are ignored. Repeated `-I DIR`, attached `-IDIR`, `--import-path DIR`,
and `--import-path=DIR` forms work with every command, including `repl` and `lsp`.
Place these options after the command name and before a query's `--` separator.

From the RDL directory, try the shipped example:

```sh
cargo run --offline -- run -I examples -I examples/imports/library examples/imports/main.rdl
```

The entry file imports the existing race example as `races` and a small library
as `stars.defaults`. It prints only the qualifying race:

```text
[{ name = "Terrans"; points = 129 }]
```

All imports are resolved before world validation, so references can point forward
across files. Each canonical source file is loaded once per transaction, even
with repeated imports, diamond dependencies, or symlink aliases. Distinct files
with conflicting declaration names remain validation errors. Import cycles are
rejected with a dependency chain. Missing modules report the import location and
the candidate paths searched. Read, parse, and validation failures leave the
previous world intact. The graph is limited to 1000 files and 64 import levels.

Imported bare expressions are checked and evaluated, but `run` prints results
only from entry files. A `let` bound query in a library remains available to the
world. REPL input stays query-only; edit imports in the entry files and use
`:reload`, which rereads all dependencies using the same search path.

## Language server

Build the executable with `cargo build --offline --locked`, then configure your
editor to run `target/debug/rdl lsp` (or `rdl.exe lsp` on Windows). The server
uses [LSP 3.17](https://github.com/microsoft/language-server-protocol/blob/gh-pages/_specifications/lsp/3.17/specification.md)
over standard input/output. For JetBrains, see the
[LSP4IJ setup and importable template](editors/jetbrains/README.md).

Features include validation diagnostics, symbol/keyword and field completion,
hover, go-to-definition, document symbols, and semantic highlighting. References can cross source files.
Query variables and local bindings are scoped, and derived fields include their
declared types. Incomplete files get best-effort indexing, including dangling
member-access dots. The authoritative diagnostics always use the original text
and the same parser, checker, and validation runtime as `rdl check`.

The server accepts full replacements and incremental UTF-16 edits, ignores stale
document versions, and publishes versioned diagnostics for open files. Syntax
errors are reported separately for each file; when all files parse, world
validation reports its first error. Correcting errors clears prior diagnostics.
Diagnostics always reflect the current editor buffer contents.

Unlike the CLI's explicit file loading, the server discovers `.rdl` files under
workspace folders unless given `rdl lsp FILE...` or initialization options
`{"files": ["schema.rdl", "facts.rdl"]}`. Open buffers override disk sources.
The resolver follows imports outside the discovered set, including libraries
outside the workspace. `initializationOptions.importPaths` adds search directories
relative to the first workspace folder. These precede command-line `-I` and
`RDL_PATH`; workspace folders also serve as local module roots after entry-file
directories. For an explicit entry point when the RDL directory is the workspace:

```json
{
  "files": ["examples/imports/main.rdl"],
  "importPaths": ["examples", "examples/imports/library"]
}
```

The language server offers module-name completion after `import`, hover and
go-to-file navigation for resolved imports, and normal cross-file symbol
navigation. Unsaved imported files override disk contents, including new files
not yet saved. Removing an import drops dependencies that are neither entry
files nor open editor buffers. Import resolution is recomputed for current text;
errors are reported on the importing document. To limit a world to its imports,
set `files` explicitly rather than relying on workspace-wide discovery.

One server session represents one world. Discovery skips hidden/build/dependency
directories and symlinks, and is limited to 1000 files and 32 directory levels.
Saving, closing, changing workspace folders, or a client file-watch notification
refreshes disk sources. Dynamic file watchers are registered when supported by
the client. The server never writes source files; edits arrive through
synchronized editor buffers.

This first version is synchronous and revalidates the world on each change.
Completion, hover, and semantic symbol roles use a best-effort symbol index.
Full incremental type inference, formatting, rename, references,
quick fixes, and request cancellation are not implemented. Large-world
performance and live JetBrains integration have not been qualified.

### Syntax highlighting

The [TextMate bundle](editors/textmate/README.md) provides syntax coloring without
running the language server. In JetBrains, add `editors/textmate` under
**Settings → Editor → TextMate Bundles**; select the directory containing
`package.json`. Its grammar handles nested comments, raw multiline strings,
escapes, numbers and ranges, type variables, declarations, and query keywords.
The normal editor theme selects the colors.

`rdl lsp` also provides full-document
[semantic tokens](https://github.com/microsoft/language-server-protocol/blob/gh-pages/_specifications/lsp/3.17/language/semanticTokens.md).
Symbol roles come from the current editor index; lexical coloring remains
available for incomplete or invalid input. Multiline spans are split into
non-overlapping single-line tokens and positions count UTF-16 units. The legend
respects the client's supported token types and modifiers. Clients supporting
semantic refresh receive notifications when the world changes. Range and delta
semantic-token requests are not advertised.

## Declarations and values

The default world contains named, immutable entities. Types and traits may be
declared before or after the objects that use them.

```ocaml
trait type primary_racial_trait = { name : string; }
trait type secondary_racial_trait = { name : string; point_cost : integer; }

trait hyper_expansion : primary_racial_trait = {
  name = "Hyper Expansion";
}

object type race = {
  name : string;
  primary : primary_racial_trait option;
  secondary : secondary_racial_trait set;
}

race terrans = {
  name = "Terrans";
  primary = hyper_expansion;
  secondary = {};
}
```

`race terrans = ...` inserts an entity. The declaration identifier establishes
identity; display names need not be unique. Trait and object references preserve
identity, including in query results. `let` binds ordinary values without
asserting new entities:

```ocaml
let starting_population : int = 25000;
let comfortable_gravity = 0.5 .. 2.0;
let names = query { from r : race select r.name }
```

Object and trait types are nominal: two declarations with identical fields are
still different types. Entity identifiers share one namespace. Duplicate types,
identifiers, fields, and set entries are errors. Types and value identifiers are
case-sensitive ASCII identifiers; string contents are UTF-8.

Primitive types:

| Type | Representation |
| --- | --- |
| `int`, `integer` | Two spellings of the same signed 64-bit integer type |
| `real` | Finite IEEE 754 binary64 number |
| `string` | UTF-8 text |
| `bool` | `true` or `false`, also the result of predicates |

Ordinary strings support `\n`, `\r`, `\t`, `\"`, and `\\`. Triple-quoted strings
are raw multiline strings. An initial newline and final blank closing line are
removed, then common indentation is stripped; interior blank lines are retained.
Comments use `//` or nested `(* ... *)`.

Braces without labels contain a set; braces with `name = value` entries contain
named fields. A type supplies the field schema. Mixing the two forms is rejected.
`{}` means an empty set, or an empty record when a record type is expected.
Semicolons separate fields and set entries; the trailing semicolon is optional.
Top-level declarations and query clauses do not need semicolons.

## Inline traits and parameterized templates

Objects may select traits directly alongside their stored fields:

```ocaml
component Band {
  lower: integer;
  upper: integer;
  require self.lower < self.upper;
}
trait gravity(lower: integer, upper: integer) {
  provides Band { lower = lower; upper = upper; }
}
object type Example = {
  name: string;
  requires one Band
}
Example humanoid = {
  name = "Humanoid";
  gravity(15, 85)
}
```

This binds arguments to a declarative trait template. The body may provide
components, modify numeric components, declare typed trait metadata, and use
`require` to validate parameters. It adds no imperative statements, mutation or
I/O. Parameter and effect types are checked even when a template is unused;
argument requirements are checked when applied. Provided component constraints
are checked on the completed object composition, after modifiers, just as for
named traits.
Parameters use the existing concrete RDL types. A template is not a global
trait fact and must receive its arguments before it can be selected.

Templates may share a name when their arities differ, borrowing Prolog's
`name/arity` convention. For example, the Stars! library defines `gravity/1`
with an `Immunity` argument and `gravity/2` with two integer bounds:

```ocaml
gravity(immune)
temperature(15, 85)
radiation(immune)
```

The number of arguments selects the declaration, then its parameter types and
constraints are checked. Overloads with the same name and arity are rejected,
even if their parameter types differ. This is arity dispatch; it adds no
unification or backtracking. A template family cannot share its name with a
named trait, value or object. Overloaded applications require explicit
`name(...)` argument lists; unambiguous templates retain curried ML application.

Named traits are selected as bare lines; applications use `name(arg1, arg2)`.
Trait lines have optional semicolons. Stored field assignments still use the
ordinary record syntax. Duplicate inline choices, wrong argument counts/types,
and conflicting component providers fail validation. Different applications of
the same template can coexist when their components permit it. Argument types
and nominal identities are retained; applying a template does not assert a named
entity. Queries over trait types continue to enumerate named trait facts.

`humanoid.traits` exposes the inline choices as an immutable set. Existing
schemas with a stored `traits` set can mix that set with inline choices: they
are merged as a set and its declared element type and cardinality still apply.
An explicitly declared property named `traits` takes precedence over the implicit
property. Trait references in other stored fields continue to contribute effects.

Pure values can reuse applications, such as
`let broad_gravity = gravity(15, 85)`, followed by a bare `broad_gravity` line
inside another object. No user-defined general functions were added. Existing
ML application syntax remains valid, including `real (Record {...}).field`;
attached parentheses also support explicit argument lists for existing pure
builtins, for example `min(1, 2)`.

## Components and trait bundles

A scalar component declares its primitive type directly. Its `self` is that
value; constraints and calculations need no `.value` wrapper:

```ocaml
component CarryingCapacity : integer {
  require self >= 0
}

component GrowthRate : real {
  require self > 0.0
}

trait StandardPopulation {
  provides CarryingCapacity = 1_000_000
  provides GrowthRate = 0.15
}

trait HyperExpansionPopulation {
  modifies CarryingCapacity { multiply 0.5 }
  modifies GrowthRate { multiply 2.0 }
}

object type PopulationExample = {
  requires one CarryingCapacity
  requires one GrowthRate
  traits: Trait set;
}

PopulationExample example = {
  traits = { StandardPopulation; HyperExpansionPopulation };
}
```

`example.CarryingCapacity` is `500000` and `example.GrowthRate` is `0.3`.
Scalar components can use `integer`/`int`, `real`, `string`, or `bool`. Primitive
arithmetic, comparisons, conversion functions and numeric aggregations use the
underlying value, including value equality. Arithmetic produces ordinary primitive values. Component
identity remains available for inheritance, selection and validation; values
of two unrelated scalar components are not interchangeable in typed assignments.
A bare primitive can be provided or assigned where the component type is known.
`CarryingCapacity 100` also constructs a detached component value.

Record components keep named fields. Abstract components declare contracts;
concrete descendants must supply inherited stored field values. Membership is
nominal, and multiple bases use comma-separated names:

```ocaml
abstract component PrimaryRacialTrait {
  name: string
  description: string
  base_points: integer
}

component HyperExpansion : PrimaryRacialTrait {
  name = "Hyper-Expansion"
  description = "Doubled growth and halved population capacity."
  base_points = 40
}

trait HyperExpansionPRT {
  provides HyperExpansion
}
```

Abstract components cannot be provided or instantiated directly. Scalar bases
can define constraints and composition inherited by concrete scalar descendants.
An empty abstract kind can classify either scalar or record components. A scalar
cannot inherit stored record fields, or incompatible scalar primitive types.

### Cardinality declarations

Inside an object type these declarations create read-only component properties
named after their component type. They resolve from the object's selected traits:

| Declaration | Cardinality | Access |
| --- | --- | --- |
| `requires one PrimaryRacialTrait` | Exactly one | `race.PrimaryRacialTrait` |
| `allows one Flag` | Zero or one | `object.Flag`, or `none` |
| `allows optional Flag` | Zero or one, synonym | Same |
| `allows many SecondaryRacialTrait` | Zero or more | `race.SecondaryRacialTrait`, a set |
| `requires some Tag` | One or more | `object.Tag`, a nonempty set |

Each named type must be a component. The count applies to matching concrete
components, including descendants, rather than the number of selected traits.
`requires many`, `requires optional`, and `allows some` are rejected to avoid
contradictory required/optional wording. These declarations accept optional
semicolons; ordinary schema members still need separators.

The previous explicit slot syntax remains supported for aliases and numeric
bounds: `component primary: PrimaryRacialTrait option;` or
`component extras: Extra set[0..3];`. A scalar slot exposes the scalar value
under its chosen alias, e.g. `component capacity: CarryingCapacity;` yields
`object.capacity`, with no `.value` field.

### Provisions, modifications and composition

`Trait` is the built-in base category for all selectable traits, including old
`trait type` declarations. An explicitly typed bundle can mix metadata and effects:

```ocaml
trait type NamedTrait = { name: string; }
trait Standard : NamedTrait {
  name = "Standard population";
  provides CarryingCapacity = 1_000_000
}
```

Composition collects trait references from stored fields and sets. A selected
trait is applied once even if referenced in multiple fields. Object references
are not followed to select another object's traits. Concrete identity is
preserved in sets, including different scalar components with the same value.
Repeated grants of the same empty marker coalesce; multiple providers of a
scalar or a record with stored fields are errors. Modifiers of missing components
are errors even when the object has no slot exposing them.

For records, `provides C { field = value }` supplies the named fields; `:` is also
accepted for provision values. Assignments in record component definitions are
fixed values: providers and constructors cannot override them, while concrete
descendants can specialize at definition time. `provides C` uses fixed/default
fields and `C {}` constructs a detached record. For record modifiers the field
can be omitted only when there is exactly one stored field. Scalar modifiers
always target the component directly; `.value` is an error.

Without a custom rule, numeric composition adds all `add` contributions, then
applies all `multiply` contributions. Trait identifiers establish deterministic
order independent of selection, import, and declaration order. Integer-only
operations are checked 64-bit arithmetic. Real operations use binary64; an
integer destination is rounded once at the end, half away from zero, with checked
conversion. Component requirements validate the completed result.

A scalar can own a full composition expression:

```ocaml
component CappedCapacity : integer {
  compose = min 2_000_000
    (round ((real base + List.sum additions) * List.product multipliers))
  require self >= 0
}
```

`base` and `self` during composition are the original provided scalar;
`additions` and `multipliers` are lists of real contributions. Empty lists have
sum zero and product one. The expression chooses its own rounding and can branch
or cap the result. Record composition remains supported: return a field record,
read `base.field`, and use `additions.field`/`multipliers.field` for each numeric
stored field's contribution list. Inherited rules apply to descendants; conflicting
inherited composition rules require an explicit override.

An optional `with (owner: Race)` after either component block supplies the
containing object to composition, derived properties and requirements. A rule can
read `owner.OtherComponent`; dependency cycles fail validation. Fixed values and
trait effect expressions are context-free; owner-dependent calculations belong
in `compose` or `derive`. Component slots are checked on each composed object,
and all provided components are validated even if no slot exposes them.
Components themselves do not create world objects: query objects to inspect
resolved components, or `from t: Trait` to inspect selectable trait definitions.

Numeric literals accept separators such as `1_000_000`. Uppercase spellings
`Integer`/`Int`, `Real`, `String`, and `Bool` are also accepted as types.

The Stars! schema in `examples/stars/schema.rdl` owns the race/habitability types,
requires one identity and PRT, permits any number of secondary components, and
requires scalar carrying capacity and growth. Example traits supply those values;
Settlers selects JOAT. The PRT catalog preserves all ten source names,
descriptions and signed costs. HE growth/capacity and JOAT capacity effects are
implemented; other game mechanics remain future work. Example point formulas
and the secondary trait's `point_cost` are illustrative, not Stars! pricing.

The [race property and trait-effect inventory](../../docs/RACE-PROPERTIES-AND-TRAIT-EFFECTS.md)
records the broader source rules and distinguishes contextual calculations from
constant multipliers.

## Options, cardinality, and ranges

An optional property accepts its underlying value directly, or `none`; there is
no `Some` constructor. An omitted optional field defaults to `none`. An omitted
set defaults to empty when its minimum cardinality is zero.

| Field type | Allowed values |
| --- | --- |
| `primary_racial_trait` | Exactly one primary trait |
| `primary_racial_trait option` | Zero or one primary trait |
| `secondary_racial_trait set` | Any number of distinct secondary traits |
| `secondary_racial_trait set[0..3]` | At most three |
| `secondary_racial_trait set[1..4]` | Between one and four |
| `secondary_racial_trait set[2..2]` | Exactly two |
| `secondary_racial_trait set[1..*]` | One or more |

These limits apply to the declared field. An object schema should declare one
field for each constrained trait category.

`range` is a built-in generic record with `lower` and `upper` numeric fields.
The inclusive shorthand stores the endpoints without enumerating the interval:

```ocaml
20 .. 80
range { lower = 20; upper = 80; }
0.5 .. 2.0
```

Both bounds must be numeric, and the lower bound cannot exceed the upper bound.
Two integer endpoints produce an `int range`; a real endpoint produces a
`real range`. An explicit generic range definition can add rules:

```ocaml
type 'a range = {
  lower : 'a;
  upper : 'a;
  require self.lower <= self.upper;
}
```

Type variables use the ML apostrophe spelling. This version supports one type
parameter per definition. A variant can combine a tag and an existing type:

```ocaml
type 'a habitability =
  | immune
  | 'a range
```

In a union, a bare name that does not name a declared type introduces a nullary
constructor, such as `immune`. A known type or a parameterized type denotes a
typed alternative. Record constructors can also be written explicitly:

```ocaml
type habitability =
  | immune
  | band of { lower : int; upper : int; }
```

Record alternatives also retain their enclosing type parameter:

```ocaml
type 'a Box = | empty | box of { value : 'a; }
let count : (int Box) = box { value = 7; };
```

Like generic record constructors, variant constructors infer the parameter from
supplied fields whose declared type is that parameter. Field access and match
bindings use the inferred type; incompatible instantiations are rejected.

## Validation and derivation

Use `require` for predicates and `derive` for read-only properties:

```ocaml
object type colony = {
  population : int;
  require self.population >= 0;
  derive population_units : int = self.population / 100;
}
```

Rules can also be attached with a `with` block. Owner context is declared once
on the value type and supplied implicitly through the object access path:

```ocaml
type 'a habitability =
  | immune
  | 'a range
with (owner : race) {
  derive points : int =
    let modifier =
      if owner.PrimaryRacialTrait.name = "Hyper-Expansion" then 20 else 0
    in
    match self with
    | immune -> -100 + modifier
    | range { lower; upper } ->
        round (100.0 - (real upper - real lower)) + modifier;
}
```

`self` is the setting; `owner` is the containing race. Access is simply
`terrans.gravity.points`. Ownership follows nested value fields, including
reusable values copied into different entities. Following an entity reference
switches to that entity's context. A detached reusable value has no owner;
owner-dependent rules are deferred until attachment, and directly requesting an
owner-dependent derived value without an owner is an error.

Retained trait arguments are validated when the application is selected by an
object. An ownerless argument uses that object's owner context for deferred
checks; an argument read from another object retains its original context.
Reusing an application checks ownerless arguments separately for each owner.
Trait bodies and effects still evaluate in their application-time context.

Loading parses all files, registers declarations, checks schemas and expressions,
resolves facts, and evaluates applicable requirements and derived values. Checks
include missing/extra fields, wrong types or trait categories, cardinality,
duplicates, bounds, non-exhaustive matches, cyclic dependencies, and numeric
errors. Rules are type-checked even when their object type has no instances.
Derived values cannot be supplied as fields in declarations. Loading is atomic:
failure leaves the prior world intact. Errors include source file, line, and
column; field and dependency paths add context to validation failures.

Calculations support local `let ... in`, `if ... then ... else`, exhaustive
`match ... with`, field access, function application, arithmetic, comparisons,
and Boolean operators. Match patterns include tags, named record alternatives,
`none`, and `_`. Parentheses disambiguate a nested match from its outer arms.
Arithmetic precedence is conventional; `..` binds below arithmetic and above
comparisons. Function application binds above arithmetic; field access binds
above application.

Integer arithmetic is checked; division truncates toward zero. Real arithmetic
rejects infinity, NaN, and division by zero. Mixed numeric arithmetic promotes
to real; scalar assignment to `real` also accepts an integer. Conversions to
real have the normal binary64 precision limit. Comparing integer and real
values requires an explicit conversion. `sqrt` returns a binary64 real and rejects
negative inputs. `round` rounds half away from zero;
`floor`, `ceil`, and `int`/`integer` return checked integers, with `int` truncating
toward zero. Other built-ins are `real`, `abs`, `min`, and `max`.

## Queries

`from r : race` ranges over all named entities of that exact type. Trait types
can also be queried. Multiple `from` clauses form a Cartesian product, filtered
by the `where` clauses. Multiple filters mean AND. `select` returns a list of
values, records, or entity references. A single `order by` expression supports
`ascending` (the default) or `descending`. Ties retain the original entity-name
order, making results deterministic.

Set functions are `Set.contains value set` and `Set.size set`. Query results
support `List.length`, `List.sum`, and `List.product`. Summation accepts homogeneous numeric
results, checks overflow, and returns the appropriate typed zero for an empty
result. Product returns the appropriate typed one for an empty result.
Local bindings and nested queries can be used inside derived rules:

```ocaml
derive secondary_points : int =
  List.sum (query {
    from t : secondary_racial_trait
    where Set.contains t self.secondary
    select t.point_cost
  });
```

Queries are checked even if their source is empty or all rows are filtered out.
They do not insert entities or modify facts. There are limits of 32 recursive
parser/checker frames, 32 dependency/type-chain levels, 128 evaluator frames,
20,000,000 evaluation steps per request, and 10,000 intermediate query rows.
Record payloads and trait metadata share immutable storage, with copies made on
normalization or owner attachment when necessary. Evaluation borrows definitions
instead of copying their expression trees. Derived properties are lazy: validation
checks stored values, component cardinalities and `require` constraints, evaluating
a derived property only when a constraint or query reads it. Every derived body
is still statically type checked. Unused runtime errors and cycles are reported
when the property is first accessed. A computed result is normalized and its own
constraints are checked before it is cached.

Successful results belonging to world values are cached for that world snapshot,
including properties forced by constraints. Keys retain the value revision,
nominal types, component identity and owner; coercions receive fresh revision IDs.
Failures and incomplete validation are never cached. Temporary query calculations
are discarded after each request. Successful loads, extensions and REPL reloads
start a fresh cache; a failed reload preserves the last valid world and cache.
Concurrent queries share a synchronized cache. Each request has a fresh work
budget and dependency stack. Numeric evaluation order remains unchanged.

Stored record fields may declare checked defaults, for example
`population: integer = 0`; these use global scope and are overridden by supplied
values. Defaults do not turn a field into a derived, read-only property.
These are implementation guardrails, not performance guarantees.

To benchmark the source-derived Humanoid score after a release build:

```sh
cargo build --offline --release
python3 tools/benchmark_pricing.py --repl --runs 3
```

The REPL benchmark reports world loading, the first lazy calculation and repeated
cached reads separately, checking that every result is 25 advantage points. Omit
`--repl` for independent CLI runs that each include world loading and a cold
calculation. `--max-seconds` is an optional machine-specific performance gate;
ordinary correctness tests do not depend on timing. The Stars! native comparison
harness checks calculation correctness.

## Embedding and development

```rust
use rdl::World;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let mut world = World::new();
    let text = std::fs::read_to_string("examples/races.rdl")?;
    world.load("examples/races.rdl", &text)?;
    let points = world.query("<query>", "terrans.habitability_points")?;
    assert_eq!(points.as_int(), Some(129));
    Ok(())
}
```

`World::load_sources` accepts multiple `(source_name, source_text)` pairs as one
transaction. It performs no filesystem reads and rejects unresolved imports.
For file-backed worlds, use the shared loader:

```rust
use rdl::{World, imports::SourceLoader};

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let mut world = World::new();
    let loader = SourceLoader::new(vec!["examples".into(), "examples/imports/library".into()]);
    let graph = loader.load_files(&["examples/imports/main.rdl".into()]);
    graph.load_into(&mut world)?;
    assert_eq!(world.query("<query>", "terrans.population")?.as_int(), Some(25000));
    Ok(())
}
```

`SourceGraph` exposes resolved sources, import links, and errors. Editors can
use `SourceLoader::resolve` with a custom reader for unsaved buffers; a partial
graph is retained for diagnostics and navigation, and cannot be loaded into a
world while import errors remain. Load all entry files together. The library
loader uses only its supplied paths; the CLI and LSP also append
`imports::environment_paths()` from `RDL_PATH`.

`World::query` accepts exactly one expression and rejects
declarations. `Value` exposes scalar accessors, collection items, record fields,
and reference identifiers. The parsed syntax tree is available through
`parser::parse` and `ast`.

```sh
cargo test --offline
cargo clippy --offline --all-targets -- -D warnings
cargo fmt --check
```

Tests exercise source loading, rule validation, queries, owner context, numeric
boundaries, transactions, diagnostics, and the real CLI and piped REPL. The
handwritten parser and checker are a language implementation, not a complete ML
compiler or a formally verified model. Current limits include one type parameter,
direct-field inference for generic record constructors, and no general
user-defined functions, module namespaces/export lists, persistent database, mutation, named
worlds, or range enumeration. REPL line editing/history and completion are not
implemented. The existing Stars! specification collection has not been extended
to cover this experimental language.
