#!/usr/bin/env python3
"""Host production control assertions against linked production code."""
import subprocess
from linked_witness import binary
def main():subprocess.run([str(binary("host-production"))],check=True)
if __name__=="__main__":main()
