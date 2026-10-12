"""Run one test against a private, invisible Weston compositor."""
import argparse
import os
import pathlib
import subprocess
import tempfile
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--weston', required=True)
    parser.add_argument('--scale', type=int, default=1)
    parser.add_argument('--renderer', default='software')
    parser.add_argument('--timeout', type=float, default=45)
    parser.add_argument('command', nargs=argparse.REMAINDER)
    args = parser.parse_args()
    if not args.command:
        parser.error('a test command is required')
    args.command[0] = str(pathlib.Path(args.command[0]).resolve())
    with tempfile.TemporaryDirectory(prefix='stars-wayland-') as directory:
        runtime = pathlib.Path(directory)
        env = dict(os.environ, XDG_RUNTIME_DIR=directory, WAYLAND_DISPLAY='stars-test',
                   SDL_VIDEODRIVER='wayland', SDL_AUDIODRIVER='dummy',
                   STARS_SDL_RENDERER=args.renderer, SDL_VIDEO_WAYLAND_ALLOW_LIBDECOR='0',
                   LIBGL_ALWAYS_SOFTWARE='1', STARS_SETTINGS_DIR=str(runtime / 'settings'))
        env.pop('DISPLAY', None)
        with (runtime / 'weston.log').open('w+') as log:
            wm = subprocess.Popen([args.weston, '--backend=headless', '--renderer=gl',
                                   '--socket=stars-test', '--idle-time=0', '--no-config',
                                   '--width=1280', '--height=960', f'--scale={args.scale}'],
                                  env=env, stdout=log, stderr=subprocess.STDOUT)
            try:
                deadline = time.monotonic() + 10
                while not (runtime / 'stars-test').exists():
                    if wm.poll() is not None or time.monotonic() >= deadline:
                        log.seek(0)
                        raise RuntimeError('Private Weston failed to start:\n' + log.read())
                    time.sleep(0.05)
                return subprocess.run(args.command, env=env, cwd=directory, timeout=args.timeout).returncode
            finally:
                if wm.poll() is None:
                    wm.terminate()
                    try:
                        wm.wait(timeout=5)
                    except subprocess.TimeoutExpired:
                        wm.kill()
                        wm.wait()


if __name__ == '__main__':
    raise SystemExit(main())
