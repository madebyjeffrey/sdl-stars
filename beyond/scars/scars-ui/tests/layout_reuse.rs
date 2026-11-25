use scars_ui::*;
use std::{cell::Cell, rc::Rc};

#[derive(Default)]
struct Metrics(Cell<usize>);
impl Measure for Metrics {
    fn text(&self, text: &str, style: TextStyle, _: f64) -> TextLayout {
        self.0.set(self.0.get() + 1);
        TextLayout {
            lines: vec![],
            size: Size::new(text.len() as f64 * 8., style.size + 6.),
            baseline: style.size,
            placeholders: vec![],
            hit_test: Rc::new(|_| 0),
            range_rects: Rc::new(|_| vec![]),
        }
    }
    fn image(&self, _: &Bitmap) -> Result<Size, Error> {
        Ok(Size::new(1., 1.))
    }
}
fn keyed(host: &Host, key: &str) -> Node {
    host.snapshot()
        .unwrap()
        .into_iter()
        .find(|n| n.key.as_deref() == Some(key))
        .unwrap()
}

#[test]
fn checkbox_label_is_optically_centered_with_its_control() {
    let checked = State::new(false);
    let host = Host::new({
        let checked = checked.clone();
        move || view! { checkbox("Checkbox label", bind: checked, key: "check"); }
    });
    let metrics = Metrics::default();
    host.layout(Size::new(240., 160.), &metrics).unwrap();
    let node = keyed(&host, "check");
    let text_center = node.text_bounds.y + node.text_bounds.height / 2.;
    let control_center = node.bounds.y + node.bounds.height / 2.;
    assert!((text_center - control_center - 3.).abs() < 0.01);
}

#[test]
fn input_notification_and_draw_share_settled_geometry() {
    let host = Host::new(|| {
        view! { scroll(key: "scroll", height: 100, gap: 0) {
            button("First", key: "first", height: 60, on_click: || {});
            button("Second", key: "second", height: 60, on_click: || {});
            label("Tail", height: 300);
        }}
    });
    let metrics = Metrics::default();
    let size = Size::new(240., 160.);
    host.layout_if_needed(size, &metrics).unwrap();
    let initial = metrics.0.get();
    // The native adapter syncs before input, notifications, and twice per draw.
    for _ in 0..4 {
        host.layout_if_needed(size, &metrics).unwrap();
    }
    assert_eq!(
        metrics.0.get(),
        initial,
        "unchanged sync must not remeasure"
    );
    let first = keyed(&host, "first");
    assert!(host.scroll(Point { x: 20., y: 20. }, 60.).unwrap());
    host.layout_if_needed(size, &metrics).unwrap();
    assert!(metrics.0.get() > initial);
    assert_eq!(keyed(&host, "first").bounds.y, first.bounds.y - 60.);
    assert_eq!(
        host.hit_test(Point { x: 20., y: 20. }).unwrap(),
        Some(keyed(&host, "second").id)
    );
    let scrolled = metrics.0.get();
    for _ in 0..3 {
        host.layout_if_needed(size, &metrics).unwrap();
    }
    assert_eq!(
        metrics.0.get(),
        scrolled,
        "draw must reuse scrolled geometry"
    );
}

#[test]
fn resize_theme_model_and_forced_measurement_invalidate_geometry() {
    let text = State::new(String::from("Before"));
    let host = Host::new({
        let text = text.clone();
        move || View::label(text.get()).key("label")
    });
    let metrics = Metrics::default();
    let size = Size::new(240., 160.);
    host.layout_if_needed(size, &metrics).unwrap();
    let initial = metrics.0.get();
    host.layout_if_needed(Size::new(320., 160.), &metrics)
        .unwrap();
    assert!(metrics.0.get() > initial);
    let initial = metrics.0.get();
    host.set_theme(Theme::light());
    host.layout_if_needed(Size::new(320., 160.), &metrics)
        .unwrap();
    assert!(metrics.0.get() > initial);
    text.set("After".into());
    host.snapshot().unwrap(); // A query may rebuild before the next native sync.
    host.layout_if_needed(size, &metrics).unwrap();
    assert_eq!(keyed(&host, "label").text, "After");
    assert!(keyed(&host, "label").bounds.width > 0.);
    let initial = metrics.0.get();
    host.layout(size, &metrics).unwrap(); // Explicitly force changed provider metrics.
    assert!(metrics.0.get() > initial);
    assert!(
        host.layout_if_needed(Size::new(f64::NAN, 160.), &metrics)
            .is_err()
    );
    host.layout_if_needed(size, &metrics).unwrap();
}

#[test]
fn borrowed_state_projections_track_changes_without_requiring_clone() {
    #[derive(PartialEq)]
    struct Data {
        title: String,
        unused: Vec<u8>,
    }
    let state = State::new(Data {
        title: "Before".into(),
        unused: vec![0; 4096],
    });
    let host = Host::new({
        let state = state.clone();
        move || View::label(state.with(|d| d.title.clone())).key("label")
    });
    let metrics = Metrics::default();
    let size = Size::new(240., 160.);
    host.layout_if_needed(size, &metrics).unwrap();
    let generation = host.generation();
    state.set(Data {
        title: "After".into(),
        unused: vec![],
    });
    host.layout_if_needed(size, &metrics).unwrap();
    assert_eq!(host.generation(), generation + 1);
    assert_eq!(keyed(&host, "label").text, "After");
}

#[test]
fn composition_invalidates_geometry_without_publishing_the_binding() {
    let text = State::new(String::from("Original"));
    let host = Host::new({
        let text = text.clone();
        move || View::text_field("Name", text.binding()).key("field")
    });
    let metrics = Metrics::default();
    let size = Size::new(240., 160.);
    host.layout_if_needed(size, &metrics).unwrap();
    host.focus(keyed(&host, "field").id).unwrap();
    host.layout_if_needed(size, &metrics).unwrap();
    let initial = metrics.0.get();
    host.mark_text("Composing a longer name", 0..0, Some(0..8))
        .unwrap();
    assert_eq!(text.get(), "Original");
    host.layout_if_needed(size, &metrics).unwrap();
    assert!(metrics.0.get() > initial);
    assert!(host.text_input().unwrap().unwrap().marked.is_some());
    let composed = metrics.0.get();
    host.layout_if_needed(size, &metrics).unwrap();
    assert_eq!(metrics.0.get(), composed);
}
