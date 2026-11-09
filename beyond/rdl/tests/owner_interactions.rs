use rdl::World;

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
