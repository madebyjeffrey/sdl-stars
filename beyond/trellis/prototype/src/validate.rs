use crate::*;

pub(crate) fn definitions(definitions: &Definitions) -> Vec<Violation> {
    let mut rg_violations = Vec::new();
    for definition in definitions.rg_attributes.values() {
        if matches!(definition.source, AttributeSource::Numeric)
            && !matches!(definition.value_type, ValueType::Integer | ValueType::Real)
        {
            rg_violations.push(Violation::new(
                &definition.sz_name,
                "numeric composition requires a numeric attribute",
            ));
        }
    }
    for definition in definitions.rg_entities.values() {
        for id in &definition.rg_attributes {
            if !definitions.rg_attributes.contains_key(id) {
                rg_violations.push(Violation::new(
                    &definition.sz_name,
                    format!("unknown attribute {}", id.0),
                ));
            }
        }
        for slot in definition.rg_slots.values() {
            if slot.c_max.is_some_and(|c_max| c_max < slot.c_min) {
                rg_violations.push(Violation::new(
                    &slot.sz_name,
                    "maximum cardinality is below minimum",
                ));
            }
        }
    }
    rg_violations
}

fn value(value: &Value, value_type: ValueType) -> Result<(), String> {
    if value.value_type() != value_type {
        return Err(format!(
            "expected {value_type:?}, got {:?}",
            value.value_type()
        ));
    }
    if !value.finite() {
        return Err("real values must be finite".into());
    }
    Ok(())
}

pub(crate) fn entity(definitions: &Definitions, entity: &Entity) -> Vec<Violation> {
    let mut rg_violations = Vec::new();
    let mut rg_selected: Vec<&PartInstance> = Vec::new();
    let mut rg_bases = BTreeMap::<AttributeId, usize>::new();
    let sz_entity = format!("entity {}", entity.id.0);
    let Some(definition) = definitions.rg_entities.get(&entity.id_type) else {
        return vec![Violation::new(sz_entity, "unknown entity type")];
    };

    for (id, input) in &entity.rg_inputs {
        let Some(attribute) = definitions
            .rg_attributes
            .get(id)
            .filter(|_| definition.rg_attributes.contains(id))
        else {
            rg_violations.push(Violation::new(
                &sz_entity,
                format!("unknown input attribute {}", id.0),
            ));
            continue;
        };
        let sz_path = format!("{sz_entity}.{}", attribute.sz_name);
        if !matches!(attribute.source, AttributeSource::Input) {
            rg_violations.push(Violation::new(
                sz_path,
                "calculated attributes cannot be assigned",
            ));
        } else if let Err(sz_error) = value(input, attribute.value_type)
            .and_then(|()| attribute.check.map_or(Ok(()), |check| check(input)))
        {
            rg_violations.push(Violation::new(sz_path, sz_error));
        }
    }
    for id in &definition.rg_attributes {
        let attribute = &definitions.rg_attributes[id];
        if matches!(attribute.source, AttributeSource::Input) && !entity.rg_inputs.contains_key(id)
        {
            rg_violations.push(Violation::new(
                format!("{sz_entity}.{}", attribute.sz_name),
                "missing required input",
            ));
        }
    }
    for (id, slot) in &definition.rg_slots {
        let c_parts = entity.rg_slots.get(id).map_or(0, Vec::len);
        if c_parts < slot.c_min || slot.c_max.is_some_and(|c_max| c_parts > c_max) {
            rg_violations.push(Violation::new(
                format!("{sz_entity}.{}", slot.sz_name),
                format!(
                    "slot requires {}..{:?} parts, got {c_parts}",
                    slot.c_min, slot.c_max
                ),
            ));
        }
    }
    for (id_slot, rg_parts) in &entity.rg_slots {
        let Some(slot) = definition.rg_slots.get(id_slot) else {
            rg_violations.push(Violation::new(
                &sz_entity,
                format!("unknown slot {}", id_slot.0),
            ));
            continue;
        };
        for part in rg_parts {
            let sz_path = format!("{sz_entity}.{}", slot.sz_name);
            let Some(part_definition) = definitions.rg_parts.get(&part.id) else {
                rg_violations.push(Violation::new(
                    sz_path,
                    format!("unknown part {}", part.id.0),
                ));
                continue;
            };
            let sz_path = format!("{sz_path}.{}", part_definition.sz_name);
            let c_before = rg_violations.len();
            if part_definition.id_family != slot.id_family {
                rg_violations.push(Violation::new(
                    &sz_path,
                    "part family is incompatible with slot",
                ));
            }
            if rg_selected
                .iter()
                .any(|other| part.canonical_cmp(other).is_eq())
            {
                rg_violations.push(Violation::new(
                    &sz_path,
                    "part instance selected more than once",
                ));
            }
            rg_selected.push(part);
            for (id, argument) in &part.rg_arguments {
                if let Some(parameter) = part_definition.rg_parameters.get(id) {
                    if let Err(sz_error) = value(argument, parameter.value_type) {
                        rg_violations.push(Violation::new(
                            format!("{sz_path}.{}", parameter.sz_name),
                            sz_error,
                        ));
                    }
                } else {
                    rg_violations.push(Violation::new(
                        &sz_path,
                        format!("unknown parameter {}", id.0),
                    ));
                }
            }
            for (id, parameter) in &part_definition.rg_parameters {
                if !part.rg_arguments.contains_key(id) {
                    rg_violations.push(Violation::new(
                        format!("{sz_path}.{}", parameter.sz_name),
                        "missing parameter",
                    ));
                }
            }
            if rg_violations.len() != c_before {
                continue;
            }
            if let Some(check) = part_definition.check
                && let Err(sz_error) = check(part)
            {
                rg_violations.push(Violation::new(&sz_path, sz_error));
                continue;
            }
            let rg_effects = match (part_definition.expand)(part) {
                Ok(rg_effects) => rg_effects,
                Err(sz_error) => {
                    rg_violations.push(Violation::new(&sz_path, sz_error));
                    continue;
                }
            };
            for effect in rg_effects {
                let Some(attribute) = definitions
                    .rg_attributes
                    .get(&effect.id_attribute)
                    .filter(|_| definition.rg_attributes.contains(&effect.id_attribute))
                else {
                    rg_violations.push(Violation::new(
                        &sz_path,
                        "contribution targets an unknown attribute",
                    ));
                    continue;
                };
                if !matches!(attribute.source, AttributeSource::Numeric) {
                    rg_violations.push(Violation::new(
                        &sz_path,
                        "contribution requires a numeric-composed attribute",
                    ));
                    continue;
                }
                let value_type = if effect.operation == Operation::Multiply {
                    ValueType::Real
                } else {
                    attribute.value_type
                };
                if let Err(sz_error) = value(&effect.value, value_type) {
                    rg_violations.push(Violation::new(&sz_path, sz_error));
                }
                if effect.operation == Operation::Base {
                    *rg_bases.entry(effect.id_attribute).or_default() += 1;
                }
            }
        }
    }
    for id in &definition.rg_attributes {
        let attribute = &definitions.rg_attributes[id];
        if matches!(attribute.source, AttributeSource::Numeric) {
            let c_bases = rg_bases.get(id).copied().unwrap_or(0);
            if c_bases != 1 {
                rg_violations.push(Violation::new(
                    format!("{sz_entity}.{}", attribute.sz_name),
                    format!("expected one base provider, got {c_bases}"),
                ));
            }
        }
    }
    rg_violations
}
