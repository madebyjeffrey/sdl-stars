#!/usr/bin/env python3
"""Production forecasting assertions against linked production code."""
import subprocess
from linked_witness import binary
def main():subprocess.run([str(binary("production-forecast"))],check=True)
if __name__=="__main__":main()
