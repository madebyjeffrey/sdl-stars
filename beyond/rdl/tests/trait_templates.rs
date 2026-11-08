use rdl::World;

const SCHEMA: &str = r#"
component Band { lower: integer; upper: integer; require self.lower < self.upper; }
component Capacity: integer { require self > 0 }
component Flag {}
trait band(lower: integer, upper: integer) {
  require lower >= 0 && upper <= 100
  provides Band { lower = lower; upper = upper; }
}
trait capacity(value: integer) { provides Capacity = value }
trait scale(factor: real) { modifies Capacity { multiply factor } }
trait marked { provides Flag }
object type Item = {
  name: string;
  requires one Band
  requires one Capacity
  allows one Flag
}
"#;

fn world() -> World {
    let mut world = World::new();
    world.load("schema", SCHEMA).unwrap();
    world
}

#[test]
fn inline_applications_bind_parameters_and_preserve_named_traits() {
    let mut world = world();
    world
        .load(
            "items",
            r#"Item a = { name = "A"; band(15, 85) capacity(100) scale(0.5) marked }
      Item b = { band(20, 80); name = "B"; capacity(200); }"#,
        )
        .unwrap();
    assert_eq!(world.query("q", "a.Band.lower").unwrap().as_int(), Some(15));
    assert_eq!(world.query("q", "a.Capacity").unwrap().as_int(), Some(50));
    assert_eq!(world.query("q", "b.Capacity").unwrap().as_int(), Some(200));
    assert_eq!(
        world.query("q", "Set.size a.traits").unwrap().as_int(),
        Some(4)
    );
    assert_eq!(
        world
            .query("q", "Set.contains (band(15,85)) a.traits")
            .unwrap()
            .as_bool(),
        Some(true)
    );
    assert_eq!(world.object_count(), 2);
    assert_eq!(world.trait_count(), 1);
    assert_eq!(
        world
            .query("q", "List.length (query { from t:Trait select t })")
            .unwrap()
            .as_int(),
        Some(1)
    );
}

#[test]
fn template_definitions_and_query_values_do_not_assert_global_facts() {
    let world = world();
    assert_eq!(
        world.query("q", "band(15, 85)").unwrap().to_string(),
        "band(15, 85)"
    );
    assert_eq!(world.object_count(), 0);
    assert_eq!(world.trait_count(), 1);
    assert!(
        world
            .query("q", "trait extra { provides Flag }")
            .unwrap_err()
            .message
            .contains("query-only")
    );
}

#[test]
fn wrong_arity_and_types_fail_even_when_no_objects_are_present() {
    let world = world();
    for expr in [
        "band(15)",
        "band()",
        "band(15,85,99)",
        "band(\"low\",85)",
        "band(15.0,85)",
    ] {
        assert!(world.query("bad", expr).is_err(), "{expr}");
    }
    let mut world = world;
    for source in [
        "trait invalid(value: missing) { provides Capacity = 1 }",
        "trait invalid(value: integer, value: integer) {}",
        "trait invalid(self: integer) {}",
        "trait invalid(value: integer) { provides Capacity = false }",
        "trait invalid(value: integer) { require value }",
        "trait invalid(value: integer) { modifies Capacity { multiply false } }",
    ] {
        assert!(world.load("bad", source).is_err(), "{source}");
    }
}

#[test]
fn requirements_validate_arguments_and_components_at_the_application() {
    let world = world();
    for expr in ["band(-1,85)", "band(80,20)", "capacity(0)"] {
        let error = world.query("application", expr).unwrap_err();
        assert_eq!(error.span.source, "application");
        assert!(error.message.contains("require"));
    }
}

#[test]
fn duplicates_are_detected_and_different_instances_still_conflict_as_providers() {
    let mut world = world();
    for choices in [
        "band(15,85) band(15,85) capacity(100)",
        "band(15,85) band(20,80) capacity(100)",
    ] {
        assert!(
            world
                .load("bad", &format!("Item a = {{name=\"A\"; {choices}}}"))
                .is_err()
        );
    }
    assert_eq!(world.object_count(), 0);
}

