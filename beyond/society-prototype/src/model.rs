//! Disposable editor state. All rule validation and pricing come from RDL.
use rdl::{World, imports::SourceLoader};
use scars_window::ui::*;
use std::{cell::RefCell, path::PathBuf, rc::Rc, sync::mpsc, time::Duration};

pub const RG_PRT: [(&str, &str); 10] = [
    ("hyper_expansion", "Hyper-Expansion"),
    ("super_stealth", "Super Stealth"),
    ("war_monger", "War Monger"),
    ("claim_adjuster", "Claim Adjuster"),
    ("inner_strength", "Inner-Strength"),
    ("space_demolition", "Space Demolition"),
    ("packet_physics", "Packet Physics"),
    ("interstellar_traveler", "Interstellar Traveler"),
    ("alternate_reality", "Alternate Reality"),
    ("jack_of_all_trades", "Jack of All Trades"),
];
pub const RG_LRT: [(&str, &str); 14] = [
    ("lrt_ife", "Improved Fuel Efficiency"),
    ("lrt_tt", "Total Terraforming"),
    ("lrt_arm", "Advanced Remote Mining"),
    ("lrt_isb", "Improved Starbases"),
    ("lrt_gr", "Generalized Research"),
    ("lrt_ur", "Ultimate Recycling"),
    ("lrt_ma", "Mineral Alchemy"),
    ("lrt_nrse", "No Ram Scoop Engines"),
    ("lrt_ce", "Cheap Engines"),
    ("lrt_obrm", "Only Basic Remote Mining"),
    ("lrt_nas", "No Advanced Scanners"),
    ("lrt_lsp", "Low Starting Population"),
    ("lrt_bet", "Bleeding Edge Technology"),
    ("lrt_rs", "Regenerating Shields"),
];
pub const RG_LRT_DESCRIPTION: [&str; 14] = [
    "Engines use 15% less fuel. Also unlocks the Fuel Mizer and Galaxy Scoop; the Galaxy Scoop is incompatible with NRSE.",
    "Unlocks broad terraforming across all three environment axes, with greater range at higher Biology. Terraforming costs less to produce.",
    "Adds advanced remote-mining hulls and robot miners with more capable mining equipment.",
    "Discounts starbase construction and unlocks the Space Dock and Ultra Station hulls.",
    "Spreads annual research across all six fields: the selected field gets about half, and the other five get a smaller share each.",
    "Improves mineral and resource recovery when ships are scrapped at a planet you control. A starbase improves the salvage return.",
    "Cuts mineral alchemy costs from 100 resources to 25 per unit. Each unit produces one kiloton of all three minerals.",
    "Forbids most fuel-scoop engines and unlocks the Interspace-10. The Fuel Mizer remains available, even with IFE.",
    "Cuts engine construction costs in half and increases starting Propulsion technology by one level.",
    "Restricts remote-mining equipment to basic models and increases carrying capacity by 10%. Overrides ARM's advanced-miner access.",
    "Disallows advanced ship and planetary scanner equipment. Built-in racial scanner abilities remain available.",
    "Homeworld starts with 17,500 colonists instead of 25,000, before game-mode adjustments.",
    "Improves ship-part miniaturization: each excess tech level saves more, up to a higher maximum. Newest components can cost more when technology is low.",
    "Restores some shields each round for the first 15 rounds of combat. A ship with no shields cannot regenerate them.",
];
pub const RG_AXIS: [&str; 3] = ["gravity", "temperature", "radiation"];
pub const RG_TECH: [&str; 6] = [
    "energy",
    "weapons",
    "propulsion",
    "construction",
    "electronics",
    "biotechnology",
];
pub const RG_ECONOMY: [&str; 7] = [
    "population_efficiency",
    "factory_output",
    "factory_cost",
    "factory_operation",
    "mine_output",
    "mine_cost",
    "mine_operation",
];
pub const RG_LEFTOVER: [&str; 5] = [
    "surface_minerals",
    "concentrations",
    "mines",
    "factories",
    "defenses",
];
pub const RG_PRESETS: [&str; 6] = [
    "humanoid",
    "rabbitoid",
    "insectoid",
    "nucleotid",
    "silicanoid",
    "antetheral",
];
pub const RG_STEPS: [(&str, &str); 7] = [
    ("identity", "Identity"),
    ("primary", "Primary trait"),
    ("traits", "Additional traits"),
    ("environment", "Environment"),
    ("economy", "Economy"),
    ("research", "Research"),
    ("review", "Review"),
];

