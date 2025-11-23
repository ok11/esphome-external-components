#!/bin/bash
#
# Main entry point for integration tests
# This script:
# 1. Starts Docker Compose
# 2. Waits for ESPHome to be ready
# 3. Runs the Python test suite
# 4. Collects results
# 5. Tears down containers
# 6. Exits with appropriate code
#

set -e

# Get the directory of this script (integration root)
INTEGRATION_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuration
CLEANUP_ON_FAILURE=${CLEANUP_ON_FAILURE:-true}
KEEP_RUNNING=${KEEP_RUNNING:-false}

echo "======================================================================"
echo "ESPHome Integration Test Suite"
echo "======================================================================"
echo ""

# Change to integration directory
cd "$INTEGRATION_DIR"

# Function to cleanup
cleanup() {
  if [ "$KEEP_RUNNING" = "false" ]; then
    echo ""
    echo "Cleaning up Docker containers..."
    docker-compose down -v 2>/dev/null || true
    echo "✓ Cleanup complete"
  else
    echo ""
    echo -e "${YELLOW}Note: Containers are still running (KEEP_RUNNING=true)${NC}"
    echo "To stop them manually, run: cd $INTEGRATION_DIR && docker-compose down"
  fi
}

# Trap to ensure cleanup happens
trap cleanup EXIT

# Step 1: Build and start Docker containers
echo "Step 1: Building and starting Docker containers..."
echo "----------------------------------------------------------------------"
if docker-compose --env-file ../../.env up -d --build; then
  echo -e "${GREEN}✓ Containers started${NC}"
else
  echo -e "${RED}✗ Failed to start containers${NC}"
  exit 1
fi
echo ""

# Step 2: Wait for ESPHome to be ready
echo "Step 2: Waiting for ESPHome to be ready..."
echo "----------------------------------------------------------------------"
if bash "$INTEGRATION_DIR/scripts/wait_for_esphome.sh"; then
  echo -e "${GREEN}✓ ESPHome is ready${NC}"
else
  echo -e "${RED}✗ ESPHome failed to start${NC}"
  docker-compose logs esphome
  exit 1
fi
echo ""

# Step 3: Run the test suite
echo "Step 3: Running integration tests..."
echo "----------------------------------------------------------------------"
if "$INTEGRATION_DIR/../../.venv/bin/python" "$INTEGRATION_DIR/test_runner.py"; then
  TEST_RESULT=0
  echo ""
  echo -e "${GREEN}✓ All tests passed!${NC}"
else
  TEST_RESULT=1
  echo ""
  echo -e "${RED}✗ Some tests failed${NC}"
fi
echo ""

# Step 4: Display logs if tests failed
if [ $TEST_RESULT -ne 0 ]; then
  echo "ESPHome logs (last 50 lines):"
  echo "----------------------------------------------------------------------"
  docker-compose logs --tail=50 esphome
  echo ""
fi

# Step 5: Summary
echo "======================================================================"
if [ $TEST_RESULT -eq 0 ]; then
  echo -e "${GREEN}Integration Test Suite: PASSED${NC}"
else
  echo -e "${RED}Integration Test Suite: FAILED${NC}"
fi
echo "======================================================================"

exit $TEST_RESULT
