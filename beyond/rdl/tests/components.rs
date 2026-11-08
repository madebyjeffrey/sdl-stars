use rdl::World;

const SCHEMA: &str = r#"
abstract component PrimaryRacialTrait {
    name: string
    base_points: integer
}
component HyperExpansion : PrimaryRacialTrait {
    name = "Hyper-Expansion"
    base_points = 40
}
component CarryingCapacity { value: integer }
component GrowthRate { value: real }
trait StandardPopulation {
    provides CarryingCapacity { value: 1_000_000 }
    provides GrowthRate { value: 0.15 }
}
trait HyperExpansionPRT {
    provides HyperExpansion
    modifies CarryingCapacity { multiply 0.5 }
    modifies GrowthRate.value { multiply 2.0 }
}
object type race = {
    traits: Trait set;
    component primary: PrimaryRacialTrait option;
    component capacity: CarryingCapacity;
    component growth: GrowthRate;
}
race example = { traits = { StandardPopulation; HyperExpansionPRT }; }
"#;

fn load(source: &str) -> World {
    let mut world = World::new();
    world.load("components.rdl", source).unwrap();
    world
}

fn reject(source: &str, message: &str) {
    let mut world = World::new();
    let error = world.load("components.rdl", source).unwrap_err();
    assert!(
        error.message.contains(message),
        "expected {message:?}, got {error}"
    );
    assert_eq!(world.object_count(), 0, "failed loads must be atomic");
}

#[test]
fn traits_provide_components_and_modifiers_resolve_on_the_object() {
    let world = load(SCHEMA);
    assert_eq!(
        world.query("q", "example.capacity.value").unwrap().as_int(),
        Some(500_000)
    );
    assert_eq!(
        world.query("q", "example.growth.value").unwrap().as_real(),
        Some(0.3)
    );
    assert_eq!(
        world
            .query("q", "(HyperExpansion {}).base_points")
            .unwrap()
            .as_int(),
        Some(40)
    );
    assert!(
        world
            .query("q", "example.primary != none")
            .unwrap()
            .as_bool()
            .unwrap()
    );
    // Definitions and selected trait bundles do not become additional objects.
    assert_eq!(world.object_count(), 1);
    assert_eq!(world.trait_count(), 2);
}

#[test]
fn abstract_components_require_values_and_cannot_be_constructed_or_provided() {
    reject(
        "abstract component P { name: string } component H : P {}",
        "must supply inherited value 'name'",
    );
    reject(
        "abstract component P {} trait T { provides P }",
        "cannot provide abstract",
    );
    reject(
        "abstract component P {} let p = P {}",
        "cannot instantiate abstract",
    );
    reject(
        "abstract component P {} component H : P { typo = 1 }",
        "unknown component field",
    );
    reject(
        "component A : B {} component B : A {}",
        "component inheritance",
    );
    reject("component A : Missing {}", "unknown component");
    reject(
        "abstract component P { name: string } component H : P { name = 1 }",
        "expected string",
    );
}

#[test]
fn fixed_component_values_cannot_be_overridden_by_a_provider_or_constructor() {
    reject(
        "component C { value: int = 1 } trait T { provides C { value = 2 } }",
        "cannot override fixed",
    );
    reject(
        "component C { value: int = 1 } let c = C { value = 2 }",
        "cannot override fixed",
    );
}

#[test]
fn providers_and_modifiers_are_validated_even_without_objects() {
    reject("trait T { provides Missing }", "unknown component");
    reject(
        "component C { value: int } trait T { provides C }",
        "missing required field",
    );
    reject(
        "component C { value: int } trait T { modifies C.value { multiply \"bad\" } }",
        "numeric",
    );
    reject(
        "component C { value: int } trait T { modifies C.typo { add 2 } }",
        "unknown component field",
    );
    reject(
        "component C { value: string } trait T { modifies C { add 2 } }",
        "numeric component field",
    );
    reject(
        "component C { a: int; b: int } trait T { modifies C { add 2 } }",
        "specify a field",
    );
    reject(
        "component C {} trait T { provides C provides C }",
        "duplicate provider",
    );
}

