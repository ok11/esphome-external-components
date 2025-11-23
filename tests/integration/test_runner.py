#!/usr/bin/env python3
"""
Integration Test Runner for ESPHome External Components

This script orchestrates all integration tests:
1. Waits for ESPHome to be ready
2. Runs compilation tests
3. Generates test report
"""

import sys
import time
import subprocess
import pathlib

import requests

# Test configuration
ESPHOME_HOST = "localhost"
ESPHOME_PORT = 6052
MAX_WAIT_TIME = 60  # seconds


def wait_for_esphome():
    """Wait for ESPHome to be ready"""
    print("Waiting for ESPHome to be ready...")
    start_time = time.time()

    while time.time() - start_time < MAX_WAIT_TIME:
        try:
            response = requests.get(f"http://{ESPHOME_HOST}:{ESPHOME_PORT}", timeout=2)
            if response.status_code == 200:
                print("✓ ESPHome is ready")
                return True
        except Exception:
            pass

        time.sleep(2)
        print("  Still waiting...")

    print("✗ ESPHome did not become ready in time")
    return False


def run_compilation_tests():
    """Run compilation tests for all test configs"""
    print("\n" + "=" * 60)
    print("Running Compilation Tests")
    print("=" * 60)

    test_configs_dir = pathlib.Path(__file__).parent / "test_configs"
    test_configs = list(test_configs_dir.glob("*.yaml"))

    if not test_configs:
        print("✗ No test configurations found")
        return False

    results = []
    for config in test_configs:
        print(f"\nTesting: {config.name}")
        print("-" * 60)

        # Run ESPHome compile command
        cmd = [
            "docker", "exec", "esphome-test",
            "esphome", "compile", f"/config/test_configs/{config.name}"
        ]

        try:
            # Stream output to stdout/stderr in real-time
            result = subprocess.run(
                cmd,
                timeout=900,  # 15 minutes timeout
                check=False
            )

            if result.returncode == 0:
                print(f"✓ {config.name} compiled successfully")
                results.append((config.name, True, None))
            else:
                print(f"✗ {config.name} failed to compile")
                results.append((config.name, False, "See output above"))
        except subprocess.TimeoutExpired:
            print(f"✗ {config.name} compilation timed out")
            results.append((config.name, False, "Timeout"))
        except Exception as e:
            print(f"✗ {config.name} error: {e}")
            results.append((config.name, False, str(e)))

    return results


def print_summary(results):
    """Print test summary"""
    print("\n" + "=" * 60)
    print("Test Summary")
    print("=" * 60)

    passed = sum(1 for _, success, _ in results if success)
    failed = len(results) - passed

    print(f"\nTotal: {len(results)} tests")
    print(f"✓ Passed: {passed}")
    print(f"✗ Failed: {failed}")

    if failed > 0:
        print("\nFailed tests:")
        for name, success, error in results:
            if not success:
                print(f"  - {name}")
                if error:
                    print(f"    Error: {error[:100]}...")

    return failed == 0


def main():
    """Main test execution"""
    print("=" * 60)
    print("ESPHome Integration Test Runner")
    print("=" * 60)

    # Wait for ESPHome
    if not wait_for_esphome():
        print("\n✗ Tests aborted: ESPHome not ready")
        return 1

    # Run compilation tests
    results = run_compilation_tests()

    if not results:
        print("\n✗ No tests were run")
        return 1

    # Print summary
    all_passed = print_summary(results)

    print("\n" + "=" * 60)
    if all_passed:
        print("✓ All tests passed!")
        print("=" * 60)
        return 0
    else:
        print("✗ Some tests failed")
        print("=" * 60)
        return 1


if __name__ == "__main__":
    sys.exit(main())
