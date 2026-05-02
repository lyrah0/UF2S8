#!/usr/bin/env bash

# Script to regenerate clean compilation databases for both assembler and emulator
# It uses 'bear' to capture the build and 'jq' to deduplicate entries.

set -e

REPO_ROOT=$(git rev-parse --show-toplevel 2>/dev/null || pwd)

update_db() {
    local dir=$1
    local target=$2
    echo "Updating compilation database in $dir..."
    pushd "$dir" > /dev/null
    
    # Clean previous build
    make clean > /dev/null 2>&1 || true
    
    # Capture only the compilation of object files to avoid running 'check' or 'tests'
    # which might fail if the DB or code has issues.
    bear -- make "$target" > /dev/null || echo "Warning: make $target failed, database might be incomplete."
    
    # Deduplicate entries and clean up
    if [ -f compile_commands.json ]; then
        echo "Filtering and deduplicating entries..."
        # Remove GCC-specific flags that confuse Clang-based tools (clangd, clang-tidy)
        # and keep only the first entry per file.
        jq 'map(
            .arguments |= map(select(
                (. | startswith("-fanalyzer") | not) and
                (. | startswith("--param") | not) and
                (. | startswith("-ftrack-macro-expansion") | not) and
                (. | startswith("-fvect-cost-model") | not) and
                (. | startswith("-mrecip") | not) and
                (. | startswith("-mtls-dialect") | not) and
                (. | startswith("-fdevirtualize-at-ltrans") | not) and
                (. | startswith("-fuse-linker-plugin") | not) and
                (. | startswith("-fmerge-all-constants") | not) and
                (. | startswith("-fno-plt") | not) and
                (. | startswith("-fipa-pta") | not) and
                (. | startswith("-flto") | not) and
                (. | startswith("-fweb") | not) and
                (. | startswith("-fgcse-las") | not) and
                (. | startswith("-fgcse-sm") | not)
            ))
        ) | unique_by(.file)' compile_commands.json > compile_commands.json.tmp && mv compile_commands.json.tmp compile_commands.json
    else
        echo "Error: compile_commands.json not generated in $dir"
    fi
    
    popd > /dev/null
}

update_db "$REPO_ROOT/assembler" "bin/assembler"
update_db "$REPO_ROOT/emulator" "bin/emulator"
update_db "$REPO_ROOT/compiler" "bin/compiler"

echo "All compilation databases updated successfully."
