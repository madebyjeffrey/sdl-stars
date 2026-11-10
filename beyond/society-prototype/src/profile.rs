//! Deterministic CPU scroll workload using the real wizard, layout and renderer.
//! This isolates CPU stages; it does not measure Metal presentation or AppKit/AX work.
use crate::{Result, ui::Designer};
use scars_window::{skia, ui::*};
use std::{rc::Rc, time::Instant};

fn percentile(rg: &[f64], fraction: f64) -> f64 {
    let mut sorted = rg.to_vec();
    sorted.sort_by(f64::total_cmp);
    sorted[((sorted.len() - 1) as f64 * fraction).round() as usize]
}

pub fn scroll(
    designer: &Rc<Designer>,
    c_frames: usize,
    limit: Option<f64>,
    force: bool,
) -> Result<()> {
    if !(30..=2000).contains(&c_frames) {
        return Err("Use 30–2000 profiling frames".into());
    }
    if limit.is_some_and(|max| !max.is_finite() || max <= 0.) {
        return Err("Use a positive, finite profiling budget in milliseconds".into());
    }
    let renderer = Renderer::default();
    let layout = |host: &Host, viewport| {
        if force {
            host.layout(viewport, &renderer)
        } else {
            host.layout_if_needed(viewport, &renderer)
        }
    };
    let viewport = Size::new(1360., 800.);
    let mut surface =
        skia::surfaces::raster_n32_premul((1360, 800)).ok_or("Cannot create profiling surface")?;
    println!(
        "Layout mode: {}",
        if force {
            "forced (comparison)"
        } else {
            "reuse settled geometry"
        }
    );
    let mut f_slow = false;
    for page in ["identity", "traits"] {
        designer.wizard.go_to("identity");
        if page == "traits" {
            designer.wizard.next();
            designer.wizard.next();
        }
        let copy = designer.clone();
        let host = Host::new(move || copy.view());
        layout(&host, viewport)?;
        let nodes = host.scene()?;
        let scroll = nodes
            .iter()
            .find(|n| n.key.as_deref() == Some(&format!("page-scroll-{page}")))
            .ok_or("Missing page scroll view")?;
        let id = scroll.id;
        let bounds = scroll.bounds;
        let point = Point {
            x: bounds.x + bounds.width / 2.,
            y: bounds.y + bounds.height / 2.,
        };
        let mut delta = 12.;
        let mut rg_input_layout = Vec::new();
        let mut rg_scroll = Vec::new();
        let mut rg_notification_layout = Vec::new();
        let mut rg_draw_layout = Vec::new();
        let mut rg_paint = Vec::new();
        let mut rg_total = Vec::new();
        let mut generation = 0;
        let mut c_images = 0;
        for i in 0..c_frames + 20 {
            if i == 20 {
                generation = host.generation();
                c_images = renderer.cache_stats().images;
            }
            let start = Instant::now();
            // Mounted::run synchronizes before input. Its draw callback currently
            // synchronizes once without an application underlay. A
            // coalesced accessibility notification also synchronizes geometry.
            layout(&host, viewport)?;
            let input_layout = Instant::now();
            let info = host.scroll_info(id)?.ok_or("Missing scroll geometry")?;
            if info.max.y <= 0. {
                return Err("Profiling page does not overflow".into());
            }
            if info.offset.y >= info.max.y {
                delta = -12.;
            }
            if info.offset.y <= 0. {
                delta = 12.;
            }
            if !host.scroll_at(point, Point { x: 0., y: delta })? {
                return Err("Profiling scroll did not move".into());
            }
            host.pointer_cursor(point)?;
            let scrolled = Instant::now();
            layout(&host, viewport)?;
            let notifications = Instant::now();
            layout(&host, viewport)?;
            let laid_out = Instant::now();
            surface
                .canvas()
                .clear(skia::Color::new(host.theme().window));
            renderer.paint(&host, surface.canvas(), 0.)?;
            let painted = Instant::now();
            if i >= 20 {
                rg_input_layout.push((input_layout - start).as_secs_f64() * 1000.);
                rg_scroll.push((scrolled - input_layout).as_secs_f64() * 1000.);
                rg_notification_layout.push((notifications - scrolled).as_secs_f64() * 1000.);
                rg_draw_layout.push((laid_out - notifications).as_secs_f64() * 1000.);
                rg_paint.push((painted - laid_out).as_secs_f64() * 1000.);
                rg_total.push((painted - start).as_secs_f64() * 1000.);
            }
        }
        println!(
            "{page}: {c_frames} measured frames; {} nodes; {} view rebuilds; image cache {c_images} → {}",
            nodes.len(),
            host.generation() - generation,
            renderer.cache_stats().images
        );
        for (name, samples) in [
            ("input layout", &rg_input_layout),
            ("scroll routing", &rg_scroll),
            ("notification layout", &rg_notification_layout),
            ("draw layout", &rg_draw_layout),
            ("CPU paint", &rg_paint),
            ("total CPU workload", &rg_total),
        ] {
            println!(
                "  {name:20} p50 {:8.3} ms  p95 {:8.3} ms",
                percentile(samples, 0.50),
                percentile(samples, 0.95)
            );
        }
        f_slow |= limit.is_some_and(|max| percentile(&rg_total, 0.95) > max);
        if host.generation() != generation {
            return Err("Scrolling rebuilt the view tree".into());
        }
        if renderer.cache_stats().images != c_images {
            return Err("Image cache changed after scroll warmup".into());
        }
    }
    if f_slow {
        return Err(format!(
            "Scroll workload exceeded the requested {:.2} ms p95 budget",
            limit.unwrap()
        )
        .into());
    }
    Ok(())
}