#[test]
fn missing_and_conflicting_providers_and_kind_cardinality_fail() {
    reject(
        &SCHEMA.replace("StandardPopulation; HyperExpansionPRT", "HyperExpansionPRT"),
        "missing component",
    );
    reject(
        &format!(
            "{SCHEMA}\ntrait Other {{ provides CarryingCapacity {{ value = 5 }} }}\nrace conflict = {{ traits = {{ StandardPopulation; Other }}; }}"
        ),
        "multiple providers",
    );
    reject(
        &format!(
            "{SCHEMA}\ncomponent Other : PrimaryRacialTrait {{ name = \"Other\"; base_points = 0 }}\ntrait OtherPRT {{ provides Other }}\nrace conflict = {{ traits = {{ StandardPopulation; HyperExpansionPRT; OtherPRT }}; }}"
        ),
        "at most one",
    );
    let required = SCHEMA
        .replace("PrimaryRacialTrait option", "PrimaryRacialTrait set[1..1]")
        .replace(
            "StandardPopulation; HyperExpansionPRT",
            "StandardPopulation",
        );
    reject(&required, "cardinality");
}

#[test]
fn composition_is_order_independent_and_rounds_integer_results_once() {
    let source = r#"
component C { value: int }
trait Base { provides C { value = 5 } }
trait Add { modifies C { add 2 } }
trait Multiply { modifies C { multiply 0.5 multiply 0.5 } }
object type item = { traits: Trait set; component c: C; }
item a = { traits = { Base; Add; Multiply }; }
item b = { traits = { Multiply; Base; Add }; }
"#;
    let world = load(source);
    assert_eq!(world.query("q", "a.c.value").unwrap().as_int(), Some(2));
    assert_eq!(
        world.query("q", "a.c.value = b.c.value").unwrap().as_bool(),
        Some(true)
    );
}

#[test]
fn components_can_define_a_full_composition_expression() {
    let source = r#"
component C {
    value: int
    compose = {
        value = if List.length multipliers.value > 1 then 0
                else min 100 (round ((real base.value + List.sum additions.value) * List.product multipliers.value))
    }
    require self.value >= 0
}
trait Base { provides C { value = 80 } }
trait Bonus { modifies C { add 50 multiply 0.5 } }
trait Another { modifies C { multiply 2.0 } }
object type item = { traits: Trait set; component c: C; }
item a = { traits = { Base; Bonus }; }
item b = { traits = { Base; Bonus; Another }; }
"#;
    let world = load(source);
    assert_eq!(world.query("q", "a.c.value").unwrap().as_int(), Some(65));
    assert_eq!(world.query("q", "b.c.value").unwrap().as_int(), Some(0));
    reject(
        "component C { value: int; compose = { value = \"bad\" } }",
        "expected int",
    );
}

#[test]
fn component_requirements_check_the_final_result() {
    reject(
        "component C { value: int; require self.value >= 0 } trait B { provides C { value = 1 } modifies C { add -2 } } object type O = { traits: Trait set; component c: C; } O o = { traits = { B }; }",
        "require failed",
    );
    reject(
        "component C { value: int } trait B { provides C { value = 9223372036854775807 } modifies C { add 1 } } object type O = { traits: Trait set; component c: C; } O o = { traits = { B }; }",
        "overflow",
    );
}

#[test]
fn nominal_marker_identity_and_multiple_component_bases_are_preserved() {
    let world = load(
        r#"
abstract component Primary {}
abstract component Special {}
component A : Primary, Special {}
component B : Primary {}
trait T { provides A provides B }
object type O = { traits: Trait set; component all: Primary set; component special: Special; }
O o = { traits = { T }; }
"#,
    );
    assert_eq!(
        world.query("q", "Set.size o.all").unwrap().as_int(),
        Some(2)
    );
    assert_eq!(
        world.query("q", "o.special = (A {})").unwrap().as_bool(),
        Some(true)
    );
}

#[test]
fn component_composition_can_read_other_components_through_its_owner() {
    let world = load(
        r#"
component Factor { value: int }
component C {
    value: int
    compose = { value = base.value * owner.factor.value }
} with (owner: O)
trait T { provides Factor { value = 2 } provides C { value = 3 } }
object type O = { traits: Trait set; component c: C; component factor: Factor; }
O o = { traits = { T }; }
"#,
    );
    assert_eq!(world.query("q", "o.c.value").unwrap().as_int(), Some(6));
    reject(
        r#"
component C { value: int; compose = { value = owner.c.value } } with (owner: O)
trait T { provides C { value = 1 } }
object type O = { traits: Trait set; component c: C; }
O o = { traits = { T }; }
"#,
        "cyclic dependency",
    );
}

