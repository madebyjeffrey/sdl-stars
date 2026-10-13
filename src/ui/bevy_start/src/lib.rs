//! Bevy owns only rendering and button state. SDL owns the window/event loop;
//! the C caller runs the original actions after each update returns.
use bevy::{
    app::PluginsState,
    platform::sync::Mutex,
    prelude::*,
    render::pipelined_rendering::PipelinedRenderingPlugin,
    ui::UiGlobalTransform,
    window::{PrimaryWindow, RawHandleWrapper, RawHandleWrapperHolder, WindowWrapper},
};
use raw_window_handle::{
    AppKitDisplayHandle, AppKitWindowHandle, DisplayHandle, HandleError, HasDisplayHandle,
    HasWindowHandle, RawDisplayHandle, RawWindowHandle, WaylandDisplayHandle, WaylandWindowHandle,
    Win32WindowHandle, WindowHandle, WindowsDisplayHandle, XlibDisplayHandle, XlibWindowHandle,
};
use std::{
    ffi::{CStr, c_char},
    num::NonZeroIsize,
    panic::{AssertUnwindSafe, catch_unwind},
    ptr::NonNull,
    sync::Arc,
};

#[repr(C)]
pub struct BevyWindow {
    gr_backend: u32,
    h_window: usize,
    h_display: usize,
    i_screen: i32,
    dx: u32,
    dy: u32,
    r_scale: f32,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct BevyInput {
    dx: u32,
    dy: u32,
    r_scale: f32,
    x: f32,
    y: f32,
    f_press: i32,
    f_release: i32,
    id_key: i32,
}

// The C caller retains the SDL window until the App (including its renderer)
// has been dropped. Store integer handles so only the borrowing methods below
// expose the native pointers, on the main thread.
struct NativeWindow(BevyWindow);

impl HasWindowHandle for NativeWindow {
    fn window_handle(&self) -> Result<WindowHandle<'_>, HandleError> {
        let raw = match self.0.gr_backend {
            1 => RawWindowHandle::AppKit(AppKitWindowHandle::new(
                NonNull::new(self.0.h_window as *mut _).ok_or(HandleError::Unavailable)?,
            )),
            2 => RawWindowHandle::Win32(Win32WindowHandle::new(
                NonZeroIsize::new(self.0.h_window as isize).ok_or(HandleError::Unavailable)?,
            )),
            3 => RawWindowHandle::Xlib(XlibWindowHandle::new(self.0.h_window as _)),
            4 => RawWindowHandle::Wayland(WaylandWindowHandle::new(
                NonNull::new(self.0.h_window as *mut _).ok_or(HandleError::Unavailable)?,
            )),
            _ => return Err(HandleError::NotSupported),
        };
        // SAFETY: handles come from a live SDL window owned by the C caller.
        Ok(unsafe { WindowHandle::borrow_raw(raw) })
    }
}

impl HasDisplayHandle for NativeWindow {
    fn display_handle(&self) -> Result<DisplayHandle<'_>, HandleError> {
        let raw = match self.0.gr_backend {
            1 => RawDisplayHandle::AppKit(AppKitDisplayHandle::new()),
            2 => RawDisplayHandle::Windows(WindowsDisplayHandle::new()),
            3 => RawDisplayHandle::Xlib(XlibDisplayHandle::new(
                NonNull::new(self.0.h_display as *mut _),
                self.0.i_screen,
            )),
            4 => RawDisplayHandle::Wayland(WaylandDisplayHandle::new(
                NonNull::new(self.0.h_display as *mut _).ok_or(HandleError::Unavailable)?,
            )),
            _ => return Err(HandleError::NotSupported),
        };
        // SAFETY: the display outlives the SDL window and this App.
        Ok(unsafe { DisplayHandle::borrow_raw(raw) })
    }
}

#[derive(Component)]
struct StartButton(i32);

#[derive(Resource)]
struct StartState {
    input: BevyInput,
    f_continue: bool,
    id_focus: i32,
    id_pressed: i32,
    id_action: i32,
}

struct StartScreen {
    app: App,
    window: Entity,
}

