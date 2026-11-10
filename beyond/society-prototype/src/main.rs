//! Standalone, disposable SCARS/RDL society design study. No game-file writes.
mod model;
mod profile;
mod ui;
use model::*;
use scars_window::{skia, ui::*};
use std::{
    rc::Rc,
    time::{Duration, Instant},
};
use ui::Designer;

type Result<T> = std::result::Result<T, Box<dyn std::error::Error>>;
fn option(args: &[String], name: &str) -> Result<Option<String>> {
    match args.iter().position(|v| v == name) {
        Some(i) => Ok(Some(
            args.get(i + 1)
                .filter(|s| !s.starts_with("--"))
                .ok_or_else(|| format!("{name} requires a value"))?
                .clone(),
        )),
        None => Ok(None),
    }
}
fn wait_for_balance(model: &Model) -> Result<()> {
    let start = Instant::now();
    while model.result.get().is_none() {
        model.poll();
        if start.elapsed() > Duration::from_secs(30) {
            return Err("Pricing timed out".into());
        }
        std::thread::sleep(Duration::from_millis(20));
    }
    Ok(())
}
fn verify(designer: &Rc<Designer>) -> Result<()> {
    let model = &designer.model;
    for (i, name) in RG_PRESETS.iter().enumerate() {
        let preset = &model.rules.rg_presets[i];
        let actual = Rules::evaluate(&model.rules.world, preset)?;
        let expected = model
            .rules
            .world
            .query("verification", &format!("{name}.points"))?
            .as_int()
            .ok_or("Missing preset price")?;
        if actual.c_points != expected {
            return Err(format!("{name}: {} != {expected}", actual.c_points).into());
        }
        println!(
            "{name}: {} points; serialized choices match source preset",
            actual.c_points
        );
    }
    let mut draft = model.design.get();
    draft.sz_name = "Étoile \\\"".into();
    Rules::evaluate(&model.rules.world, &draft)?;
    draft.rg_habitat[0] = [80, 20];
    if Rules::evaluate(&model.rules.world, &draft).is_ok() {
        return Err("Invalid habitat was accepted".into());
    }
    let copy = designer.clone();
    let host = Host::new(move || copy.view());
    model.attach(Some(host.clone()));
    host.set_theme(Theme::default());
    let renderer = Renderer::default();
    host.layout(Size::new(1360., 980.), &renderer)?;
    if !designer.wizard.next() || !designer.wizard.next() {
        return Err("Cannot open the additional traits page".into());
    }
    host.layout(Size::new(1360., 980.), &renderer)?;
    let nodes = host.snapshot()?;
    for (i, description) in RG_LRT_DESCRIPTION.iter().enumerate() {
        let key = format!("trait-{i}");
        let checkbox = nodes
            .iter()
            .find(|n| n.key.as_deref() == Some(&key))
            .ok_or_else(|| format!("Missing {key}"))?;
        if checkbox.help != *description || !nodes.iter().any(|n| n.text == *description) {
            return Err(format!("Trait description mismatch: {key}").into());
        }
    }
    designer.wizard.go_to("identity");
    host.layout(Size::new(1360., 980.), &renderer)?;
    // Exercise mounted actions, rather than only the model's setters.
    let id = host
        .snapshot()?
        .into_iter()
        .find(|n| n.key.as_deref() == Some("sigil-15"))
        .ok_or("Sigil button missing")?
        .id;
    host.activate(id)?;
    wait_for_balance(model)?;
    if model.design.get().i_sigil != 15 {
        return Err("Sigil selection failed".into());
    }
    model.edit(|d| d.c_growth = 20);
    model.edit(|d| d.c_growth = 15);
    wait_for_balance(model)?;
    if model.result.get().unwrap()?.c_points != 25 {
        return Err("Latest edit did not win".into());
    }
    for _ in 0..6 {
        if !designer.wizard.next() {
            return Err("Valid wizard did not advance".into());
        }
    }
    if !designer.wizard.finish() || !model.f_finished.get() {
        return Err("Valid design did not finish".into());
    }
    model.f_finished.set(false);
    model.edit(|d| {
        d.rg_immune = [true; 3];
        d.c_growth = 20;
    });
    wait_for_balance(model)?;
    if model.finish_valid().is_ok() || designer.wizard.finish() {
        return Err("Over-budget design finished".into());
    }
    model.edit(|d| *d = model.rules.rg_presets[0].clone());
    wait_for_balance(model)?;
    designer.wizard.go_to("identity");
    for layout in ["guided", "focus", "workbench"] {
        model.sz_layout.set(layout.into());
        for (_, id) in RG_STEPS.iter().enumerate() {
            host.layout(Size::new(1360., 980.), &renderer)?;
            if designer.wizard.current().as_deref() != Some(id.0) {
                return Err("Unexpected wizard page".into());
            }
            if id.0 != "review" && !designer.wizard.next() {
                return Err("Cannot advance layout".into());
            }
        }
        designer.wizard.go_to("identity");
    }
    host.cancel_deadline("society-pricing");
    model.attach(None);
    println!(
        "PASS: preset parity, UTF-8/escaping, invalid habitat, sigil action, latest-result pricing, finish/budget guards, seven pages in three layouts"
    );
    Ok(())
}
fn contrast(a: u32, b: u32) -> f64 {
    let luminance = |color: u32| {
        [16, 8, 0]
            .into_iter()
            .zip([0.2126, 0.7152, 0.0722])
            .map(|(shift, weight)| {
                let channel = ((color >> shift) & 255) as f64 / 255.;
                weight
                    * if channel <= 0.04045 {
                        channel / 12.92
                    } else {
                        ((channel + 0.055) / 1.055).powf(2.4)
                    }
            })
            .sum::<f64>()
    };
    let a = luminance(a);
    let b = luminance(b);
    (a.max(b) + 0.05) / (a.min(b) + 0.05)
}
fn verify_contrast(designer: &Rc<Designer>) -> Result<()> {
    let copy = designer.clone();
    let host = Host::new(move || copy.view());
    let renderer = Renderer::default();
    for (name, theme) in [
        ("light", Theme::light()),
        ("dark", Theme::default()),
        ("light again", Theme::light()),
    ] {
        host.set_theme(theme);
        for (page, keys) in [
            ("identity", vec!["sigil-0", "sigil-15"]),
            ("primary", vec!["primary-0", "primary-9", "nav-primary"]),
        ] {
            if page == "identity" {
                designer.wizard.go_to(page);
            } else {
                designer.wizard.next();
            }
            host.layout(Size::new(1360., 980.), &renderer)?;
            let nodes = host.scene()?;
            for key in keys {
                let node = nodes
                    .iter()
                    .find(|n| n.key.as_deref() == Some(key))
                    .ok_or("Contrast target missing")?;
                let background = node.style.background.unwrap_or(if node.value.selected() {
                    theme.selected
                } else {
                    theme.panel
                });
                let ratio = contrast(node.text_style.color, background);
                if ratio < 4.5 {
                    return Err(
                        format!("{name}: {key} text contrast {ratio:.2}:1 is below 4.5:1").into(),
                    );
                }
                println!("{name}: {key} contrast {ratio:.2}:1");
            }
        }
    }
    println!("PASS: selected/unselected trait and sigil contrast, including live theme changes");
    Ok(())
}
fn verify_sigils(designer: &Rc<Designer>) -> Result<()> {
    let copy = designer.clone();
    let host = Host::new(move || copy.sigil_gallery());
    let renderer = Renderer::default();
    for theme in [Theme::light(), Theme::default()] {
        host.set_theme(theme);
        for detailed in [false, true] {
            designer.model.f_detailed.set(detailed);
            for i in 0..16 {
                host.layout(Size::new(600., 600.), &renderer)?;
                let nodes = host.scene()?;
                let find = |key: String| {
                    nodes
                        .iter()
                        .find(|n| n.key.as_ref() == Some(&key))
                        .ok_or("Sigil test target missing")
                };
                let tile = find(format!("sigil-cell-{i}"))?;
                let icon = find(format!("sigil-image-{i}"))?;
                let button = find(format!("sigil-{i}"))?;
                for (area, point) in [
                    (
                        "artwork",
                        Point {
                            x: icon.bounds.x + icon.bounds.width / 2.,
                            y: icon.bounds.y + icon.bounds.height / 2.,
                        },
                    ),
                    (
                        "padding",
                        Point {
                            x: tile.bounds.x + 3.,
                            y: tile.bounds.y + 3.,
                        },
                    ),
                    (
                        "label",
                        Point {
                            x: button.text_bounds.x + button.text_bounds.width / 2.,
                            y: button.text_bounds.y + button.text_bounds.height / 2.,
                        },
                    ),
                ] {
                    // Native event dispatch lays out again after a selection
                    // rebuild; reproduce that boundary before the next hit.
                    host.layout(Size::new(600., 600.), &renderer)?;
                    if host.hit_test(point)? != Some(button.id) {
                        return Err(format!(
                            "Sigil {i}: {area} at {point:?} does not hit {:?}; tile {:?}, button {:?}, hit {:?}", button.id, tile.bounds, button.bounds, host.hit_test(point)?
                        )
                        .into());
                    }
                    for kind in [PointerKind::Down, PointerKind::Up] {
                        host.pointer(Pointer {
                            kind,
                            position: point,
                            button: 0,
                            modifiers: Modifiers::default(),
                        })?;
                    }
                    if designer.model.design.get().i_sigil != i {
                        return Err(format!("Sigil {i}: {area} did not select it").into());
                    }
                }
                designer.model.edit(|d| d.i_sigil = (i + 1) % 16);
                host.layout(Size::new(600., 600.), &renderer)?;
                host.focus(button.id)?;
                host.key(KeyEvent {
                    key: Key::Space,
                    down: true,
                    repeat: false,
                    modifiers: Modifiers::default(),
                })?;
                if designer.model.design.get().i_sigil != i {
                    return Err("Keyboard did not select the focused sigil".into());
                }
            }
        }
    }
    println!(
        "PASS: all 16 sigils select through artwork, padding, label and keyboard in both themes and artwork styles"
    );
    Ok(())
}
fn main() -> Result<()> {
    let args: Vec<String> = std::env::args().collect();
    if args.iter().any(|s| s == "--help") {
        println!(
            "Society Designer prototype\n  --render FILE.png    Render without opening a window\n  --page ID            identity / primary / traits / environment / economy / research / review\n  --layout NAME        guided / focus / workbench\n  --size WIDTHxHEIGHT  Preview dimensions (default 1360x980)\n  --light / --dark     Initial appearance (native default follows macOS)\n  --verify-sigils      Check complete tile hit targets and keyboard selection\n  --sigils             Preview just the sigil selector with --render\n  --verify-contrast    Check readable theme-dependent selections\n  --source             Inspect generated RDL\n  --basic              Use basic sigil sheet\n  --verify             Check model integration and mounted flow\n  --profile-scroll     Measure CPU scrolling on sigil and trait pages\n  --profile-force-layout  Bypass geometry reuse for comparison\n  --profile-frames N   Measured scroll iterations (30–2000; default 120)\n  --profile-max-ms N   Optional machine-specific p95 CPU budget\n  --smoke              Open the native window and quit after two seconds"
        );
        return Ok(());
    }
    let designer = Designer::new(Model::new(Rules::load()?))?;
    if args.iter().any(|s| s == "--profile-scroll") {
        let frames = option(&args, "--profile-frames")?
            .map(|s| s.parse())
            .transpose()?
            .unwrap_or(120);
        let limit = option(&args, "--profile-max-ms")?
            .map(|s| s.parse())
            .transpose()?;
        return profile::scroll(
            &designer,
            frames,
            limit,
            args.iter().any(|s| s == "--profile-force-layout"),
        );
    }
    if args.iter().any(|s| s == "--verify-sigils") {
        return verify_sigils(&designer);
    }
    if args.iter().any(|s| s == "--verify-contrast") {
        return verify_contrast(&designer);
    }
    if args.iter().any(|s| s == "--verify") {
        return verify(&designer);
    }
    if args.iter().any(|s| s == "--source") {
        designer.model.f_source.set(true);
    }
    if args.iter().any(|s| s == "--basic") {
        designer.model.f_detailed.set(false);
    }
    if let Some(layout) = option(&args, "--layout")? {
        if !["guided", "focus", "workbench"].contains(&layout.as_str()) {
            return Err("Unknown layout".into());
        }
        designer.model.sz_layout.set(layout);
    }
    if let Some(page) = option(&args, "--page")? {
        let index = RG_STEPS
            .iter()
            .position(|(id, _)| *id == page)
            .ok_or("Unknown page")?;
        for _ in 0..index {
            if !designer.wizard.next() {
                return Err("Cannot advance preview".into());
            }
        }
    }
    let mut size = (1360, 980);
    if let Some(sz) = option(&args, "--size")? {
        let (w, h) = sz.split_once('x').ok_or("Use WIDTHxHEIGHT")?;
        size = (w.parse::<i32>()?, h.parse::<i32>()?);
        if size.0 < 800 || size.1 < 600 || size.0 > 4096 || size.1 > 4096 {
            return Err("Preview dimensions must be 800x600 through 4096x4096".into());
        }
    }
    if let Some(path) = option(&args, "--render")? {
        let copy = designer.clone();
        let sigils_only = args.iter().any(|s| s == "--sigils");
        let host = Host::new(move || {
            if sigils_only {
                view! { column(padding: 24) { embed(copy.sigil_gallery()); } }
            } else {
                copy.view()
            }
        });
        host.set_theme(if args.iter().any(|s| s == "--light") {
            Theme::light()
        } else {
            Theme::default()
        });
        let renderer = Renderer::default();
        host.layout(Size::new(size.0 as f64, size.1 as f64), &renderer)?;
        let mut surface =
            skia::surfaces::raster_n32_premul(size).ok_or("Cannot create preview surface")?;
        surface
            .canvas()
            .clear(skia::Color::new(host.theme().window));
        renderer.paint(&host, surface.canvas(), 0.)?;
        let png = surface
            .image_snapshot()
            .encode(None, skia::EncodedImageFormat::PNG, None)
            .ok_or("Cannot encode preview")?;
        std::fs::write(&path, png.as_bytes())?;
        println!("Rendered {path}");
        return Ok(());
    }
    #[cfg(target_os = "macos")]
    {
        use scars_window::{Appearance, Application, Window};
        let app = Application::named("Society Designer")?;
        let copy = designer.clone();
        let window = app.open_window(
            Window::builder("Stars! Beyond · Society Designer")
                .size(size.0 as f64, size.1 as f64)
                .theme_toggle(if args.iter().any(|s| s == "--light") {
                    Appearance::Light
                } else if args.iter().any(|s| s == "--dark") {
                    Appearance::Dark
                } else {
                    Appearance::System
                })
                .full_screen_button()
                .view(move || copy.view()),
        )?;
        let host = window.view_host().ok_or("Missing window view")?;
        designer.model.attach(Some(host.clone()));
        if args.iter().any(|s| s == "--smoke") {
            app.quit_after(Duration::from_secs(2));
        }
        app.run()?;
        host.cancel_deadline("society-pricing");
        designer.model.attach(None);
    }
    #[cfg(not(target_os = "macos"))]
    return Err("SCARS currently provides its native window backend on macOS. Use --render for a CPU preview.".into());
    Ok(())
}
