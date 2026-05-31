#!/bin/bash
# CodeQL database creation for ryu_ldn_nx
# Creates a C++ CodeQL database scoped to sysmodule/source/ only (excl. Atmosphere-libs)
set -euo pipefail

echo "[codeql] Creating C++ database..."

DB_DIR="/workspace/.codeql-db"
rm -rf "$DB_DIR"

# We need the cross-compiler for include resolution
DEVKITA64=${DEVKITA64:-/opt/devkitpro/devkitA64}
DEVKITPRO=${DEVKITPRO:-/opt/devkitpro}
export PATH="${DEVKITPRO}/tools/bin:${DEVKITA64}/bin:${PATH}"

# Symlink pre-built libstratosphere
LIB_DIR="/workspace/sysmodule/Atmosphere-libs/libstratosphere/lib/nintendo_nx_arm64_armv8a/release"
PREBUILT="/opt/ryu_ldn_nx/libstratosphere/lib/nintendo_nx_arm64_armv8a/release/libstratosphere.a"
if [ -f "$PREBUILT" ] && [ ! -f "$LIB_DIR/libstratosphere.a" ]; then
    mkdir -p "$LIB_DIR"
    ln -sf "$PREBUILT" "$LIB_DIR/libstratosphere.a"
fi

codeql database create "$DB_DIR" \
    --language=cpp \
    --source-root=/workspace \
    --overwrite \
    --command="" 2>&1 || true

# CodeQL's auto-detection may pull Atmosphere-libs. We do a manual
# build-tracer approach instead.
echo "[codeql] Using build-tracer for targeted extraction..."
codeql database init --source-root=/workspace --language=cpp "$DB_DIR"

# Build a compile_commands.json for just our source files
SOURCES_DIR="/workspace/sysmodule/source"
CPP_FILES=$(find "$SOURCES_DIR" -name "*.cpp" ! -path "*/Atmosphere-libs/*" | sort)

INCLUDE_FLAGS=""
for dir in "$SOURCES_DIR" \
    "$SOURCES_DIR/config" "$SOURCES_DIR/debug" "$SOURCES_DIR/network" \
    "$SOURCES_DIR/protocol" "$SOURCES_DIR/ldn" "$SOURCES_DIR/bsd" "$SOURCES_DIR/p2p" \
    "/workspace/sysmodule/Atmosphere-libs/libstratosphere/include" \
    "/workspace/sysmodule/Atmosphere-libs/libvapours/include" \
    "/opt/ryu_ldn_nx/libstratosphere/include" \
    "/opt/ryu_ldn_nx/libvapours/include" \
    "${DEVKITPRO}/libnx/include" \
    "${DEVKITA64}/aarch64-none-elf/include" \
    "${DEVKITA64}/aarch64-none-elf/include/c++/14.2.0" \
    "${DEVKITA64}/aarch64-none-elf/include/c++/14.2.0/aarch64-none-elf"; do
    INCLUDE_FLAGS="$INCLUDE_FLAGS -I$dir"
done

mkdir -p /workspace/build-logs

# Extract each file using the build tracer
for f in $CPP_FILES; do
    codeql database trace-command "$DB_DIR" -- aarch64-none-elf-g++ -D__SWITCH__ -std=c++23 $INCLUDE_FLAGS -c "$f" -o /dev/null
done >> /workspace/build-logs/codeql-trace.log 2>&1

codeql database finalize "$DB_DIR"
echo "[codeql] ✅ Database created at $DB_DIR"