#[derive(Clone, Debug, PartialEq)]
pub struct Design {
    pub sz_name: String,
    pub sz_plural: String,
    pub i_sigil: usize,
    pub i_portrait: i64,
    pub i_prt: usize,
    pub rg_lrt: [bool; 14],
    pub rg_habitat: [[i64; 2]; 3],
    pub rg_immune: [bool; 3],
    pub c_growth: i64,
    pub rg_economy: [i64; 7],
    pub f_cheaper_factories: bool,
    pub rg_research: [String; 6],
    pub f_start_high: bool,
    pub c_allocation: i64,
    pub i_leftover: usize,
}

fn quoted(sz: &str) -> String {
    format!(
        "\"{}\"",
        sz.replace('\\', "\\\\")
            .replace('"', "\\\"")
            .replace('\n', "\\n")
            .replace('\r', "\\r")
            .replace('\t', "\\t")
    )
}
impl Design {
    pub fn rdl(&self) -> String {
        let mut sz = format!(
            "(* Society design. Sigil artwork index: {}. *)\nRace society = {{\n  name = {};\n  plural_name = {};\n  portrait({})\n",
            self.i_sigil,
            quoted(&self.sz_name),
            quoted(&self.sz_plural),
            self.i_portrait
        );
        for (i, axis) in RG_AXIS.iter().enumerate() {
            if self.rg_immune[i] {
                sz += &format!("  {axis}(immune)\n");
            } else {
                sz += &format!(
                    "  {axis}({}, {})\n",
                    self.rg_habitat[i][0], self.rg_habitat[i][1]
                );
            }
        }
        sz += &format!("  growth({})\n", self.c_growth);
        for (i, key) in RG_ECONOMY.iter().enumerate() {
            sz += &format!("  {key}({})\n", self.rg_economy[i]);
        }
        for (i, key) in RG_TECH.iter().enumerate() {
            sz += &format!("  {key}_research({})\n", self.rg_research[i]);
        }
        sz += &format!(
            "  research_allocation({})\n  leftover_points({})\n  standard_capacity\n  {}\n",
            self.c_allocation, RG_LEFTOVER[self.i_leftover], RG_PRT[self.i_prt].0
        );
        for (i, (key, _)) in RG_LRT.iter().enumerate() {
            if self.rg_lrt[i] {
                sz += &format!("  {key}\n");
            }
        }
        if self.f_cheaper_factories {
            sz += "  cheaper_factories\n";
        }
        if self.f_start_high {
            sz += "  higher_starting_technology\n";
        }
        sz + "}\n"
    }
    pub fn identity_valid(&self) -> Result<(), String> {
        if self.sz_name.trim().is_empty() || self.sz_plural.trim().is_empty() {
            return Err("Give your society both a singular and a plural name.".into());
        }
        if self.sz_name.len() > 31 || self.sz_plural.len() > 31 {
            return Err(
                "Each name can use up to 31 UTF-8 bytes. Shorten it without splitting a character."
                    .into(),
            );
        }
        Ok(())
    }
    pub fn habitat_valid(&self) -> Result<(), String> {
        for (i, sz) in RG_AXIS.iter().enumerate() {
            if !self.rg_immune[i] && self.rg_habitat[i][1] - self.rg_habitat[i][0] < 2 {
                return Err(format!(
                    "The {sz} upper bound must be at least two above its lower bound."
                ));
            }
        }
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq)]
pub struct Evaluation {
    pub c_points: i64,
    pub c_growth: i64,
    pub c_capacity: i64,
}
#[derive(Clone)]
pub struct Rules {
    pub world: World,
    pub rg_presets: Vec<Design>,
    pub rg_descriptions: Vec<String>,
}
impl Rules {
    pub fn load() -> Result<Self, Box<dyn std::error::Error>> {
        let mut world = World::new();
        let path =
            PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../rdl/examples/stars/predefined.rdl");
        SourceLoader::new(vec![
            PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../rdl/examples"),
        ])
        .load_files(&[path])
        .load_into(&mut world)?;
        let mut rg_presets = Vec::new();
        for name in RG_PRESETS {
            let integer = |field: &str| -> Result<i64, Box<dyn std::error::Error>> {
                world
                    .query("preset", &format!("{name}.{field}"))?
                    .as_int()
                    .ok_or_else(|| format!("Expected integer: {field}").into())
            };
            let string = |field: &str| -> Result<String, Box<dyn std::error::Error>> {
                Ok(world
                    .query("preset", &format!("{name}.{field}"))?
                    .as_str()
                    .ok_or("Expected string")?
                    .to_owned())
            };
            let flags = integer("flag_mask")?;
            let mut rg_habitat = [[15, 85]; 3];
            let mut rg_immune = [false; 3];
            let mut rg_research = std::array::from_fn(|_| String::new());
            for (i, axis) in RG_AXIS.iter().enumerate() {
                let lo = integer(&format!("Habitability.{axis}.lower"))?;
                rg_immune[i] = lo == -1;
                if lo != -1 {
                    rg_habitat[i] = [lo, integer(&format!("Habitability.{axis}.upper"))?];
                }
            }
            for (i, tech) in RG_TECH.iter().enumerate() {
                rg_research[i] = ["expensive", "standard", "cheap"]
                    [integer(&format!("ResearchPreferences.costs.{tech}"))? as usize]
                    .into();
            }
            let fields = [
                "PopulationResourceDivisor",
                "FactoryOutput",
                "FactoryCost",
                "FactoryOperationLimit",
                "MineOutput",
                "MineCost",
                "MineOperationLimit",
            ];
            let mut rg_economy = [0; 7];
            for (i, field) in fields.iter().enumerate() {
                rg_economy[i] = integer(field)?;
            }
            rg_economy[0] *= 100;
            rg_presets.push(Design {
                sz_name: string("name")?,
                sz_plural: string("plural_name")?,
                i_sigil: rg_presets.len(),
                i_portrait: integer("Portrait")?,
                i_prt: integer("prt_id")? as usize,
                rg_lrt: std::array::from_fn(|i| flags & (1 << i) != 0),
                rg_habitat,
                rg_immune,
                c_growth: integer("SelectedGrowth")?,
                rg_economy,
                f_cheaper_factories: flags & (1 << 31) != 0,
                rg_research,
                f_start_high: flags & (1 << 29) != 0,
                c_allocation: integer("ResearchAllocationPercent")?,
                i_leftover: integer("LeftoverAllocation")? as usize,
            });
        }
        let components = [
            "HyperExpansion",
            "SuperStealth",
            "WarMonger",
            "ClaimAdjuster",
            "InnerStrength",
            "SpaceDemolition",
            "PacketPhysics",
            "InterstellarTraveler",
            "AlternateReality",
            "JackOfAllTrades",
        ];
        let mut rg_descriptions = Vec::new();
        for component in components {
            let value = world.query("trait", &format!("({component} {{}}).description"))?;
            // Legacy wording is adapted only in the displayed description.
            let sz = value
                .as_str()
                .ok_or("Expected trait description")?
                .replace("races", "societies")
                .replace("race", "society")
                .replace("step four", "Environment");
            rg_descriptions.push(sz.split_whitespace().collect::<Vec<_>>().join(" "));
        }
        Ok(Self {
            world,
            rg_presets,
            rg_descriptions,
        })
    }
    pub fn evaluate(world: &World, design: &Design) -> Result<Evaluation, String> {
        design.identity_valid()?;
        design.habitat_valid()?;
        let mut world = world.clone();
        world
            .load("society.rdl", &design.rdl())
            .map_err(|e| e.to_string())?;
        let integer = |field| {
            world
                .query("balance", &format!("society.{field}"))
                .map_err(|e| e.to_string())?
                .as_int()
                .ok_or_else(|| "Expected integer result".to_owned())
        };
        Ok(Evaluation {
            c_points: integer("points")?,
            c_growth: integer("effective_growth_percent")?,
            c_capacity: integer("CarryingCapacity")?,
        })
    }
}

