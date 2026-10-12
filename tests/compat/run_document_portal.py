"""Receive a real OpenFile descriptor on a private bus and export only that file."""
import argparse
import os
import signal
import shutil
import subprocess
import tempfile
from pathlib import Path
import dbus
import dbus.service
from dbus.mainloop.glib import DBusGMainLoop
from gi.repository import GLib


parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', required=True, type=Path)
parser.add_argument('command', nargs=argparse.REMAINDER)
args = parser.parse_args()
assert os.environ.get('STARS_TEST_PRIVATE_BUS') == '1'
DBusGMainLoop(set_as_default=True)
loop = GLib.MainLoop()
bus = dbus.SessionBus()
name = dbus.service.BusName('org.freedesktop.portal.Desktop', bus)
received = []


class Portal(dbus.service.Object):
    @dbus.service.method('org.freedesktop.portal.OpenURI', in_signature='sha{sv}', out_signature='o')
    def OpenFile(self, parent, descriptor, options):
        assert parent == '' or str(parent).startswith(('x11:', 'wayland:'))
        args.output.parent.mkdir(parents=True, exist_ok=True)
        with os.fdopen(descriptor.take(), 'rb') as source, args.output.open('wb') as target:
            shutil.copyfileobj(source, target)
        received.append(args.output.stat().st_size)
        return dbus.ObjectPath('/org/freedesktop/portal/desktop/request/1/stars_document')


portal = Portal(bus, '/org/freedesktop/portal/desktop')
with tempfile.TemporaryDirectory(prefix='stars-document-portal-') as directory:
    root = Path(directory)
    fallback = root / 'xdg-open'
    fallback.write_text('#!/bin/sh\nexit 99\n'); fallback.chmod(0o755)
    # Isolate guide exports without hiding the host's user Flatpak install.
    flatpak_user = os.environ.get('FLATPAK_USER_DIR', str(Path(os.environ.get('XDG_DATA_HOME', Path.home() / '.local/share')) / 'flatpak'))
    env = dict(os.environ, PATH=str(root) + ':' + os.environ['PATH'], XDG_DATA_HOME=directory,
               FLATPAK_USER_DIR=flatpak_user,
               SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy', STARS_SDL_RENDERER='software')
    command = list(args.command)
    if command and command[0] == '--': command.pop(0)
    client = subprocess.Popen(command, cwd=directory, env=env, start_new_session=True)
    def poll():
        if client.poll() is None: return True
        loop.quit(); return False
    def timeout():
        os.killpg(client.pid, signal.SIGKILL); loop.quit(); return False
    GLib.timeout_add(20, poll)
    GLib.timeout_add_seconds(120, timeout)
    try:
        loop.run()
        assert client.wait() == 0
        assert len(received) == 1 and received[0] > 0
    finally:
        if client.poll() is None:
            os.killpg(client.pid, signal.SIGKILL); client.wait()

print(f'OpenFile portal received exactly one completed document: {args.output}')
