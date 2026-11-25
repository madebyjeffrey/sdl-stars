//! Three native layouts of the same wizard: Guided, Focus and Workbench.
use crate::model::*;
use scars_window::{skia, ui::*};
use std::rc::Rc;

pub const RG_SIGILS: [&str; 16] = [
    "Vortex",
    "Blades",
    "Balance",
    "Sentinel",
    "Revenant",
    "Verdant",
    "Twin stars",
    "Infinity",
    "Crystal",
    "Voyager",
    "Oracle",
    "Nexus",
    "Serpent",
    "Spiral",
    "Prism",
    "Atom",
];

pub struct Designer {
    pub model: Model,
    pub wizard: Wizard,
    rg_numbers: Vec<Numeric<i64>>,
    rg_basic: Vec<Bitmap>,
    rg_detailed: Vec<Bitmap>,
}
fn atlas(bytes: &[u8]) -> Result<Vec<Bitmap>, Box<dyn std::error::Error>> {
    let image = skia::Image::from_encoded(skia::Data::new_copy(bytes))
        .ok_or("Cannot decode sigil sheet")?;
    // The supplied sheets have uneven gutters. Equal quarters include the tips
    // of the fourth row in the third row's icons.
    let rg_x = [0, 324, 634, 944, 1269];
    let rg_y = [0, 320, 614, 900, 1240];
    if image.width() != rg_x[4] || image.height() != rg_y[4] {
        return Err("Sigil sheet dimensions changed; update the atlas gutters".into());
    }
    let mut result = Vec::new();
    for i in 0..16 {
        let x = i % 4;
        let y = i / 4;
        let rect = skia::IRect::new(rg_x[x], rg_y[y], rg_x[x + 1], rg_y[y + 1]);
        let cell = image
            .make_subset(None, rect, Default::default())
            .ok_or("Cannot read sigil cell")?;
        let png = cell
            .encode(None, skia::EncodedImageFormat::PNG, None)
            .ok_or("Cannot decode sigil")?;
        result.push(Bitmap::from_encoded(png.as_bytes().to_vec()));
    }
    Ok(result)
}
impl Designer {
    pub fn new(model: Model) -> Result<Rc<Self>, Box<dyn std::error::Error>> {
        let mut rg_numbers = Vec::new();
        for (i, (lo, hi, step)) in [
            (700, 2500, 100),
            (5, 15, 1),
            (5, 25, 1),
            (5, 25, 1),
            (5, 25, 1),
            (2, 15, 1),
            (5, 25, 1),
        ]
        .into_iter()
        .enumerate()
        {
            rg_numbers.push(Numeric::new(
                model.bind(move |d| d.rg_economy[i], move |d, n| d.rg_economy[i] = n),
                lo,
                hi,
                step,
            ));
        }
        for i in 0..3 {
            for j in 0..2 {
                rg_numbers.push(Numeric::new(
                    model.bind(
                        move |d| d.rg_habitat[i][j],
                        move |d, n| d.rg_habitat[i][j] = n,
                    ),
                    0,
                    100,
                    1,
                ));
            }
        }
        rg_numbers.push(Numeric::new(
            model.bind(|d| d.c_growth, |d, n| d.c_growth = n),
            1,
            20,
            1,
        ));
        rg_numbers.push(Numeric::new(
            model.bind(|d| d.c_allocation, |d, n| d.c_allocation = n),
            0,
            100,
            1,
        ));
        let wizard = model.wizard(&rg_numbers)?;
        Ok(Rc::new(Self {
            model,
            wizard,
            rg_numbers,
            rg_basic: atlas(include_bytes!("../../sigils-basic.png"))?,
            rg_detailed: atlas(include_bytes!("../../sigils-detailed.png"))?,
        }))
    }
    fn sigil(&self, i: usize, size: f64) -> View {
        let rg = if self.model.f_detailed.get() {
            &self.rg_detailed
        } else {
            &self.rg_basic
        };
        View::image(rg[i].clone(), ContentMode::Fit)
            .width(size)
            .height(size)
            .accessible_label(format!("{} sigil", RG_SIGILS[i]))
    }
    fn note(sz: impl Into<String>) -> View {
        View::label(sz)
            .wrap(true)
            .label_style(LabelStyle::Secondary)
            .font_size(13.)
    }
    fn heading(sz: impl Into<String>, subtitle: impl Into<String>) -> View {
        view! { column(gap: 8, shrink: 0) {
            label(sz, font_size: 28, bold: true);
            embed(Self::note(subtitle));
        }}
    }
    fn number(&self, i: usize, label: &str, help: &str) -> View {
        view! { row(gap: 22, align: Align::Center) {
            numeric_field(label, &self.rg_numbers[i], width: 220, shrink: 0, key: format!("number-{i}"));
            embed(Self::note(help).grow(1.).basis(0.));
        }}
    }
    fn identity(&self) -> View {
        let name = self
            .model
            .bind(|d| d.sz_name.clone(), |d, sz| d.sz_name = sz);
        let plural = self
            .model
            .bind(|d| d.sz_plural.clone(), |d, sz| d.sz_plural = sz);
        let d = self.model.design.get();
        let mut presets = Vec::new();
        for (i, preset) in self.model.rules.rg_presets.iter().enumerate() {
            let model = self.model.clone();
            presets.push(
                View::button(&preset.sz_name, move || {
                    let next = model.rules.rg_presets[i].clone();
                    model.edit(|d| *d = next);
                })
                .key(format!("preset-{i}")),
            );
        }
        view! { column(gap: 22) {
            embed(Self::heading("A people. A possibility.", "Choose an identity for the society you will lead among the stars."));
            row(gap: 18, align: Align::Center) {
                embed(self.sigil(d.i_sigil, 100.));
                column(grow: 1, gap: 10) {
                    label("SINGULAR NAME", font_size: 11, label_style: LabelStyle::Status);
                    text_field("For one member", bind: name, key: "society-name", accessible_label: "Singular society name");
                    label("PLURAL NAME", font_size: 11, label_style: LabelStyle::Status);
                    text_field("For your people", bind: plural, key: "society-plural", accessible_label: "Plural society name");
                }
            }
            embed(Self::note(format!("Names: {} / 31 and {} / 31 UTF-8 bytes", d.sz_name.len(), d.sz_plural.len())));
            column(gap: 10) {
                label("START FROM A TEMPLATE", font_size: 11, label_style: LabelStyle::Status);
                embed(View::row(presets).wrap(true).gap(6.));
                embed(Self::note("A template replaces the current choices, including names. Every choice remains editable."));
            }
            column(gap: 10) {
                row(gap: 16, align: Align::Center) {
                    label("CHOOSE YOUR SIGIL", font_size: 11, label_style: LabelStyle::Status, grow: 1);
                    switch("Detailed artwork", bind: self.model.f_detailed);
                }
                embed(self.sigil_gallery());
                embed(Self::note("Your symbol expresses identity. It has no effect on traits or points."));
            }
        }}
    }
    pub fn sigil_gallery(&self) -> View {
        let d = self.model.design.get();
        let mut gallery = Vec::new();
        for row in 0..4 {
            let mut cells = Vec::new();
            for col in 0..4 {
                let i = row * 4 + col;
                let model = self.model.clone();
                let select = View::toggle(
                    RG_SIGILS[i],
                    model.bind(
                        move |d| d.i_sigil == i,
                        move |d, f| {
                            if f {
                                d.i_sigil = i;
                            }
                        },
                    ),
                )
                .key(format!("sigil-{i}"))
                .width(122.)
                .height(112.)
                .padding([80., 0., 8., 0.])
                .radius(8.)
                .font_size(12.)
                .accessible_label(format!(
                    "Select {} sigil{}",
                    RG_SIGILS[i],
                    if i == d.i_sigil { ", selected" } else { "" }
                ));
                // The toggle owns the entire tile; the decorative image does
                // not intercept pointer hits or add another keyboard stop.
                cells.push(view! { layer(width: 122, height: 112, shrink: 0, key: format!("sigil-cell-{i}")) {
                    embed(select);
                    embed(self.sigil(i, 64.).key(format!("sigil-image-{i}")).accessible_label("").position(Position::Absolute).left(29).top(10));
                }});
            }
            gallery.push(View::row(cells).gap(8.));
        }
        View::column(gallery).gap(8.)
    }
    fn primary(&self) -> View {
        let d = self.model.design.get();
        let choices = RG_PRT
            .iter()
            .enumerate()
            .map(|(i, (_, title))| {
                let model = self.model.clone();
                View::toggle(
                    *title,
                    model.bind(
                        move |d| d.i_prt == i,
                        move |d, f| {
                            if f {
                                d.i_prt = i;
                            }
                        },
                    ),
                )
                .key(format!("primary-{i}"))
                .height(38.)
            })
            .collect();
        view! { column(gap: 24) {
            embed(Self::heading("What sets you apart?", "Choose one primary trait. This shapes your society's defining capabilities."));
            row(gap: 22) {
                embed(View::column(choices).gap(8.).width(220.).shrink(0.));
                panel(grow: 1, basis: 0, padding: 20, gap: 16) {
                    label(RG_PRT[d.i_prt].1, font_size: 21, label_style: LabelStyle::Status, wrap: true);
                    label(&self.model.rules.rg_descriptions[d.i_prt], wrap: true, line_height: 22);
                    separator();
                    embed(Self::note("The balance includes all interactions. Trait costs cannot always be added in isolation."));
                }
            }
        }}
    }
    fn traits(&self) -> View {
        let mut choices = Vec::new();
        for (i, (_, title)) in RG_LRT.iter().enumerate() {
            let description = RG_LRT_DESCRIPTION[i];
            let choice = View::checkbox(
                *title,
                self.model
                    .bind(move |d| d.rg_lrt[i], move |d, f| d.rg_lrt[i] = f),
            )
            .key(format!("trait-{i}"))
            .accessible_help(description);
            choices.push(
                View::column(vec![
                    choice,
                    Self::note(description).padding([0., 8., 0., 30.]),
                ])
                .gap(2.),
            );
        }
        view! { column(gap: 22) {
            embed(Self::heading("Strengths have tradeoffs.", "Add capabilities and limitations to make the design your own."));
            panel(padding: 20, gap: 14) { embed(View::column(choices).gap(12.)); }
            embed(Self::note("Select any combination. The original rules account for trait interactions and penalties for taking many traits."));
        }}
    }
    fn environment(&self) -> View {
        let d = self.model.design.get();
        let mut axes = Vec::new();
        for (i, title) in ["Gravity", "Temperature", "Radiation"].iter().enumerate() {
            let immune = self
                .model
                .bind(move |d| d.rg_immune[i], move |d, f| d.rg_immune[i] = f);
            axes.push(view! { panel(padding: 18, gap: 10) {
                row(gap: 16, align: Align::Center) {
                    label(*title, bold: true, label_style: LabelStyle::Status, grow: 1);
                    checkbox("Immune", bind: immune, key: format!("immune-{i}"));
                }
                if !d.rg_immune[i] {
                    row(gap: 14) {
                        numeric_field("Lower bound", &self.rg_numbers[7 + i * 2], width: 180, key: format!("habitat-{i}-lo"));
                        numeric_field("Upper bound", &self.rg_numbers[8 + i * 2], width: 180, key: format!("habitat-{i}-hi"));
                    }
                    embed(Self::note(format!("Lower {}  ·  Preferred {}  ·  Upper {}", d.rg_habitat[i][0], (d.rg_habitat[i][0] + d.rg_habitat[i][1]) / 2, d.rg_habitat[i][1])));
                } else { embed(Self::note("All values on this environmental axis are habitable.")); }
            }});
        }
        view! { column(gap: 18) {
            embed(Self::heading("Where will you thrive?", "Balance the worlds you can inhabit against how quickly your population grows."));
            embed(Self::note("Environmental coordinates use the model's exact 0–100 scale. Lower and upper bounds define the habitable range."));
            embed(View::column(axes).gap(14.));
            embed(self.number(13, "Selected growth (%)", "Growth under ideal conditions. Hyper-Expansion doubles the selected rate."));
        }}
    }
    fn economy(&self) -> View {
        let d = self.model.design.get();
        view! { column(gap: 22) {
            embed(Self::heading("Build a lasting civilization.", "Choose how your population, factories and mines turn a world into a home."));
            if d.i_prt == 8 {
                panel(padding: 16) { label("Alternate Reality uses different resource rules and does not operate factories or mines. These stored choices remain visible for inspection.", wrap: true, label_style: LabelStyle::Status); }
            }
            embed(self.number(0, "Colonists per resource", "Lower values produce more resources from the same population. AR uses this selection as its efficiency divisor."));
            separator();
            label("FACTORIES", font_size: 11, label_style: LabelStyle::Status);
            embed(self.number(1, "Output", "Resources per ten operating factories."));
            embed(self.number(2, "Build cost", "Resources required to construct one factory."));
            embed(self.number(3, "Operation limit", "Factories operable per 10,000 colonists."));
            checkbox("Factories cost one less germanium", bind: self.model.bind(|d| d.f_cheaper_factories, |d, f| d.f_cheaper_factories = f));
            separator();
            label("MINES", font_size: 11, label_style: LabelStyle::Status);
            embed(self.number(4, "Output", "Nominal kilotons per ten operating mines."));
            embed(self.number(5, "Build cost", "Resources required to construct one mine."));
            embed(self.number(6, "Operation limit", "Mines operable per 10,000 colonists."));
        }}
    }
    fn research(&self) -> View {
        let mut rows = Vec::new();
        for (i, title) in [
            "Energy",
            "Weapons",
            "Propulsion",
            "Construction",
            "Electronics",
            "Biotechnology",
        ]
        .iter()
        .enumerate()
        {
            let bind = self.model.bind(
                move |d| d.rg_research[i].clone(),
                move |d, s| d.rg_research[i] = s,
            );
            rows.push(view! { column(gap: 8) {
                label(*title, bold: true);
                segmented(bind: bind, options: vec![OptionItem::new("expensive", "75% more"), OptionItem::new("standard", "Standard"), OptionItem::new("cheap", "50% less")], key: format!("research-{i}"));
            }});
        }
        view! { column(gap: 22) {
            embed(Self::heading("Choose your frontiers.", "Specialize in selected fields, or pursue a more balanced path."));
            embed(View::column(rows).gap(18.));
            checkbox("Higher starting technology in expensive fields", bind: self.model.bind(|d| d.f_start_high, |d, f| d.f_start_high = f));
            embed(self.number(14, "Research allocation (%)", "Initial resource allocation to research."));
        }}
    }
    fn review(&self) -> View {
        let d = self.model.design.get();
        let leftovers = self.model.bind(
            |d| RG_LEFTOVER[d.i_leftover].to_owned(),
            |d, s| {
                if let Some(i) = RG_LEFTOVER.iter().position(|v| *v == s) {
                    d.i_leftover = i;
                }
            },
        );
        view! { column(gap: 22) {
            embed(Self::heading("Ready for the stars?", "Review your choices and decide how to use any unspent design points."));
            row(gap: 22, align: Align::Center) {
                embed(self.sigil(d.i_sigil, 130.));
                column(gap: 8, grow: 1) {
                    label(&d.sz_plural, font_size: 26, bold: true);
                    label(RG_PRT[d.i_prt].1, label_style: LabelStyle::Status);
                    label(format!("{} additional traits · {}% selected growth", d.rg_lrt.iter().filter(|f| **f).count(), d.c_growth), wrap: true);
                }
            }
            radio_group("Spend leftover points on", bind: leftovers, options: vec![
                OptionItem::new("surface_minerals", "Surface minerals"), OptionItem::new("concentrations", "Mineral concentrations"),
                OptionItem::new("mines", "Starting mines"), OptionItem::new("factories", "Starting factories"), OptionItem::new("defenses", "Starting defenses")
            ]);
            embed(Self::note("Create society validates the current choices and keeps the completed design in this session. This prototype does not write a game file."));
            label("DESIGN SUMMARY", font_size: 11, label_style: LabelStyle::Status);
            label(format!("Environment: {}", self.habitat_summary()), wrap: true, line_height: 22);
            label(format!("Additional traits: {}", if d.rg_lrt.iter().any(|f| *f) { RG_LRT.iter().enumerate().filter(|(i, _)| d.rg_lrt[*i]).map(|(_, (_, title))| *title).collect::<Vec<_>>().join(", ") } else { "None".into() }), wrap: true, line_height: 22);
            label(format!("Population: {} colonists per resource\nFactories: {} output · {} build cost · {} operation limit\nMines: {} output · {} build cost · {} operation limit\nCheaper factories: {}", d.rg_economy[0], d.rg_economy[1], d.rg_economy[2], d.rg_economy[3], d.rg_economy[4], d.rg_economy[5], d.rg_economy[6], if d.f_cheaper_factories {"Yes"} else {"No"}), wrap: true, line_height: 22);
            label(format!("Research: {}\nResearch allocation: {}% · Higher starting technology: {}", RG_TECH.iter().zip(&d.rg_research).map(|(name, cost)| format!("{name}: {cost}")).collect::<Vec<_>>().join(", "), d.c_allocation, if d.f_start_high {"Yes"} else {"No"}), wrap: true, line_height: 22);
        }}
    }
    fn habitat_summary(&self) -> String {
        let d = self.model.design.get();
        RG_AXIS
            .iter()
            .enumerate()
            .map(|(i, axis)| {
                format!(
                    "{axis} {}",
                    if d.rg_immune[i] {
                        "immune".into()
                    } else {
                        format!("{}–{}", d.rg_habitat[i][0], d.rg_habitat[i][1])
                    }
                )
            })
            .collect::<Vec<_>>()
            .join(" · ")
    }
    fn balance(&self) -> View {
        let (value, subtitle, style): (String, String, LabelStyle) = match self.model.result.get() {
            None => ("…".into(), "Updating balance".into(), LabelStyle::Secondary),
            Some(Err(_)) => (
                "—".into(),
                "Check your choices".into(),
                LabelStyle::Standard,
            ),
            Some(Ok(r)) if r.c_points < 0 => (
                r.c_points.to_string(),
                "Points over budget".into(),
                LabelStyle::Standard,
            ),
            Some(Ok(r)) => (
                r.c_points.to_string(),
                "Design points available".into(),
                LabelStyle::Status,
            ),
        };
        view! { column(gap: 4) {
            label(value, font_size: 38, label_style: style, bold: true);
            label(subtitle, font_size: 12, label_style: LabelStyle::Secondary, wrap: true);
        }}
    }
    fn dossier(&self) -> View {
        let d = self.model.design.get();
        view! { panel(padding: 22, width: 244, shrink: 0, gap: 18) {
            label("YOUR SOCIETY", font_size: 11, label_style: LabelStyle::Status);
            embed(self.sigil(d.i_sigil, 172.));
            label(&d.sz_plural, font_size: 22, bold: true, wrap: true);
            label(RG_PRT[d.i_prt].1, label_style: LabelStyle::Status, wrap: true);
            separator(); embed(self.balance()); separator();
            if let Some(Ok(result)) = self.model.result.get() {
                label(format!("{}% ideal growth", result.c_growth), bold: true);
                label(format!("{} reference capacity", result.c_capacity), wrap: true);
            }
            embed(Self::note(self.habitat_summary()));
            label(format!("{} additional traits", d.rg_lrt.iter().filter(|f| **f).count()), label_style: LabelStyle::Secondary);
            spacer(grow: 1);
            embed(Self::note("Every choice contributes to a whole. Experiment, then revisit earlier steps to find your balance."));
        }}
    }
    fn navigation(&self, f_vertical: bool) -> View {
        let info = self.wizard.info();
        let buttons = info
            .steps
            .iter()
            .enumerate()
            .map(|(i, step)| {
                let wizard = self.wizard.clone();
                let id = step.id.clone();
                let read = wizard.clone();
                let selected = id.clone();
                View::toggle(
                    format!("{:02}  {}", i + 1, step.title),
                    Binding::new(
                        move || read.current().as_deref() == Some(selected.as_str()),
                        move |f| {
                            if f {
                                wizard.go_to(&id);
                            }
                        },
                    ),
                )
                .enabled(step.can_review)
                .key(format!("nav-{}", step.id))
                .height(42.)
            })
            .collect();
        if f_vertical {
            View::column(buttons).gap(8.).width(185.).shrink(0.)
        } else {
            View::row(buttons).gap(6.).wrap(true)
        }
    }
    pub fn view(&self) -> View {
        let model = &self.model;
        let f_source = model.f_source.get();
        let layout = model.sz_layout.get();
        let source = model.bind(|d| format!("import settings\n\n{}", d.rdl()), |_, _| {});
        let pages = vec![
            ("identity", self.identity()),
            ("primary", self.primary()),
            ("traits", self.traits()),
            ("environment", self.environment()),
            ("economy", self.economy()),
            ("research", self.research()),
            ("review", self.review()),
        ]
        .into_iter()
        .map(|(id, body)| {
            (
                id,
                view! { scroll(grow: 1, key: format!("page-scroll-{id}")) { column(padding: [8., 24., 8., 8.], gap: 18, width: percent(100.)) { embed(body); } } },
            )
        })
        .collect();
        let contents = wizard_view(&self.wizard, pages).grow(1.);
        let wizard = self.wizard.clone();
        let resume = model.f_finished.clone();
        let cancelled = model.f_cancelled.clone();
        view! { column(padding: 24, gap: 20) {
            row(gap: 18, align: Align::Center, shrink: 0) {
                column(gap: 4, grow: 1) {
                    label("S T A R S !   /   B E Y O N D", font_size: 11, label_style: LabelStyle::Status);
                    label("Society Designer", font_size: 24, bold: true);
                }
                label("PROTOTYPE", font_size: 11, label_style: LabelStyle::Secondary);
                toggle("Inspect RDL", bind: model.f_source, key: "inspect-rdl");
            }
            separator();
            if model.f_finished.get() || model.f_cancelled.get() {
                panel(grow: 1, padding: 40, gap: 24, align: Align::Center) {
                    embed(self.sigil(model.design.get().i_sigil, 190.));
                    label(if model.f_finished.get() {format!("Meet the {}.", model.design.get().sz_plural)} else {"Design paused.".into()}, font_size: 32, bold: true);
                    label(if model.f_finished.get() {"Your society is complete in this session. No game file has been saved."} else {"Your choices are still here. Resume editing or close the window."}, wrap: true);
                    button("Return to design", on_click: move || { resume.set(false); cancelled.set(false); wizard.go_to("identity"); });
                }
            } else {
                if layout != "guided" { embed(self.navigation(false)); }
                if layout == "focus" { row(gap: 16, align: Align::Center) { embed(self.balance()); label(model.design.get().sz_plural, grow: 1, font_size: 20, label_style: LabelStyle::Status); } }
                row(grow: 1, gap: 24) {
                    if layout == "guided" { embed(self.navigation(true)); }
                    column(grow: 1, basis: 0, gap: 12) {
                        if f_source {
                            embed(Self::heading("The design, expressed.", "This is the existing RDL syntax. The schema still calls its object type Race; the interface calls it Society."));
                            text_editor("RDL design", bind: source, read_only: true, line_height: 20, grow: 1, key: "rdl-source");
                            embed(wizard_navigation(&self.wizard));
                        } else { embed(contents); }
                    }
                    if layout == "guided" { embed(self.dossier()); }
                    if layout == "workbench" {
                        panel(width: 360, shrink: 0, padding: 18, gap: 14) {
                            label("LIVE MODEL", label_style: LabelStyle::Status, font_size: 11);
                            embed(self.balance());
                            text_editor("RDL design", bind: model.bind(|d| d.rdl(), |_, _| {}), read_only: true, line_height: 20, grow: 1, key: "workbench-source");
                        }
                    }
                }
            }
            row(gap: 16, align: Align::Center, shrink: 0) {
                label("LAYOUT STUDY", font_size: 10, label_style: LabelStyle::Secondary);
                segmented(bind: model.sz_layout, options: vec![OptionItem::new("guided", "Guided"), OptionItem::new("focus", "Focus"), OptionItem::new("workbench", "Workbench")], key: "layout-choice");
                spacer(grow: 1);
                label("SCARS + RDL", font_size: 11, label_style: LabelStyle::Secondary);
            }
        }}
    }
}
