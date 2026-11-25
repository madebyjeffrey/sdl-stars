use rdl::World;

#[test]
fn constructors_in_generic_rules_keep_the_enclosing_parameter_scope() {
    let mut world = World::new();
    world
        .load(
            "schema",
            r#"
        type 'a Box={value:'a;}
        type 'a Holder={value:'a;derive wrapped:('a Box)=Box {value=self.value;};}
        let i=Holder {value=7;};
        let s=Holder {value="seven";};
        let r=Holder {value=7.5;};
    "#,
        )
        .unwrap();
    assert_eq!(
        world.query("q", "i.wrapped.value").unwrap().as_int(),
        Some(7)
    );
    assert_eq!(
        world.query("q", "s.wrapped.value").unwrap().as_str(),
        Some("seven")
    );
    assert_eq!(
        world.query("q", "r.wrapped.value").unwrap().as_real(),
        Some(7.5)
    );
    assert!(world.load("bad", "type Bad={value:'a;}").is_err());
}

#[test]
fn generic_rule_results_validate_in_their_owner_and_remain_distinct_in_templates() {
    let mut world = World::new();
    world.load("schema", r#"
        type 'a Box={value:'a;} with(owner:O){require owner.maximum>0;derive limit:int=owner.maximum;}
        type 'a Holder={value:'a;derive wrapped:('a Box)=Box {value=self.value;};}
        trait type Result={n:int;}
        trait sample(h:(int Holder)):Result {n=h.wrapped.limit;}
        object type O={maximum:int;h:(int Holder);}
        O a={maximum=10;h={value=7;};}
        O b={maximum=20;h={value=7;};}
        O bad={maximum=0;h={value=7;};}
    "#).unwrap();
    for _ in 0..2 {
        let result = world
            .query("q", "{a=sample(a.h).n;b=sample(b.h).n;}")
            .unwrap();
        assert_eq!(result.fields().unwrap()["a"].as_int(), Some(10));
        assert_eq!(result.fields().unwrap()["b"].as_int(), Some(20));
        let error = world.query("q", "sample(bad.h)").unwrap_err();
        assert!(error.message.contains("require failed"), "{error}");
    }
}
