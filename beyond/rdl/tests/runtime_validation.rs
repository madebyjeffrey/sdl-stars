use rdl::World;

#[test]
fn reused_values_are_validated_in_each_owners_context() {
    let mut world = World::new();
    world
        .load(
            "schema",
            r#"
        type Limit = { value: integer; }
        with (owner: Item) {
            require self.value <= owner.maximum;
            derive remaining: integer = owner.maximum - self.value;
        }
        object type Item = { maximum: integer; limit: Limit; }
        let shared = Limit {value=7;};
        Item a={maximum=10;limit=shared;}
        Item b={maximum=20;limit=shared;}
    "#,
        )
        .unwrap();
    assert_eq!(
        world
            .query("q", "a.limit.remaining + b.limit.remaining")
            .unwrap()
            .as_int(),
        Some(16)
    );
    let error = world
        .load("bad", "Item invalid={maximum=6;limit=shared;}")
        .unwrap_err();
    assert!(error.message.contains("require failed"), "{error}");
    assert_eq!(world.object_count(), 2);
    assert_eq!(
        world.query("q", "a.limit.remaining").unwrap().as_int(),
        Some(3)
    );
}

#[test]
fn composition_rechecks_constraints_after_modification() {
    let mut world = World::new();
    world.load("schema", r#"
        component Capacity { value: integer; require self.value > 0; derive twice: integer=self.value*2; }
        let shared=Capacity {value=10;};
        trait base {provides Capacity=shared}
        trait small {modifies Capacity.value {multiply 0.5}}
        trait invalid {modifies Capacity.value {multiply -1.0}}
        object type Item={requires one Capacity}
        Item a={base}
        Item b={base small}
    "#).unwrap();
    assert_eq!(
        world
            .query("q", "a.Capacity.twice+b.Capacity.twice+shared.twice")
            .unwrap()
            .as_int(),
        Some(50)
    );
    let error = world.load("bad", "Item bad={base invalid}").unwrap_err();
    assert!(error.message.contains("require failed"), "{error}");
    assert_eq!(world.object_count(), 2);
    assert_eq!(
        world.query("q", "b.Capacity.twice").unwrap().as_int(),
        Some(10)
    );
}

#[test]
fn cached_providers_keep_per_object_arguments_and_owner_dependencies() {
    let mut world = World::new();
    world
        .load(
            "schema",
            r#"
        component Factor:integer {}
        component Amount:integer {compose=base*owner.Factor} with (owner:Item)
        trait settings(value:integer,factor:integer) {provides Amount=value provides Factor=factor}
        object type Item={requires one Amount requires one Factor}
        Item a={settings(10,2)}
        Item b={settings(10,3)}
        Item c={settings(20,2)}
    "#,
        )
        .unwrap();
    for _ in 0..2 {
        assert_eq!(
            world
                .query("q", "a.Amount+b.Amount+c.Amount+a.Factor+b.Factor")
                .unwrap()
                .as_int(),
            Some(95)
        );
    }
}

#[test]
fn failed_validation_and_cycles_are_not_accepted_on_reuse() {
    let mut world = World::new();
    world.load("schema", "type Positive={n:integer;require self.n>0;} type Loop={n:integer;derive next:integer=self.next;}").unwrap();
    for _ in 0..2 {
        assert!(
            world
                .query("q", "Positive {n=0;}")
                .unwrap_err()
                .message
                .contains("require failed")
        );
        assert!(
            world
                .query("q", "(Loop {n=0;}).next")
                .unwrap_err()
                .message
                .contains("cyclic")
        );
        assert_eq!(
            world.query("q", "(Positive {n=1;}).n").unwrap().as_int(),
            Some(1)
        );
    }
}

#[test]
fn object_self_keeps_its_fact_identity_when_passed_to_calculations() {
    let mut world = World::new();
    world
        .load(
            "self-reference",
            r#"
        component Capacity: integer {require self > 0}
        trait baseline {provides Capacity=10}
        object type Item={
            value:integer;
            require self.value >= 0;
            requires one Capacity
            derive identity:Item=self;
            derive points:integer=(Pricing {item=self;}).total;
        }
        type Pricing={item:Item;derive total:integer=self.item.value+self.item.Capacity;}
        Item a={value=5;baseline}
        Item b={value=8;baseline}
    "#,
        )
        .unwrap();
    assert_eq!(world.query("q", "a.points").unwrap().as_int(), Some(15));
    assert_eq!(world.query("q", "b.points").unwrap().as_int(), Some(18));
    assert_eq!(
        world.query("q", "a.identity=a").unwrap().as_bool(),
        Some(true)
    );
    assert_eq!(
        world.query("q", "a.identity=b").unwrap().as_bool(),
        Some(false)
    );
    assert!(
        world
            .load("invalid-fact", "Item invalid={value=-1;baseline}")
            .unwrap_err()
            .message
            .contains("require failed")
    );
    world
        .load(
            "cycle",
            "object type Cyclic={derive points:integer=self.points;} Cyclic invalid={}",
        )
        .unwrap();
    assert!(
        world
            .query("q", "invalid.points")
            .unwrap_err()
            .message
            .contains("cyclic")
    );
    assert_eq!(world.query("q", "a.points").unwrap().as_int(), Some(15));
}