#[test]
fn legacy_trait_sets_merge_with_inline_choices_and_keep_cardinality() {
    let mut world = world();
    world
        .load(
            "bags",
            r#"
      object type Bag = { traits: Trait set; requires one Capacity }
      Bag a = { traits = { capacity(100); scale(2) }; scale(2.0) }
    "#,
        )
        .unwrap();
    assert_eq!(world.query("q", "a.Capacity").unwrap().as_int(), Some(200));
    assert_eq!(
        world.query("q", "Set.size a.traits").unwrap().as_int(),
        Some(2)
    );
    assert!(
        world
            .load(
                "bounded",
                "object type Single={traits:Trait set[0..1];} Single s={capacity(1) marked}"
            )
            .is_err()
    );
}

#[test]
fn typed_template_metadata_and_owner_context_remain_available() {
    let mut world = World::new();
    world
        .load(
            "owners",
            r#"
      trait type Feature = { value: integer; }
      with (owner: Owner) { derive total: integer = self.value + owner.bonus; }
      trait feature(value: integer): Feature { value = value; }
      object type Owner = { bonus: integer; feature: Feature; }
      Owner a = {bonus=10; feature=feature(2);}
      Owner b = {bonus=20; feature=feature(2);}
    "#,
        )
        .unwrap();
    assert_eq!(
        world.query("q", "a.feature.total").unwrap().as_int(),
        Some(12)
    );
    assert_eq!(
        world.query("q", "b.feature.total").unwrap().as_int(),
        Some(22)
    );
    assert!(world.query("q", "feature(2).total").is_err());
}

#[test]
fn named_values_can_reuse_template_applications_and_templates_may_be_aliased() {
    let mut world = world();
    world.load("aliases", "let apply_band = band; let broad_band = apply_band(15,85); Item a = {name=\"A\"; broad_band capacity(100)}").unwrap();
    assert_eq!(world.query("q", "a.Band.upper").unwrap().as_int(), Some(85));
    assert_eq!(
        world
            .query("q", "Set.contains (band(15,85)) a.traits")
            .unwrap()
            .as_bool(),
        Some(true)
    );
}

#[test]
fn recursive_template_instantiation_is_bounded() {
    let mut world = World::new();
    world
        .load(
            "recursive",
            "trait type Recursive={next:Trait;} trait loop(n:integer):Recursive {next=loop(n);}",
        )
        .unwrap();
    assert!(
        world
            .query("q", "loop(1)")
            .unwrap_err()
            .message
            .contains("cyclic")
    );
}

#[test]
fn explicit_argument_lists_preserve_spaced_ml_application_precedence() {
    let mut world = world();
    world.load("values", "type Box={value:integer;}").unwrap();
    assert_eq!(
        world
            .query("q", "real (Box {value=4;}).value")
            .unwrap()
            .as_real(),
        Some(4.0)
    );
    assert_eq!(world.query("q", "min(3,4)").unwrap().as_int(), Some(3));
    assert_eq!(world.query("q", "sqrt(16)").unwrap().as_real(), Some(4.0));
    assert_eq!(
        world.query("q", "band (15, 85)").unwrap().to_string(),
        "band(15, 85)"
    );
}

#[test]
fn instance_identity_retains_nominal_arguments_and_normalizes_primitive_coercions() {
    let mut world = world();
    world
        .load(
            "identity",
            r#"
      abstract component Number: integer {}
      component A: Number {}
      component B: Number {}
      trait type Tagged={value:Number;}
      trait tag(value:Number):Tagged {value=value;}
      let n:integer=2;
    "#,
        )
        .unwrap();
    assert_eq!(
        world.query("q", "tag(A 1)=tag(B 1)").unwrap().as_bool(),
        Some(false)
    );
    assert_eq!(
        world.query("q", "(tag(A 1)).value").unwrap().as_int(),
        Some(1)
    );
    assert_eq!(
        world.query("q", "scale(n)=scale(2.0)").unwrap().as_bool(),
        Some(true)
    );
}

#[test]
fn scalar_arguments_and_signed_zero_have_value_identity() {
    let mut world = world();
    world
        .load("aliases", "component Count:integer {} let alias=Count 2;")
        .unwrap();
    assert_eq!(
        world
            .query("q", "scale(alias)=scale(2.0)")
            .unwrap()
            .as_bool(),
        Some(true)
    );
    assert_eq!(
        world
            .query("q", "scale(-0.0)=scale(0.0)")
            .unwrap()
            .as_bool(),
        Some(true)
    );
}

#[test]
fn explicit_derived_traits_property_is_not_shadowed_by_the_implicit_bag() {
    let mut world = world();
    world.load("derived", "object type Derived={ derive traits:string=\"description\"; allows one Flag } Derived d={marked}").unwrap();
    assert_eq!(
        world.query("q", "d.traits").unwrap().as_str(),
        Some("description")
    );
}

