use std::collections::BTreeMap;
use std::sync::Arc;
use trellis_prototype::{stars::*, *};

fn knowledge(definitions: Definitions, rg_entities: Vec<Entity>) -> KnowledgeBase {
    let mut knowledge = KnowledgeBase::new(definitions).expect("valid definitions");
    let mut transaction = knowledge.transaction();
    for entity in rg_entities {
        transaction.put(entity);
    }
    transaction.commit().unwrap();
    knowledge
}

fn default_race() -> Entity {
    race(EntityId(1), Scope::Global, JACK_OF_ALL_TRADES, 15, false)
}

#[test]
fn reference_growth_and_capacity_have_sourced_explanations() {
    let knowledge = knowledge(
        definitions(),
        vec![
            default_race(),
            race(EntityId(2), Scope::Global, HYPER_EXPANSION, 18, true),
        ],
    );
    let context = Context::global();
    let mut evaluator = Evaluator::new(knowledge.view(&context));
    assert_eq!(
        evaluator.evaluate(EntityId(1), CAPACITY).unwrap().value,
        Value::Integer(1_200_000)
    );
    assert_eq!(
        evaluator.evaluate(EntityId(2), CAPACITY).unwrap().value,
        Value::Integer(550_000)
    );
    assert_eq!(
        evaluator
            .evaluate(EntityId(2), SELECTED_GROWTH)
            .unwrap()
            .value,
        Value::Integer(18)
    );
    let effective = evaluator.evaluate(EntityId(2), EFFECTIVE_GROWTH).unwrap();
    assert_eq!(effective.value, Value::Integer(36));
    let Explanation::Derived {
        rg_dependencies,
        rg_steps,
    } = &effective.explanation
    else {
        panic!("derived trace")
    };
    assert_eq!(rg_dependencies.len(), 1);
    assert_eq!(rg_dependencies[0].value, Value::Real(0.36));
    assert_eq!(rg_steps[0].value, Value::Integer(36));
    let growth = evaluator.evaluate(EntityId(2), GROWTH_RATE).unwrap();
    assert!(Arc::ptr_eq(&growth, &rg_dependencies[0]));
    assert!(Arc::ptr_eq(
        &effective,
        &evaluator.evaluate(EntityId(2), EFFECTIVE_GROWTH).unwrap()
    ));
    let sz_trace = effective.render();
    assert!(sz_trace.contains("growth(percent=18) [growth]"));
    assert!(sz_trace.contains("hyper_expansion() [primary_trait]"));
    assert!(sz_trace.contains("fraction x 100, rounded to percent = 36"));
    let sz_capacity = evaluator.explain(EntityId(2), CAPACITY).unwrap();
    assert!(sz_capacity.contains("only_basic_remote_mining() [lesser_traits]"));
    assert!(sz_capacity.contains("round once: half away from zero"));
}

#[test]
fn contexts_filter_enumeration_direct_reads_and_rule_dependencies() {
    let mut definitions = definitions();
    let id_hidden_read = AttributeId(20);
    definitions.rg_attributes.insert(
        id_hidden_read,
        AttributeDefinition {
            sz_name: "hidden_read".into(),
            value_type: ValueType::Integer,
            source: AttributeSource::Derived(hidden_read),
            check: None,
        },
    );
    definitions
        .rg_entities
        .get_mut(&RACE)
        .unwrap()
        .rg_attributes
        .insert(id_hidden_read);
    let knowledge = knowledge(
        definitions,
        vec![
            default_race(),
            race(
                EntityId(2),
                Scope::Game(GameId(10)),
                HYPER_EXPANSION,
                18,
                true,
            ),
            race(
                EntityId(3),
                Scope::Game(GameId(20)),
                JACK_OF_ALL_TRADES,
                15,
                false,
            ),
        ],
    );
    let global = Context::global();
    let game = Context::for_game(GameId(10));
    assert_eq!(knowledge.view(&global).entity_ids(), vec![EntityId(1)]);
    assert_eq!(
        knowledge.view(&game).entity_ids(),
        vec![EntityId(1), EntityId(2)]
    );
    let mut evaluator = Evaluator::new(knowledge.view(&game));
    assert_eq!(
        evaluator.evaluate(EntityId(3), CAPACITY).unwrap_err(),
        EvalError::NotVisible(EntityId(3))
    );
    assert_eq!(
        evaluator.evaluate(EntityId(1), id_hidden_read).unwrap_err(),
        EvalError::NotVisible(EntityId(3))
    );
    let restricted = game
        .restrict_to([EntityId(2)])
        .restrict_to([EntityId(1), EntityId(2)]);
    assert_eq!(knowledge.view(&restricted).entity_ids(), vec![EntityId(2)]);
}

