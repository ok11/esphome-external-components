# Integration Tests

This directory contains integration tests that validate the climate_ir_woleix component against a real ESPHome installation running in Docker.

## Overview

Unlike unit tests that use mocks, integration tests:

- Use real ESPHome in a Docker container
- Compile actual YAML configurations
- Validate external component loading
- Test real-world scenarios

## Structure

```text
integration/
├── docker-compose.yml              # Docker orchestration
├── test_configs/                   # ESPHome YAML configurations
│   ├── basic_climate.yaml          # Minimal configuration
│   ├── full_features.yaml          # Full-featured configuration
│   └── ...
├── test_runner.py                  # Main test orchestrator
├── scripts/                        # Helper scripts
│   └── wait_for_esphome.sh         # Wait for ESPHome ready
├── run_tests.sh                    # Main entry point
├── output/                         # Compiled firmware (gitignored)
└── README.md                       # This file
```

## Quick Start

### Run All Integration Tests

```bash
cd tests/integration
./run_tests.sh
```

This script will:

1. Build and start ESPHome in Docker
2. Wait for ESPHome to be ready
3. Run all compilation tests
4. Display results
5. Clean up containers

### Keep Containers Running

Useful for debugging:

```bash
KEEP_RUNNING=true ./run_tests.sh
```

Then manually inspect:

```bash
# View logs
docker-compose logs -f esphome

# Access dashboard
open http://localhost:6052

# Execute commands in container
docker exec -it esphome-test bash

# When done
docker-compose down
```

## Prerequisites

### Required

- Docker Desktop (macOS, Windows) or Docker Engine (Linux)
- Docker Compose
- Python 3.8+
- bash

### Optional (for manual testing)

- curl (for healthchecks)
- requests Python library

### Installation

**macOS:**

```bash
# Install Docker Desktop from https://www.docker.com/products/docker-desktop
brew install python
```

**Ubuntu:**

```bash
sudo apt-get update
sudo apt-get install -y docker.io docker-compose python3 python3-pip
sudo usermod -aG docker $USER  # Add yourself to docker group
# Log out and back in for group changes to take effect
```

## Test Configurations

### basic_climate.yaml

Minimal configuration to ensure component loads and compiles:

- ESP8266 platform
- Basic WiFi setup
- Simple climate component configuration
- No optional features

**Purpose:** Verify basic integration works

### full_features.yaml

Comprehensive configuration with all features:

- ESP32 platform
- Temperature sensor integration
- Web server enabled
- Full climate features
- Status indicators

**Purpose:** Test all component capabilities

### Adding New Test Configs

Create a new `.yaml` file in `test_configs/`:

```yaml
esphome:
  name: test-your-scenario
  platform: ESP8266
  board: nodemcuv2

external_components:
  - source: /config/external_components
    components: [ climate_ir_woleix ]

# ... rest of your configuration
```

The test runner automatically discovers and tests all YAML files in `test_configs/`.

## Manual Testing

### Start ESPHome Container

```bash
docker-compose up -d
```

### Access ESPHome Dashboard

Open in browser: <http://localhost:6052>

### Compile a Configuration Manually

```bash
docker exec esphome-test esphome compile /config/test_configs/basic_climate.yaml
```

### View Logs

```bash
docker-compose logs -f esphome
```

### Stop Containers

```bash
docker-compose down
```

### Clean Everything

```bash
docker-compose down -v
rm -rf output/
```

## Writing Tests

### Current Test Coverage

The test runner currently performs:

1. **Compilation Tests**: Verifies each YAML config compiles successfully
2. **Error Detection**: Catches compilation failures and reports errors
3. **Timeout Handling**: Detects stuck compilations

### Future Enhancements

Potential additions for more comprehensive testing:

1. **API Connectivity Tests**

   ```python
   # Using aioesphomeapi
   async def test_api_connection():
       client = APIClient(...)
       await client.connect()
       entities = await client.list_entities_services()
       # Verify climate entity exists
   ```

2. **Entity Validation Tests**

   ```python
   async def test_climate_entity_traits():
       # Connect to API
       # Find climate entity
       # Verify temperature range, modes, etc.
   ```

3. **State Tests**

   ```python
   async def test_climate_state_changes():
       # Send commands via API
       # Verify state updates
   ```

### Extending test_runner.py

Add new test functions to `test_runner.py`:

