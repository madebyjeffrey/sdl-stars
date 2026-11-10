use rdl::World;

#[test]
fn direct_constructors_validate_the_enclosing_union() {
    let mut world = World::new();
    world
        .load(
            "schema",
            r#"
        type 'a Box=|box of {value:'a;} with {require false;}
        type Checked=|checked of {n:int;} with {require match self with |checked {n}->n>0;}
    "#,
        )
        .unwrap();
    for expr in ["(box {value=1;}).value", "(checked {n=0;}).n"] {
        for _ in 0..2 {
            let error = world.query("q", expr).unwrap_err();
            assert!(error.message.contains("require failed"), "{error}");
        }
    }
    assert_eq!(
        world.query("q", "(checked {n=1;}).n").unwrap().as_int(),
        Some(1)
    );
}

#[test]
fn variant_results_compose_with_generic_records_and_keep_case_fields() {
    let mut world = World::new();
    world
        .load(
            "schema",
            r#"
        type 'a Box=|empty|box of {value:'a;}
        type 'a Holder={value:'a;}
        let h:((int Box) Holder)=Holder {value=box {value=7;};};
        let s=Holder {value=box {value="seven";};};
        let nested=box {value=box {value=8;};};
        object type O={}
    "#,
        )
        .unwrap();
    assert_eq!(
        world
            .query("q", "match h.value with |empty->0 |box {value}->value")
            .unwrap()
            .as_int(),
        Some(7)
    );
    assert_eq!(
        world
            .query("q", "match s.value with |empty->\"\" |box {value}->value")
            .unwrap()
            .as_str(),
        Some("seven")
    );
    assert_eq!(
        world
            .query("q", "match nested.value with |empty->0 |box {value}->value")
            .unwrap()
            .as_int(),
        Some(8)
    );
    assert_eq!(
        world
            .query(
                "q",
                "(let b=box {value=2.0;} in List.sum (query {from o:O select b.value}))"
            )
            .unwrap()
            .as_real(),
        Some(0.0)
    );
}

#[test]
fn inferred_union_arguments_preserve_owner_validation_and_template_context() {
    let mut world = World::new();
    world
        .load(
            "schema",
            r#"
        type 'a Box=|box of {value:'a;}
        with(owner:O){require match self with |box {value}->value<=owner.maximum;derive limit:int=owner.maximum;}
        type 'a Holder={value:'a;}
        trait type Result={n:int;}
        trait sample(h:((int Box) Holder)):Result {n=h.value.limit;}
        trait keep(h:((int Box) Holder)) {}
        object type O={maximum:int;h:((int Box) Holder);}
        O a={maximum=10;h=Holder {value=box {value=1;};};}
        O b={maximum=20;h=Holder {value=box {value=1;};};}
    "#,
        )
        .unwrap();
    let value = world
        .query("q", "{a=sample(a.h).n;b=sample(b.h).n;}")
        .unwrap();
    assert_eq!(value.fields().unwrap()["a"].as_int(), Some(10));
    assert_eq!(value.fields().unwrap()["b"].as_int(), Some(20));
    let error = world
        .load(
            "bad",
            "O bad={maximum=0;h=Holder {value=box {value=0;};};keep(Holder {value=box {value=1;};})}",
        )
        .unwrap_err();
    assert!(error.message.contains("require failed"), "{error}");
    assert_eq!(world.object_count(), 2);
}
