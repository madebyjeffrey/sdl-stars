#!/usr/bin/env python3
"""Production lifecycle and starbase navigation through linked code."""
import subprocess
from linked_witness import binary
def main():subprocess.run([str(binary("production-lifecycle"))],check=True)
if __name__=="__main__":main()
