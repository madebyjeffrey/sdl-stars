#!/usr/bin/env python3
"""Production preset assertions through linked handlers and helpers."""
import subprocess
from linked_witness import binary
def main():subprocess.run([str(binary("production-presets"))],check=True)
if __name__=="__main__":main()
