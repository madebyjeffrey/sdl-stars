use rdl::{World, imports::SourceLoader, parser};

fn example() -> World {
    let mut world = World::new();
    let path = std::path::PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("examples/races.rdl");
    SourceLoader::default()
        .load_files(&[path])
        .load_into(&mut world)
        .unwrap();
    world
}

fn reject(source: &str, expected: &str) {
    let error = World::new().load("invalid.rdl", source).unwrap_err();
    assert!(
        error.to_string().contains(expected),
        "expected {expected:?}, got {error}"
    );
    assert_eq!(error.span.source, "invalid.rdl");
    assert!(error.span.line > 0 && error.span.column > 0);
}

#[test]
fn example_derives_points_from_owner_properties_and_traits() {
    let world = example();
    assert_eq!(world.object_count(), 3);
    assert_eq!(world.trait_count(), 14);
    assert_eq!(
        world
            .query("q", "terrans.habitability_points")
            .unwrap()
            .as_int(),
        Some(129)
    );
    assert_eq!(
        world
            .query("q", "settlers.habitability_points")
            .unwrap()
            .as_int(),
        Some(120)
    );
    assert_eq!(
        world.query("q", "terrans.gravity.points").unwrap().as_int(),
        Some(129)
    );
    assert_eq!(
        world
            .query("q", "settlers.gravity.points")
            .unwrap()
            .as_int(),
        Some(100)
    );
}

#[test]
fn queries_filter_sort_and_project_without_semicolons() {
    let value = example()
        .query(
            "q",
            r#"
        query {
          from r : race
          where r.population >= 18000
          where Set.contains hyper_expansion r.traits
          order by r.population descending
          select { name = r.name; points = r.habitability_points; }
        }
    "#,
        )
        .unwrap();
    let rows = value.items().unwrap();
    assert_eq!(rows.len(), 1);
    assert_eq!(rows[0].fields().unwrap()["name"].as_str(), Some("Terrans"));
    assert_eq!(rows[0].fields().unwrap()["points"].as_int(), Some(129));
}

#[test]
fn optional_values_need_no_some_constructor() {
    let mut world = example();
    world
        .load("optional.rdl", r#"planet empty = { name = "Empty"; }"#)
        .unwrap();
    assert_eq!(
        world.query("q", "earth.owner = terrans").unwrap().as_bool(),
        Some(true)
    );
    assert_eq!(
        world.query("q", "empty.owner = none").unwrap().as_bool(),
        Some(true)
    );
    assert_eq!(
        world
            .query("q", "Set.size settlers.secondary")
            .unwrap()
            .as_int(),
        Some(0)
    );
}

#[test]
fn query_sources_include_traits_and_queries_return_references() {
    let world = example();
    let rows = world
        .query("q", "query { from t : secondary_racial_trait select t }")
        .unwrap();
    assert_eq!(
        rows.items().unwrap()[0].reference(),
        Some("improved_fuel_efficiency")
    );
    assert_eq!(world.object_count(), 3);
}

#[test]
fn forward_references_work_across_files_and_object_graphs() {
    let mut world = World::new();
    world
        .load_sources(&[
            (
                "first",
                "planet earth = { owner = terrans; } race terrans = { name = \"Terrans\"; }",
            ),
            (
                "second",
                "object type planet = { owner : race; } object type race = { name : string; }",
            ),
        ])
        .unwrap();
    assert_eq!(
        world.query("q", "earth.owner.name").unwrap().as_str(),
        Some("Terrans")
    );
    let rows = world
        .query(
            "q",
            r#"query {
        from p : planet
        from r : race
        where p.owner = r
        select r.name
    }"#,
        )
        .unwrap();
    assert_eq!(rows.items().unwrap()[0].as_str(), Some("Terrans"));
}

