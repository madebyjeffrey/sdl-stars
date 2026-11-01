#!/usr/bin/env python3
"""Queue editor assertions through the linked production handler."""
import subprocess
from linked_witness import binary
def main():subprocess.run([str(binary("production-editor"))],check=True)
if __name__=="__main__":main()
