"""Build the Rust static library and pass its native dependencies to CMake's linker."""
import argparse
import pathlib
import shlex
import subprocess
import sys

parser = argparse.ArgumentParser()
parser.add_argument("--cargo", required=True)
parser.add_argument("--manifest", required=True)
parser.add_argument("--target-dir", required=True)
parser.add_argument("--link-flags", required=True)
parser.add_argument("--target", default="")
args = parser.parse_args()
command = [args.cargo, "rustc", "--locked", "--manifest-path", args.manifest,
           "--target-dir", args.target_dir]
if args.target:
    command += ["--target", args.target]
command += ["--", "--print=native-static-libs"]
result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
sys.stdout.write(result.stdout)
if result.returncode:
    sys.exit(result.returncode)
for line in result.stdout.splitlines():
    if "native-static-libs:" in line:
        flags = shlex.split(line.split("native-static-libs:", 1)[1])
        # Rust's diagnostic uses compiler flags. The response file goes directly
        # to the linker, which needs no driver-only threading switch.
        native = []
        seen = set()
        i = 0
        while i < len(flags):
            flag = flags[i]
            group = (flag, flags[i + 1]) if flag == "-framework" else (flag,)
            i += len(group)
            if flag == "-pthread" or (sys.platform == "darwin" and flag in {"-lSystem", "-lc", "-lm"}):
                continue
            if sys.platform != "darwin" or group not in seen:
                native.extend(group)
                seen.add(group)
        pathlib.Path(args.link_flags).write_text("\n".join(native) + "\n")
        break
else:
    sys.exit("Rust did not report its native link dependencies")
