use rdl::World;

#[test]
fn generic_variants_infer_substitute_and_match_payload_types() {
    let mut world = World::new();
    world
        .load(
            "schema",
            r#"
        type 'a Box=|empty|box of {value:'a;}
        let i:(int Box)=box {value=7;};
        let s:(string Box)=box {value="seven";};
        let r:(real Box)=box {value=7.5;};
        let e:(int Box)=empty;
    "#,
        )
        .unwrap();
    assert_eq!(
        world
            .query("q", "match i with | empty -> 0 | box {value} -> value")
            .unwrap()
            .as_int(),
        Some(7)
    );
    assert_eq!(
        world
            .query("q", "match s with | empty -> \"\" | box {value} -> value")
            .unwrap()
            .as_str(),
        Some("seven")
    );
    assert_eq!(
        world
            .query("q", "match r with | empty -> 0.0 | box {value} -> value")
            .unwrap()
            .as_real(),
        Some(7.5)
    );
    assert_eq!(
        world
            .query("q", "match e with | empty -> 0 | box {value} -> value")
            .unwrap()
            .as_int(),
        Some(0)
    );
    assert_eq!(
        world
            .query("q", "(box {value=9;}).value + 1")
            .unwrap()
            .as_int(),
        Some(10)
    );
    assert_eq!(
        world
            .query("q", "match box {value=9;} with | box {value} -> value + 1")
            .unwrap()
            .as_int(),
        Some(10)
    );
}

#[test]
fn generic_variants_reject_wrong_and_inconsistent_type_arguments_statically() {
    let mut world = World::new();
    world
        .load(
            "schema",
            "type 'a Box=|box of {value:'a;} type 'a Pair=|pair of {a:'a;b:'a;}",
        )
        .unwrap();
    for source in [
        "let bad:(int Box)=box {value=\"bad\";};",
        "object type O={derive bad:(int Box)=box {value=\"bad\";};}",
        "let bad=pair {a=1;b=\"bad\";};",
        "object type O={derive bad:(int Pair)=pair {a=1;b=\"bad\";};}",
    ] {
        assert!(world.load("bad", source).is_err(), "{source}");
    }
}

#[test]
fn generic_variant_payloads_keep_constraints() {
    let mut world = World::new();
    world
        .load(
            "schema",
            "type Positive={n:int;require self.n>0;} type 'a Box=|box of {value:'a;}",
        )
        .unwrap();
    assert_eq!(
        world
            .query("q", "(box {value=Positive {n=1;};}).value.n")
            .unwrap()
            .as_int(),
        Some(1)
    );
    let error = world
        .query("q", "box {value=Positive {n=0;};}")
        .unwrap_err();
    assert!(error.message.contains("require failed"), "{error}");
}
