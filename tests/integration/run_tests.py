#!/usr/bin/env python3
"""
Integration Test Runner for ESPHome External Components

This script orchestrates all integration tests:
1. Starts Docker Compose
2. Waits for ESPHome to be ready
3. Runs compilation tests
4. Cleans up containers
5. Reports results
"""

import sys
import time
import subprocess
import os
from pathlib import Path

# ANSI color codes
RED = '\033[0;31m'
GREEN = '\033[0;32m'
YELLOW = '\033[1;33m'
NC = '\033[0m'  # No Color

# Test configuration
ESPHOME_HOST = "localhost"
ESPHOME_PORT = 6053
MAX_WAIT_TIME = 60  # seconds
KEEP_RUNNING = os.environ.get('KEEP_RUNNING', 'false').lower() == 'true'


def print_color(message, color=''):
    """Print colored message"""
    if color:
        print(f"{color}{message}{NC}")
    else:
        print(message)


def print_header(message):
    """Print section header"""
    print("\n" + "=" * 70)
    print(message)
    print("=" * 70 + "\n")


def print_step(message):
    """Print step header"""
    print(f"\n{message}")
    print("-" * 70)


def wait_for_esphome():
    """Wait for ESPHome to be ready"""
    print("Waiting for ESPHome to be ready...")
    start_time = time.time()
    
    while time.time() - start_time < MAX_WAIT_TIME:
        try:
            import requests
            response = requests.get(f"http://{ESPHOME_HOST}:{ESPHOME_PORT}", timeout=2)
            if response.status_code == 200:
                print_color("✓ ESPHome is ready", GREEN)
                return True
        except Exception:
            pass
        
        time.sleep(2)
        print("  Still waiting...")
    
    print_color("✗ ESPHome did not become ready in time", RED)
    return False


def run_compilation_tests():
    """Run compilation tests for all test configs"""
    print_step("Running Compilation Tests")
    
    integration_dir = Path(__file__).parent
    test_configs_dir = integration_dir / "test_configs"
    test_configs = list(test_configs_dir.glob("*.yaml"))
    
    if not test_configs:
        print_color("✗ No test configurations found", RED)
        return []
    
    results = []
    for config in test_configs:
        print(f"\nTesting: {config.name}")
        print("-" * 70)
        
        # Run ESPHome compile command
        cmd = [
            "docker", "exec", "esphome-test",
            "esphome", "compile", f"/config/test_configs/{config.name}"
        ]
        
        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=300  # 5 minutes timeout
            )
            
            if result.returncode == 0:
                print_color(f"✓ {config.name} compiled successfully", GREEN)
                results.append((config.name, True, None))
            else:
                print_color(f"✗ {config.name} failed to compile", RED)
                print(f"Error output:\n{result.stderr[-500:]}")  # Last 500 chars
                results.append((config.name, False, result.stderr))
        except subprocess.TimeoutExpired:
            print_color(f"✗ {config.name} compilation timed out", RED)
            results.append((config.name, False, "Timeout"))
        except Exception as e:
            print_color(f"✗ {config.name} error: {e}", RED)
            results.append((config.name, False, str(e)))
    
    return results


def print_summary(results):
    """Print test summary"""
    print_header("Test Summary")
    
    passed = sum(1 for _, success, _ in results if success)
    failed = len(results) - passed
    
    print(f"\nTotal: {len(results)} tests")
    print_color(f"✓ Passed: {passed}", GREEN)
    if failed > 0:
        print_color(f"✗ Failed: {failed}", RED)
    
    if failed > 0:
        print("\nFailed tests:")
        for name, success, error in results:
            if not success:
                print(f"  - {name}")
                if error and len(error) > 0:
                    print(f"    Error: {error[:100]}...")
    
    return failed == 0


def start_containers():
    """Start Docker Compose containers"""
    print_step("Building and starting Docker containers...")
    
    try:
        result = subprocess.run(
            ["docker-compose", "up", "-d", "--build"],
            capture_output=True,
            text=True,
            check=True
        )
        print_color("✓ Containers started", GREEN)
        return True
    except subprocess.CalledProcessError as e:
        print_color("✗ Failed to start containers", RED)
        print(e.stderr)
        return False


def stop_containers():
    """Stop Docker Compose containers"""
    if not KEEP_RUNNING:
        print("\nCleaning up Docker containers...")
        try:
            subprocess.run(
                ["docker-compose", "down", "-v"],
                capture_output=True,
                check=False
            )
            print("✓ Cleanup complete")
        except Exception as e:
            print(f"Warning: Cleanup failed: {e}")
    else:
        integration_dir = Path(__file__).parent
        print_color(f"\nNote: Containers are still running (KEEP_RUNNING=true)", YELLOW)
        print(f"To stop them manually, run: cd {integration_dir} && docker-compose down")


def show_logs_on_failure():
    """Show ESPHome logs if tests failed"""
    print("\nESPHome logs (last 50 lines):")
    print("-" * 70)
    try:
        result = subprocess.run(
            ["docker-compose", "logs", "--tail=50", "esphome"],
            capture_output=True,
            text=True
        )
        print(result.stdout)
    except Exception as e:
        print(f"Could not retrieve logs: {e}")


def main():
    """Main test execution"""
    print_header("ESPHome Integration Test Suite")
    
    # Change to integration directory
    integration_dir = Path(__file__).parent
    os.chdir(integration_dir)
    
    try:
        # Step 1: Start containers
        if not start_containers():
            return 1
        
        # Step 2: Wait for ESPHome
        print_step("Waiting for ESPHome to be ready...")
        if not wait_for_esphome():
            print_color("\n✗ Tests aborted: ESPHome not ready", RED)
            show_logs_on_failure()
            return 1
        
        # Step 3: Run tests
        print_step("Running integration tests...")
        results = run_compilation_tests()
        
        if not results:
            print_color("\n✗ No tests were run", RED)
            return 1
        
        # Step 4: Print summary
        all_passed = print_summary(results)
        
        # Step 5: Show logs if failed
        if not all_passed:
            show_logs_on_failure()
        
        # Final result
        print_header("Integration Test Suite: " + ("PASSED" if all_passed else "FAILED"))
        
        return 0 if all_passed else 1
        
    finally:
        stop_containers()


if __name__ == "__main__":
    sys.exit(main())
