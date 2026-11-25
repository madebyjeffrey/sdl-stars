use rdl::{World, imports::SourceLoader};

fn stars() -> World {
    let mut world = World::new();
    let path = std::path::PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("examples/predefined.rdl");
    SourceLoader::default()
        .load_files(&[path])
        .load_into(&mut world)
        .unwrap();
    world
}

fn integer(world: &World, expression: &str) -> i64 {
    world.query("test", expression).unwrap().as_int().unwrap()
}

#[test]
fn all_presets_are_queryable_with_raw_settings_and_resolved_traits() {
    let world = stars();
    let races = world
        .query(
            "test",
            "query { from r: Race order by r.DesignSettings.source_index ascending select r }",
        )
        .unwrap();
    assert_eq!(races.items().unwrap().len(), 6);
    let expected = [
        ("humanoid", 9, 15, 1_200_000, 0, 0),
        ("rabbitoid", 7, 20, 1_000_000, 4, 2_147_484_931),
        ("insectoid", 2, 10, 1_000_000, 3, 8_456),
        ("nucleotid", 1, 10, 1_000_000, 2, 536_870_924),
        ("silicanoid", 0, 12, 550_000, 4, 4_641),
        ("antetheral", 5, 7, 1_000_000, 5, 1_476),
    ];
    for (race, prt, growth, capacity, lesser, flags) in expected {
        for (field, expected) in [
            ("prt_id", prt),
            ("effective_growth_percent", growth),
            ("CarryingCapacity", capacity),
            ("lrt_count", lesser),
            ("flag_mask", flags),
        ] {
            assert_eq!(
                integer(&world, &format!("{race}.{field}")),
                expected,
                "{race}.{field}"
            );
        }
    }
    assert_eq!(integer(&world, "nucleotid.Habitability.radiation.lower"), 0);
    assert_eq!(integer(&world, "random.flag_mask"), 1 << 30);
    assert_eq!(
        world
            .query("test", "random.generate_before_play")
            .unwrap()
            .as_bool(),
        Some(true)
    );
}

#[test]
fn contextual_population_preserves_units_rounding_and_growth_selection() {
    let world = stars();
    assert_eq!(
        integer(
            &world,
            "(Colony {race=silicanoid; habitat=100; population=250;}).capacity_colonists"
        ),
        550_000
    );
    assert_eq!(
        integer(
            &world,
            "(Colony {race=silicanoid; habitat=100; population=250;}).population_change"
        ),
        30
    );
    assert_eq!(
        integer(
            &world,
            "(Colony {race=humanoid; habitat=50; population=100;}).capacity_colonists"
        ),
        600_000
    );
    assert_eq!(
        integer(
            &world,
            "(Colony {race=humanoid; habitat=(-10); population=100; delta_remainder=37;}).population_change"
        ),
        -1
    );
    assert_eq!(
        integer(
            &world,
            "(Colony {race=humanoid; habitat=(-10); population=100; delta_remainder=37;}).next_remainder"
        ),
        37
    );
    assert_eq!(
        integer(
            &world,
            "(PlanetHabitat {race=humanoid; gravity=50; temperature=50; radiation=50;}).percent"
        ),
        100
    );
    assert_eq!(
        integer(
            &world,
            "(PlanetHabitat {race=humanoid; gravity=0; temperature=0; radiation=0;}).percent"
        ),
        -45
    );
}

#[test]
fn starting_technology_and_population_respect_context() {
    let world = stars();
    assert_eq!(
        integer(
            &world,
            "(StartingState {race=rabbitoid;}).technology.propulsion"
        ),
        7
    );
    assert_eq!(
        integer(
            &world,
            "(StartingState {race=rabbitoid; tutorial=true;}).technology.propulsion"
        ),
        6
    );
    assert_eq!(
        integer(
            &world,
            "(StartingState {race=nucleotid;}).technology.electronics"
        ),
        5
    );
    assert_eq!(
        integer(
            &world,
            "(StartingState {race=nucleotid;}).technology.energy"
        ),
        3
    );
    assert_eq!(
        integer(
            &world,
            "(StartingState {race=rabbitoid; galaxy_size=1;}).home_colonists"
        ),
        20_000
    );
    assert_eq!(
        integer(
            &world,
            "(StartingState {race=rabbitoid; galaxy_size=1;}).second_colonists"
        ),
        10_000
    );
    assert_eq!(
        integer(
            &world,
            "(StartingState {race=silicanoid; accelerated=true;}).home_colonists"
        ),
        85_000
    );
    assert_eq!(
        integer(
            &world,
            "(StartingFleetCount {state=StartingState {race=silicanoid;}; template_index=12;}).home_fleets"
        ),
        3
    );
    assert_eq!(
        integer(
            &world,
            "(StartingFleetCount {state=StartingState {race=nucleotid;}; template_index=15;}).home_fleets"
        ),
        2
    );
    assert_eq!(
        integer(
            &world,
            "(StartingPartUpgrade {state=StartingState {race=rabbitoid;}; category=\"engine\"; initial_index=1;}).result"
        ),
        5
    );
}

