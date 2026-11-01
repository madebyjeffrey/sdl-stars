#!/usr/bin/env python3
"""S02 state witnesses against linked C++; profile/path operations are doubles.

Runs defined-domain inputs only. This checks native field widths and partial
effects; it does not establish real filesystem outcomes or whole-program refinement.
"""
import subprocess
from linked_witness import binary

def main():
    subprocess.run([str(binary("s02-state"))],check=True)


if __name__=="__main__":
    main()
