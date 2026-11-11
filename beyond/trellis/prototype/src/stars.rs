//! Small reference-capacity/growth examples, not a complete Stars! ruleset.
use crate::*;
use std::collections::{BTreeMap, BTreeSet};

pub const RACE: EntityTypeId = EntityTypeId(1);
pub const NAME: AttributeId = AttributeId(1);
pub const CAPACITY: AttributeId = AttributeId(2);
pub const SELECTED_GROWTH: AttributeId = AttributeId(3);
pub const GROWTH_RATE: AttributeId = AttributeId(4);
pub const EFFECTIVE_GROWTH: AttributeId = AttributeId(5);
pub const POPULATION: SlotId = SlotId(1);
pub const PRIMARY: SlotId = SlotId(2);
pub const GROWTH: SlotId = SlotId(3);
pub const LESSER: SlotId = SlotId(4);
pub const FAMILY_POPULATION: FamilyId = FamilyId(1);
pub const FAMILY_PRIMARY: FamilyId = FamilyId(2);
pub const FAMILY_GROWTH: FamilyId = FamilyId(3);
pub const FAMILY_LESSER: FamilyId = FamilyId(4);
pub const STANDARD: PartId = PartId(1);
pub const HYPER_EXPANSION: PartId = PartId(2);
pub const JACK_OF_ALL_TRADES: PartId = PartId(3);
pub const GROWTH_SETTING: PartId = PartId(4);
pub const OBRM: PartId = PartId(5);
pub const PERCENT: ParameterId = ParameterId(1);

pub fn definitions() -> Definitions {
    let mut definitions = Definitions::default();
    for (id, sz_name, value_type, source, check) in [
        (
            NAME,
            "name",
            ValueType::Text,
            AttributeSource::Input,
            Some(nonempty as CheckValue),
        ),
        (
            CAPACITY,
            "carrying_capacity",
            ValueType::Integer,
            AttributeSource::Numeric,
            Some(positive_capacity as CheckValue),
        ),
        (
            SELECTED_GROWTH,
            "selected_growth_percent",
            ValueType::Integer,
            AttributeSource::Numeric,
            None,
        ),
        (
            GROWTH_RATE,
            "growth_rate",
            ValueType::Real,
            AttributeSource::Numeric,
            Some(valid_rate as CheckValue),
        ),
        (
            EFFECTIVE_GROWTH,
            "effective_growth_percent",
            ValueType::Integer,
            AttributeSource::Derived(effective_growth),
            None,
        ),
    ] {
        definitions.rg_attributes.insert(
            id,
            AttributeDefinition {
                sz_name: sz_name.into(),
                value_type,
                source,
                check,
            },
        );
    }
    for (id, sz_name, id_family, expand) in [
        (
            STANDARD,
            "standard_capacity",
            FAMILY_POPULATION,
            standard as ExpandPart,
        ),
        (
            HYPER_EXPANSION,
            "hyper_expansion",
            FAMILY_PRIMARY,
            hyper as ExpandPart,
        ),
        (
            JACK_OF_ALL_TRADES,
            "jack_of_all_trades",
            FAMILY_PRIMARY,
            joat as ExpandPart,
        ),
        (
            GROWTH_SETTING,
            "growth",
            FAMILY_GROWTH,
            growth as ExpandPart,
        ),
        (
            OBRM,
            "only_basic_remote_mining",
            FAMILY_LESSER,
            obrm as ExpandPart,
        ),
    ] {
        definitions.rg_parts.insert(
            id,
            PartDefinition {
                sz_name: sz_name.into(),
                id_family,
                rg_parameters: if id == GROWTH_SETTING {
                    BTreeMap::from([(
                        PERCENT,
                        ParameterDefinition {
                            sz_name: "percent".into(),
                            value_type: ValueType::Integer,
                        },
                    )])
                } else {
                    BTreeMap::new()
                },
                check: if id == GROWTH_SETTING {
                    Some(check_growth)
                } else {
                    None
                },
                expand,
            },
        );
    }
    let rg_slots = [
        (POPULATION, "population", FAMILY_POPULATION, 1, Some(1)),
        (PRIMARY, "primary_trait", FAMILY_PRIMARY, 1, Some(1)),
        (GROWTH, "growth", FAMILY_GROWTH, 1, Some(1)),
        (LESSER, "lesser_traits", FAMILY_LESSER, 0, Some(14)),
    ]
    .into_iter()
    .map(|(id, sz_name, id_family, c_min, c_max)| {
        (
            id,
            SlotDefinition {
                sz_name: sz_name.into(),
                id_family,
                c_min,
                c_max,
            },
        )
    })
    .collect();
    definitions.rg_entities.insert(
        RACE,
        EntityDefinition {
            sz_name: "Race".into(),
            rg_slots,
            rg_attributes: BTreeSet::from([
                NAME,
                CAPACITY,
                SELECTED_GROWTH,
                GROWTH_RATE,
                EFFECTIVE_GROWTH,
            ]),
        },
    );
    definitions
}

