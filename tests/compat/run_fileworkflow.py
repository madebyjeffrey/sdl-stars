"""Use a private portal to select real copied game/race files; no game-state injection."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import dbus
import dbus.service
from dbus.mainloop.glib import DBusGMainLoop
from gi.repository import GLib

parser = argparse.ArgumentParser()
parser.add_argument('--client', required=True)
args = parser.parse_args()
assert os.environ.get('STARS_TEST_PRIVATE_BUS') == '1'
DBusGMainLoop(set_as_default=True)
bus = dbus.SessionBus()
name = dbus.service.BusName('org.freedesktop.portal.Desktop', bus)
loop = GLib.MainLoop()
requests = []
calls = []

class Request(dbus.service.Object):
    @dbus.service.signal('org.freedesktop.portal.Request', signature='ua{sv}')
    def Response(self, status, result): pass
    @dbus.service.method('org.freedesktop.portal.Request')
    def Close(self): pass

class Chooser(dbus.service.Object):
    def choose(self, parent, title, options, sender, save):
        assert options['modal'] and options['filters']
        assert parent == '' or str(parent).startswith(('x11:', 'wayland:'))
        calls.append(save)
        assert calls == [False, False, True, True, False, True][:len(calls)]
        if len(calls) == 6:
            assert options['current_name'] == race.name
        path = '/org/freedesktop/portal/desktop/request/' + str(sender)[1:].replace('.', '_') + '/' + str(options['handle_token'])
        request = Request(bus, path)
        requests.append(request)
        cancel = len(calls) in (1, 3)
        uri = race_repeat.as_uri() if len(calls) == 6 else race.as_uri() if save or len(calls) == 5 else game.as_uri()
        def respond():
            request.Response(dbus.UInt32(1 if cancel else 0), dbus.Dictionary(
                {} if cancel else {'uris': dbus.Array([uri], signature='s')}, signature='sv'))
            return False
        GLib.timeout_add(60, respond)
        return dbus.ObjectPath(path)
    @dbus.service.method('org.freedesktop.portal.FileChooser', in_signature='ssa{sv}', out_signature='o', sender_keyword='sender')
    def OpenFile(self, parent, title, options, sender): return self.choose(parent, title, options, sender, False)
    @dbus.service.method('org.freedesktop.portal.FileChooser', in_signature='ssa{sv}', out_signature='o', sender_keyword='sender')
    def SaveFile(self, parent, title, options, sender): return self.choose(parent, title, options, sender, True)

with tempfile.TemporaryDirectory(prefix='stars-file-workflow-') as directory:
    root = Path(directory) / 'étoiles-Ω'
    root.mkdir()
    fixture = Path(__file__).resolve().parents[1] / 'scaffold/fixtures/regression/native/oneai1/checkpoints/000'
    for suffix in ('.m1', '.xy', '.hst'):
        shutil.copyfile(fixture / ('game' + suffix), root / ('café-Ω' + suffix))
    game, race = root / 'café-Ω.m1', root / 'race-long-ΩΩΩΩ-native-filename.r1'
    race_repeat = root / 'race-repeat-é-Ω.r1'
    chooser = Chooser(bus, '/org/freedesktop/portal/desktop')
    env = dict(os.environ, STARS_SETTINGS_DIR=str(root), STARS_TEST_RACE_PATH=str(race), STARS_TEST_RACE_REPEAT_PATH=str(race_repeat),
               STARS_FILE_DIALOG_BACKEND='portal', SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy', STARS_SDL_RENDERER='software')
    client = subprocess.Popen([str(Path(args.client).resolve())], cwd=root, env=env)
    def poll():
        if client.poll() is None: return True
        loop.quit()
        return False
    def timeout():
        client.kill(); loop.quit(); return False
    GLib.timeout_add(30, poll)
    GLib.timeout_add_seconds(45, timeout)
    loop.run()
    assert client.wait() == 0
    assert calls == [False, False, True, True, False, True]
    assert race.is_file() and race.stat().st_size > 0
    assert race_repeat.is_file() and race_repeat.stat().st_size > 0
print('Native portal game open/cancel and race save/cancel/reload passed on UTF-8 paths')
