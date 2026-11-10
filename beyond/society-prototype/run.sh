#!/bin/sh
set -eu
cd "$(dirname "$0")/../../../cpp-demos/rust"
exec cargo run --locked --release -p society-prototype -- "$@"
