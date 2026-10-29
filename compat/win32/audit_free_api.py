#!/usr/bin/env python3
"""Reproduce the lexical coverage comparison; a definition is not conformance."""
import csv
import hashlib
import json
import re
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
UPSTREAM = ROOT / 'compat/win32/upstream/free-api'
ORIGIN = json.loads((UPSTREAM / 'ORIGIN.json').read_text())
for name, expected in ORIGIN['sha256'].items():
    if hashlib.sha256((UPSTREAM / name).read_bytes()).hexdigest() != expected:
        raise SystemExit(f'Upstream reference changed: {name}')

# Erasing comments/literals preserves line numbers for source references.
def strip(text):
    pattern = r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\''
    return re.sub(pattern, lambda m: ''.join('\n' if c == '\n' else ' ' for c in m[0]), text, flags=re.S)

functions = {}
aliases = {}
statuses = {}
for path in sorted(UPSTREAM.rglob('*')):
    if path.suffix not in ('.cpp', '.h', '.hpp'):
        continue
    original = path.read_text()
    clean = strip(original)
    location = str(path.relative_to(UPSTREAM))
    # Internal namespace helpers (e.g. GetActiveWindow) are not API exports.
    pattern = r'\b(?:WINAPI[V]?\s+|static\s+inline\s+\w+\s+)([A-Za-z_]\w*)\s*\([^;{}]*\)\s*\{'
    if path.name.startswith('crt_'):
        pattern = r'\b([A-Za-z_]\w*)\s*\([^;{}]*\)\s*\{'
    for match in re.finditer(pattern, clean):
        functions.setdefault(match[1], f'{location}:{clean.count(chr(10), 0, match.start()) + 1}')
    for match in re.finditer(r'^\s*#define\s+(\w+)\s+(\w+)\s*$', clean, re.M):
        # Headers contain both branches; prefer ANSI when both aliases occur.
        if match[1] not in aliases or match[2].endswith('A'):
            aliases[match[1]] = match[2]
    for match in re.finditer(r'/\*\*(.*?)\*/\s*[^;{}]*?\b(\w+)\s*\([^;{}]*\)\s*;', original, re.S):
        status = re.findall(r'Status:\s*([A-Z_]+)', match[1])
        if status:
            statuses[match[2]] = status[-1]

rows = list(csv.DictReader((ROOT / 'docs/WIN32-API-CALLS.csv').open()))
counts = Counter(row['source_symbol'] for row in rows)
categories = {row['source_symbol']: row['category'] for row in rows}
adapted = {'SetRect', 'IntersectRect', 'GetTickCount', 'RegisterClass', 'CreateWindow',
           'DestroyWindow', 'ShowWindow', 'GetClientRect', 'MoveWindow', 'SetWindowText',
           'PostMessage', 'PeekMessage', 'GetMessage', 'PostQuitMessage', 'DispatchMessage',
           'TranslateMessage', 'DefWindowProc'}
extended = {'CopyRect', 'EqualRect', 'InflateRect', 'OffsetRect', 'PtInRect', 'GetWindowLongPtr',
            'SetWindowLongPtr', 'IsWindowVisible', 'GetWindowText', 'GetClassNameA', 'GetFocus',
            'CallWindowProc', 'SendMessage'}
legacy = {'MakeProcInstance', 'FreeProcInstance', 'GetCurrentTime', 'Yield', 'ExitWindows'}
known_stubs = {'MessageBoxA', 'LoadCursorA', 'LoadIconA', 'FindResourceA', 'LoadResource',
               'SizeofResource', 'LockResource', 'FreeResource', 'GetModuleHandleA', 'GetSystemMetrics', 'GetDeviceCaps'}
output = []
local_functions = {}
for path in sorted((ROOT / 'compat/win32/src').glob('*.c')):
    clean = strip(path.read_text())
    # Test redirection retains the original API spelling inside this macro.
    clean = re.sub(r'\bSTARS_TEST_IMPL\((\w+)\)', r'\1', clean)
    for match in re.finditer(r'\bWINAPIV?\s+(\w+)\s*\([^;{}]*\)\s*\{', clean):
        local_functions[match[1]] = f'{path.relative_to(ROOT)}:{clean.count(chr(10), 0, match.start()) + 1}'
    for symbol in counts:
        if categories[symbol] == 'Microsoft CRT dependencies':
            match = re.search(r'\b' + re.escape(symbol) + r'\s*\([^;{}]*\)\s*\{', clean)
            if match:
                local_functions[symbol] = f'{path.relative_to(ROOT)}:{clean.count(chr(10), 0, match.start()) + 1}'
for symbol in sorted(counts):
    candidate = symbol
    seen = set()
    while candidate in aliases and candidate not in seen:
        seen.add(candidate)
        candidate = aliases[candidate]
    if candidate not in functions and symbol + 'A' in functions:
        candidate = symbol + 'A'
    found = functions.get(candidate, '')
    status = statuses.get(candidate, '')
    local_candidate = {'CreateWindow': 'CreateWindowExA', 'CreateDialog': 'CreateDialogParamA',
                       'DialogBox': 'DialogBoxParamA', 'ExitWindows': 'StarsCompatExitWindows'}.get(symbol, symbol)
    if local_candidate not in local_functions and symbol + 'A' in local_functions:
        local_candidate = symbol + 'A'
    local_found = local_functions.get(local_candidate, '')
    if symbol in adapted | extended and not local_found:
        raise SystemExit(f'Missing local implementation: {symbol}')
    if symbol == 'ExitWindows':
        decision = 'platform difference; rejects desktop logoff and exits Stars only'
    elif symbol in adapted:
        decision = 'adapted in local C++20 runtime'
    elif local_found:
        decision = 'implemented as local C++20 extension'
    elif symbol in legacy:
        decision = 'local header macro; no separate implementation'
    elif found and (candidate in known_stubs or status == 'STUB'):
        decision = 'stub or restricted semantics; replace for Stars'
    elif found:
        decision = 'definition present; review and adapt before linking'
    else:
        decision = 'no definition found; implement for Stars'
    output.append([categories[symbol], symbol, counts[symbol], candidate if found else '', found, status, decision, local_found])
with (ROOT / 'docs/FREE-API-COVERAGE.csv').open('w', newline='') as f:
    writer = csv.writer(f, lineterminator='\n')
    writer.writerow(['category', 'source_symbol', 'calls', 'upstream_symbol', 'upstream_definition', 'upstream_declared_status', 'decision', 'local_definition'])
    writer.writerows(output)
print(f"Upstream {ORIGIN['commit']}: {len(output)} source symbols, {sum(bool(r[4]) for r in output)} definition matches")
for decision, count in sorted(Counter(row[-2] for row in output).items()):
    print(f'{count}: {decision}')
apis = [row for row in output if row[0] not in {'Legacy compatibility macros', 'Microsoft CRT dependencies'}]
cImplemented = sum(bool(row[-1]) for row in apis)
print(f'Local Win32 API definitions: {cImplemented}/{len(apis)} ({100 * cImplemented / len(apis):.1f}%; not runtime qualification)')
crt = [row for row in output if row[0] == 'Microsoft CRT dependencies']
print(f'Local CRT coverage: {sum(bool(row[-1]) for row in crt)}/{len(crt)} (separate from Win32 percentage)')
