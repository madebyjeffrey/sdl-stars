"""Check native argv forwarding and legacy parser bounds before GUI startup."""
import os
import subprocess
import sys

executable = sys.argv[1]
for arguments in ([], ['-s12345', '-g10', 'game.hst'], ['a' * 79], ['a' * 79] * 12):
    result = subprocess.run([executable, *arguments], env={**os.environ, 'STARS_TEST_COMMAND': ' '.join(arguments)},
                            capture_output=True, text=True, timeout=5)
    assert result.returncode == 37, (arguments, result)
for arguments in (['space name.hst'], ['tab\tname'], ['a' * 80], ['a' * 79] * 13):
    result = subprocess.run([executable, *arguments], capture_output=True, text=True, timeout=5)
    assert result.returncode == 2 and 'stars-sdl:' in result.stderr, (arguments, result)
result = subprocess.run([executable, '--version'], capture_output=True, text=True, timeout=5)
assert result.returncode == 0 and result.stdout.startswith('stars-sdl ') and 'linux-' in result.stdout, result
for arguments in (['--headless'], ['--headless', 'game.hst'], ['game.hst', '--headless'],
                  ['--headless', '-s12345', '--headless', '-g10', 'game.hst']):
    expected = ' '.join(argument for argument in arguments if argument != '--headless')
    result = subprocess.run([executable, *arguments], env={**os.environ,
        'STARS_TEST_COMMAND': expected, 'STARS_TEST_HEADLESS': '1',
        'SDL_VIDEODRIVER': 'stars-invalid-driver', 'SDL_AUDIODRIVER': 'stars-invalid-driver',
        'STARS_SDL_RENDERER': 'stars-invalid-renderer'}, capture_output=True, text=True, timeout=5)
    assert result.returncode == 37, (arguments, result)
print('Native entry argument, bounds, version and exit-code contracts passed')
