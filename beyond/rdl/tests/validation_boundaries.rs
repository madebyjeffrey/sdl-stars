use rdl::World;

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
    let error = world
        .load("invalid", "let bad=take({n=-1;});")
        .unwrap_err();
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
