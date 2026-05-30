#!/bin/bash
# clang-tidy runner for ryu_ldn_nx project source files
# Scans sysmodule/source/ (not Atmosphere-libs/) using the project's
# cross-compiler flags from the sysmodule Makefile environment.
set -euo pipefail

echo "[lint] Running clang-tidy on ryu_ldn_nx source files..."

# Build the compilation database from the sysmodule Makefile
cd /workspace/sysmodule

# We need the cross-compiler from devkitA64 in PATH
DEVKITA64=${DEVKITA64:-/opt/devkitpro/devkitA64}
DEVKITPRO=${DEVKITPRO:-/opt/devkitpro}
export PATH="${DEVKITPRO}/tools/bin:${DEVKITA64}/bin:${PATH}"

# Symlink pre-built libstratosphere so includes resolve
LIB_DIR="/workspace/sysmodule/Atmosphere-libs/libstratosphere/lib/nintendo_nx_arm64_armv8a/release"
PREBUILT="/opt/ryu_ldn_nx/libstratosphere/lib/nintendo_nx_arm64_armv8a/release/libstratosphere.a"
if [ -f "$PREBUILT" ] && [ ! -f "$LIB_DIR/libstratosphere.a" ]; then
    mkdir -p "$LIB_DIR"
    ln -sf "$PREBUILT" "$LIB_DIR/libstratosphere.a"
fi

# Collect all .cpp files from sysmodule/source/ (exclude Atmosphere-libs)
SOURCES_DIR="/workspace/sysmodule/source"
mapfile -t CPP_FILES < <(find "$SOURCES_DIR" -name "*.cpp" ! -path "*/Atmosphere-libs/*" | sort)

# Build include flags matching the sysmodule Makefile setup
INCLUDE_FLAGS=(
    "-I$SOURCES_DIR"
    "-I$SOURCES_DIR/config"
    "-I$SOURCES_DIR/debug"
    "-I$SOURCES_DIR/network"
    "-I$SOURCES_DIR/protocol"
    "-I$SOURCES_DIR/ldn"
    "-I$SOURCES_DIR/bsd"
    "-I$SOURCES_DIR/p2p"
    "-I/workspace/sysmodule/Atmosphere-libs/libstratosphere/include"
    "-I/workspace/sysmodule/Atmosphere-libs/libvapours/include"
    "-I/opt/ryu_ldn_nx/libstratosphere/include"
    "-I/opt/ryu_ldn_nx/libvapours/include"
    "-I${DEVKITPRO}/libnx/include"
    "-I${DEVKITA64}/aarch64-none-elf/include"
    "-I${DEVKITA64}/aarch64-none-elf/include/c++/14.2.0"
    "-I${DEVKITA64}/aarch64-none-elf/include/c++/14.2.0/aarch64-none-elf"
    "-D__SWITCH__"
    "-std=c++23"
)

# Build compile_commands.json for clang-tidy
echo "[lint] Generating compile_commands.json..."
COMPILE_DB="/workspace/sysmodule/build/compile_commands.json"
mkdir -p /workspace/sysmodule/build

echo "[" > "$COMPILE_DB"
first=true
for f in "${CPP_FILES[@]}"; do
    if [ "$first" = true ]; then
        first=false
    else
        echo "," >> "$COMPILE_DB"
    fi
    printf '  {\n    "directory": "/workspace/sysmodule",\n    "command": "aarch64-none-elf-g++ %s -c %s",\n    "file": "%s"\n  }' \
        "${INCLUDE_FLAGS[*]}" "$f" "$f" >> "$COMPILE_DB"
done
echo "]" >> "$COMPILE_DB"

# Count files
NUM_FILES=${#CPP_FILES[@]}
echo "[lint] Found $NUM_FILES source files to check"

# Run clang-tidy with the compilation database
# -p points to the build directory containing compile_commands.json
# We use a subset of checks appropriate for embedded/system C++
CHECKS="
    -*,clang-analyzer-*,
    bugprone-*,
    -bugprone-easily-swappable-parameters,
    -bugprone-implicit-widening-of-multiplication-result,
    -bugprone-narrowing-conversions,
    -bugprone-reserved-identifier,
    misc-*,
    -misc-const-correctness,
    -misc-include-cleaner,
    -misc-non-private-member-variables-in-classes,
    -misc-no-recursion,
    -misc-unused-parameters,
    -misc-use-anonymous-namespace,
    modernize-*,
    -modernize-avoid-c-arrays,
    -modernize-use-trailing-return-type,
    -modernize-macro-to-enum,
    performance-*,
    -performance-enum-size,
    -performance-avoid-endl,
    portability-*,
    readability-*,
    -readability-function-cognitive-complexity,
    -readability-identifier-length,
    -readability-identifier-naming,
    -readability-magic-numbers,
    -readability-redundant-access-specifiers,
    -readability-use-anyofallof
"
# Collapse whitespace
CHECKS=$(echo "$CHECKS" | tr -d '\n' | tr -s ',')

echo "[lint] Running clang-tidy (checks enabled, this may take a minute)..."
# Run on all source files at once
clang-tidy -p=/workspace/sysmodule/build \
    --checks="$CHECKS" \
    "${CPP_FILES[@]}" 2>&1 | tee /workspace/build-logs/clang-tidy.log

EXIT_CODE=${PIPESTATUS[0]}
if [ $EXIT_CODE -eq 0 ]; then
    echo "[lint] ✅ clang-tidy passed with no errors"
else
    echo "[lint] ❌ clang-tidy found $EXIT_CODE issue(s) — see /workspace/build-logs/clang-tidy.log"
fi
exit $EXIT_CODE
