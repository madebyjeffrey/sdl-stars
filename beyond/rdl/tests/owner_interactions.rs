use rdl::World;

#[test]
fn nested_objects_validate_their_own_self_and_keep_the_containing_owner() {
    let mut world = World::new();
    world
        .load(
            "schema",
            r#"
        type Limit={n:int;} with(owner:Node){require self.n<=owner.n;}
        object type Node={
            n:int;child:Node option;limit:Limit option;
            require self.n>0;
            derive twice:int=self.n*2;
            derive identity:Node=self;
        }
        Node root={n=10;child={n=7;limit={n=9;};};}
    "#,
        )
        .unwrap();
    for _ in 0..2 {
        assert_eq!(
            world
                .query(
                    "q",
                    "(let c=root.child in match c with |Node -> c.twice |none -> 0)"
                )
                .unwrap()
                .as_int(),
            Some(14)
        );
        assert_eq!(
            world.query("q", "root.identity=root").unwrap().as_bool(),
            Some(true)
        );
    }
    for source in [
        "Node bad={n=1;child={n=-1;};}",
        "Node bad={n=1;child={n=7;limit={n=2;};};}",
    ] {
        let error = world.load("bad", source).unwrap_err();
        assert!(error.message.contains("require failed"), "{error}");
        assert_eq!(world.object_count(), 1);
    }
}

const FEATURES: &str = r#"
trait type Feature={n:int;} with (owner:O) {derive total:int=self.n+owner.bonus;}
trait feature(n:int):Feature {n=n;}
trait type Result={n:int;}
trait sample(f:Feature):Result {n=f.total;}
trait guarded(f:Feature):Result {require f.total<20; n=f.total;}
type 'a Holder={value:'a;}
trait wrapped(h:(Feature Holder)):Result {n=h.value.total;}
object type O={bonus:int;f:Feature;h:(Feature Holder);}
O a={bonus=10;f=feature(2);h={value=feature(2);};}
O b={bonus=20;f=feature(2);h={value=feature(2);};}
"#;

#[test]
fn trait_evaluation_cache_distinguishes_argument_owners() {
    let mut world = World::new();
    world.load("schema", FEATURES).unwrap();
    for expr in [
        "{a=sample(a.f).n;b=sample(b.f).n;}",
        "{b=sample(b.f).n;a=sample(a.f).n;}",
        "{a=wrapped(a.h).n;b=wrapped(b.h).n;}",
    ] {
        let value = world.query("q", expr).unwrap();
        assert_eq!(value.fields().unwrap()["a"].as_int(), Some(12));
        assert_eq!(value.fields().unwrap()["b"].as_int(), Some(22));
    }
    // A retained result must not make a later request reuse the other owner's result.
    world.load("warm", "let cached=sample(a.f);").unwrap();
    assert_eq!(
        world.query("q", "sample(b.f).n").unwrap().as_int(),
        Some(22)
    );
    let error = world
        .query("q", "{a=guarded(a.f);b=guarded(b.f);}")
        .unwrap_err();
    assert!(
        error.message.contains("trait requirement failed"),
        "{error}"
    );
    assert_eq!(
        world.query("q", "guarded(a.f).n").unwrap().as_int(),
        Some(12)
    );
    assert_eq!(world.query("q", "a.f=b.f").unwrap().as_bool(), Some(true));
}
