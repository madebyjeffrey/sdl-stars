"""Run each Acutest case in a fresh process when SDK APIs are unavailable."""
import subprocess
import sys


def main():
    exe = sys.argv[1]
    listed = subprocess.run([exe, '--list'], capture_output=True, text=True, check=True)
    names = [line.strip() for line in listed.stdout.splitlines() if line.startswith('  ')]
    if not names:
        raise RuntimeError(f'No test cases listed by {exe}: {listed.stdout}')
    failed = False
    for name in names:
        result = subprocess.run([exe, '--no-exec', '--color=never', '--', name])
        failed |= result.returncode != 0
    return int(failed)


if __name__ == '__main__':
    sys.exit(main())
