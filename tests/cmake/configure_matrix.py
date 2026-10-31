"""Qualify fresh ordinary/SDK test registries without sharing CMake caches."""
import argparse
import json
from pathlib import Path
import platform
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--preset', required=True)
    parser.add_argument('--work', type=Path, required=True)
    parser.add_argument('--run-sdk', action='store_true')
    args = parser.parse_args()
    work = args.work.resolve()
    if work.exists():
        parser.error('Use a fresh work directory; existing evidence is never deleted')
    work.mkdir(parents=True)
    windows = platform.system() == 'Windows'
    if args.run_sdk and not windows:
        parser.error('--run-sdk requires a native Windows SDK runner')
    receipts = []
    for name, ordinary, sdk in [('ordinary', True, False), ('sdk', False, True),
                                 ('both', True, True), ('neither', False, False)]:
        build = work / name
        command = ['cmake', '--preset', args.preset, '-B', str(build),
                   '-DSTARS_BUILD_TESTS=' + ('ON' if ordinary else 'OFF'),
                   '-DSTARS_BUILD_WIN32_REFERENCE_TESTS=' + ('ON' if sdk else 'OFF'),
                   '-DPython3_EXECUTABLE=' + sys.executable]
        with (work / (name + '-configure.log')).open('w') as output:
            result = subprocess.run(command, cwd=ROOT, stdout=output, stderr=subprocess.STDOUT)
        if sdk and not windows:
            text = (work / (name + '-configure.log')).read_text()
            assert result.returncode and 'SDK reference tests require a Windows toolchain' in text, text
            receipts.append({'configuration': name, 'unsupported_toolchain_rejected': True})
            continue
        assert result.returncode == 0, f'{name}: see configure log'
        registry = json.loads(subprocess.check_output(['ctest', '--test-dir', str(build), '--show-only=json-v1'], text=True))
        names = {test['name'] for test in registry['tests']}
        references = {n for n in names if n.endswith('_reference')}
        assert ('test_cpp_compilation_manifest' in names) == ordinary, (name, names)
        assert ('test_sdk_type_isolation' in names) == sdk, (name, names)
        assert len(references) == (38 if sdk else 0), (name, references)
        if not ordinary and not sdk:
            assert not names, names
        if ordinary:
            subprocess.run([sys.executable, str(ROOT / 'tests/cmake/check_cpp_manifest.py'),
                            str(ROOT), str(build / 'compile_commands.json')], check=True)
        if sdk and args.run_sdk:
            with (work / (name + '-build.log')).open('w') as output:
                subprocess.run(['cmake', '--build', str(build), '--target', 'stars-sdk-reference-build',
                                'stars-sdk-type-isolation', '--parallel', '2'], check=True,
                               stdout=output, stderr=subprocess.STDOUT)
            with (work / (name + '-tests.log')).open('w') as output:
                subprocess.run(['ctest', '--test-dir', str(build), '--output-on-failure',
                                '--timeout', '30', '--no-tests=error',
                                '-R', r'^(test_sdk_type_isolation|test_.*_reference)$'], check=True,
                               stdout=output, stderr=subprocess.STDOUT)
        receipts.append({'configuration': name, 'tests': len(names),
                         'sdk_vectors': len(references), 'sdk_runtime': sdk and args.run_sdk})
        print(f'{name}: fresh configure and registry pass ({len(names)} tests, {len(references)} SDK vectors)', flush=True)
    (work / 'matrix.json').write_text(json.dumps(receipts, indent=2) + '\n')


if __name__ == '__main__':
    main()
