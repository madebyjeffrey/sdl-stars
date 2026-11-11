use std::collections::BTreeMap;
use std::fmt::Write;
use std::sync::Arc;

use crate::*;

#[derive(Clone, Debug, PartialEq)]
pub struct AppliedContribution {
    pub id_attribute: AttributeId,
    pub id_slot: SlotId,
    pub id_part: PartId,
    pub sz_origin: String,
    pub operation: Operation,
    pub value: Value,
}

#[derive(Clone, Debug, PartialEq)]
pub struct Step {
    pub sz_description: String,
    pub value: Value,
}

#[derive(Clone, Debug, PartialEq)]
pub enum Explanation {
    Input,
    Numeric {
        rg_terms: Vec<AppliedContribution>,
        f_rounded: bool,
    },
    Derived {
        rg_dependencies: Vec<Arc<Evaluation>>,
        rg_steps: Vec<Step>,
    },
}

#[derive(Clone, Debug, PartialEq)]
pub struct Evaluation {
    pub id_entity: EntityId,
    pub id_attribute: AttributeId,
    pub sz_name: String,
    pub value: Value,
    pub explanation: Explanation,
}

impl Evaluation {
    pub fn render(&self) -> String {
        let mut sz_output = String::new();
        self.render_at(&mut sz_output, 0);
        sz_output
    }

    fn render_at(&self, sz_output: &mut String, c_indent: usize) {
        let sz_indent = " ".repeat(c_indent);
        writeln!(sz_output, "{sz_indent}{} = {}", self.sz_name, self.value).unwrap();
        match &self.explanation {
            Explanation::Input => writeln!(
                sz_output,
                "{sz_indent}  stored input on entity {}",
                self.id_entity.0
            )
            .unwrap(),
            Explanation::Numeric {
                rg_terms,
                f_rounded,
            } => {
                for operation in [Operation::Base, Operation::Add, Operation::Multiply] {
                    for term in rg_terms.iter().filter(|term| term.operation == operation) {
                        writeln!(
                            sz_output,
                            "{sz_indent}  {operation:?} {} from {}",
                            term.value, term.sz_origin
                        )
                        .unwrap();
                    }
                }
                if *f_rounded {
                    writeln!(sz_output, "{sz_indent}  round once: half away from zero").unwrap();
                }
            }
            Explanation::Derived {
                rg_dependencies,
                rg_steps,
            } => {
                for dependency in rg_dependencies {
                    dependency.render_at(sz_output, c_indent + 2);
                }
                for step in rg_steps {
                    writeln!(
                        sz_output,
                        "{sz_indent}  {} = {}",
                        step.sz_description, step.value
                    )
                    .unwrap();
                }
            }
        }
    }
}

#[derive(Default)]
struct Frame {
    rg_dependencies: Vec<Arc<Evaluation>>,
    rg_steps: Vec<Step>,
}

/// One read-only evaluation session. Its memo is never reused against edited data.
pub struct Evaluator<'a> {
    view: View<'a>,
    rg_memo: BTreeMap<(EntityId, AttributeId), Arc<Evaluation>>,
    rg_contributions: BTreeMap<EntityId, Vec<AppliedContribution>>,
    rg_active: Vec<(EntityId, AttributeId)>,
    rg_frames: Vec<Frame>,
}

impl<'a> Evaluator<'a> {
    pub fn new(view: View<'a>) -> Self {
        Self {
            view,
            rg_memo: BTreeMap::new(),
            rg_contributions: BTreeMap::new(),
            rg_active: Vec::new(),
            rg_frames: Vec::new(),
        }
    }

    pub fn evaluate(
        &mut self,
        id_entity: EntityId,
        id_attribute: AttributeId,
    ) -> Result<Arc<Evaluation>, EvalError> {
        let entity = self.view.entity(id_entity)?;
        let definitions = self.view.definitions();
        let definition = &definitions.rg_entities[&entity.id_type];
        if !definition.rg_attributes.contains(&id_attribute) {
            return Err(EvalError::UnknownAttribute(id_attribute));
        }
        let attribute = definitions.rg_attributes[&id_attribute].clone();
        let key = (id_entity, id_attribute);
        if let Some(evaluation) = self.rg_memo.get(&key) {
            return Ok(Arc::clone(evaluation));
        }
        if let Some(i_start) = self.rg_active.iter().position(|active| *active == key) {
            let mut rg_path = self.rg_active[i_start..].to_vec();
            rg_path.push(key);
            return Err(EvalError::Cycle(rg_path));
        }
        self.rg_active.push(key);
        self.rg_frames.push(Frame::default());
        let result = match attribute.source {
            AttributeSource::Input => {
                Ok((entity.rg_inputs[&id_attribute].clone(), Explanation::Input))
            }
            AttributeSource::Numeric => self.numeric(id_entity, id_attribute, attribute.value_type),
            AttributeSource::Derived(rule) => rule(self, id_entity).map(|value| {
                (
                    value,
                    Explanation::Derived {
                        rg_dependencies: Vec::new(),
                        rg_steps: Vec::new(),
                    },
                )
            }),
        };
        let frame = self.rg_frames.pop().expect("balanced evaluation frame");
        self.rg_active.pop();
        let (value, mut explanation) = result?;
        if value.value_type() != attribute.value_type || !value.finite() {
            return Err(EvalError::Invalid(format!(
                "{} returned the wrong type or a non-finite value",
                attribute.sz_name
            )));
        }
        if let Some(check) = attribute.check {
            check(&value).map_err(|sz_error| {
                EvalError::Invalid(format!("{}: {sz_error}", attribute.sz_name))
            })?;
        }
        if let Explanation::Derived {
            rg_dependencies,
            rg_steps,
        } = &mut explanation
        {
            *rg_dependencies = frame.rg_dependencies;
            *rg_steps = frame.rg_steps;
        }
        let evaluation = Arc::new(Evaluation {
            id_entity,
            id_attribute,
            sz_name: attribute.sz_name,
            value,
            explanation,
        });
        self.rg_memo.insert(key, Arc::clone(&evaluation));
        Ok(evaluation)
    }

