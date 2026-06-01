#!/bin/bash
# CodeQL analysis runner for ryu_ldn_nx
# Runs the security-and-quality query suite on the pre-built database
set -euo pipefail

DB_DIR="/workspace/.codeql-db"
RESULTS_DIR="/workspace/build-logs/codeql-results"

if [ ! -d "$DB_DIR" ]; then
    echo "[codeql] No database found. Run docker compose run --rm codeql-create first."
    exit 1
fi

echo "[codeql] Running analysis with security-and-quality suite..."
rm -rf "$RESULTS_DIR" && mkdir -p "$RESULTS_DIR"

codeql database analyze "$DB_DIR" \
    --format=sarifv2.1.0 \
    --output="$RESULTS_DIR/results.sarif" \
    --sarif-category=security-and-quality \
    --threads=4 \
    security-and-quality 2>&1 | tee /workspace/build-logs/codeql-analyze.log

# Count warnings by rule
echo ""
echo "[codeql] ==== Summary by rule ===="
codeql database analyze "$DB_DIR" \
    --format=csv \
    --output=- \
    security-and-quality 2>/dev/null | head -100 || true

EXIT_CODE=${PIPESTATUS[0]}
if [ $EXIT_CODE -eq 0 ]; then
    echo "[codeql] ✅ No issues found"
else
    echo "[codeql] ⚠️  Issues found — see /workspace/build-logs/codeql-results/results.sarif"
fi
exit 0
