"""Accept the guide as one isolated file in real headless Firefox, without HTTP hosting."""
import argparse
import base64
import json
import os
import re
import shutil
import socket
import subprocess
import tempfile
import time
import urllib.request
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('guide', type=Path)
    parser.add_argument('--geckodriver', default=shutil.which('geckodriver'))
    parser.add_argument('--firefox', default='/usr/lib/firefox/firefox')
    parser.add_argument('--screenshot', type=Path)
    parser.add_argument('--expect-page')
    args = parser.parse_args()
    if not args.geckodriver:
        parser.error('Install geckodriver or pass --geckodriver')
    if args.screenshot:
        args.screenshot.parent.mkdir(parents=True, exist_ok=True)
    source = args.guide.read_text()
    guide = json.loads(re.search(r'id="guide-data">(.*?)</script>', source, re.S)[1])
    with socket.socket() as reservation:
        reservation.bind(('127.0.0.1', 0))
        port = reservation.getsockname()[1]
    with tempfile.TemporaryDirectory(prefix='stars-help-browser-') as directory:
        root = Path(directory)
        isolated = root / 'exported-guide.html'
        shutil.copyfile(args.guide, isolated)
        log = (root / 'geckodriver.log').open('w')
        process = subprocess.Popen([args.geckodriver, '--host', '127.0.0.1', '--port', str(port)],
                                   env=dict(os.environ, MOZ_HEADLESS='1', MOZ_NO_REMOTE='1'), stdout=log, stderr=log)
        session = None
        def call(method, path, data=None):
            request = urllib.request.Request(f'http://127.0.0.1:{port}' + path, method=method,
                                             data=None if data is None else json.dumps(data).encode(),
                                             headers={'Content-Type': 'application/json'})
            with urllib.request.urlopen(request, timeout=60) as response:
                value = json.load(response)['value']
            if isinstance(value, dict) and 'error' in value:
                raise RuntimeError(value)
            return value
        def execute(script, arguments=None):
            return call('POST', '/session/' + session + '/execute/sync', {'script': script, 'args': arguments or []})
        def ready(key=None):
            for _ in range(300):
                if execute("return document.readyState==='complete' && document.querySelector('main').dataset.page && document.fonts.status==='loaded' && Array.from(document.images).every(i=>i.complete && i.naturalWidth>0) && (!arguments[0] || document.querySelector('main').dataset.page===arguments[0])", [key]):
                    return
                time.sleep(.02)
            raise RuntimeError((key, execute("return {page:document.querySelector('main').dataset.page,fonts:document.fonts.status,images:Array.from(document.images).filter(i=>!i.complete||!i.naturalWidth).map(i=>({src:i.src.slice(0,60),w:i.naturalWidth,h:i.naturalHeight}))}")))
        def click(selector):
            element = call('POST', '/session/' + session + '/element', {'using': 'css selector', 'value': selector})
            identity = element['element-6066-11e4-a52e-4f735466cecf']
            call('POST', '/session/' + session + '/element/' + identity + '/click', {})
        try:
            for _ in range(100):
                try:
                    call('GET', '/status'); break
                except OSError:
                    if process.poll() is not None: raise RuntimeError('geckodriver exited')
                    time.sleep(.05)
            value = call('POST', '/session', {'capabilities': {'alwaysMatch': {'browserName': 'firefox',
                'moz:firefoxOptions': {'binary': args.firefox, 'args': ['-headless'], 'prefs': {
                    'network.dns.disabled': True, 'network.http.speculative-parallel-limit': 0,
                    'datareporting.healthreport.uploadEnabled': False, 'toolkit.telemetry.enabled': False,
                    'app.update.auto': False, 'browser.shell.checkDefaultBrowser': False}}}}})
            session = value['sessionId']
            call('POST', '/session/' + session + '/window/rect', {'width': 1280, 'height': 1000})
            call('POST', '/session/' + session + '/url', {'url': isolated.as_uri()})
            ready()
            assert execute('return location.protocol') == 'file:'
            if args.expect_page:
                assert execute("return document.querySelector('main').dataset.page") == args.expect_page
            # Walk every original topic; no source folder or resource server exists.
            for key, page in guide['pages'].items():
                execute('location.hash=arguments[0]', [key]); ready(key)
                assert execute('return document.title') == re.sub(r'[ \t\r\n\f]+', ' ', page['title']).strip(), key
                assert execute("return Array.from(document.querySelectorAll('main a[href]')).every(a=>a.getAttribute('href').startsWith('#'))"), key
                assert execute("return Array.from(document.images).filter(i=>i.src.startsWith('data:image/svg')).every(i=>{const c=document.createElement('canvas'); c.width=c.height=96; const x=c.getContext('2d'); x.drawImage(i,0,0,96,96); const d=x.getImageData(0,0,96,96).data; for(let j=0;j<d.length;j+=4) if(d[j+3] && Math.min(d[j],d[j+1],d[j+2])<240) return true; return false;})"), key
            if args.screenshot:
                key = next(key for key, page in guide['pages'].items() if 'data:image/svg+xml' in page['body'])
                execute('location.hash=arguments[0]', [key]); ready(key)
                args.screenshot.with_name(args.screenshot.stem + '-diagram' + args.screenshot.suffix).write_bytes(base64.b64decode(call('GET', '/session/' + session + '/screenshot')))
            execute('location.hash=arguments[0]', [guide['contents']]); ready(guide['contents'])
            if args.screenshot:
                args.screenshot.write_bytes(base64.b64decode(call('GET', '/session/' + session + '/screenshot')))
            assert execute("return document.fonts.check('16px StarsHelpSans')")
            target = execute("return document.querySelector('main .image-hotspot').getAttribute('href')")
            click('main .image-hotspot'); ready(target[1:])
            assert execute('return location.hash') == target
            key = next(key for key, page in guide['pages'].items() if 'data-popup=' in page['body'])
            execute('location.hash=arguments[0]', [key]); ready(key)
            target = execute("return document.querySelector('main a[data-popup]').getAttribute('href').slice(1)")
            click('main a[data-popup]')
            for _ in range(200):
                if execute("const f=document.querySelector('iframe'); return document.getElementById('popup').open && f.contentDocument && f.contentDocument.querySelector('main') && f.contentDocument.fonts.status==='loaded'"): break
                time.sleep(.02)
            assert execute("return document.querySelector('iframe').title") == guide['pages'][target]['title']
            assert execute("return document.querySelector('iframe').contentDocument.querySelector('main').textContent.length>0")
            if args.screenshot:
                args.screenshot.with_name(args.screenshot.stem + '-popup' + args.screenshot.suffix).write_bytes(base64.b64decode(call('GET', '/session/' + session + '/screenshot')))
            click('#close-popup'); assert not execute("return document.getElementById('popup').open")
            click('nav a[href="#index.html"]'); ready('index.html')
            execute("const f=document.getElementById('topic-filter'); f.value='Research'; f.dispatchEvent(new Event('input'))")
            assert 0 < execute("return document.querySelectorAll('#topics li:not([hidden])').length") < len(guide['pages']) - 1
            click('nav a:first-child'); ready(guide['contents']); click('#back'); ready('index.html')
            assert execute('return location.hash') == '#index.html'
            assert execute("return performance.getEntriesByType('resource').every(r=>!/^https?:/.test(r.name))")
            print(f"Offline Firefox acceptance: {len(guide['pages'])-1} topics, fonts/images, diagram navigation, popup, index filter and Back passed; one file, no content server.")
        finally:
            if session:
                try: call('DELETE', '/session/' + session)
                except Exception: pass
            process.terminate(); process.wait(timeout=10); log.close()


if __name__ == '__main__':
    main()