#[test]
fn failed_load_is_atomic() {
    let mut world = example();
    assert!(world.load("bad", "race bad = { name = 3; }").is_err());
    assert_eq!(world.object_count(), 3);
    assert_eq!(
        world.query("q", "terrans.population").unwrap().as_int(),
        Some(25000)
    );
    assert!(world.query("q", "bad").is_err());
}

#[test]
fn duplicate_identity_is_rejected_even_across_types() {
    reject(
        "object type race = {} object type planet = {} race x = {} planet x = {}",
        "duplicate",
    );
}

#[test]
fn entities_with_equal_display_names_keep_distinct_identity() {
    let mut world = World::new();
    world.load("s", "object type race = { name : string; } race a = { name = \"Same\"; } race b = { name = \"Same\"; }").unwrap();
    assert_eq!(world.query("q", "a = b").unwrap().as_bool(), Some(false));
}

#[test]
fn traits_are_nominally_typed() {
    reject(
        "trait type primary = {} trait type secondary = {} trait x : secondary = {} object type race = { primary : primary option; } race r = { primary = x; }",
        "expected",
    );
}

#[test]
fn trait_cardinality_bounds_are_checked() {
    let schema = "trait type t = {} trait a : t = {} trait b : t = {} object type race = { traits : t set[0..1]; }";
    reject(
        &format!("{schema} race r = {{ traits = {{ a; b; }}; }}"),
        "cardinality",
    );
    reject(
        "trait type t = {} object type race = { traits : t set[1..*]; } race r = {}",
        "missing required field",
    );
}

#[test]
fn duplicate_set_entries_are_errors_and_order_does_not_affect_equality() {
    reject("let x = { 1; 1; };", "duplicate set entry");
    let world = World::new();
    assert_eq!(
        world.query("q", "{ 1; 2; } = { 2; 1; }").unwrap().as_bool(),
        Some(true)
    );
}

#[test]
fn missing_extra_duplicate_and_derived_fields_are_rejected() {
    reject(
        "object type race = { name : string; } race r = {}",
        "missing required field",
    );
    reject(
        "object type race = {} race r = { extra = 1; }",
        "unknown or derived field",
    );
    reject("let x = { name = 1; name = 2; };", "duplicate field");
    reject(
        "object type race = { derive points : int = 1; } race r = { points = 2; }",
        "unknown or derived field",
    );
    reject(
        "object type race = { x : int; x : int; }",
        "duplicate field",
    );
}

#[test]
fn validation_rules_may_read_derived_values() {
    reject(
        "object type race = { x : int; derive points : int = self.x * 2; require self.points >= 0; } race r = { x = -1; }",
        "require failed",
    );
}

#[test]
fn unused_rules_are_type_checked() {
    reject(
        "object type race = { derive points : int = self.missing; }",
        "unknown field",
    );
    reject(
        "object type race = { derive points : int = \"bad\"; }",
        "expected int",
    );
    reject("object type race = { require 1; }", "expected bool");
}

#[test]
fn queries_over_empty_worlds_are_still_checked() {
    let mut world = World::new();
    world
        .load("s", "object type race = { name : string; }")
        .unwrap();
    assert!(
        world
            .query("q", "query { from r : race select r.missing }")
            .unwrap_err()
            .message
            .contains("unknown field")
    );
    assert!(
        world
            .query("q", "query { from r : race where 1 select r }")
            .is_err()
    );
    assert_eq!(
        world
            .query("q", "query { from r : race select r.name }")
            .unwrap()
            .items()
            .unwrap()
            .len(),
        0
    );
}

#[test]
fn expression_branches_are_checked_even_when_not_evaluated() {
    assert!(
        World::new()
            .query("q", "if true then 1 else missing")
            .is_err()
    );
    assert!(World::new().query("q", "false && 1").is_err());
    assert_eq!(
        World::new()
            .query("q", "false && (1 / 0 = 1)")
            .unwrap()
            .as_bool(),
        Some(false)
    );
}

