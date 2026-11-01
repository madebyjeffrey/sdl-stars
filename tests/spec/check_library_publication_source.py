#!/usr/bin/env python3
"""Check linked M01 publication controllers against an independent interpreter.

File/path/metadata/inspection helpers are scripted providers over disjoint role
objects. This tests source control flow and projected partial effects, not real
filesystem I/O, recursive helper algorithms, admission, UI or crash durability.
"""
from pathlib import Path
import json
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def ret(value): return ('return', value)
def ask(kind, a, b, yes, no): return ('ask', (kind, a, b), yes, no)
def notice(n, nxt): return ('notice', n, nxt)
def ignore(kind, a, b, nxt): return ask(kind, a, b, nxt, nxt)
def guard(tag, nxt): return ask('format', tag, 0, nxt, ret(0))


def program(profile):
    fail_stage = ignore('remove', 3, 0, notice(1, ret(0)))
    install_fail = lambda prior: ignore('remove', 3, 0,
        ignore('remove', 4, 0, notice(2, ret(0))) if prior else notice(2, ret(0)))
    if profile <= 1:
        retire_failed = ask('replace', 4, 1, ignore('remove', 4, 0, notice(5, ret(0))), notice(4, ret(0)))
        def prior(orders):
            installed = ask('remove', 2, 0, ignore('remove', 4, 0, ret(1)), retire_failed) if orders else ignore('remove', 4, 0, ret(1))
            return ask('copy', 1, 4, ask('replace', 3, 1, installed, install_fail(True)), ignore('remove', 3, 0, notice(3, ret(0))))
        installed = ask('probe', 1, 2, prior(True), prior(False)) if profile == 0 else ask('replace', 3, 1, ret(1), install_fail(False))
        return guard(1, guard(2, guard(3, ask('copy', 0, 3, ask('probe', 2, 3, installed, fail_stage), fail_stage))))
    if profile == 2:
        def done(held=False):
            end = ask('replace', 6, 5, notice(6, ret(0)), notice(4, ret(0))) if held else notice(6, ret(0))
            return ask('probe', 3, 3, ignore('remove', 3, 1, end), end)
        publish = ask('replace', 3, 5, ask('remove', 6, 1, ret(1), notice(7, ret(1))), ask('replace', 6, 5, done(), done(True)))
        hold = ask('replace', 5, 6, publish, done())
        prepare = ask('build', 1, 3, ask('build', 2, 3, ask('build', 3, 3, ask('probe', 4, 6, hold, done()), done()), done()), done())
        return guard(4, ask('probe', 5, 5, ask('allocate', 1, 3, prepare, ret(0)), ret(0)))
    if profile == 3:
        fail = ask('probe', 3, 3, ignore('remove', 3, 1, notice(8, ret(0))), notice(8, ret(0)))
        staged = ask('build', 4, 3, ask('build', 3, 3, ask('replace', 3, 1, ret(1), fail), fail), fail)
        return guard(4, ask('probe', 5, 5, ask('probe', 6, 1, guard(5,
            ask('probe', 7, 1, ask('probe', 8, 1, ask('allocate', 1, 3, staged, ret(0)), ret(0)), ret(0))), ret(0)), ret(0)))
    clear = lambda nxt: ('clear', nxt)
    fail = ignore('remove', 3, 1, ignore('remove', 7, 0, clear(notice(9, ret(0)))))
    return ask('allocate', 2, 3, ask('build', 5, 3, ask('build', 6, 3, ask('build', 7, 3,
        ask('remove', 7, 0, ask('replace', 3, 7, clear(ret(1)), fail), fail), fail), fail), fail),
        ignore('remove', 7, 0, clear(ret(0))))


