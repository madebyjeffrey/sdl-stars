use super::*;

fn count(world: &World) -> usize {
    world.evaluation_cache.lock().unwrap().derived_evaluations
}

fn simple_world() -> World {
    let mut world = World::new();
    world
        .load(
            "schema",
            "object type Item={n:integer;derive points:integer=self.n*2;} Item a={n=5;}",
        )
        .unwrap();
    world
}

#[test]
fn successful_properties_evaluate_once_per_world_snapshot() {
    let mut world = simple_world();
    assert_eq!(count(&world), 0);
    for _ in 0..3 {
        assert_eq!(world.query("q", "a.points").unwrap().as_int(), Some(10));
    }
    assert_eq!(count(&world), 1);
    assert!(world.load("failed", "Item invalid={n=\"bad\";}").is_err());
    assert_eq!(world.query("q", "a.points").unwrap().as_int(), Some(10));
    assert_eq!(count(&world), 1);
    world.load("extension", "Item b={n=7;}").unwrap();
    assert_eq!(count(&world), 0);
    assert_eq!(world.query("q", "a.points").unwrap().as_int(), Some(10));
    assert_eq!(count(&world), 1);
}

#[test]
fn constraint_dependencies_are_eager_and_seed_the_same_cache() {
    let mut world = World::new();
    world.load("constraints", "object type Item={n:integer;derive checked:integer=self.n*2;require self.checked>=0;derive unused:integer=1/0;} Item a={n=2;}").unwrap();
    assert_eq!(count(&world), 1);
    assert_eq!(world.query("q", "a.checked").unwrap().as_int(), Some(4));
    assert_eq!(count(&world), 1);
    assert!(world.query("q", "a.unused").is_err());
    assert_eq!(count(&world), 2);
    assert!(world.query("q", "a.unused").is_err());
    assert_eq!(count(&world), 3);
    assert_eq!(world.query("q", "a.checked").unwrap().as_int(), Some(4));
    assert_eq!(count(&world), 3);
}

#[test]
fn temporary_calculations_are_not_retained_between_requests() {
    let mut world = World::new();
    world
        .load(
            "schema",
            "type Box={n:integer;derive twice:integer=self.n*2;}",
        )
        .unwrap();
    for n in 1..20 {
        assert_eq!(
            world
                .query("q", &format!("(Box {{n={n};}}).twice"))
                .unwrap()
                .as_int(),
            Some(n * 2)
        );
        assert!(
            world
                .evaluation_cache
                .lock()
                .unwrap()
                .derived_values
                .is_empty()
        );
    }
}

#[test]
fn simultaneous_readers_share_one_memoized_result() {
    let world = Arc::new(simple_world());
    let threads: Vec<_> = (0..4)
        .map(|_| {
            let world = Arc::clone(&world);
            std::thread::spawn(move || world.query("q", "a.points").unwrap().as_int())
        })
        .collect();
    for thread in threads {
        assert_eq!(thread.join().unwrap(), Some(10));
    }
    assert_eq!(count(&world), 1);
}

#[test]
fn results_forced_during_construction_keep_their_nested_memos() {
    let mut world = World::new();
    world.load("schema", "type Child={n:integer;derive points:integer=self.n*2;} type Parent={n:integer;derive child:Child=Child{n=self.n;};require self.child.points>0;} let parent=Parent{n=3;};").unwrap();
    assert_eq!(count(&world), 2);
    for _ in 0..3 {
        assert_eq!(
            world.query("q", "parent.child.points").unwrap().as_int(),
            Some(6)
        );
    }
    assert_eq!(count(&world), 2);
}