#[test]
fn bounds_are_inclusive_values_and_reversed_ranges_fail() {
    let world = World::new();
    assert_eq!(
        world.query("q", "(20 .. 80).lower").unwrap().as_int(),
        Some(20)
    );
    assert_eq!(
        world.query("q", "(20 .. 80).upper").unwrap().as_int(),
        Some(80)
    );
    assert!(
        world
            .query("q", "80 .. 20")
            .unwrap_err()
            .message
            .contains("lower bound")
    );
    assert!(world.query("q", "\"a\" .. \"z\"").is_err());
    assert_eq!(
        world.query("q", "(20 .. 20).upper").unwrap().as_int(),
        Some(20)
    );
}

#[test]
fn range_literal_equals_explicit_typed_fields() {
    assert_eq!(
        World::new()
            .query("q", "20 .. 80 = range { lower = 20; upper = 80; }")
            .unwrap()
            .as_bool(),
        Some(true)
    );
}

#[test]
fn generic_range_definitions_can_add_requirements() {
    let mut world = World::new();
    world.load("s", "type 'a range = { lower : 'a; upper : 'a; require self.lower <= self.upper; } object type band = { value : int range; } band b = { value = 20 .. 80; }").unwrap();
    assert_eq!(
        world.query("q", "b.value.upper").unwrap().as_int(),
        Some(80)
    );
}

#[test]
fn detached_contextual_calculations_need_an_owner() {
    let world = example();
    assert!(world.query("q", "(20 .. 80).points").is_err());
}

#[test]
fn derived_cycles_report_dependencies() {
    let mut world = World::new();
    world
        .load(
            "cycles",
            "object type race = { derive a : int = self.b; derive b : int = self.a; } race r = {}",
        )
        .unwrap();
    assert!(
        world
            .query("q", "r.a")
            .unwrap_err()
            .message
            .contains("cyclic dependency")
    );
}

#[test]
fn value_and_alias_cycles_fail() {
    reject("let a = b; let b = a;", "cyclic");
    reject("type a = b type b = a", "cyclic");
}

#[test]
fn malformed_types_and_unknown_names_fail() {
    reject("object type race = { name : strnig; }", "unknown type");
    reject("type 'a box = { x : 'b; }", "unbound type variable");
    reject(
        "object type race = { x : string range; }",
        "range endpoints",
    );
    reject(
        "object type race = { x : int habitability; }",
        "unknown type",
    );
    reject("let x = missing;", "unknown name");
    reject("trait type t = {} race r = {}", "unknown type");
}

#[test]
fn non_exhaustive_and_invalid_matches_fail() {
    reject(
        "type h = | immune | int range object type race = { h : h; derive points : int = match self.h with | immune -> 0; }",
        "non-exhaustive",
    );
    reject(
        "type h = | immune | int range object type race = { h : h; derive points : int = match self.h with | missing -> 0 | _ -> 1; }",
        "not a case",
    );
}

#[test]
fn constructor_variants_support_record_patterns() {
    let mut world = World::new();
    world.load("s", "type h = | immune | band of { lower : int; upper : int; } object type race = { h : h; derive width : int = match self.h with | immune -> 0 | band { lower; upper } -> upper - lower; } race r = { h = band { lower = 20; upper = 80; }; }").unwrap();
    assert_eq!(world.query("q", "r.width").unwrap().as_int(), Some(60));
}

#[test]
fn signed_64_bit_boundaries_and_overflow_are_checked() {
    let world = World::new();
    assert_eq!(
        world.query("q", "-9223372036854775808").unwrap().as_int(),
        Some(i64::MIN)
    );
    assert_eq!(
        world.query("q", "9223372036854775807").unwrap().as_int(),
        Some(i64::MAX)
    );
    for source in [
        "9223372036854775808",
        "9223372036854775807 + 1",
        "-9223372036854775808 / -1",
        "abs (-9223372036854775808)",
        "int 9223372036854775808.0",
    ] {
        assert!(world.query("q", source).is_err(), "accepted {source}");
    }
}