    pub fn explain(
        &mut self,
        id_entity: EntityId,
        id_attribute: AttributeId,
    ) -> Result<String, EvalError> {
        Ok(self.evaluate(id_entity, id_attribute)?.render())
    }

    fn contributions(&mut self, id_entity: EntityId) -> Result<&[AppliedContribution], EvalError> {
        if !self.rg_contributions.contains_key(&id_entity) {
            let definitions = self.view.definitions();
            let entity = self.view.entity(id_entity)?;
            let mut rg_parts: Vec<_> = entity
                .rg_slots
                .iter()
                .flat_map(|(id_slot, rg_parts)| rg_parts.iter().map(move |part| (*id_slot, part)))
                .collect();
            let mut rg_terms = Vec::new();
            rg_parts
                .sort_by(|(id_a, a), (id_b, b)| a.canonical_cmp(b).then_with(|| id_a.cmp(id_b)));
            for (id_slot, part) in rg_parts {
                let definition = &definitions.rg_parts[&part.id];
                let sz_arguments = part
                    .rg_arguments
                    .iter()
                    .map(|(id, value)| format!("{}={value}", definition.rg_parameters[id].sz_name))
                    .collect::<Vec<_>>()
                    .join(", ");
                let sz_slot = &definitions.rg_entities[&entity.id_type].rg_slots[&id_slot].sz_name;
                let sz_origin = format!("{}({sz_arguments}) [{sz_slot}]", definition.sz_name);
                for effect in (definition.expand)(part).map_err(EvalError::Invalid)? {
                    rg_terms.push(AppliedContribution {
                        id_attribute: effect.id_attribute,
                        id_slot,
                        id_part: part.id,
                        sz_origin: sz_origin.clone(),
                        operation: effect.operation,
                        value: effect.value,
                    });
                }
            }
            self.rg_contributions.insert(id_entity, rg_terms);
        }
        Ok(&self.rg_contributions[&id_entity])
    }

    fn numeric(
        &mut self,
        id_entity: EntityId,
        id_attribute: AttributeId,
        value_type: ValueType,
    ) -> Result<(Value, Explanation), EvalError> {
        let rg_terms: Vec<_> = self
            .contributions(id_entity)?
            .iter()
            .filter(|term| term.id_attribute == id_attribute)
            .cloned()
            .collect();
        let base = rg_terms
            .iter()
            .find(|term| term.operation == Operation::Base)
            .ok_or_else(|| EvalError::Invalid("missing base provider".into()))?;
        let mut value = base.value.clone();
        let mut r_product = 1.0;
        let mut f_scaled = false;
        for term in rg_terms
            .iter()
            .filter(|term| term.operation == Operation::Add)
        {
            value = match (&value, &term.value) {
                (Value::Integer(a), Value::Integer(b)) => Value::Integer(
                    a.checked_add(*b)
                        .ok_or_else(|| EvalError::Arithmetic("integer addition overflow".into()))?,
                ),
                (Value::Real(a), Value::Real(b)) => Value::Real(finite(a + b)?),
                _ => return Err(EvalError::Invalid("addition type mismatch".into())),
            };
        }
        for term in rg_terms
            .iter()
            .filter(|term| term.operation == Operation::Multiply)
        {
            let Value::Real(r_factor) = term.value else {
                return Err(EvalError::Invalid(
                    "multiplier requires a real ratio".into(),
                ));
            };
            r_product = finite(r_product * r_factor)?;
            f_scaled = true;
        }
        let f_rounded = value_type == ValueType::Integer && f_scaled;
        if f_scaled {
            value = match value {
                Value::Integer(n_value) => {
                    let r_rounded = finite(n_value as f64 * r_product)?.round();
                    // i64::MAX as f64 is already 2^63, outside the signed range.
                    if r_rounded < i64::MIN as f64 || r_rounded >= -(i64::MIN as f64) {
                        return Err(EvalError::Arithmetic(
                            "scaled integer is out of range".into(),
                        ));
                    }
                    Value::Integer(r_rounded as i64)
                }
                Value::Real(r_value) => Value::Real(finite(r_value * r_product)?),
                _ => return Err(EvalError::Invalid("numeric base requires a number".into())),
            };
        }
        Ok((
            value,
            Explanation::Numeric {
                rg_terms,
                f_rounded,
            },
        ))
    }
}

fn finite(r_value: f64) -> Result<f64, EvalError> {
    if r_value.is_finite() {
        Ok(r_value)
    } else {
        Err(EvalError::Arithmetic("non-finite arithmetic result".into()))
    }
}

impl Inputs for Evaluator<'_> {
    fn get(&mut self, id_entity: EntityId, id_attribute: AttributeId) -> Result<Value, EvalError> {
        let evaluation = self.evaluate(id_entity, id_attribute)?;
        if let Some(frame) = self.rg_frames.last_mut() {
            frame.rg_dependencies.push(Arc::clone(&evaluation));
        }
        Ok(evaluation.value.clone())
    }

    fn note(&mut self, sz_description: &str, value: Value) {
        if let Some(frame) = self.rg_frames.last_mut() {
            frame.rg_steps.push(Step {
                sz_description: sz_description.into(),
                value,
            });
        }
    }
}