const OVERLOADS: &str = r#"
type Immunity = | immune
trait type Choice = { label: string; }
component Habitat { lower: integer; upper: integer; }
trait gravity(mode: Immunity): Choice {
  label = "immune";
  provides Habitat { lower = -1; upper = -1; }
}
trait gravity(lower: integer, upper: integer): Choice {
  label = "bounded";
  require lower >= 0 && upper <= 100 && lower < upper
  provides Habitat { lower = lower; upper = upper; }
}
object type Race = { requires one Habitat }
"#;

#[test]
fn trait_overloads_dispatch_by_arity_and_compose_the_selected_definition() {
    let mut world = World::new();
    world.load("schema", OVERLOADS).unwrap();
    world.load("facts", "Race a={gravity(immune)} Race b={gravity(15,85)} let habitat=gravity; Race c={habitat(immune)}").unwrap();
    for (query, expected) in [
        ("a.Habitat.lower", -1),
        ("b.Habitat.lower", 15),
        ("b.Habitat.upper", 85),
        ("c.Habitat.upper", -1),
    ] {
        assert_eq!(world.query("q", query).unwrap().as_int(), Some(expected));
    }
    assert_eq!(
        world
            .query("q", "gravity(immune).label")
            .unwrap()
            .to_string(),
        "\"immune\""
    );
    assert_eq!(
        world
            .query("q", "gravity(15,85).label")
            .unwrap()
            .to_string(),
        "\"bounded\""
    );
    assert_eq!(
        world
            .query("q", "Set.size {gravity(immune); gravity(15,85)}")
            .unwrap()
            .as_int(),
        Some(2)
    );
    assert_eq!(
        world
            .query("q", "Set.contains (gravity(immune)) a.traits")
            .unwrap()
            .as_bool(),
        Some(true)
    );
    assert_eq!(world.trait_count(), 0);
}

#[test]
fn overloads_are_checked_independently_and_reject_ambiguous_or_missing_signatures() {
    let mut world = World::new();
    world.load("schema", OVERLOADS).unwrap();
    for query in [
        "gravity(1)",
        "gravity(immune,85)",
        "gravity(80,20)",
        "gravity(15.0,85)",
    ] {
        assert!(world.query("q", query).is_err(), "{query}");
    }
    for query in ["gravity()", "gravity(1,2,3)"] {
        let message = world.query("q", query).unwrap_err().message;
        assert!(
            message.contains("gravity/1") && message.contains("gravity/2"),
            "{message}"
        );
    }
    assert!(
        world
            .query("q", "gravity immune")
            .unwrap_err()
            .message
            .contains("explicit argument list")
    );
    for source in [
        "trait gravity(value:integer) {}", // Same arity, even with a different parameter type.
        "trait gravity {}",
        "let gravity=1;",
        "trait gravity(a:integer,b:integer,c:integer) { provides Habitat {lower=false;upper=0;} }",
    ] {
        assert!(world.load("bad", source).is_err(), "{source}");
    }
    // A failed extension must not publish part of a template family.
    assert!(world.query("q", "gravity(1,2,3)").is_err());
    assert!(world.query("q", "gravity(immune)").is_ok());
}

#[test]
fn overloads_support_zero_arity_distinct_metadata_and_cross_source_forward_references() {
    let mut world = World::new();
    world
        .load_sources(&[
            (
                "facts",
                "let empty_value=choice(); let single_value=choice(3);",
            ),
            (
                "one",
                "trait type Number={number:integer;} trait choice(n:integer):Number {number=n;}",
            ),
            (
                "zero",
                "trait type Label={label:string;} trait choice():Label {label=\"empty\";}",
            ),
        ])
        .unwrap();
    assert_eq!(
        world.query("q", "single_value.number").unwrap().as_int(),
        Some(3)
    );
    assert_eq!(
        world.query("q", "empty_value.label").unwrap().to_string(),
        "\"empty\""
    );
    assert!(world.query("q", "choice().number").is_err());
    assert!(world.query("q", "choice(3).label").is_err());
    let mut named = World::new();
    named.load("named", "trait choice {}").unwrap();
    assert!(
        named
            .load("conflict", "trait choice(n:integer) {}")
            .is_err()
    );
}
