use rdl::World;

#[test]
fn partial_functions_preserve_empty_aggregate_types() {
    let mut world = World::new();
    world.load("schema", "object type O={}").unwrap();
    for populate in [false, true] {
        if populate {
            world.load("fact", "O a={}").unwrap();
        }
        for filter in ["", "where false"] {
            for (aggregate, identity) in [("sum", 0.0), ("product", 1.0)] {
                let expr = format!(
                    "let f=min 2.0 in List.{aggregate} (query {{from x:O {filter} select f 3.0}})"
                );
                let expected = if populate && filter.is_empty() {
                    2.0
                } else {
                    identity
                };
                assert_eq!(
                    world.query("q", &format!("({expr})")).unwrap().as_real(),
                    Some(expected)
                );
                assert_eq!(
                    world
                        .query("q", &format!("({expr}) = {expected:?}"))
                        .unwrap()
                        .as_bool(),
                    Some(true)
                );
                let expr = expr.replace("2.0", "2").replace("3.0", "3");
                assert_eq!(
                    world.query("q", &format!("({expr})")).unwrap().as_int(),
                    Some(expected as i64)
                );
            }
        }
    }
}