fn button_input(
    mut state: ResMut<StartState>,
    mut buttons: Query<(
        &StartButton,
        &ComputedNode,
        &UiGlobalTransform,
        &mut BackgroundColor,
        &mut BorderColor,
    )>,
) {
    let input = state.input;
    if input.id_key == -2 || input.id_key == -3 {
        let step = if input.id_key == -2 { 1 } else { 3 };
        state.id_focus = (state.id_focus + step) % 4;
        if state.id_focus == 2 && !state.f_continue {
            state.id_focus = (state.id_focus + step) % 4;
        }
    }
    let id_key = if input.id_key == -4 {
        state.id_focus
    } else {
        input.id_key
    };
    if (0..4).contains(&id_key) && (id_key != 2 || state.f_continue) {
        state.id_action = id_key;
    }
    for (button, node, transform, mut color, mut border) in &mut buttons {
        let enabled = button.0 != 2 || state.f_continue;
        let hovered = node.contains_point(*transform, Vec2::new(input.x, input.y) * input.r_scale);
        if enabled && hovered && input.f_press != 0 {
            state.id_pressed = button.0;
            state.id_focus = button.0;
        }
        if enabled && hovered && input.f_release != 0 && state.id_pressed == button.0 {
            state.id_action = button.0;
        }
        *color = if !enabled {
            Color::srgb(0.08, 0.10, 0.16)
        } else if hovered || state.id_focus == button.0 {
            Color::srgb(0.12, 0.27, 0.43)
        } else {
            Color::srgb(0.07, 0.14, 0.23)
        }
        .into();
        border.set_all(if enabled && state.id_focus == button.0 {
            Color::srgb(0.5, 0.8, 1.0)
        } else {
            Color::srgb(0.19, 0.31, 0.45)
        });
    }
    if input.f_release != 0 {
        state.id_pressed = -1;
    }
}

fn text(
    commands: &mut ChildSpawner,
    font: &Handle<Font>,
    label: impl Into<String>,
    size: f32,
    color: Color,
) {
    commands.spawn((
        Text::new(label),
        TextFont {
            font: font.clone().into(),
            font_size: FontSize::Px(size),
            ..default()
        },
        TextColor(color),
    ));
}

fn create_screen(
    window: BevyWindow,
    labels: [String; 4],
    version: String,
    f_continue: bool,
) -> Result<StartScreen, HandleError> {
    let mut app = App::new();
    app.add_plugins(WindowPlugin {
        primary_window: None,
        ..default()
    });
    let mut resolution = bevy::window::WindowResolution::new(window.dx, window.dy);
    resolution.set_scale_factor(window.r_scale);
    let native = WindowWrapper::new(NativeWindow(window));
    let raw = RawHandleWrapper::new(&native)?;
    let entity = app
        .world_mut()
        .spawn((
            Window {
                resolution,
                ..default()
            },
            PrimaryWindow,
            raw.clone(),
            RawHandleWrapperHolder(Arc::new(Mutex::new(Some(raw)))),
        ))
        .id();
    // A synchronous renderer ensures the borrowed SDL window has no frames
    // in flight when StarsBevyDestroy returns. SDL remains the only event loop.
    app.add_plugins(
        DefaultPlugins
            .build()
            .disable::<WindowPlugin>()
            .disable::<PipelinedRenderingPlugin>()
            .disable::<bevy::log::LogPlugin>()
            .disable::<bevy::app::PanicHandlerPlugin>()
            .disable::<bevy::app::TerminalCtrlCHandlerPlugin>(),
    )
    .insert_resource(ClearColor(Color::srgb(0.015, 0.025, 0.055)))
    .insert_resource(StartState {
        input: BevyInput {
            dx: 800,
            dy: 600,
            r_scale: 1.0,
            x: -100.0,
            y: -100.0,
            f_press: 0,
            f_release: 0,
            id_key: -1,
        },
        f_continue,
        id_focus: 0,
        id_pressed: -1,
        id_action: -1,
    })
    .add_systems(Update, button_input);
    let font = app
        .world_mut()
        .resource_mut::<Assets<Font>>()
        .add(Font::from_bytes(
            include_bytes!("../../../../res/fonts/LiberationSans-Regular.ttf").to_vec(),
        ));
    app.world_mut().spawn(Camera2d);
    app.world_mut()
        .spawn(Node {
            width: percent(100),
            height: percent(100),
            flex_direction: FlexDirection::Column,
            align_items: AlignItems::Center,
            justify_content: JustifyContent::Center,
            row_gap: px(22),
            padding: UiRect::all(px(28)),
            ..default()
        })
        .with_children(|parent| {
            text(parent, &font, "Stars!", 100.0, Color::srgb(0.85, 0.92, 1.0));
            text(parent, &font, version, 18.0, Color::srgb(0.55, 0.68, 0.83));
            parent
                .spawn(Node {
                    width: percent(100),
                    justify_content: JustifyContent::Center,
                    flex_wrap: FlexWrap::Wrap,
                    column_gap: px(14),
                    row_gap: px(14),
                    margin: UiRect::top(px(45)),
                    ..default()
                })
                .with_children(|row| {
                    for (id, label) in labels.into_iter().enumerate() {
                        row.spawn((
                            StartButton(id as i32),
                            Node {
                                width: px(160),
                                height: px(54),
                                border: UiRect::all(px(1)),
                                border_radius: BorderRadius::all(px(8)),
                                align_items: AlignItems::Center,
                                justify_content: JustifyContent::Center,
                                ..default()
                            },
                            BackgroundColor(Color::srgb(0.07, 0.14, 0.23)),
                            BorderColor::all(Color::srgb(0.19, 0.31, 0.45)),
                        ))
                        .with_children(|button| {
                            text(
                                button,
                                &font,
                                label.replace('&', ""),
                                17.0,
                                if id == 2 && !f_continue {
                                    Color::srgb(0.3, 0.36, 0.45)
                                } else {
                                    Color::srgb(0.85, 0.92, 1.0)
                                },
                            )
                        });
                    }
                });
        });
    while app.plugins_state() == PluginsState::Adding {
        bevy::tasks::tick_global_task_pools_on_main_thread();
        std::thread::yield_now();
    }
    app.finish();
    app.cleanup();
    Ok(StartScreen {
        app,
        window: entity,
    })
}

