#!/usr/bin/env python3
"""Planet/queue ownership assertions against linked production code."""
import subprocess
from linked_witness import binary
def main():subprocess.run([str(binary("planet-editor"))],check=True)
if __name__=="__main__":main()