fn hidden_read(inputs: &mut dyn Inputs, _: EntityId) -> Result<Value, EvalError> {
    inputs.get(EntityId(3), CAPACITY)
}

#[test]
fn invalid_batches_and_dropped_previews_do_not_publish_changes() {
    let mut knowledge = knowledge(definitions(), vec![default_race()]);
    let context = Context::global();
    let original = knowledge
        .view(&context)
        .entity(EntityId(1))
        .unwrap()
        .clone();
    let mut changed = original.clone();
    changed
        .rg_inputs
        .insert(NAME, Value::Text("Changed".into()));
    let mut transaction = knowledge.transaction();
    transaction.put(changed);
    transaction.put(race(EntityId(2), Scope::Global, HYPER_EXPANSION, 21, false));
    assert!(transaction.view(&context).is_err());
    let rg_errors = transaction.commit().unwrap_err();
    assert!(
        rg_errors
            .iter()
            .any(|error| error.sz_path.contains("growth")
                && error.sz_message.contains("1 through 20"))
    );
    assert_eq!(
        knowledge.view(&context).entity(EntityId(1)).unwrap(),
        &original
    );
    assert!(knowledge.view(&context).entity(EntityId(2)).is_err());
    {
        let mut transaction = knowledge.transaction();
        transaction.put(race(EntityId(2), Scope::Global, HYPER_EXPANSION, 18, false));
        let mut evaluator = Evaluator::new(transaction.view(&context).unwrap());
        assert_eq!(
            evaluator
                .evaluate(EntityId(2), EFFECTIVE_GROWTH)
                .unwrap()
                .value,
            Value::Integer(36)
        );
    }
    assert!(knowledge.view(&context).entity(EntityId(2)).is_err());
}

#[test]
fn invalid_parameters_slots_and_calculated_assignments_are_rejected() {
    let mut rg_cases = Vec::new();
    let mut entity = default_race();
    entity.rg_slots.get_mut(&GROWTH).unwrap()[0]
        .rg_arguments
        .clear();
    rg_cases.push((entity, "missing parameter"));
    let mut entity = default_race();
    entity.rg_slots.get_mut(&GROWTH).unwrap()[0]
        .rg_arguments
        .insert(PERCENT, Value::Real(15.0));
    rg_cases.push((entity, "expected Integer"));
    let mut entity = default_race();
    entity.rg_slots.remove(&PRIMARY);
    rg_cases.push((entity, "slot requires"));
    let mut entity = default_race();
    entity
        .rg_slots
        .insert(PRIMARY, vec![PartInstance::new(OBRM)]);
    rg_cases.push((entity, "incompatible"));
    let mut entity = default_race();
    entity.rg_slots.insert(
        LESSER,
        vec![PartInstance::new(OBRM), PartInstance::new(OBRM)],
    );
    rg_cases.push((entity, "more than once"));
    let mut entity = default_race();
    entity.rg_inputs.insert(CAPACITY, Value::Integer(7));
    rg_cases.push((entity, "cannot be assigned"));
    let mut entity = default_race();
    entity.rg_inputs.insert(NAME, Value::Text(String::new()));
    rg_cases.push((entity, "name must not be empty"));
    let mut entity = default_race();
    entity.rg_slots.insert(SlotId(99), Vec::new());
    rg_cases.push((entity, "unknown slot"));
    for (entity, sz_expected) in rg_cases {
        let mut knowledge = KnowledgeBase::new(definitions()).unwrap();
        let mut transaction = knowledge.transaction();
        transaction.put(entity);
        assert!(
            transaction
                .commit()
                .unwrap_err()
                .iter()
                .any(|error| error.sz_message.contains(sz_expected)),
            "{sz_expected}"
        );
    }
}

