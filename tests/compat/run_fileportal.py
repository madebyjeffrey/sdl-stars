"""Portal wire fixture, run only inside dbus-run-session (never on the user's bus)."""
import argparse
import os
import pathlib
import subprocess
import tempfile
import urllib.parse
import dbus
import dbus.service
from dbus.mainloop.glib import DBusGMainLoop
from gi.repository import GLib

parser = argparse.ArgumentParser()
parser.add_argument('--client', required=True)
parser.add_argument('--driver', choices=('dummy', 'x11'), default='dummy')
args = parser.parse_args()
assert os.environ.get('STARS_TEST_PRIVATE_BUS') == '1'
DBusGMainLoop(set_as_default=True)
loop = GLib.MainLoop()
bus = dbus.SessionBus()
name = dbus.service.BusName('org.freedesktop.portal.Desktop', bus)
requests = []
errors = []
attempts = {}

class Request(dbus.service.Object):
    def __init__(self, path, title, uri, destination):
        super().__init__(bus, path)
        self.title, self.uri, self.destination = title, uri, destination
        self.closed = False
    def Response(self, status, result):
        message = dbus.lowlevel.SignalMessage(self._object_path, 'org.freedesktop.portal.Request', 'Response')
        message.set_destination(self.destination)
        message.append(status, result, signature='ua{sv}')
        bus.send_message(message)
    @dbus.service.method('org.freedesktop.portal.Request')
    def Close(self):
        self.closed = True
    def respond(self):
        if self.closed:
            return False
        result = {'uris': dbus.Array([self.uri], signature='s')}
        status = 0
        if self.title == 'Portal cancel': status = 1
        if self.title == 'Portal failed': status = 2
        if self.title == 'Portal bad URI': result['uris'] = dbus.Array(['file:///tmp/%00bad'], signature='s')
        if self.title == 'Portal remote': result['uris'] = dbus.Array(['file://remote/tmp/file'], signature='s')
        if self.title == 'Portal malformed': result['uris'] = dbus.String('wrong type')
        if self.title == 'Portal empty': result['uris'] = dbus.Array([], signature='s')
        if self.title == 'Portal multiple': result['uris'] = dbus.Array([self.uri, self.uri], signature='s')
        result['current_filter'] = dbus.Struct(('Host files', dbus.Array([dbus.Struct((dbus.UInt32(0), '*.[hH][sS][tT]'))], signature='(us)')))
        self.Response(dbus.UInt32(status), dbus.Dictionary(result, signature='sv'))
        return False

class Chooser(dbus.service.Object):
    def choose(self, parent, title, options, sender, save):
        try:
            if title == 'Portal Wayland parent':
                assert parent == 'wayland:stars-export-abcdefghijklmnopqrstuvwxyz-abcdefghijklmnopqrstuvwxyz-abcdefghijklmnopqrstuvwxyz'
            else:
                assert parent == '' or str(parent).startswith('x11:')
            assert options['modal']
            if not save: assert not options['multiple']
            filters = options['filters']
            assert str(filters[0][0]) == 'Stars files'
            assert [str(item[1]) for item in filters[0][1]] == ['*.[mM]*', '*.[hH][sS][tT]', '*.[rR]*']
            assert str(filters[2][1][0][1]) == '*'
            attempt = attempts.get(str(title), 0)
            attempts[str(title)] = attempt + 1
            retry = str(title) in ('Portal retry', 'Portal directory', 'Portal overwrite') and attempt != 0
            assert options['current_filter'] == filters[1 if retry else 0]
            folder = bytes(options['current_folder'])
            assert folder.endswith(b'\0') and folder[:-1].decode() == str(root / 'retry' if retry else root)
            if retry and save: assert options['current_name'] == 'existing.r1'
        except Exception as error:
            errors.append(repr(error))
            raise
        if title == 'Portal unavailable':
            raise dbus.exceptions.DBusException('No FileChooser backend', name='org.freedesktop.DBus.Error.UnknownMethod')
        token = str(options['handle_token'])
        path = '/org/freedesktop/portal/desktop/request/' + str(sender)[1:].replace('.', '_') + '/' + token
        uri = (root / ('new race' if save else 'café-Ω.m1')).as_uri()
        if title == 'Portal retry': uri = (root / 'retry' / ('missing.m1' if attempt == 0 else 'café-Ω.m1')).as_uri()
        if title == 'Portal directory': uri = (root / 'retry' if attempt == 0 else root / 'retry' / 'café-Ω.m1').as_uri()
        if title == 'Portal overwrite': uri = (root / 'retry' / ('existing.r1' if attempt == 0 else 'another.r1')).as_uri()
        request = Request(path, str(title), uri, str(sender))
        requests.append(request)
        if title == 'Portal early': request.respond()  # Response before the method reply.
        elif title not in ('Portal quit', 'Portal destroy'): GLib.timeout_add(60, request.respond)
        return dbus.ObjectPath(path)
    @dbus.service.method('org.freedesktop.portal.FileChooser', in_signature='ssa{sv}', out_signature='o', sender_keyword='sender')
    def OpenFile(self, parent, title, options, sender): return self.choose(parent, title, options, sender, False)
    @dbus.service.method('org.freedesktop.portal.FileChooser', in_signature='ssa{sv}', out_signature='o', sender_keyword='sender')
    def SaveFile(self, parent, title, options, sender): return self.choose(parent, title, options, sender, True)

with tempfile.TemporaryDirectory(prefix='stars-portal-') as directory:
    root = pathlib.Path(directory)
    (root / 'café-Ω.m1').write_bytes(b'fixture')
    (root / 'retry').mkdir()
    (root / 'retry' / 'café-Ω.m1').write_bytes(b'fixture')
    (root / 'retry' / 'existing.r1').write_bytes(b'preserve')
    chooser = Chooser(bus, '/org/freedesktop/portal/desktop')
    env = dict(os.environ, STARS_FILE_DIALOG_BACKEND='portal', SDL_VIDEODRIVER=args.driver, SDL_AUDIODRIVER='dummy', STARS_SDL_RENDERER='software')
    client = subprocess.Popen([args.client, str(root)], env=env)
    def poll():
        if client.poll() is None: return True
        GLib.timeout_add(100, finish)  # Drain Close calls after client disconnect.
        return False
    def finish():
        loop.quit()
        return False
    def timeout():
        errors.append('Client timeout')
        client.kill()
        loop.quit()
        return False
    GLib.timeout_add(30, poll)
    GLib.timeout_add_seconds(25, timeout)
    loop.run()
    assert client.wait() == 0, 'Portal client failed'
    assert not errors, errors
    assert all(request.closed for request in requests if request.title in ('Portal quit', 'Portal destroy'))
    assert all(attempts[title] == 2 for title in ('Portal retry', 'Portal directory', 'Portal overwrite'))
    assert (root / 'retry' / 'existing.r1').read_bytes() == b'preserve'
print('Private-bus portal protocol fixture passed')