def check(case):
    p, variant = case['profile'], case['variant']
    assert case['calls'] <= (11,7,14,12,12)[p], case
    store = [1, 2 if p == 0 else 0, 2 if p <= 1 else 0, 0, 0, 115 if p in (2,3) else 0, 0, 100 if p == 4 else 0]
    if variant == 2: store[3] = 2
    if variant == 3: store[4] = 2
    node, at = program(p), 0
    while node[0] != 'return':
        event = case['events'][at]; at += 1
        if node[0] == 'notice':
            assert event == ['notice', node[1]], (case, node)
            node = node[2]; continue
        if node[0] == 'clear':
            assert event == ['clear'], (case, node)
            node = node[1]; continue
        kind, a, b = node[1]
        # Family cleanup groups shallow enumeration and final stage removal.
        assert event[:3] == [kind, a, 0 if p == 4 and kind == 'remove' and a == 3 else b], (case, node)
        ok, residue, after = event[3:]
        if kind == 'copy':
            if ok:
                assert 0 < store[a] < 100 and store[b] == 0
                store[b] = store[a]
            elif residue:
                assert store[b] == 0 and residue == 3 and store[a] in (1,2)
                store[b] = residue
        elif kind == 'replace' and ok:
            assert store[a]
            store[b], store[a] = store[a], 0
        elif kind == 'remove':
            if ok: store[a] = 0
            elif residue:
                assert store[a] >= 100 and residue >= 100 and ((residue-100) & (store[a]-100)) == residue-100
                store[a] = residue
        elif kind == 'allocate' and ok:
            assert store[b] == 0
            store[b] = 100
        elif kind == 'build':
            assert residue >= 100
            store[b] = residue
        elif kind == 'probe' and ok and a in (1,2,3,5):
            assert store[b] >= 100 if a in (3,5) else 0 < store[b] < 100
        assert store == after, (case, node, store, after)
        node = node[2 if ok else 3]
    assert case['events'][at:] == [['end']], (case,node)
    assert case['result'] == node[1] and case['store'] == store, (case,node,store)
    return tuple(tuple(e[:5]) for e in case['events'])


def main():
    # Enumerate every reached Boolean reply combination, including repeated
    # restore/cleanup calls, plus partial residue and stage/previous collisions.
    profiles = [(p,v,bits) for p,bits in ((0,11),(1,7),(2,14),(3,12),(4,12)) for v in (0,1)]
    profiles += [(p,v,11 if p==0 else 7) for p in (0,1) for v in (2,3)]
    profiles += [(4,4,12)]+[(4,v,0) for v in range(5,22)]
    profiles += [(p,v,0) for p in (0,1) for v in (22,23,24)]
    inputs = ''.join(f'{p} {m} {v}\n' for p,v,bits in profiles for m in range(1<<bits))
    with tempfile.TemporaryDirectory(prefix='stars-library-publication-') as tmp:
        from linked_witness import binary as linked_binary
        tmp=Path(tmp);binary=linked_binary("library-publication");output=tmp/'cases.jsonl'
        with output.open('w') as stream:
            subprocess.run([str(binary)],input=inputs,text=True,stdout=stream,check=True)
        count=0;unique=[set() for _ in range(5)];witnesses={}
        for line in output.open():
            case=json.loads(line);trace=check(case);count+=1;unique[case['profile']].add(trace)
            witnesses[case['profile'],case['mask'],case['variant']]=case
        # Explicit outcomes prevent all-failure/short-circuit coverage masquerading
        # as successful publication or ordinary restoration coverage.
        for p in range(5): assert witnesses[p,0,0]['result']==1
        q=witnesses[0,1<<5,0];assert q['result']==1 and q['store'][1]==1 and q['store'][2]==2
        q=witnesses[1,0,2];assert q['result']==0 and q['store'][3]==0
        q=witnesses[0,1<<6,1];assert q['result']==0 and q['store'][4]==3 and q['store'][1]==2
        q=witnesses[0,(1<<8)|(1<<9),0];assert q['result']==0 and q['store'][1]==1 and q['store'][4]==2
        q=witnesses[2,(1<<8)|(1<<9),0];assert q['result']==0 and q['store'][5]==115 and ['notice',6] in q['events']
        q=witnesses[2,1<<9,1];assert q['result']==1 and q['store'][6]==101 and ['notice',7] in q['events']
        q=witnesses[4,1<<3,1];assert q['result']==0 and ['clear'] in q['events']
        for p in (0,1):
            for v in (22,23):assert witnesses[p,0,v]['result']==0
            assert witnesses[p,0,24]['result']==1
        print(f'{count} linked-controller cases; distinct traces by later/first/replace/separate/family: '+','.join(str(len(s)) for s in unique))
        print('Scripted helper receipts only; recursive copy/validation, native filesystem semantics, admission, UI and crash qualification remain open.')
        subprocess.run([str(linked_binary("library-copy"))],cwd=tmp,check=True)


if __name__=='__main__':main()