#[test]
fn equipment_requires_both_traits_and_technology() {
    let world = stars();
    for (part, expected) in [
        ("part_iengineFuelMizer", true),
        ("part_iengineGalaxyScoop", false),
        ("part_iengineInterspace10", false),
    ] {
        let q = format!(
            "(EquipmentEligibility {{race=rabbitoid; part={part}; technology=(StartingState {{race=rabbitoid;}}).technology;}}).eligible"
        );
        assert_eq!(world.query("test", &q).unwrap().as_bool(), Some(expected));
    }
    assert_eq!(
        integer(
            &world,
            "(StarbaseCost {race=nucleotid; assembled_channel=101;}).final_cost"
        ),
        41
    );
    assert_eq!(
        integer(
            &world,
            "(PartCost {race=insectoid; cost=101; excess_tech=1; has_tech_requirement=true; category=\"weapon\";}).final_cost"
        ),
        73
    );
    assert!(world.query("test","PartCost {race=silicanoid; cost=100; excess_tech=0; has_tech_requirement=true; bypass_miniaturization=true;}").is_err());
}

#[test]
fn deterministic_random_draws_expose_packet_and_engine_probabilities() {
    let world = stars();
    let q = "(FleetFuel {race=rabbitoid; engine_table_entry=120; requested_warp=8; balk_draw=0; current_fuel=0; fuel_capacity=500;}).balks";
    assert_eq!(world.query("test", q).unwrap().as_bool(), Some(true));
    assert_eq!(
        integer(
            &world,
            "(FleetFuel {race=rabbitoid; engine_table_entry=120; requested_warp=8; current_fuel=0; fuel_capacity=500;}).effective_entry"
        ),
        102
    );
    assert_eq!(
        integer(
            &world,
            "(MineTraversal {race=nucleotid; type_id=0; effective_warp=6;}).hit_per_thousand_per_distance"
        ),
        3
    );
    assert_eq!(
        integer(
            &world,
            "(DesignCloak {race=nucleotid; component_points=0; starbase=true;}).percent"
        ),
        76
    );
}

#[test]
fn immune_integral_and_wizard_balance_are_calculated_not_stored() {
    let world = stars();
    assert_eq!(
        integer(&world, "(IntegratedHabitability {race=silicanoid;}).innate"),
        23_958_000
    );
    assert_eq!(
        integer(
            &world,
            "(WizardPricing {race=silicanoid;}).advantage_points"
        ),
        9
    );
}

#[test]
fn custom_trait_bundles_can_supply_lesser_components() {
    let mut world = stars();
    let choices = humanoid_settings(&world).join(" ");
    world.load("custom", &format!("trait custom_lesser {{ provides LrtCheapEngines }} Race custom = {{name=\"Custom\";plural_name=\"Customs\"; {choices} jack_of_all_trades custom_lesser }}")).unwrap();
    assert_eq!(
        world.query("test", "custom.ce").unwrap().as_bool(),
        Some(true)
    );
    assert_eq!(integer(&world, "custom.flag_mask"), 256);
}

#[test]
fn gate_factor_death_and_rounded_total_damage_have_distinct_status() {
    let world = stars();
    for (mass, status) in [(499, 1), (500, 0), (501, -2)] {
        let context = format!(
            "GateJump {{race=humanoid;source_range=250;source_mass=100;destination_mass=100;distance=0;empty_mass={mass};}}"
        );
        assert_eq!(integer(&world, &format!("({context}).status")), status);
        assert_eq!(integer(&world, &format!("({context}).damage_percent")), 100);
    }
}

#[test]
fn catalog_queries_include_unused_templates_and_empty_base_slots() {
    let world = stars();
    assert_eq!(
        integer(
            &world,
            "List.length (query {from t:StartingTemplate where not t.starbase where (StartingFleetCount {state=StartingState {race=nucleotid;};template_index=t.index;}).home_fleets > 0 select t})"
        ),
        4
    );
    assert_eq!(
        integer(
            &world,
            "(StartingBaseSlot {state=StartingState {race=rabbitoid;};template_index=0;slot={slot=0;category=\"base_special\";part_index=0;quantity=0;};}).quantity"
        ),
        1
    );
    assert_eq!(
        integer(
            &world,
            "(StartingBaseSlot {state=StartingState {race=rabbitoid;tutorial=true;};template_index=0;slot={slot=0;category=\"base_special\";part_index=0;quantity=0;};}).quantity"
        ),
        0
    );
}