#[test]
fn division_by_zero_and_non_finite_reals_fail() {
    let world = World::new();
    for source in ["1 / 0", "1.0 / 0.0", "1e309", "1e308 * 1e308"] {
        assert!(world.query("q", source).is_err(), "accepted {source}");
    }
}

#[test]
fn real_conversions_and_rounding_are_explicit() {
    let world = World::new();
    assert_eq!(world.query("q", "round 2.5").unwrap().as_int(), Some(3));
    assert_eq!(world.query("q", "round (-2.5)").unwrap().as_int(), Some(-3));
    assert_eq!(
        world.query("q", "real 3 + 0.5").unwrap().as_real(),
        Some(3.5)
    );
    assert!(world.query("q", "1 < 1.5").is_err());
}

#[test]
fn integer_is_an_alias_for_int() {
    let mut world = World::new();
    world.load("s", "object type race = { a : int; b : integer; derive total : integer = self.a + self.b; } race r = { a = 20; b = 80; }").unwrap();
    assert_eq!(world.query("q", "r.total").unwrap().as_int(), Some(100));
}

#[test]
fn utf8_and_multiline_strings_preserve_text() {
    let world = example();
    assert_eq!(
        world.query("q", "terrans.description").unwrap().as_str(),
        Some("An example race design.\n\nUnicode text: café, 星.")
    );
    assert_eq!(
        world.query("q", r#""a\n\"b""#).unwrap().as_str(),
        Some("a\n\"b")
    );
}

#[test]
fn nested_comments_and_query_line_breaks_work() {
    assert_eq!(
        World::new()
            .query("q", "(* outer (* inner *) *) 1 + // comment\n 2")
            .unwrap()
            .as_int(),
        Some(3)
    );
    reject("(* missing end", "unterminated comment");
}

#[test]
fn repl_api_rejects_all_world_writes() {
    let world = example();
    for source in [
        "race other = {}",
        "let count = 3",
        "object type other = {}",
        "trait other : primary_racial_trait = {}",
    ] {
        assert!(
            world
                .query("q", source)
                .unwrap_err()
                .message
                .contains("query-only")
        );
    }
    assert_eq!(
        world.query("q", "(let x = 20 in x + 1)").unwrap().as_int(),
        Some(21)
    );
    assert_eq!(world.object_count(), 3);
}

#[test]
fn diagnostics_name_the_file_line_and_column() {
    let error = World::new()
        .load(
            "facts.rdl",
            "\nobject type race = {\n  population : missing;\n}",
        )
        .unwrap_err();
    assert_eq!(error.span.line, 3);
    assert_eq!(error.span.column, 3);
    assert!(error.to_string().starts_with("facts.rdl:3:3:"));
}

#[test]
fn query_results_and_local_bindings_do_not_assert_entities() {
    let mut world = World::new();
    world.load("s", "object type race = { name : string; } race r = { name = \"R\"; } let names = query { from r : race select r.name }").unwrap();
    assert_eq!(world.object_count(), 1);
    assert_eq!(
        world.query("q", "List.length names").unwrap().as_int(),
        Some(1)
    );
}

#[test]
fn nesting_is_bounded_without_panicking() {
    let text = format!("{}1{}", "(".repeat(200), ")".repeat(200));
    assert!(
        parser::parse("deep", &text)
            .unwrap_err()
            .message
            .contains("nesting limit")
    );
}

#[test]
fn shared_nested_values_use_each_owners_context() {
    let mut world = World::new();
    world
        .load(
            "s",
            r#"
        type h = | immune | int range
        with (owner : race) {
          derive points : int = owner.bonus;
        }
        type settings = { h : h; }
        let standard = settings { h = 20 .. 80; }
        object type race = { bonus : int; settings : settings; }
        race a = { bonus = 10; settings = standard; }
        race b = { bonus = 20; settings = standard; }
    "#,
        )
        .unwrap();
    assert_eq!(
        world.query("q", "a.settings.h.points").unwrap().as_int(),
        Some(10)
    );
    assert_eq!(
        world.query("q", "b.settings.h.points").unwrap().as_int(),
        Some(20)
    );
    assert!(
        world
            .query("q", "standard.h.points")
            .unwrap_err()
            .message
            .contains("owning")
    );
}

#[test]
fn constructor_queries_validate_requirements_before_projecting() {
    let mut world = World::new();
    world
        .load(
            "s",
            "type 'a range = { lower : 'a; upper : 'a; require self.lower >= 0; }",
        )
        .unwrap();
    assert!(
        world
            .query("q", "(-1 .. 3).lower")
            .unwrap_err()
            .message
            .contains("require failed")
    );
    assert!(
        world
            .query("q", "(range { lower = -1; upper = 3; }).lower")
            .is_err()
    );
}

#[test]
fn numeric_comparisons_do_not_silently_lose_integer_precision() {
    let world = World::new();
    assert!(
        world
            .query("q", "9007199254740993 = 9007199254740992.0")
            .is_err()
    );
    assert_eq!(
        world.query("q", "-0.0 = 0.0").unwrap().as_bool(),
        Some(true)
    );
    assert_eq!(
        world.query("q", "-0.0 < 0.0").unwrap().as_bool(),
        Some(false)
    );
}

#[test]
fn top_level_query_validation_is_atomic_even_for_empty_sources() {
    let mut world = World::new();
    assert!(
        world
            .load(
                "s",
                "object type race = {} query { from r : race select r.missing }"
            )
            .is_err()
    );
    assert_eq!(world.type_names().count(), 0);
}

#[test]
fn point_rules_can_sum_selected_trait_properties() {
    let world = example();
    assert_eq!(
        world
            .query("q", "terrans.secondary_points")
            .unwrap()
            .as_int(),
        Some(10)
    );
    assert_eq!(
        world
            .query("q", "settlers.secondary_points")
            .unwrap()
            .as_int(),
        Some(0)
    );
}

#[test]
fn empty_numeric_queries_sum_to_their_typed_zero() {
    let mut world = World::new();
    world
        .load("s", "object type reading = { value : real; }")
        .unwrap();
    assert_eq!(
        world
            .query("q", "List.sum (query { from r : reading select r.value })")
            .unwrap()
            .as_real(),
        Some(0.0)
    );
    assert!(
        world
            .query("q", "List.sum (query { from r : reading select \"bad\" })")
            .is_err()
    );
}

#[test]
fn field_defaults_are_checked_and_can_be_overridden() {
    let mut world = World::new();
    world.load("defaults", "type Input = { value: real = 9.0; derive root: real = sqrt self.value; } object type Result = { input: Input; } Result a={input={};} Result b={input={value=16.0;};}").unwrap();
    assert_eq!(
        world.query("q", "a.input.root").unwrap().as_real(),
        Some(3.0)
    );
    assert_eq!(
        world.query("q", "b.input.root").unwrap().as_real(),
        Some(4.0)
    );
    reject("type Input = { value: integer = true; }", "expected");
    reject("type Input = { value: integer = self.value; }", "unknown");
    assert!(
        world
            .query("q", "sqrt (-1)")
            .unwrap_err()
            .message
            .contains("non-finite")
    );
}

#[test]
fn derived_cache_distinguishes_nominal_types_and_anonymous_values() {
    let mut world = World::new();
    world.load("cache", "type A={x:integer;derive y:integer=self.x+1;} type B={x:integer;derive y:integer=self.x+2;}").unwrap();
    assert_eq!(
        world
            .query("q", "(A{x=2;}).y+(B{x=2;}).y+(A{x=3;}).y")
            .unwrap()
            .as_int(),
        Some(11)
    );
    world
        .load(
            "cyclic",
            "type Cycle={derive x:integer=self.x;} let bad=Cycle{};",
        )
        .unwrap();
    assert!(
        world
            .query("q", "bad.x")
            .unwrap_err()
            .message
            .contains("cyclic")
    );
}
