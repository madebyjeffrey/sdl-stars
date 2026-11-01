#!/usr/bin/env python3
"""Transport preset/control assertions against linked production helpers."""
import subprocess
from linked_witness import binary
def main():subprocess.run([str(binary("transport-presets"))],check=True)
if __name__=="__main__":main()