#[test]
fn unused_component_expressions_are_validated_and_integer_aliases_work() {
    reject("component C { value: int = 1 / 0 }", "division by zero");
    reject(
        "component C { value: int = -1; require self.value >= 0 }",
        "require failed",
    );
    reject(
        "component C { value: int } trait T { provides C { value = 1 / 0 } }",
        "division by zero",
    );
    reject(
        "component C { value: int } trait T { modifies C { multiply 1 / 0 } }",
        "division by zero",
    );
    let world = load(
        "component C { value: Integer } trait T { provides C { value: 1_000_000 } } object type O = { traits: Trait set; component c: C; } O o = { traits = { T }; }",
    );
    assert_eq!(
        world.query("q", "o.c.value").unwrap().as_int(),
        Some(1_000_000)
    );
}

#[test]
fn abstract_bases_own_inherited_composition_and_validation() {
    let world = load(
        r#"
abstract component C {
    value: int
    compose = { value = min 100 (round (real base.value * List.product multipliers.value)) }
    require self.value >= 0
}
component Concrete : C { value = 60 }
trait T { provides Concrete modifies C { multiply 2.0 } }
object type O = { traits: Trait set; component c: C; }
O o = { traits = { T }; }
"#,
    );
    assert_eq!(world.query("q", "o.c.value").unwrap().as_int(), Some(100));
}

#[test]
fn diamond_inheritance_deduplicates_shared_contracts_and_rejects_ambiguity() {
    load(
        r#"
abstract component Root { value: int; compose = base }
abstract component A : Root {}
abstract component B : Root {}
component Leaf : A, B { value = 5 }
"#,
    );
    reject(
        r#"
abstract component Root { value: int }
component A : Root { value = 1 }
component B : Root { value = 2 }
component Leaf : A, B {}
"#,
        "conflicting inherited field",
    );
    load(
        r#"
abstract component Root { value: int }
component A : Root { value = 1 }
component B : Root { value = 2 }
component Leaf : A, B { value = 3 }
"#,
    );
}

#[test]
fn stars_catalog_and_race_examples_use_the_new_components() {
    use rdl::imports::SourceLoader;
    let path = std::path::PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("examples/races.rdl");
    let mut world = World::new();
    SourceLoader::default()
        .load_files(&[path])
        .load_into(&mut world)
        .unwrap();
    for (component, points) in [
        ("HyperExpansion", 40),
        ("SuperStealth", 95),
        ("WarMonger", 45),
        ("ClaimAdjuster", 10),
        ("InnerStrength", -100),
        ("SpaceDemolition", -150),
        ("PacketPhysics", 120),
        ("InterstellarTraveler", 180),
        ("AlternateReality", 90),
        ("JackOfAllTrades", -66),
    ] {
        assert_eq!(
            world
                .query("q", &format!("({component} {{}}).base_points"))
                .unwrap()
                .as_int(),
            Some(points)
        );
    }
    assert_eq!(
        world
            .query("q", "terrans.CarryingCapacity")
            .unwrap()
            .as_int(),
        Some(500_000)
    );
    assert_eq!(
        world
            .query("q", "settlers.CarryingCapacity")
            .unwrap()
            .as_int(),
        Some(1_200_000)
    );
    assert_eq!(
        world.query("q", "terrans.GrowthRate").unwrap().as_real(),
        Some(0.3)
    );
}

#[test]
fn fixed_values_also_reject_overrides_in_structural_assignments() {
    reject(
        "component C { value: int = 1 } let c: C = { value = 2 }",
        "cannot override fixed",
    );
    reject(
        "component C { value: int = 1 } object type O = { c: C; } O o = { c = { value = 2 }; }",
        "cannot override fixed",
    );
}

#[test]
fn grouped_numbers_preserve_the_signed_integer_boundary() {
    let world = load("let smallest: Integer = -9_223_372_036_854_775_808;");
    assert_eq!(
        world.query("q", "smallest").unwrap().as_int(),
        Some(i64::MIN)
    );
    reject("let too_large = 9_223_372_036_854_775_808;", "64-bit range");
    reject("let malformed = 1__000;", "separators");
    reject("let malformed = -9__223_372_036_854_775_808;", "separators");
}
