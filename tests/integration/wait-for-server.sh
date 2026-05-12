#!/usr/bin/env bash
# wait-for-server.sh — Wait for LdnServer to accept TCP connections
# Uses /dev/tcp (bash) or python3 as fallback (no nc dependency)
# Usage: ./wait-for-server.sh [host] [port] [timeout_seconds]
set -e

HOST="${1:-localhost}"
PORT="${2:-30456}"
TIMEOUT="${3:-60}"

echo "[wait] Waiting for LdnServer at ${HOST}:${PORT} (timeout: ${TIMEOUT}s)"

# Try TCP using bash /dev/tcp, fall back to python3
check_tcp() {
    if (echo > "/dev/tcp/${HOST}/${PORT}") 2>/dev/null; then
        return 0
    fi
    # Fallback: python3
    python3 -c "
import socket, sys
s = socket.socket()
s.settimeout(2)
try:
    s.connect(('${HOST}', ${PORT}))
    s.close()
    sys.exit(0)
except:
    sys.exit(1)
" 2>/dev/null
}

elapsed=0
while ! check_tcp; do
    if [ "$elapsed" -ge "$TIMEOUT" ]; then
        echo "[wait] TIMEOUT — server not ready after ${TIMEOUT}s" >&2
        exit 1
    fi
    sleep 1
    elapsed=$((elapsed + 1))
    printf "[wait] Still waiting... (%ds)\r" "$elapsed"
done

echo "[wait] Server is ready at ${HOST}:${PORT} (took ~${elapsed}s)"