/// All exports are called synchronously on the main thread with live C pointers.
/// Panics are contained here so unwinding never crosses the C ABI.
///
/// # Safety
/// The SDL window/display must remain alive until StarsBevyDestroy returns.
/// Labels must point to four valid NUL-terminated strings, and version to one.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn StarsBevyCreate(
    window: *const BevyWindow,
    labels: *const *const c_char,
    version: *const c_char,
    f_continue: i32,
) -> *mut std::ffi::c_void {
    catch_unwind(AssertUnwindSafe(|| {
        // SAFETY: the caller supplies four NUL-terminated labels and a version.
        let labels = std::array::from_fn(|i| {
            unsafe { CStr::from_ptr(*labels.add(i)) }
                .to_string_lossy()
                .into_owned()
        });
        let version = unsafe { CStr::from_ptr(version) }
            .to_string_lossy()
            .into_owned();
        let window = unsafe { std::ptr::read(window) };
        create_screen(window, labels, version, f_continue != 0)
            .map(|screen| Box::into_raw(Box::new(screen)).cast())
            .unwrap_or(std::ptr::null_mut())
    }))
    .unwrap_or(std::ptr::null_mut())
}

/// Advance one frame and return the selected original action ID, or -1.
///
/// # Safety
/// Screen must be a live pointer from StarsBevyCreate, and input valid for this
/// call. Calls must stay on the creating thread and must not overlap.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn StarsBevyUpdate(
    screen: *mut std::ffi::c_void,
    input: *const BevyInput,
) -> i32 {
    catch_unwind(AssertUnwindSafe(|| {
        // SAFETY: the opaque pointer is created/destroyed only by this module.
        let screen = unsafe { &mut *screen.cast::<StartScreen>() };
        let input = unsafe { *input };
        let resolution = &screen
            .app
            .world()
            .get::<Window>(screen.window)
            .unwrap()
            .resolution;
        if resolution.physical_width() != input.dx.max(1)
            || resolution.physical_height() != input.dy.max(1)
            || resolution.scale_factor() != input.r_scale
        {
            let mut window = screen
                .app
                .world_mut()
                .get_mut::<Window>(screen.window)
                .unwrap();
            window.resolution.set_scale_factor(input.r_scale);
            window
                .resolution
                .set_physical_resolution(input.dx.max(1), input.dy.max(1));
        }
        let mut state = screen.app.world_mut().resource_mut::<StartState>();
        state.input = input;
        state.id_action = -1;
        screen.app.update();
        screen.app.world().resource::<StartState>().id_action
    }))
    .unwrap_or(-2)
}

/// Release the renderer before the C caller frees its native window.
///
/// # Safety
/// Screen must be null or a live pointer from StarsBevyCreate, passed here only
/// once, on the creating thread with no update in progress.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn StarsBevyDestroy(screen: *mut std::ffi::c_void) {
    if !screen.is_null() {
        let _ = catch_unwind(AssertUnwindSafe(|| {
            // SAFETY: C destroys each successfully created screen exactly once.
            drop(unsafe { Box::from_raw(screen.cast::<StartScreen>()) });
        }));
    }
}
