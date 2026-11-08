#!/usr/bin/env python3
"""Time the real Humanoid pricing query and verify its native-checked score.

Build with `cargo build --offline --release` first. --max-seconds supplies an
optional local performance gate; normal correctness tests do not depend on time.
CLI runs include loading and validating the source world. --repl separately
measures startup, the first lazy calculation, and subsequent cached reads.
"""
import argparse
import queue
from pathlib import Path
import statistics
import subprocess
import time
import tempfile
import threading

RDL = Path(__file__).resolve().parents[1]


def benchmark_repl(args):
    command = [str(args.rdl.resolve()), 'repl', str(RDL / 'examples/predefined.rdl')]
    lines = queue.Queue()
    with tempfile.TemporaryFile(mode='w+t') as errors:
        started = time.perf_counter()
        process = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                   stderr=errors, text=True, bufsize=1)

        def collect():
            for line in process.stdout:
                lines.put(line)
            lines.put(None)

        reader = threading.Thread(target=collect, daemon=True)
        reader.start()

        def exchange(expression, timeout):
            process.stdin.write(expression + '\n')
            process.stdin.flush()
            try:
                line = lines.get(timeout=timeout)
            except queue.Empty:
                raise SystemExit(f'FAIL: REPL response exceeded {timeout:g}s') from None
            if line is None:
                errors.seek(0)
                raise SystemExit(f'FAIL: REPL exited: {errors.read().strip()}')
            return line.strip()

        try:
            status = exchange(':world', 60)
            print(f'world load: {time.perf_counter() - started:.3f}s; {status}', flush=True)
            timings = []
            for index in range(args.runs + 1):
                started = time.perf_counter()
                result = exchange('humanoid.points', args.max_seconds or 60)
                elapsed = time.perf_counter() - started
                if result != '25':
                    raise SystemExit(f'FAIL: expected 25 advantage points, got {result!r}')
                label = 'first calculation' if index == 0 else f'cached read {index}'
                print(f'{label}: {elapsed:.6f}s; advantage_points = 25', flush=True)
                if index:
                    timings.append(elapsed)
            print(f'cached median: {statistics.median(timings):.6f}s')
            process.stdin.write(':quit\n')
            process.stdin.close()
            if process.wait(timeout=10) != 0:
                errors.seek(0)
                raise SystemExit(f'FAIL: REPL exited: {errors.read().strip()}')
        finally:
            if process.poll() is None:
                process.kill()
                process.wait()
            reader.join(timeout=1)
            process.stdout.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rdl', type=Path, default=RDL / 'target/release/rdl')
    parser.add_argument('--runs', type=int, default=3)
    parser.add_argument('--repl', action='store_true', help='measure lazy and cached reads in one world')
    parser.add_argument('--max-seconds', type=float)
    args = parser.parse_args()
    if args.runs < 1 or (args.max_seconds is not None and args.max_seconds <= 0):
        parser.error('runs and max-seconds must be positive')
    if args.repl:
        benchmark_repl(args)
        return
    command = [str(args.rdl.resolve()), 'query', str(RDL / 'examples/predefined.rdl'),
               '--', 'humanoid.points']
    timings = []
    for index in range(args.runs):
        started = time.perf_counter()
        try:
            result = subprocess.run(command, text=True, capture_output=True,
                                    timeout=args.max_seconds, check=True)
        except subprocess.TimeoutExpired:
            raise SystemExit(f'FAIL: pricing exceeded {args.max_seconds:g}s') from None
        if result.stdout.strip() != '25':
            raise SystemExit(f'FAIL: expected 25 advantage points, got {result.stdout!r}')
        elapsed = time.perf_counter() - started
        timings.append(elapsed)
        print(f'run {index + 1}: {elapsed:.3f}s; advantage_points = 25', flush=True)
        if args.max_seconds is not None and elapsed > args.max_seconds:
            raise SystemExit(f'FAIL: pricing exceeded {args.max_seconds:g}s')
    print(f'median: {statistics.median(timings):.3f}s (includes world validation)')


if __name__ == '__main__':
    main()