fn extra_base(_: &PartInstance) -> Result<Vec<Contribution>, String> {
    Ok(vec![Contribution::new(
        CAPACITY,
        Operation::Base,
        Value::Integer(7),
    )])
}

fn infinite_factor(_: &PartInstance) -> Result<Vec<Contribution>, String> {
    Ok(vec![Contribution::new(
        CAPACITY,
        Operation::Multiply,
        Value::Real(f64::INFINITY),
    )])
}

#[test]
fn conflicting_providers_and_nonfinite_contributions_are_rejected() {
    for (expand, sz_expected) in [
        (extra_base as ExpandPart, "got 2"),
        (infinite_factor as ExpandPart, "finite"),
    ] {
        let mut definitions = definitions();
        definitions.rg_parts.get_mut(&OBRM).unwrap().expand = expand;
        let mut knowledge = KnowledgeBase::new(definitions).unwrap();
        let mut transaction = knowledge.transaction();
        transaction.put(race(EntityId(1), Scope::Global, HYPER_EXPANSION, 18, true));
        assert!(
            transaction
                .commit()
                .unwrap_err()
                .iter()
                .any(|error| error.sz_message.contains(sz_expected))
        );
    }
}

fn five(_: &PartInstance) -> Result<Vec<Contribution>, String> {
    Ok(vec![Contribution::new(
        CAPACITY,
        Operation::Base,
        Value::Integer(5),
    )])
}

fn half(_: &PartInstance) -> Result<Vec<Contribution>, String> {
    Ok(vec![Contribution::new(
        CAPACITY,
        Operation::Multiply,
        Value::Real(0.5),
    )])
}

#[test]
fn integer_composition_rounds_once_after_the_product() {
    let mut definitions = definitions();
    definitions.rg_parts.get_mut(&STANDARD).unwrap().expand = five;
    definitions.rg_parts.get_mut(&OBRM).unwrap().expand = half;
    let knowledge = knowledge(
        definitions,
        vec![race(EntityId(1), Scope::Global, HYPER_EXPANSION, 18, true)],
    );
    let context = Context::global();
    // 5 * (0.5 * 0.5) = 1.25 -> 1; rounding between factors would produce 2.
    assert_eq!(
        Evaluator::new(knowledge.view(&context))
            .evaluate(EntityId(1), CAPACITY)
            .unwrap()
            .value,
        Value::Integer(1)
    );
}

fn maximum(_: &PartInstance) -> Result<Vec<Contribution>, String> {
    Ok(vec![Contribution::new(
        CAPACITY,
        Operation::Base,
        Value::Integer(i64::MAX),
    )])
}

fn add_one(_: &PartInstance) -> Result<Vec<Contribution>, String> {
    Ok(vec![Contribution::new(
        CAPACITY,
        Operation::Add,
        Value::Integer(1),
    )])
}

#[test]
fn arithmetic_overflow_is_reported_without_saturating_casts() {
    let mut definitions = definitions();
    definitions.rg_parts.get_mut(&STANDARD).unwrap().expand = maximum;
    definitions.rg_parts.get_mut(&OBRM).unwrap().expand = add_one;
    let knowledge = knowledge(
        definitions,
        vec![
            race(EntityId(1), Scope::Global, JACK_OF_ALL_TRADES, 15, true),
            race(EntityId(2), Scope::Global, JACK_OF_ALL_TRADES, 15, false),
        ],
    );
    let context = Context::global();
    let mut evaluator = Evaluator::new(knowledge.view(&context));
    assert!(matches!(
        evaluator.evaluate(EntityId(1), CAPACITY),
        Err(EvalError::Arithmetic(_))
    ));
    assert!(matches!(
        evaluator.evaluate(EntityId(2), CAPACITY),
        Err(EvalError::Arithmetic(_))
    ));
}

