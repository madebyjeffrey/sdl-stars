"""Exercise strict-language, independent-reference and test-routing rejections."""
from pathlib import Path
import json
import subprocess
import sys
import tempfile

root, manifest = map(Path, sys.argv[1:])
rows = json.loads(manifest.read_text())
index = next(i for i, row in enumerate(rows) if row['file'].endswith('/src/core/planet.cpp'))
for addition, reason in [
    (' -std=gnu++20', 'Owned production source lacks ISO C++20'),
    (' -x c++', 'Owned production source has a language override'),
    (' -Wno-error=pedantic', 'Owned production source disables strict diagnostics'),
    (' -DRandom=StarsTest_Random', 'Test symbol substitution'),
    (' -DSTARS_TEST_STATE_DOUBLE=1', 'Test symbol substitution'),
    (' -o stars_core-test-0123456789abcdef.dir/planet.cpp.o', 'Private production test clone'),
]:
    changed = [dict(row) for row in rows]
    changed[index]['command'] = changed[index]['command'] + addition
    with tempfile.TemporaryDirectory(prefix='stars-cpp-policy-') as work:
        candidate = Path(work) / 'compile_commands.json'
        candidate.write_text(json.dumps(changed))
        result = subprocess.run([sys.executable, str(root / 'tests/cmake/check_cpp_manifest.py'),
                                 str(root), str(candidate)], capture_output=True, text=True)
        assert result.returncode and reason in result.stderr, result.stdout + result.stderr
for file, command, reason in [
    ('src/core/planet.c', rows[index]['command'], 'Owned production source has a migration extension'),
    ('tests/reference/sdk/win32_abi.c', 'c++ -x c++ -std=c++20', 'Independent frozen-declaration reference must remain C'),
    ('sdk-reference/win32_abi.c', 'cc -std=c11', 'Generated SDK language adapter remains'),
]:
    with tempfile.TemporaryDirectory(prefix='stars-cpp-policy-') as work:
        changed = [dict(row) for row in rows]
        path = (root / file).resolve() if not file.startswith('sdk-reference/') else Path(work) / file
        changed.insert(0, {'file': str(path), 'command': command})
        candidate = Path(work) / 'compile_commands.json'
        candidate.write_text(json.dumps(changed))
        result = subprocess.run([sys.executable, str(root / 'tests/cmake/check_cpp_manifest.py'),
                                 str(root), str(candidate)], capture_output=True, text=True)
        assert result.returncode and reason in result.stderr, result.stdout + result.stderr
print('Rejected GNU mode, language overrides, weakened diagnostics, symbol substitution, state doubles, private production clones, owned C and generated/converted SDK entries')

subprocess.run([sys.executable, str(root / 'tests/cmake/test_sdk_probe_policy.py')], check=True)
