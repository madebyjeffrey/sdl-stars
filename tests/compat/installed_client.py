"""Launch an installed product on private X11 and close its title normally.

Run with xvfb-run -a; never point this check at the user's desktop display.
No test hooks or application source paths are passed to the product.
"""
import argparse
import ctypes as C
import os
import subprocess
import tempfile
import time
from pathlib import Path


class Attributes(C.Structure):
    _fields_ = [('x', C.c_int), ('y', C.c_int), ('width', C.c_int), ('height', C.c_int),
                ('border_width', C.c_int), ('depth', C.c_int), ('visual', C.c_void_p),
                ('root', C.c_ulong), ('class_', C.c_int), ('bit_gravity', C.c_int),
                ('win_gravity', C.c_int), ('backing_store', C.c_int), ('backing_planes', C.c_ulong),
                ('backing_pixel', C.c_ulong), ('save_under', C.c_int), ('colormap', C.c_ulong),
                ('map_installed', C.c_int), ('map_state', C.c_int), ('all_event_masks', C.c_long),
                ('your_event_mask', C.c_long), ('do_not_propagate_mask', C.c_long),
                ('override_redirect', C.c_int), ('screen', C.c_void_p)]


class ClientMessage(C.Structure):
    _fields_ = [('type', C.c_int), ('serial', C.c_ulong), ('send_event', C.c_int),
                ('display', C.c_void_p), ('window', C.c_ulong), ('message_type', C.c_ulong),
                ('format', C.c_int), ('data', C.c_long * 5)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('command', nargs=argparse.REMAINDER)
    args = parser.parse_args()
    if not args.command:
        parser.error('an installed command is required')
    if 'xvfb-run.' not in os.environ.get('XAUTHORITY', ''):
        raise RuntimeError('Run this check with xvfb-run on a private display')
    lib = C.CDLL('libX11.so.6')
    lib.XOpenDisplay.argtypes = [C.c_char_p]; lib.XOpenDisplay.restype = C.c_void_p
    display = lib.XOpenDisplay(None)
    if not display:
        raise RuntimeError('No private X11 display')
    lib.XDefaultRootWindow.argtypes = [C.c_void_p]; lib.XDefaultRootWindow.restype = C.c_ulong
    lib.XQueryTree.argtypes = [C.c_void_p, C.c_ulong, C.POINTER(C.c_ulong), C.POINTER(C.c_ulong), C.POINTER(C.POINTER(C.c_ulong)), C.POINTER(C.c_uint)]
    lib.XFetchName.argtypes = [C.c_void_p, C.c_ulong, C.POINTER(C.c_void_p)]
    lib.XGetWindowAttributes.argtypes = [C.c_void_p, C.c_ulong, C.POINTER(Attributes)]
    lib.XFree.argtypes = [C.c_void_p]
    # SDL and the window manager can destroy a startup window between tree
    # discovery and inspection. Xlib's default BadWindow handler exits the
    # Python process, bypassing owned-process cleanup; treat only that race
    # as an absent window and surface other protocol errors normally below.
    errors = []
    handler_type = C.CFUNCTYPE(C.c_int, C.c_void_p, C.c_void_p)
    def handle_error(display_error, event):
        class Error(C.Structure):
            _fields_ = [('type', C.c_int), ('display', C.c_void_p), ('resource', C.c_ulong),
                        ('serial', C.c_ulong), ('code', C.c_ubyte), ('request', C.c_ubyte),
                        ('minor', C.c_ubyte)]
        error = C.cast(event, C.POINTER(Error)).contents
        if error.code != 3:
            errors.append((error.code, error.request))
        return 0
    handler = handler_type(handle_error)
    lib.XSetErrorHandler.argtypes = [handler_type]
    lib.XSetErrorHandler(handler)
    lib.XInternAtom.argtypes = [C.c_void_p, C.c_char_p, C.c_int]; lib.XInternAtom.restype = C.c_ulong
    lib.XSendEvent.argtypes = [C.c_void_p, C.c_ulong, C.c_int, C.c_long, C.c_void_p]
    lib.XFlush.argtypes = [C.c_void_p]; lib.XCloseDisplay.argtypes = [C.c_void_p]

    def windows(window):
        root, parent, children, count = C.c_ulong(), C.c_ulong(), C.POINTER(C.c_ulong)(), C.c_uint()
        if not lib.XQueryTree(display, window, C.byref(root), C.byref(parent), C.byref(children), C.byref(count)):
            return []
        result = list(children[:count.value]); lib.XFree(children)
        return result + [descendant for child in result for descendant in windows(child)]

    with tempfile.TemporaryDirectory(prefix='stars-installed-') as directory:
        env = dict(os.environ, SDL_VIDEODRIVER='x11', SDL_AUDIODRIVER='dummy',
                   STARS_SDL_RENDERER='software', STARS_SETTINGS_DIR=directory)
        log = Path(directory) / 'client.log'
        manager = subprocess.Popen(['openbox'], env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        process = None
        try:
            with log.open('w') as output:
                process = subprocess.Popen(args.command, cwd=directory, env=env, stdout=output, stderr=output)
                found = None
                deadline = time.monotonic() + 45
                while time.monotonic() < deadline and process.poll() is None:
                    for window in windows(lib.XDefaultRootWindow(display)):
                        name, attributes = C.c_void_p(), Attributes()
                        if lib.XFetchName(display, window, C.byref(name)) and name.value:
                            text = C.string_at(name).decode('utf-8', errors='replace'); lib.XFree(name)
                            if 'Stars!' in text and lib.XGetWindowAttributes(display, window, C.byref(attributes)) and attributes.map_state == 2 and attributes.width >= 400 and attributes.height >= 200:
                                found = window
                                print(f'Installed title: {text}, {attributes.width}x{attributes.height}', flush=True)
                                break
                    if found:
                        break
                    if errors:
                        raise RuntimeError(f'X11 protocol errors: {errors}')
                    time.sleep(0.1)
                if errors:
                    raise RuntimeError(f'X11 protocol errors: {errors}')
                if not found:
                    raise RuntimeError('Installed title did not appear: ' + log.read_text())
                event = ClientMessage(33, 0, 1, display, found,
                                      lib.XInternAtom(display, b'WM_PROTOCOLS', 0), 32)
                event.data[0] = lib.XInternAtom(display, b'WM_DELETE_WINDOW', 0)
                buffer = C.create_string_buffer(24 * C.sizeof(C.c_long))
                C.memmove(buffer, C.byref(event), C.sizeof(event))
                lib.XSendEvent(display, found, 0, 0, buffer); lib.XFlush(display)
                result = process.wait(timeout=20)
                if result != 0:
                    raise RuntimeError(f'Installed exit {result}: ' + log.read_text())
                print('Installed client opened and closed normally outside the repository.')
        finally:
            if process and process.poll() is None:
                process.terminate(); process.wait(timeout=10)
            manager.terminate(); manager.wait(timeout=10)
            lib.XCloseDisplay(display)


if __name__ == '__main__':
    main()
