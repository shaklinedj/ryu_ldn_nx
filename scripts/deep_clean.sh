#!/bin/bash
set -e
echo "Cleaning libstratosphere build artifacts..."
find /workspace/sysmodule/Atmosphere-libs/libstratosphere/build -type f -name '*.o' -delete 2>/dev/null || true
find /workspace/sysmodule/Atmosphere-libs/libstratosphere/build -type f -name '*.d' -delete 2>/dev/null || true
rm -f /workspace/sysmodule/Atmosphere-libs/libstratosphere/lib/nintendo_nx_arm64_armv8a/release/libstratosphere.a

echo "Cleaning sysmodule build artifacts..."
find /workspace/sysmodule/build -type f -name '*.o' -delete 2>/dev/null || true
find /workspace/sysmodule/build -type f -name '*.d' -delete 2>/dev/null || true
rm -f /workspace/sysmodule/ryu_ldn_nx.elf
rm -f /workspace/sysmodule/ryu_ldn_nx.nso
rm -f /workspace/sysmodule/ryu_ldn_nx.nsp
rm -f /workspace/sysmodule/ryu_ldn_nx.npdm

echo "Verifying clean state..."
if [ -f /workspace/sysmodule/Atmosphere-libs/libstratosphere/lib/nintendo_nx_arm64_armv8a/release/libstratosphere.a ]; then
    echo "WARNING: libstratosphere.a still exists"
    ls -la /workspace/sysmodule/Atmosphere-libs/libstratosphere/lib/nintendo_nx_arm64_armv8a/release/libstratosphere.a
else
    echo "OK: libstratosphere.a removed"
fi

count=$(find /workspace/sysmodule/Atmosphere-libs/libstratosphere/build -name '*.o' 2>/dev/null | wc -l || true)
echo "Remaining .o files in libstratosphere build: $count"

echo "Deep clean done."