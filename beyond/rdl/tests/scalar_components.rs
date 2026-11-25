use rdl::World;

fn world(source: &str) -> World {
    let mut world = World::new();
    world.load("scalar.rdl", source).unwrap();
    world
}
fn rejects(source: &str, expected: &str) {
    let mut world = World::new();
    let error = world.load("scalar.rdl", source).unwrap_err();
    assert!(
        error.message.contains(expected),
        "expected {expected:?}, got {error}"
    );
    assert_eq!(world.object_count(), 0);
}

const POPULATION: &str = r#"
component CarryingCapacity : integer { require self >= 0 }
component GrowthRate : real { require self > 0.0 }
trait StandardPopulation {
    provides CarryingCapacity = 1_000_000
    provides GrowthRate = 0.15
}
trait HyperExpansion {
    modifies CarryingCapacity { multiply 0.5 }
    modifies GrowthRate { multiply 2.0 }
}
object type Race = {
    requires one CarryingCapacity
    requires one GrowthRate
    traits: Trait set;
    derive doubled: int = self.CarryingCapacity * 2;
}
Race test = { traits = { StandardPopulation; HyperExpansion }; }
"#;

#[test]
fn scalar_components_expose_values_directly_with_arithmetic_and_constraints() {
    let w = world(POPULATION);
    assert_eq!(
        w.query("q", "test.CarryingCapacity").unwrap().as_int(),
        Some(500000)
    );
    assert_eq!(
        w.query("q", "test.GrowthRate").unwrap().as_real(),
        Some(0.3)
    );
    assert_eq!(
        w.query("q", "test.doubled").unwrap().as_int(),
        Some(1000000)
    );
    assert_eq!(
        w.query("q", "test.CarryingCapacity >= 0")
            .unwrap()
            .as_bool(),
        Some(true)
    );
    assert!(w.query("q", "test.CarryingCapacity.value").is_err());
    rejects(
        &POPULATION.replace("multiply 0.5", "multiply -0.5"),
        "require failed",
    );
}

#[test]
fn scalar_constructors_and_assignments_keep_nominal_identity() {
    let w = world(
        "component C : integer { require self > 0 } let value: C = 5; let plain: integer = value; let other = C 8;",
    );
    assert_eq!(
        w.query("q", "value + plain + other").unwrap().as_int(),
        Some(18)
    );
    rejects(
        "component C : integer {} component D : integer {} let c: C = 5; let d: D = c;",
        "nominal component",
    );
    rejects(
        "component C : integer { require self > 0 } let c: C = 0;",
        "require failed",
    );
    rejects(
        "component C : integer {} trait T { provides C }",
        "expected int",
    );
    rejects(
        "component C : integer {} trait T { provides C = \"bad\" }",
        "expected int",
    );
    rejects(
        "component C : integer {} trait T { modifies C.value { multiply 0.5 } }",
        "without .value",
    );
}

#[test]
fn scalar_composition_owns_full_calculation_and_direct_modifier_lists() {
    let w = world(
        r#"
component C : integer {
    compose = if List.length multipliers > 1 then base
              else min 100 (round ((real base + List.sum additions) * List.product multipliers))
    require self >= 0
}
trait Base { provides C = 80 }
trait Adjust { modifies C { add 50 multiply 0.5 } }
trait Extra { modifies C { multiply 2.0 } }
object type O = { traits: Trait set; requires one C }
O a = { traits = { Base; Adjust }; }
O b = { traits = { Base; Adjust; Extra }; }
"#,
    );
    assert_eq!(w.query("q", "a.C").unwrap().as_int(), Some(65));
    assert_eq!(w.query("q", "b.C").unwrap().as_int(), Some(80));
    rejects(
        "component C : integer { compose = \"bad\" }",
        "expected int",
    );
}

#[test]
fn scalar_contract_inheritance_and_multiple_kinds_work() {
    let w = world(
        r#"
abstract component Capacity : integer {
    require self > 0
    compose = min 100 (round (real base * List.product multipliers))
}
abstract component Kind {}
component CarryingCapacity : Capacity, Kind {}
trait T { provides CarryingCapacity = 80 modifies Capacity { multiply 2.0 } }
object type O = { traits: Trait set; requires one Capacity requires one Kind }
O o = { traits = { T }; }
"#,
    );
    assert_eq!(w.query("q", "o.Capacity").unwrap().as_int(), Some(100));
    rejects(
        "abstract component C : int {} trait T { provides C = 5 }",
        "abstract component",
    );
    rejects("component C : integer { value: integer }", "cannot declare");
    rejects(
        "component A : integer {} component B : string {} component C : A, B {}",
        "incompatible scalar",
    );
}

