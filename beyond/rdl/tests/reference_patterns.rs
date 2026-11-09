use rdl::World;

#[test]
fn direct_and_query_references_match_their_declared_type() {
    let mut world = World::new();
    world
        .load(
            "schema",
            "object type O={n:int;} O a={n=7;} let optional:O option=a;",
        )
        .unwrap();
    for expr in [
        "match a with | O {n} -> n",
        "match a with | O {n} -> n | _ -> 999",
        "match optional with | O {n} -> n | none -> 999",
    ] {
        assert_eq!(world.query("q", expr).unwrap().as_int(), Some(7), "{expr}");
    }
    let rows = world
        .query("q", "query {from x:O select match x with | O {n} -> n}")
        .unwrap();
    assert_eq!(rows.items().unwrap()[0].as_int(), Some(7));
}

#[test]
fn trait_references_match_the_builtin_trait_category() {
    let mut world = World::new();
    world
        .load("schema", "trait type Named={n:int;} trait t:Named={n=7;}")
        .unwrap();
    assert_eq!(
        world
            .query("q", "match t with | Named {n} -> n")
            .unwrap()
            .as_int(),
        Some(7)
    );
    let rows = world
        .query(
            "q",
            "query {from x:Trait select match x with | Trait -> true | _ -> false}",
        )
        .unwrap();
    assert_eq!(rows.items().unwrap()[0].as_bool(), Some(true));
}