#[test]
fn numeric_results_and_traces_are_independent_of_selection_order() {
    let mut definitions = definitions();
    let id_extra = PartId(10);
    let extra = definitions.rg_parts[&OBRM].clone();
    definitions.rg_parts.insert(
        id_extra,
        PartDefinition {
            sz_name: "extra_factor".into(),
            ..extra
        },
    );
    let a = race(EntityId(1), Scope::Global, HYPER_EXPANSION, 18, true).parts(
        LESSER,
        vec![PartInstance::new(OBRM), PartInstance::new(id_extra)],
    );
    let b = race(EntityId(2), Scope::Global, HYPER_EXPANSION, 18, true).parts(
        LESSER,
        vec![PartInstance::new(id_extra), PartInstance::new(OBRM)],
    );
    let knowledge = knowledge(definitions, vec![a, b]);
    let context = Context::global();
    let mut evaluator = Evaluator::new(knowledge.view(&context));
    let a = evaluator.evaluate(EntityId(1), CAPACITY).unwrap();
    let b = evaluator.evaluate(EntityId(2), CAPACITY).unwrap();
    assert_eq!(a.value, b.value);
    assert_eq!(a.explanation, b.explanation);
}

const CYCLE_A: AttributeId = AttributeId(30);
const CYCLE_B: AttributeId = AttributeId(31);
fn cycle_a(inputs: &mut dyn Inputs, id: EntityId) -> Result<Value, EvalError> {
    inputs.get(id, CYCLE_B)
}
fn cycle_b(inputs: &mut dyn Inputs, id: EntityId) -> Result<Value, EvalError> {
    inputs.get(id, CYCLE_A)
}
fn wrong_type(_: &mut dyn Inputs, _: EntityId) -> Result<Value, EvalError> {
    Ok(Value::Text("wrong".into()))
}

#[test]
fn failed_derived_rules_leave_the_dependency_stack_usable() {
    let mut definitions = definitions();
    for (id, rule) in [
        (CYCLE_A, cycle_a as DerivedRule),
        (CYCLE_B, cycle_b as DerivedRule),
        (AttributeId(32), wrong_type as DerivedRule),
    ] {
        definitions.rg_attributes.insert(
            id,
            AttributeDefinition {
                sz_name: format!("derived{}", id.0),
                value_type: ValueType::Integer,
                source: AttributeSource::Derived(rule),
                check: None,
            },
        );
        definitions
            .rg_entities
            .get_mut(&RACE)
            .unwrap()
            .rg_attributes
            .insert(id);
    }
    let knowledge = knowledge(definitions, vec![default_race()]);
    let context = Context::global();
    let mut evaluator = Evaluator::new(knowledge.view(&context));
    for _ in 0..2 {
        assert_eq!(
            evaluator.evaluate(EntityId(1), CYCLE_A).unwrap_err(),
            EvalError::Cycle(vec![
                (EntityId(1), CYCLE_A),
                (EntityId(1), CYCLE_B),
                (EntityId(1), CYCLE_A)
            ])
        );
        assert!(matches!(
            evaluator.evaluate(EntityId(1), AttributeId(32)),
            Err(EvalError::Invalid(_))
        ));
        assert_eq!(
            evaluator.evaluate(EntityId(1), CAPACITY).unwrap().value,
            Value::Integer(1_200_000)
        );
    }
}

#[test]
fn a_new_session_reads_the_committed_edit_instead_of_an_old_memo() {
    let mut knowledge = knowledge(definitions(), vec![default_race()]);
    let context = Context::global();
    let previous = Evaluator::new(knowledge.view(&context))
        .evaluate(EntityId(1), EFFECTIVE_GROWTH)
        .unwrap();
    let mut changed = knowledge
        .view(&context)
        .entity(EntityId(1))
        .unwrap()
        .clone();
    changed.rg_slots.insert(
        GROWTH,
        vec![PartInstance::new(GROWTH_SETTING).argument(PERCENT, Value::Integer(20))],
    );
    let mut transaction = knowledge.transaction();
    transaction.put(changed);
    transaction.commit().unwrap();
    let current = Evaluator::new(knowledge.view(&context))
        .evaluate(EntityId(1), EFFECTIVE_GROWTH)
        .unwrap();
    assert_eq!(previous.value, Value::Integer(15));
    assert_eq!(current.value, Value::Integer(20));
    assert!(current.render().contains("growth(percent=20)"));
    assert!(previous.render().contains("growth(percent=15)"));
}