```python
def run_api_tests():
    """Add API connectivity tests"""
    # Implementation here
    pass

# In main():
def main():
    # ... existing code ...
    
    # Add new test phase
    api_results = run_api_tests()
    
    # ... rest of code ...
```

## Docker Configuration

### Environment Variables

Set in `docker-compose.yml` or via command line:

```bash
# Keep containers running after tests
KEEP_RUNNING=true ./run_tests.sh

# Custom ESPHome port
ESPHOME_PORT=8080 docker-compose up
```

### Volumes

- `../../esphome/components` → `/config/external_components` (read-only)
  - Your external component source code
- `./test_configs` → `/config/test_configs` (read-only)
  - Test YAML configurations
- `./output` → `/config/.esphome` (read-write)
  - Compiled firmware and build artifacts

### Ports

- `6052` - ESPHome web dashboard
- `6053` - ESPHome native API (for future API testing)

## Troubleshooting

### Tests Timeout

**Symptom:** Tests hang waiting for ESPHome

**Solutions:**

1. Increase timeout: Edit `MAX_WAIT_TIME` in `test_runner.py`
2. Check Docker resources: Ensure Docker has enough CPU/RAM
3. View logs: `docker-compose logs esphome`

### Compilation Failures

**Symptom:** YAML configs fail to compile

**Solutions:**

1. Check component syntax in YAML
2. Verify external_components path is correct
3. Review ESPHome logs: `docker-compose logs esphome`
4. Test YAML manually: `docker exec esphome-test esphome compile /config/test_configs/your-config.yaml`

### Docker Image Pull Fails

**Symptom:** Cannot pull ESPHome image

**Solutions:**

1. Check Docker is running: `docker info`
2. Pull base image manually: `docker pull esphome/esphome:latest`
3. Check network connectivity
4. Check Docker Hub status

### Permission Issues

**Symptom:** Cannot write to output directory

**Solutions:**

1. Check directory permissions: `ls -la output/`
2. Create directory: `mkdir -p output`
3. On Linux, ensure user is in docker group: `groups`

### Port Already in Use

**Symptom:** Cannot bind to port 6052

**Solutions:**

1. Stop conflicting service
2. Change port in docker-compose.yml
3. Check what's using port: `lsof -i :6052` (macOS) or `netstat -tlnp | grep 6052` (Linux)

## Performance Considerations

### Build Time

First run is slow (pulls base image, installs dependencies):

- First run: ~5-10 minutes
- Subsequent runs: ~1-2 minutes per config

### Caching

Docker caches the ESPHome base image:

- Base image is cached after first pull
- Only component changes in volumes affect compilation
- No custom build layers

### Optimization Tips

1. **Minimal configs**: Keep test configs simple to reduce compilation time
2. **Parallel builds**: Run multiple configs in parallel (future enhancement)
3. **Cache warming**: Pre-pull base image: `docker pull esphome/esphome:latest`

## CI/CD Integration

### GitHub Actions

```yaml
name: Integration Tests

on: [push, pull_request]

jobs:
  integration:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Set up Docker Buildx
        uses: docker/setup-buildx-action@v2
      
      - name: Run integration tests
        run: |
          cd tests/integration
          ./run_tests.sh
      
      - name: Upload logs on failure
        if: failure()
        uses: actions/upload-artifact@v3
        with:
          name: esphome-logs
          path: tests/integration/output/
```

### GitLab CI

```yaml
integration_tests:
  image: docker:latest
  services:
    - docker:dind
  script:
    - cd tests/integration
    - ./run_tests.sh
  artifacts:
    when: on_failure
    paths:
      - tests/integration/output/
```

## Best Practices

1. **Keep configs minimal**: Only include necessary features
2. **Test edge cases**: Create configs for unusual scenarios
3. **Document failures**: Add comments explaining why a config should fail
4. **Clean between runs**: Remove output/ directory to ensure fresh builds
5. **Version ESPHome**: Pin ESPHome version in docker-compose.yml for reproducibility (e.g., `image: esphome/esphome:2025.5.2`)

## Related Documentation

- [Main Tests README](../README.md) - Overview of all tests
- [Unit Tests README](../unit/README.md) - Unit testing guide
- [ESPHome External Components](https://esphome.io/components/external_components.html)
- [Docker Documentation](https://docs.docker.com/)
