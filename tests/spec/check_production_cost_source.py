#!/usr/bin/env python3
"""Run the independent production-cost oracle against the linked implementation."""
import subprocess
from linked_witness import binary

def main():
    subprocess.run([str(binary("production-cost"))],check=True)

if __name__=="__main__":
    main()
