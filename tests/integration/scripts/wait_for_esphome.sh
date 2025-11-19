#!/bin/bash
#
# Wait for ESPHome to be ready
# This script waits for the ESPHome container to be healthy and responsive
#

set -e

ESPHOME_HOST="${ESPHOME_HOST:-localhost}"
ESPHOME_PORT="${ESPHOME_PORT:-6052}"
MAX_ATTEMPTS="${MAX_ATTEMPTS:-30}"
SLEEP_INTERVAL=2

echo "Waiting for ESPHome at $ESPHOME_HOST:$ESPHOME_PORT..."

attempt=0
while [ $attempt -lt $MAX_ATTEMPTS ]; do
  attempt=$((attempt + 1))
  
  if curl -sf "http://$ESPHOME_HOST:$ESPHOME_PORT" > /dev/null 2>&1; then
    echo "✓ ESPHome is ready (attempt $attempt/$MAX_ATTEMPTS)"
    exit 0
  fi
  
  echo "  Waiting... (attempt $attempt/$MAX_ATTEMPTS)"
  sleep $SLEEP_INTERVAL
done

echo "✗ ESPHome did not become ready after $MAX_ATTEMPTS attempts"
exit 1
