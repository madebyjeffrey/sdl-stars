#!/usr/bin/env python3
"""Run S02 witnesses against linked C++ functions, without game edits.

Only profile reads and layout storage are stubbed. This is a focused adapter
check, not a whole application/INI/OS test. The assertions mirror named Dafny
witness inputs; the algorithms under test are linked from the normal production libraries.
"""
import subprocess
from linked_witness import binary

def main():
    subprocess.run([str(binary("s02-layout"))],check=True)


if __name__=="__main__":
    main()
