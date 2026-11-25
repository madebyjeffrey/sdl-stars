use rdl::World;

#[test]
fn unused_derived_values_are_lazy_but_stored_constraints_still_run() {
    let mut world = World::new();
    world.load("lazy", r#"
        object type Item={n:integer;require self.n>=0;derive broken:integer=1/0;derive cycle:integer=self.cycle;derive points:integer=self.n*2;}
        Item a={n=5;}
    "#).unwrap();
    assert_eq!(world.query("q", "a.n").unwrap().as_int(), Some(5));
    assert_eq!(world.query("q", "a.points").unwrap().as_int(), Some(10));
    for _ in 0..2 {
        assert!(
            world
                .query("q", "a.broken")
                .unwrap_err()
                .message
                .contains("division by zero")
        );
        assert!(
            world
                .query("q", "a.cycle")
                .unwrap_err()
                .message
                .contains("cyclic")
        );
    }
    assert!(
        world
            .load("invalid", "Item b={n=-1;}")
            .unwrap_err()
            .message
            .contains("require failed")
    );
    assert_eq!(world.object_count(), 1);
    assert_eq!(world.query("q", "a.points").unwrap().as_int(), Some(10));
}

#[test]
fn constraints_force_derived_dependencies_and_validate_derived_results() {
    let mut world = World::new();
    world.load("schema",r#"
        type Positive={n:integer;require self.n>0;}
        object type Item={n:integer;derive doubled:integer=self.n*2;require self.doubled>=0;derive bad:Positive=Positive{n=-1;};}
        Item a={n=2;}
    "#).unwrap();
    assert_eq!(world.query("q", "a.doubled").unwrap().as_int(), Some(4));
    assert!(
        world
            .query("q", "a.bad")
            .unwrap_err()
            .message
            .contains("require failed")
    );
    assert!(
        world
            .load("bad", "Item b={n=-2;}")
            .unwrap_err()
            .message
            .contains("require failed")
    );
    assert!(
        world
            .load(
                "type-error",
                "object type Wrong={derive unused:integer=\"wrong\";}"
            )
            .is_err()
    );
}

#[test]
fn extending_a_world_invalidates_query_dependent_values_and_keeps_clones_isolated() {
    let mut world = World::new();
    world.load("schema", "object type Item={derive count:integer=List.length (query {from x:Item select x});} Item a={}").unwrap();
    assert_eq!(world.query("q", "a.count").unwrap().as_int(), Some(1));
    let original = world.clone();
    world.load("extension", "Item b={}").unwrap();
    assert_eq!(world.query("q", "a.count").unwrap().as_int(), Some(2));
    assert_eq!(original.query("q", "a.count").unwrap().as_int(), Some(1));
    assert_eq!(world.query("q", "b.count").unwrap().as_int(), Some(2));
}