pub fn race(id: EntityId, scope: Scope, id_primary: PartId, n_growth: i64, f_obrm: bool) -> Entity {
    Entity::new(id, RACE, scope)
        .input(NAME, Value::Text(format!("Race {}", id.0)))
        .parts(POPULATION, vec![PartInstance::new(STANDARD)])
        .parts(PRIMARY, vec![PartInstance::new(id_primary)])
        .parts(
            GROWTH,
            vec![PartInstance::new(GROWTH_SETTING).argument(PERCENT, Value::Integer(n_growth))],
        )
        .parts(
            LESSER,
            if f_obrm {
                vec![PartInstance::new(OBRM)]
            } else {
                Vec::new()
            },
        )
}

fn nonempty(value: &Value) -> Result<(), String> {
    match value {
        Value::Text(sz_value) if !sz_value.trim().is_empty() => Ok(()),
        _ => Err("name must not be empty".into()),
    }
}

fn positive_capacity(value: &Value) -> Result<(), String> {
    match value {
        Value::Integer(n_value) if *n_value > 0 => Ok(()),
        _ => Err("capacity must be positive".into()),
    }
}

fn valid_rate(value: &Value) -> Result<(), String> {
    match value {
        Value::Real(r_value) if *r_value > 0.0 && *r_value <= 1.0 => Ok(()),
        _ => Err("growth rate must be above zero and at most one".into()),
    }
}

fn check_growth(part: &PartInstance) -> Result<(), String> {
    if (1..=20).contains(&part.integer(PERCENT)?) {
        Ok(())
    } else {
        Err("selected growth must be 1 through 20 percent".into())
    }
}

fn standard(_: &PartInstance) -> Result<Vec<Contribution>, String> {
    Ok(vec![Contribution::new(
        CAPACITY,
        Operation::Base,
        Value::Integer(1_000_000),
    )])
}

fn hyper(_: &PartInstance) -> Result<Vec<Contribution>, String> {
    Ok(vec![
        Contribution::new(CAPACITY, Operation::Multiply, Value::Real(0.5)),
        Contribution::new(GROWTH_RATE, Operation::Multiply, Value::Real(2.0)),
    ])
}

fn joat(_: &PartInstance) -> Result<Vec<Contribution>, String> {
    Ok(vec![Contribution::new(
        CAPACITY,
        Operation::Multiply,
        Value::Real(1.2),
    )])
}

fn obrm(_: &PartInstance) -> Result<Vec<Contribution>, String> {
    Ok(vec![Contribution::new(
        CAPACITY,
        Operation::Multiply,
        Value::Real(1.1),
    )])
}

fn growth(part: &PartInstance) -> Result<Vec<Contribution>, String> {
    let n_percent = part.integer(PERCENT)?;
    Ok(vec![
        Contribution::new(SELECTED_GROWTH, Operation::Base, Value::Integer(n_percent)),
        Contribution::new(
            GROWTH_RATE,
            Operation::Base,
            Value::Real(n_percent as f64 / 100.0),
        ),
    ])
}

fn effective_growth(inputs: &mut dyn Inputs, id: EntityId) -> Result<Value, EvalError> {
    let Value::Real(r_rate) = inputs.get(id, GROWTH_RATE)? else {
        return Err(EvalError::Invalid("growth rate must be real".into()));
    };
    // The dependency's validated 0 < rate <= 1 bounds this conversion to 0..100.
    let value = Value::Integer((r_rate * 100.0).round() as i64);
    inputs.note("fraction x 100, rounded to percent", value.clone());
    Ok(value)
}
