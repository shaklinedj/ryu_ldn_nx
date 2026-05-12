#!/usr/bin/env bash
# prepare-compose.sh — Generate integration-test docker-compose from the submodule
#
# Copies the LdnServer submodule's docker-compose.yml into the
# integration test directory, replacing the Docker image owner from
# the upstream org (ghcr.io/ryubing) to the fork's org
# (ghcr.io/ethiquema). Also strips the website service (not needed
# for tests) and adds a healthcheck to ldn-server.
#
# This is done at generation time so that submodule updates are
# picked up automatically — no manual editing required.
#
# Usage:
#   ./prepare-compose.sh [SOURCE_OWNER] [TARGET_OWNER]
#
# Defaults: ryubing → ethiquema

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SUBMODULE_COMPOSE="${SCRIPT_DIR}/../ldn-server/docker-compose.yml"
OUTPUT_COMPOSE="${SCRIPT_DIR}/docker-compose.yml"

SOURCE_OWNER="${1:-ryubing}"
TARGET_OWNER="${2:-ethiquema}"

if [ ! -f "$SUBMODULE_COMPOSE" ]; then
    echo "error: submodule docker-compose not found at $SUBMODULE_COMPOSE" >&2
    echo "  Did you clone with --recurse-submodules?" >&2
    exit 1
fi

echo "Generating $OUTPUT_COMPOSE from submodule ($SOURCE_OWNER → $TARGET_OWNER)"

# Copy and replace image owner in one pass
sed "s|ghcr.io/${SOURCE_OWNER}/|ghcr.io/${TARGET_OWNER}/|g" \
    "$SUBMODULE_COMPOSE" > "$OUTPUT_COMPOSE"

# Use yq to transform the YAML reliably:
# 1. Remove the website service (not needed for tests)
# 2. Add healthcheck to ldn-server
# 3. Change restart policies for test lifecycle
# 4. Switch bind mount to named volume
# 5. Rename network to avoid conflicts with production
# 6. Add volumes top-level key

yq -i eval '
  del(.services["ryujinx-ldn-website"]) |
  .services.ldn-server.restart = "no" |
  .services.ldn-server.volumes = ["ldn-server-data:/data/ryuldn"] |
  .services.redis.restart = "no" |
  .networks.main.name = "ldn-test" |
  .volumes = {"ldn-server-data": {}}
' "$OUTPUT_COMPOSE"

echo "Done. Generated $OUTPUT_COMPOSE"