fn humanoid_settings(world: &World) -> Vec<String> {
    world
        .query("test", "humanoid.traits")
        .unwrap()
        .items()
        .unwrap()
        .iter()
        .filter(|value| value.reference() != Some("jack_of_all_trades"))
        .map(ToString::to_string)
        .collect()
}

#[test]
fn independent_setting_traits_recombine_without_a_race_specific_provider() {
    let mut world = stars();
    let mut choices = humanoid_settings(&world);
    for (old, new) in [
        ("factory_cost(10)", "factory_cost(9)"),
        ("growth(15)", "growth(20)"),
        ("portrait(1)", "portrait(12)"),
    ] {
        let choice = choices
            .iter_mut()
            .find(|value| value.as_str() == old)
            .unwrap();
        *choice = new.into();
    }
    world
        .load(
            "hybrid",
            &format!(
                "Race hybrid = {{name=\"Hybrid\";plural_name=\"Hybrids\";{} jack_of_all_trades}}",
                choices.join(" ")
            ),
        )
        .unwrap();
    assert_eq!(integer(&world, "hybrid.FactoryCost"), 9);
    assert_eq!(integer(&world, "hybrid.Economy.factory_cost"), 9);
    assert_eq!(integer(&world, "hybrid.Economy.factory_output"), 10);
    assert_eq!(integer(&world, "hybrid.Economy.mine_cost"), 5);
    assert_eq!(integer(&world, "hybrid.DesignSettings.portrait"), 12);
    assert_eq!(integer(&world, "hybrid.effective_growth_percent"), 20);
    assert_eq!(integer(&world, "humanoid.Economy.factory_cost"), 10);
    for race in [
        "humanoid",
        "rabbitoid",
        "insectoid",
        "nucleotid",
        "silicanoid",
        "antetheral",
    ] {
        let selected = world.query("test", &format!("{race}.traits")).unwrap();
        assert!(
            selected
                .items()
                .unwrap()
                .iter()
                .all(|value| !value.to_string().ends_with("_parameters"))
        );
    }
}

#[test]
fn independent_settings_are_required_and_conflicting_choices_are_rejected() {
    let mut world = stars();
    let mut choices = humanoid_settings(&world);
    choices.retain(|choice| choice != "factory_cost(10)");
    let source = |choices: &[String]| {
        format!(
            "Race invalid = {{name=\"Invalid\";plural_name=\"Invalids\";{} jack_of_all_trades}}",
            choices.join(" ")
        )
    };
    assert!(world.load("missing", &source(&choices)).is_err());
    choices.extend(["factory_cost(10)".into(), "factory_cost(9)".into()]);
    assert!(world.load("conflicting", &source(&choices)).is_err());
    assert_eq!(integer(&world, "humanoid.FactoryCost"), 10);
}

#[test]
fn predefined_advantage_points_match_native_scores() {
    let mut world = stars();
    let result=world.query("pricing", "query { from r:Race order by r.source_index ascending select {name=r.name;points=r.points;} }").unwrap();
    let rows = result.items().unwrap();
    let expected = [
        ("Humanoid", 25),
        ("Rabbitoid", 32),
        ("Insectoid", 43),
        ("Nucleotid", 11),
        ("Silicanoid", 9),
        ("Antetheral", 7),
    ];
    assert_eq!(rows.len(), expected.len());
    assert_eq!(integer(&world, "humanoid.points"), 25);
    assert_eq!(
        world
            .query(
                "pricing",
                "silicanoid.points=(WizardPricing {race=silicanoid;}).advantage_points"
            )
            .unwrap()
            .as_bool(),
        Some(true)
    );
    let error = world
        .load(
            "stored-points",
            "Race invalid={name=\"Invalid\";plural_name=\"Invalids\";points=999;}",
        )
        .unwrap_err();
    assert!(error.message.contains("derived field 'points'"), "{error}");
    for (row, (name, points)) in rows.iter().zip(expected) {
        let fields = row.fields().unwrap();
        assert_eq!(fields["name"].as_str(), Some(name));
        assert_eq!(fields["points"].as_int(), Some(points), "{name}");
    }
}

#[test]
fn individual_preset_import_includes_points_calculation() {
    let root = std::path::PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("examples");
    let mut world = World::new();
    SourceLoader::new(vec![root.clone()])
        .load_files(&[root.join("stars/predefined/humanoid.rdl")])
        .load_into(&mut world)
        .unwrap();
    assert_eq!(integer(&world, "humanoid.points"), 25);
    assert_eq!(
        world
            .query("q", "List.length (query {from r:Race select r})")
            .unwrap()
            .as_int(),
        Some(1)
    );
}