#[test]
fn cardinality_words_validate_composed_kind_members() {
    let source = r#"
abstract component PRT { name: string }
component Hyper : PRT { name = "Hyper" }
component Jack : PRT { name = "Jack" }
component Optional {}
abstract component LRT {}
component First : LRT {}
component Second : LRT {}
trait H { provides Hyper }
trait J { provides Jack }
trait L { provides First provides Second }
object type O = {
    requires one PRT
    allows optional Optional
    allows many LRT
    traits: Trait set;
}
O o = { traits = { H }; }
"#;
    let w = world(source);
    assert_eq!(w.query("q", "o.PRT.name").unwrap().as_str(), Some("Hyper"));
    assert_eq!(
        w.query("q", "o.Optional = none").unwrap().as_bool(),
        Some(true)
    );
    assert_eq!(w.query("q", "Set.size o.LRT").unwrap().as_int(), Some(0));
    rejects(
        &source.replace("traits = { H }", "traits = {}"),
        "exactly one",
    );
    rejects(
        &source.replace("traits = { H }", "traits = { H; J }"),
        "exactly one",
    );
    rejects(
        &source.replace("allows many LRT", "requires some LRT"),
        "cardinality",
    );
    let w = world(
        &source
            .replace("allows many LRT", "requires some LRT")
            .replace("traits = { H }", "traits = { H; L }"),
    );
    assert_eq!(w.query("q", "Set.size o.LRT").unwrap().as_int(), Some(2));
    rejects(
        "component C {} object type O = { requires many C }",
        "requires one/some",
    );
    rejects(
        "component C {} object type O = { allows some C }",
        "requires one/some",
    );
    rejects(
        "component C {} object type O = { requires one C; requires one C }",
        "duplicate field",
    );
}

#[test]
fn scalar_components_can_be_strings_and_booleans() {
    let w = world(
        r#"
component Name : string { require self != "" }
component Flag : bool { require self }
trait T { provides Name = "Test" provides Flag = true }
object type O = { traits: Trait set; requires one Name requires one Flag }
O o = { traits = { T }; }
"#,
    );
    assert_eq!(
        w.query("q", "o.Name + \" race\"").unwrap().as_str(),
        Some("Test race")
    );
    assert_eq!(w.query("q", "not o.Flag").unwrap().as_bool(), Some(false));
    rejects(
        "component S : string {} trait T { modifies S { add 1 } }",
        "numeric scalar",
    );
}

#[test]
fn scalar_component_identity_survives_sets_and_real_aggregation() {
    let w = world(
        r#"
abstract component K {}
component A : real, K {}
component B : real, K {}
trait T { provides A = 1.5 provides B = 1.5 }
object type O = { traits: Trait set; allows many K requires one A }
O o = { traits = { T }; }
"#,
    );
    assert_eq!(w.query("q", "Set.size o.K").unwrap().as_int(), Some(2));
    assert_eq!(
        w.query("q", "List.sum (query { from x: O select x.A })")
            .unwrap()
            .as_real(),
        Some(1.5)
    );
    assert_eq!(
        w.query("q", "List.sum (query { from x: O where false select x.A })")
            .unwrap()
            .as_real(),
        Some(0.0)
    );
    assert_eq!(
        w.query(
            "q",
            "List.product (query { from x: O where false select x.A })"
        )
        .unwrap()
        .as_real(),
        Some(1.0)
    );
}

#[test]
fn scalar_owner_context_can_read_dependencies_and_rejects_cycles() {
    let source = r#"
component Factor : integer {}
component Capacity : integer {
    compose = base * owner.Factor
} with (owner: O)
trait T { provides Factor = 2 provides Capacity = 5 }
object type O = { traits: Trait set; requires one Factor requires one Capacity }
O o = { traits = { T }; }
"#;
    assert_eq!(
        world(source).query("q", "o.Capacity").unwrap().as_int(),
        Some(10)
    );
    rejects(
        &source.replace("base * owner.Factor", "owner.Capacity"),
        "cyclic dependency",
    );
}

#[test]
fn scalar_default_composition_checks_overflow_and_rounds_once() {
    let source = r#"
component C : integer {}
trait Base { provides C = 5 }
trait Adjust { modifies C { add 2 multiply 0.5 multiply 0.5 } }
object type O = { traits: Trait set; requires one C }
O o = { traits = { Adjust; Base }; }
"#;
    assert_eq!(world(source).query("q", "o.C").unwrap().as_int(), Some(2));
    rejects(
        &source.replace("provides C = 5", "provides C = 9223372036854775807"),
        "overflow",
    );
    rejects(
        &source.replace(
            "multiply 0.5 multiply 0.5",
            "multiply 1.0e308 multiply 1.0e308",
        ),
        "non-finite",
    );
    rejects(
        &source.replace("traits = { Adjust; Base }", "traits = { Adjust }"),
        "missing component",
    );
    rejects(
        &source
            .replace(
                "trait Adjust",
                "trait Other { provides C = 3 }\ntrait Adjust",
            )
            .replace("Adjust; Base", "Other; Base"),
        "multiple providers",
    );
}

#[test]
fn optional_components_and_typed_bundle_metadata_remain_supported() {
    let source = r#"
component Flag : bool {}
trait type Named = { name: string; }
trait Enabled : Named { name = "Enabled"; provides Flag = true }
object type O = { traits: Trait set; allows one Flag }
O empty = {}
O full = { traits = { Enabled }; }
"#;
    let w = world(source);
    assert_eq!(
        w.query("q", "empty.Flag = none").unwrap().as_bool(),
        Some(true)
    );
    assert_eq!(
        w.query("q", "Enabled.name").unwrap().as_str(),
        Some("Enabled")
    );
    assert_eq!(
        w.query("q", "full.Flag != none").unwrap().as_bool(),
        Some(true)
    );
    rejects(
        "component C : integer {} object type O = { requires one Missing }",
        "unknown component",
    );
    rejects(
        "type Other = {} object type O = { requires one Other }",
        "unknown component",
    );
}
