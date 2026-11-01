#!/usr/bin/env python3
"""Historical C11 evidence; current C++ refinement remains a separate obligation.

Original fixture and source: immutable snapshot 82679dc.
"""
from historical_c11 import run

if __name__ == "__main__":
    run("check_race_versions_source.py")
