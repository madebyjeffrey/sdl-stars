#!/usr/bin/env python3
"""Linked profile buffers and host grammar with bounded input stubs.

UTF-8 decoding, profile line/lookup/buffer code, and host parsing come directly
from current source. Filesystem loading and localization are fixture adapters.
"""
import subprocess


def main():
    from linked_witness import binary
    subprocess.run([str(binary("s02-io"))],check=True)


if __name__=="__main__":
    main()
