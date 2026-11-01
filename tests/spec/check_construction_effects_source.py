#!/usr/bin/env python3
"""Construction assertions against the linked production implementation."""
import subprocess
from linked_witness import binary
def main():subprocess.run([str(binary("construction-effects"))],check=True)
if __name__=="__main__":main()
