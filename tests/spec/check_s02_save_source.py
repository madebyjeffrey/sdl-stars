#!/usr/bin/env python3
"""Run the original 11 save stages/42 assertions against the normal linked target."""
import subprocess
from linked_witness import target

def main():
    subprocess.run([str(target("stars-profile-save"))],check=True)

if __name__=="__main__":main()
