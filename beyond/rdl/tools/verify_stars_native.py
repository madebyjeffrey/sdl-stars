#!/usr/bin/env python3
"""Compare RDL with actual native game functions, using a configured Ninja host build.

Run `make test-host` first. No game source is changed. The oracle reuses the
host unit target's compile/link settings and objects, replacing only its test
main. Build products/assertions stay under beyond/rdl/target/stars-verification.
"""
import argparse
import json
from pathlib import Path
import shlex
import subprocess

RDL = Path(__file__).resolve().parents[1]
ROOT = RDL.parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, default=ROOT/'dist/host-release')
    parser.add_argument('--rdl', type=Path, default=RDL/'target/release/rdl')
    parser.add_argument('--section', choices=['presets','colonies','equipment','costs','gates','boundary'], action='append')
    args = parser.parse_args()
    build = args.build.resolve()
    out = RDL/'target/stars-verification'
    out.mkdir(parents=True, exist_ok=True)
    subprocess.run(['cmake','--build',str(build),'--target','test_race'], check=True, stdout=subprocess.DEVNULL)
    commands = json.loads(subprocess.check_output(['ninja','-C',str(build),'-t','compdb'], text=True))
    command = next(x['command'] for x in commands if x.get('file','').endswith('/tests/unit/test_race.cpp'))
    compile_args = shlex.split(command)
    compile_args[compile_args.index('-o')+1] = str(out/'oracle.o')
    compile_args[compile_args.index('-MF')+1] = str(out/'oracle.d')
    compile_args[compile_args.index('-MT')+1] = str(out/'oracle.o')
    compile_args[compile_args.index('-c')+1] = str(RDL/'tools/stars_oracle.cpp')
    # Headers for the original test main are relative to its source directory.
    compile_args += ['-I',str(ROOT/'tests/unit')]
    subprocess.run(compile_args, cwd=build, check=True)
    commands = subprocess.check_output(['ninja','-C',str(build),'-t','commands','test_race'], text=True).splitlines()
    link = shlex.split(commands[-1])
    link = link[link.index('&&')+1:]
    link = link[:link.index('&&')]
    link = [str(out/'oracle.o') if x.endswith('/tests/unit/test_race.cpp.o') else x for x in link]
    link[link.index('-o')+1] = str(out/'oracle')
    subprocess.run(link, cwd=build, check=True)
    for section in args.section or ['presets','colonies','equipment','costs','gates','boundary']:
        count = 0
        # Each batch retains the runtime's ordinary work budget; no privileged
        # evaluation mode or weakened validation is used for this harness.
        for index in range({'presets':7, 'colonies':6, 'equipment':10, 'costs':10, 'gates':1, 'boundary':1}[section]):
            target=out/f'{section}-{index}.rdl'
            with target.open('w') as output:
                subprocess.run([str(out/'oracle'),section,str(index)], cwd=build/'tests', stdout=output, check=True)
            subprocess.run([str(args.rdl.resolve()),'check','-I',str(RDL/'examples'),str(target)], check=True)
            count += target.read_text().count('  require ')
        print(f'{section}: {count} native comparisons passed', flush=True)



if __name__=='__main__': main()
