#![doc = include_str!("../README.md")]
mod evaluate;
mod model;
pub mod stars;
mod validate;

use std::collections::{BTreeMap, BTreeSet};

pub use evaluate::{AppliedContribution, Evaluation, Evaluator, Explanation, Step};
pub use model::*;

/// A read predicate, independent of target selection, undo, or pending edits.
#[derive(Clone, Debug)]
pub struct Context {
    scope: Scope,
    rg_selected: Option<BTreeSet<EntityId>>,
}

impl Context {
    pub fn global() -> Self {
        Self {
            scope: Scope::Global,
            rg_selected: None,
        }
    }

    pub fn for_game(id: GameId) -> Self {
        Self {
            scope: Scope::Game(id),
            rg_selected: None,
        }
    }

    pub fn restrict_to(mut self, rg_ids: impl IntoIterator<Item = EntityId>) -> Self {
        let rg_requested: BTreeSet<_> = rg_ids.into_iter().collect();
        self.rg_selected = Some(match self.rg_selected {
            Some(rg_previous) => rg_previous.intersection(&rg_requested).copied().collect(),
            None => rg_requested,
        });
        self
    }

    fn contains(&self, entity: &Entity) -> bool {
        (entity.scope == Scope::Global || entity.scope == self.scope)
            && self
                .rg_selected
                .as_ref()
                .is_none_or(|rg_ids| rg_ids.contains(&entity.id))
    }
}

pub struct KnowledgeBase {
    definitions: Definitions,
    rg_entities: BTreeMap<EntityId, Entity>,
}

impl KnowledgeBase {
    pub fn new(definitions: Definitions) -> Result<Self, Vec<Violation>> {
        let rg_violations = validate::definitions(&definitions);
        if !rg_violations.is_empty() {
            return Err(rg_violations);
        }
        Ok(Self {
            definitions,
            rg_entities: BTreeMap::new(),
        })
    }

    pub fn view<'a>(&'a self, context: &'a Context) -> View<'a> {
        View {
            source: self,
            context,
        }
    }

    pub fn transaction(&mut self) -> Transaction<'_> {
        Transaction {
            base: self,
            rg_candidates: BTreeMap::new(),
        }
    }
}

trait EntitySource {
    fn definitions(&self) -> &Definitions;
    fn find(&self, id: EntityId) -> Option<&Entity>;
    fn ids(&self) -> BTreeSet<EntityId>;
}

impl EntitySource for KnowledgeBase {
    fn definitions(&self) -> &Definitions {
        &self.definitions
    }
    fn find(&self, id: EntityId) -> Option<&Entity> {
        self.rg_entities.get(&id)
    }
    fn ids(&self) -> BTreeSet<EntityId> {
        self.rg_entities.keys().copied().collect()
    }
}

#[derive(Clone, Copy)]
pub struct View<'a> {
    source: &'a dyn EntitySource,
    context: &'a Context,
}

impl View<'_> {
    pub fn entity(&self, id: EntityId) -> Result<&Entity, EvalError> {
        self.source
            .find(id)
            .filter(|entity| self.context.contains(entity))
            .ok_or(EvalError::NotVisible(id))
    }

    pub fn entity_ids(&self) -> Vec<EntityId> {
        self.source
            .ids()
            .into_iter()
            .filter(|id| self.entity(*id).is_ok())
            .collect()
    }

    pub(crate) fn definitions(&self) -> &Definitions {
        self.source.definitions()
    }
}

/// Exclusive in-memory staging. Dropping or rejecting the batch changes nothing.
pub struct Transaction<'a> {
    base: &'a mut KnowledgeBase,
    rg_candidates: BTreeMap<EntityId, Entity>,
}

impl Transaction<'_> {
    pub fn put(&mut self, entity: Entity) {
        self.rg_candidates.insert(entity.id, entity);
    }

    /// Checks staged shape before exposing a preview; it does not publish changes.
    pub fn view<'a>(&'a self, context: &'a Context) -> Result<View<'a>, Vec<Violation>> {
        let rg_violations = self.violations();
        if !rg_violations.is_empty() {
            return Err(rg_violations);
        }
        Ok(View {
            source: self,
            context,
        })
    }

    pub fn commit(self) -> Result<(), Vec<Violation>> {
        let rg_violations = self.violations();
        if !rg_violations.is_empty() {
            return Err(rg_violations);
        }
        self.base.rg_entities.extend(self.rg_candidates);
        Ok(())
    }

    fn violations(&self) -> Vec<Violation> {
        self.rg_candidates
            .values()
            .flat_map(|entity| validate::entity(&self.base.definitions, entity))
            .collect()
    }
}

impl EntitySource for Transaction<'_> {
    fn definitions(&self) -> &Definitions {
        &self.base.definitions
    }
    fn find(&self, id: EntityId) -> Option<&Entity> {
        self.rg_candidates
            .get(&id)
            .or_else(|| self.base.rg_entities.get(&id))
    }
    fn ids(&self) -> BTreeSet<EntityId> {
        self.base
            .rg_entities
            .keys()
            .chain(self.rg_candidates.keys())
            .copied()
            .collect()
    }
}
