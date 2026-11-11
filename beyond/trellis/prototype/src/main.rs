use trellis_prototype::{stars::*, *};

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let mut knowledge = KnowledgeBase::new(definitions()).map_err(violations)?;
    let context = Context::global();
    let mut transaction = knowledge.transaction();
    transaction.put(race(
        EntityId(1),
        Scope::Global,
        JACK_OF_ALL_TRADES,
        15,
        false,
    ));
    transaction.commit().map_err(violations)?;

    let mut settlers = knowledge.view(&context).entity(EntityId(1))?.clone();
    settlers.id = EntityId(2);
    settlers
        .rg_inputs
        .insert(NAME, Value::Text("Settlers".into()));
    settlers
        .rg_slots
        .insert(PRIMARY, vec![PartInstance::new(HYPER_EXPANSION)]);
    settlers.rg_slots.insert(
        GROWTH,
        vec![PartInstance::new(GROWTH_SETTING).argument(PERCENT, Value::Integer(18))],
    );
    settlers
        .rg_slots
        .insert(LESSER, vec![PartInstance::new(OBRM)]);
    let mut transaction = knowledge.transaction();
    transaction.put(settlers.clone());
    {
        let mut evaluator = Evaluator::new(transaction.view(&context).map_err(violations)?);
        println!(
            "Preview, before commit:\n{}",
            evaluator.explain(EntityId(2), CAPACITY)?
        );
    }
    transaction.commit().map_err(violations)?;
    let mut evaluator = Evaluator::new(knowledge.view(&context));
    println!("{}", evaluator.explain(EntityId(2), EFFECTIVE_GROWTH)?);
    println!(
        "Original default:\n{}",
        evaluator.explain(EntityId(1), CAPACITY)?
    );
    drop(evaluator);

    settlers.rg_slots.insert(
        GROWTH,
        vec![PartInstance::new(GROWTH_SETTING).argument(PERCENT, Value::Integer(21))],
    );
    let mut transaction = knowledge.transaction();
    transaction.put(settlers);
    match transaction.commit() {
        Ok(()) => return Err("invalid growth unexpectedly committed".into()),
        Err(rg_errors) => println!("Rejected invalid edit:\n{}", violations(rg_errors)),
    }
    println!(
        "Current growth after rejection: {}",
        Evaluator::new(knowledge.view(&context))
            .evaluate(EntityId(2), SELECTED_GROWTH)?
            .value
    );
    Ok(())
}

fn violations(rg_errors: Vec<Violation>) -> std::io::Error {
    std::io::Error::other(
        rg_errors
            .iter()
            .map(ToString::to_string)
            .collect::<Vec<_>>()
            .join("\n"),
    )
}