#[derive(Clone)]
pub struct Model {
    pub design: State<Design>,
    pub result: State<Option<Result<Evaluation, String>>>,
    pub f_finished: State<bool>,
    pub f_cancelled: State<bool>,
    pub f_source: State<bool>,
    pub f_detailed: State<bool>,
    pub sz_layout: State<String>,
    pub rules: Rc<Rules>,
    c_revision: State<u64>,
    tx: mpsc::Sender<(u64, Design)>,
    rx: Rc<RefCell<mpsc::Receiver<(u64, Result<Evaluation, String>)>>>,
    host: Rc<RefCell<Option<Host>>>,
}
impl Model {
    pub fn new(rules: Rules) -> Self {
        let design = rules.rg_presets[0].clone();
        let result = Rules::evaluate(&rules.world, &design);
        let (tx, requests) = mpsc::channel::<(u64, Design)>();
        let (results, rx) = mpsc::channel();
        let world = rules.world.clone();
        std::thread::spawn(move || {
            while let Ok(mut request) = requests.recv() {
                for newer in requests.try_iter() {
                    request = newer;
                }
                let result = Rules::evaluate(&world, &request.1);
                if results.send((request.0, result)).is_err() {
                    break;
                }
            }
        });
        Self {
            design: State::new(design),
            result: State::new(Some(result)),
            f_finished: State::new(false),
            f_cancelled: State::new(false),
            f_source: State::new(false),
            f_detailed: State::new(true),
            sz_layout: State::new("guided".into()),
            rules: Rc::new(rules),
            c_revision: State::new(0),
            tx,
            rx: Rc::new(RefCell::new(rx)),
            host: Rc::new(RefCell::new(None)),
        }
    }
    pub fn attach(&self, host: Option<Host>) {
        *self.host.borrow_mut() = host;
    }
    pub fn edit(&self, change: impl FnOnce(&mut Design)) {
        let mut design = self.design.get();
        change(&mut design);
        if self.design.with(|current| *current == design) {
            return;
        }
        self.design.set(design.clone());
        self.f_finished.set(false);
        self.c_revision.update(|n| *n += 1);
        self.result.set(None);
        if self.tx.send((self.c_revision.get(), design)).is_err() {
            self.result.set(Some(Err(
                "The calculation worker stopped. Restart the prototype.".into(),
            )));
        } else {
            self.schedule();
        }
    }
    fn schedule(&self) {
        if let Some(host) = self.host.borrow().as_ref() {
            let model = self.clone();
            host.debounce("society-pricing", Duration::from_millis(80), move || {
                model.poll();
                if model.result.get().is_none() {
                    model.schedule();
                }
            });
        }
    }
    pub fn poll(&self) {
        for (revision, result) in self.rx.borrow().try_iter() {
            if revision == self.c_revision.get() {
                self.result.set(Some(result));
            }
        }
    }
    pub fn bind<T: Clone + 'static>(
        &self,
        read: impl Fn(&Design) -> T + 'static,
        write: impl Fn(&mut Design, T) + 'static,
    ) -> Binding<T> {
        let source = self.design.clone();
        let model = self.clone();
        Binding::new(
            move || source.with(&read),
            move |value| model.edit(|d| write(d, value)),
        )
    }
    pub fn finish_valid(&self) -> Result<(), String> {
        match self.result.get() {
            None => Err("Updating your design balance…".into()),
            Some(Err(error)) => Err(error),
            Some(Ok(result)) if result.c_points < 0 => Err(format!(
                "Your design is {} points over budget. Revisit your choices before finishing.",
                -result.c_points
            )),
            Some(Ok(_)) => Ok(()),
        }
    }
    pub fn wizard(&self, numbers: &[Numeric<i64>]) -> Result<Wizard, Error> {
        let steps = RG_STEPS
            .iter()
            .map(|(id, title)| {
                let model = self.clone();
                let id = *id;
                let numbers = numbers.to_vec();
                WizardStep::new(id, *title).validate(move || {
                    let range = match id {
                        "economy" => 0..7,
                        "environment" => 7..14,
                        "research" => 14..15,
                        _ => 0..0,
                    };
                    for i in range {
                        if id == "environment"
                            && i < 13
                            && model.design.get().rg_immune[(i - 7) / 2]
                        {
                            continue;
                        }
                        if numbers[i].validation().status != ValidationStatus::Valid {
                            return Err(
                                "Correct the unfinished numeric field before continuing.".into()
                            );
                        }
                    }
                    match id {
                        "identity" => model.design.get().identity_valid(),
                        "environment" => model.design.get().habitat_valid(),
                        "review" => model.finish_valid(),
                        _ => Ok(()),
                    }
                })
            })
            .collect();
        Wizard::new("identity", steps)?
            .options(WizardOptions {
                show_steps: false,
                finish_label: "Create society".into(),
                ..Default::default()
            })
            .map(|wizard| {
                wizard
                    .on_finish(self.f_finished.action(|s| s.set(true)))
                    .on_cancel(self.f_cancelled.action(|s| s.set(true)))
            })
    }
}
