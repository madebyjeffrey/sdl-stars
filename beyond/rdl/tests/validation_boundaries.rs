use rdl::World;

#[test]
fn template_providers_validate_the_completed_composition() {
    let mut world = World::new();
    world
        .load(
            "schema",
            r#"
            component Scalar:int {require self>0}
            component Record {n:int;require self.n>0;}
            trait named {provides Scalar=0 provides Record {n=0}}
            trait template(n:int) {provides Scalar=n provides Record {n=n}}
            trait plus {modifies Scalar {add 1} modifies Record.n {add 1}}
            object type O={requires one Scalar requires one Record}
            O a={named plus}
            O b={template(0) plus}
            "#,
        )
        .unwrap();
    assert_eq!(
        world.query("q", "a.Scalar=b.Scalar").unwrap().as_bool(),
        Some(true)
    );
    assert_eq!(
        world.query("q", "a.Record.n=b.Record.n").unwrap().as_bool(),
        Some(true)
    );
    for choices in ["named", "template(0)", "template(-1) plus"] {
        let error = world
            .load("bad", &format!("O invalid={{{choices}}}"))
            .unwrap_err();
        assert!(error.message.contains("require failed"), "{error}");
        assert_eq!(world.object_count(), 2);
    }
}

#[test]
fn trait_arguments_validate_structural_values_before_caching() {
    let mut world = World::new();
    world
        .load(
            "schema",
            "type Positive={n:int;require self.n>0;} trait take(p:Positive) {}",
        )
        .unwrap();
    for _ in 0..2 {
        let error = world.query("q", "take({n=-1;})").unwrap_err();
        assert!(error.message.contains("require failed"), "{error}");
        assert!(world.query("q", "take({n=1;})").is_ok());
    }
    let error = world.load("invalid", "let bad=take({n=-1;});").unwrap_err();
    assert!(error.message.contains("require failed"), "{error}");
    assert!(world.query("q", "bad").is_err());
}

#[test]
fn variant_payloads_validate_before_projection() {
    let mut world = World::new();
    world
        .load(
            "schema",
            "type Positive={n:int;require self.n>0;} type Wrapped=|wrap of {p:Positive;}",
        )
        .unwrap();
    for _ in 0..2 {
        let error = world.query("q", "(wrap {p={n=-1;};}).p.n").unwrap_err();
        assert!(error.message.contains("require failed"), "{error}");
        assert_eq!(
            world.query("q", "(wrap {p={n=1;};}).p.n").unwrap().as_int(),
            Some(1)
        );
    }
}
