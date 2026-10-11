"""Run one desktop test with an isolated X server and window manager."""
import argparse
import os
import pathlib
import subprocess
import sys
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--inside", action="store_true")
    parser.add_argument("--xvfb", required=True)
    parser.add_argument("--wm", required=True)
    parser.add_argument("--xprop", required=True)
    parser.add_argument("--timeout", type=float, default=20)
    parser.add_argument("executable")
    parser.add_argument("arguments", nargs=argparse.REMAINDER)
    args = parser.parse_args()
    if not args.inside:
        return subprocess.call([
            args.xvfb, "-a", sys.executable, str(pathlib.Path(__file__).resolve()),
            "--inside", "--xvfb", args.xvfb, "--wm", args.wm,
            "--xprop", args.xprop, "--timeout", str(args.timeout), args.executable, *args.arguments,
        ])
    # DISPLAY belongs to xvfb-run, never to the user's desktop compositor.
    env = dict(os.environ, SDL_VIDEODRIVER="x11")
    with open("window-manager.log", "w") as log:
        wm = subprocess.Popen([args.wm, "--sm-disable"], env=env,
                              stdout=log, stderr=subprocess.STDOUT)
        try:
            deadline = time.monotonic() + 5
            while time.monotonic() < deadline:
                if wm.poll() is not None:
                    raise RuntimeError("Openbox exited before becoming ready")
                state = subprocess.run(
                    [args.xprop, "-root", "_NET_SUPPORTING_WM_CHECK"],
                    env=env, capture_output=True, text=True, timeout=2)
                if "window id #" in state.stdout:
                    return subprocess.run([args.executable, *args.arguments], env=env, timeout=args.timeout).returncode
                time.sleep(0.05)
            raise RuntimeError("Openbox did not become ready")
        finally:
            if wm.poll() is None:
                wm.terminate()
                try:
                    wm.wait(timeout=2)
                except subprocess.TimeoutExpired:
                    wm.kill()
                    wm.wait()


if __name__ == "__main__":
    sys.exit(main())
