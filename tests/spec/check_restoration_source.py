#!/usr/bin/env python3
"""Selection/message restoration assertions against linked production code."""
import subprocess
from linked_witness import binary
def main():subprocess.run([str(binary("restoration"))],check=True)
if __name__=="__main__":main()
