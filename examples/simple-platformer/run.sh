#!/usr/bin/env bash
#
# Symlinks this example mod into the engine's mods/ directory and launches the engine.
# Run from anywhere — the script resolves paths relative to itself.
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ENGINE_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
MOD_NAME="simple-platformer"

# Ensure the engine has been built
if [ ! -f "$ENGINE_ROOT/build/gloaming" ]; then
    echo "Engine binary not found at $ENGINE_ROOT/build/gloaming"
    echo "Build the engine first:"
    echo "  cmake -B build -DCMAKE_BUILD_TYPE=Debug"
    echo "  cmake --build build"
    exit 1
fi

# Create mods/ directory if it doesn't exist
mkdir -p "$ENGINE_ROOT/mods"

# Symlink the example into mods/ (idempotent)
if [ ! -e "$ENGINE_ROOT/mods/$MOD_NAME" ]; then
    ln -s "$SCRIPT_DIR" "$ENGINE_ROOT/mods/$MOD_NAME"
    echo "Linked $MOD_NAME into mods/"
fi

# Launch the engine from the engine root (so config.json is found)
cd "$ENGINE_ROOT"
exec ./build/gloaming