#[test]
fn invalid_definition_contracts_are_rejected_before_loading_entities() {
    let mut definitions = definitions();
    definitions
        .rg_attributes
        .get_mut(&CAPACITY)
        .unwrap()
        .value_type = ValueType::Text;
    definitions
        .rg_entities
        .get_mut(&RACE)
        .unwrap()
        .rg_slots
        .get_mut(&PRIMARY)
        .unwrap()
        .c_max = Some(0);
    assert_eq!(KnowledgeBase::new(definitions).err().unwrap().len(), 2);
}

fn negative_five(_: &PartInstance) -> Result<Vec<Contribution>, String> {
    Ok(vec![Contribution::new(
        CAPACITY,
        Operation::Base,
        Value::Integer(-5),
    )])
}

#[test]
fn integer_half_ties_round_away_from_zero() {
    for (expand, n_expected) in [(five as ExpandPart, 3), (negative_five as ExpandPart, -3)] {
        let mut definitions = definitions();
        definitions.rg_parts.get_mut(&STANDARD).unwrap().expand = expand;
        definitions.rg_attributes.get_mut(&CAPACITY).unwrap().check = None;
        let knowledge = knowledge(
            definitions,
            vec![race(EntityId(1), Scope::Global, HYPER_EXPANSION, 18, false)],
        );
        let context = Context::global();
        assert_eq!(
            Evaluator::new(knowledge.view(&context))
                .evaluate(EntityId(1), CAPACITY)
                .unwrap()
                .value,
            Value::Integer(n_expected)
        );
    }
}

fn huge_factor(_: &PartInstance) -> Result<Vec<Contribution>, String> {
    Ok(vec![Contribution::new(
        CAPACITY,
        Operation::Multiply,
        Value::Real(1.0e308),
    )])
}

#[test]
fn finite_inputs_that_overflow_during_multiplication_fail_evaluation() {
    let mut definitions = definitions();
    definitions
        .rg_parts
        .get_mut(&HYPER_EXPANSION)
        .unwrap()
        .expand = huge_factor;
    definitions.rg_parts.get_mut(&OBRM).unwrap().expand = huge_factor;
    let knowledge = knowledge(
        definitions,
        vec![race(EntityId(1), Scope::Global, HYPER_EXPANSION, 18, true)],
    );
    let context = Context::global();
    assert!(matches!(
        Evaluator::new(knowledge.view(&context)).evaluate(EntityId(1), CAPACITY),
        Err(EvalError::Arithmetic(_))
    ));
}

#[test]
fn part_instance_identity_preserves_real_argument_bits() {
    let a = PartInstance::new(PartId(9)).argument(ParameterId(9), Value::Real(0.0));
    let b = PartInstance::new(PartId(9)).argument(ParameterId(9), Value::Real(-0.0));
    assert_ne!(a, b);
    assert_eq!(a, a.clone());
}

#[test]
fn unknown_entity_types_inputs_parts_and_parameters_are_rejected() {
    let mut rg_cases = Vec::new();
    let mut entity = default_race();
    entity.id_type = EntityTypeId(999);
    rg_cases.push(entity);
    let mut entity = default_race();
    entity.rg_inputs = BTreeMap::from([(AttributeId(999), Value::Integer(1))]);
    rg_cases.push(entity);
    let mut entity = default_race();
    entity
        .rg_slots
        .insert(PRIMARY, vec![PartInstance::new(PartId(999))]);
    rg_cases.push(entity);
    let mut entity = default_race();
    entity.rg_slots.get_mut(&GROWTH).unwrap()[0]
        .rg_arguments
        .insert(ParameterId(999), Value::Integer(1));
    rg_cases.push(entity);
    for entity in rg_cases {
        let mut knowledge = KnowledgeBase::new(definitions()).unwrap();
        let mut transaction = knowledge.transaction();
        transaction.put(entity);
        assert!(transaction.commit().is_err());
    }
}
