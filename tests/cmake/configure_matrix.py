"""Qualify fresh ordinary/SDK test registries without sharing CMake caches."""
import argparse
import json
from pathlib import Path
import platform
import shlex
import hashlib
import xml.etree.ElementTree as ET
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]


def sdk_reference_name(path):
    return 'test_' + path.stem + ('' if path.stem.endswith('_reference') else '_reference')


def assert_sdk_registry(references, expected):
    assert references == expected, f'SDK registry differs: missing={sorted(expected - references)}, unexpected={sorted(references - expected)}'


def assert_sdk_compilation(rows, root, expected, compiler):
    seen = []
    for row in rows:
        path = Path(row['file']).resolve()
        try:
            name = path.relative_to(root.resolve()).as_posix()
        except ValueError:
            continue
        if not name.startswith('tests/reference/sdk/') or path.suffix != '.c':
            continue
        seen.append(name)
        command = row.get('arguments') or shlex.split(row['command'], posix=platform.system() != 'Windows')
        driver = Path(command[0].strip('"')).resolve()
        assert driver == Path(compiler).resolve(), f'SDK C entry uses a different compiler: {name}: {driver}'
        assert not any(arg in ('-xc++', '-xobjective-c++', '/TP') or
                       (arg == '-x' and index + 1 < len(command) and command[index + 1] in ('c++', 'objective-c++'))
                       for index, arg in enumerate(command)), f'SDK C entry forced to C++: {name}'
    assert len(seen) == len(expected) and set(seen) == expected, f'SDK C compile inventory differs: missing={sorted(expected - set(seen))}, unexpected={sorted(set(seen) - expected)}, duplicates={len(seen) - len(set(seen))}'


def sdk_outcome(report, diagnostics, expected):
    cases = ET.parse(report).getroot().findall('testcase')
    assert {case.attrib['name'] for case in cases} == expected and len(cases) == len(expected), 'Incomplete native SDK execution receipt'
    assert not any(case.find('skipped') is not None for case in cases), 'Native SDK check was skipped/not run'
    failed = {case.attrib['name'] for case in cases
              if case.find('failure') is not None or case.find('error') is not None}
    return {'passed': len(cases) - len(failed), 'failed_diagnostics': sorted(failed & diagnostics),
            'failed_contracts': sorted(failed - diagnostics)}


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
    failures = []
    inventory = json.loads((ROOT / 'tests/reference/sdk/diagnostics.json').read_text())
    diagnostics = {row['test'] for row in inventory['diagnostics']}
    assert len(diagnostics) == 19
    expected_references = {sdk_reference_name(path)
                           for path in (ROOT / 'tests/reference/sdk').glob('*.c')}
    expected_entries = {'tests/reference/sdk/' + path.stem + '.c'
                        for path in (ROOT / 'tests/reference/sdk').glob('*.c')}
    assert len(expected_references) == len(expected_entries) == 38, 'SDK source inventory differs'
    assert not diagnostics & {'test_sdk_type_isolation', 'test_win32_abi_reference', 'test_nativeui_points_reference'}
    for row in inventory['diagnostics']:
        assert hashlib.sha256((ROOT / row['input']).read_text(encoding='utf-8').encode('utf-8')).hexdigest() == row['sha256'], 'SDK diagnostic input changed; review its classification'

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
        assert_sdk_registry(references, expected_references if sdk else set())
        if sdk:
            assert diagnostics <= references, 'SDK diagnostic inventory names an unregistered probe'
        if not ordinary and not sdk:
            assert not names, names
        if ordinary or sdk:
            manifest = build / 'compile_commands.json'
            subprocess.run([sys.executable, str(ROOT / 'tests/cmake/check_cpp_manifest.py'),
                            str(ROOT), str(manifest)], check=True)
            if sdk:
                cache = (build / 'CMakeCache.txt').read_text(encoding='utf-8')
                compiler = next(line.split('=', 1)[1] for line in cache.splitlines()
                                if line.startswith('CMAKE_C_COMPILER:'))
                assert_sdk_compilation(json.loads(manifest.read_text(encoding='utf-8')),
                                       ROOT, expected_entries, compiler)
        outcome = None
        if sdk and args.run_sdk:
            with (work / (name + '-build.log')).open('w') as output:
                subprocess.run(['cmake', '--build', str(build), '--target', 'stars-sdk-reference-build',
                                'stars-sdk-type-isolation', '--parallel', '2'], check=True,
                               stdout=output, stderr=subprocess.STDOUT)
            with (work / (name + '-tests.log')).open('w') as output:
                result = subprocess.run(['ctest', '--test-dir', str(build), '--output-on-failure',
                                '--timeout', '30', '--no-tests=error',
                                '--output-junit', str(work / (name + '-tests.xml')),
                                '-R', r'^(test_sdk_type_isolation|test_.*_reference)$'],
                               stdout=output, stderr=subprocess.STDOUT)
            outcome = sdk_outcome(work / (name + '-tests.xml'), diagnostics, references | {'test_sdk_type_isolation'})
            assert result.returncode in (0, 8), f'{name}: SDK test orchestration failed'
            if outcome['failed_contracts']:
                failures.append(name)
            print(f'{name}: SDK execution {outcome}', flush=True)
        receipts.append({'configuration': name, 'tests': len(names),
                         'sdk_vectors': len(references), 'sdk_runtime': sdk and args.run_sdk,
                         'sdk_result': result.returncode if sdk and args.run_sdk else None, 'sdk_outcome': outcome})
        print(f'{name}: fresh configure and registry pass ({len(names)} tests, {len(references)} SDK vectors)', flush=True)
    (work / 'matrix.json').write_text(json.dumps(receipts, indent=2) + '\n')
    if failures:
        raise SystemExit('Native SDK contract failures in ' + ', '.join(failures) + '; see retained test logs')


if __name__ == '__main__':
    main()
