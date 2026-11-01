#!/usr/bin/env python3
"""Exercise native preset codecs through linked preference helpers and real types.

INI/text operations are doubles; valid UTF-8 names fit the stored byte capacity.
This checks storage codecs, not M09/M10 preset application or native filesystem I/O.
"""
import subprocess



def main():
    from linked_witness import binary
    subprocess.run([str(binary("s02-presets"))],check=True)


if __name__=="__main__":
    main()
