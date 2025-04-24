#!/usr/bin/env python3
"""
Integration Test Script

This script starts the web server, sends a series of test requests (both valid and invalid),
verifies the server's responses, and shuts down the server cleanly.

Usage:
    ./integration_test.py [config_file]

Returns:
    Exit code 0 if all tests pass, 1 if any test fails
"""

import subprocess
import tempfile
import time
import os
import sys

CONFIG_FILE = sys.argv[1] if len(sys.argv) > 1 else "my_config"
TEST_PORT = 80
SERVER_BIN = "bin/server"

# Global state for server process
server_proc = None
log_file = None

# ------------------- Server Control -------------------


def start_server():
    """
    Starts the server as a background subprocess.
    Output is written to a temporary log file.
    """
    global server_proc, log_file
    log_file = tempfile.NamedTemporaryFile(delete=False)
    build_dir = os.path.abspath(os.getcwd())
    server_path = os.path.join(build_dir, SERVER_BIN)
    if not os.path.isfile(server_path):
        raise FileNotFoundError("Can't find server at {server_path}")
    server_proc = subprocess.Popen(
        [server_path, CONFIG_FILE],
        cwd=build_dir,
        stdout=log_file,
        stderr=subprocess.STDOUT
    )
    time.sleep(1)  # Wait a moment for server to start


def stop_server():
    """
    Terminates the server process and cleans up the log file.
    """
    global server_proc, log_file
    if server_proc:
        server_proc.terminate()
        server_proc.wait()
    if log_file:
        os.unlink(log_file.name)

# ------------------- Test Execution -------------------


def run_nc_test(method: bytes) -> bytes:
    """
    Sends a raw HTTP request using netcat (nc).
    Uses -w 1 to ensure it terminates after 1 second.
    """
    cmd = f"printf '{method.decode()}' | nc -w 1 localhost {TEST_PORT}"
    proc = subprocess.run(
        cmd, shell=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        timeout=2
    )
    return proc.stdout


def run_curl_test() -> bytes:
    """
    Sends a simple HTTP GET request using curl.
    """
    return subprocess.check_output([
        "curl", "-sS", f"http://localhost:{TEST_PORT}/"
    ])


def compare_output(name, actual, expected):
    """
    Compares actual vs. expected response bytes.
    If they differ, prints a unified diff for inspection.
    """
    if actual == expected:
        print(f"{name}: SUCCESS")
        return True
    else:
        print(f"{name}: FAILED")
        with tempfile.NamedTemporaryFile(mode="wb", delete=False) as af:
            af.write(actual)
            actual_path = af.name
        with tempfile.NamedTemporaryFile(mode="wb", delete=False) as ef:
            ef.write(expected)
            expected_path = ef.name
        subprocess.run(["diff", "-u", expected_path, actual_path])
        return False


def run_test(name, method, expected_bytes, use_nc=False):
    """
    Executes a single test case, using either curl or nc depending on `use_nc`.
    """
    print(f"Running test: {name}...")
    try:
        actual = run_nc_test(method) if use_nc else run_curl_test()
        return compare_output(name, actual, expected_bytes)
    except subprocess.TimeoutExpired:
        print(f"{name}: TIMED OUT")
        return False
    except subprocess.CalledProcessError as e:
        print(f"{name}: FAILED with subprocess error")
        print(e.output.decode())
        return False

# ------------------- Test Cases -------------------


def define_tests():
    """
    Returns a list of test case definitions.
    Each test is a dictionary with: name, method, expected, use_nc
    """
    return [
        {
            "name": "Valid GET",
            "method": None,
            "expected": b"".join([
                b"GET / HTTP/1.1\r\n",
                b"Host: localhost\r\n",
                b"User-Agent: curl/8.5.0\r\n",
                b"Accept: */*\r\n",
                b"\r\n"
            ]),
            "use_nc": False
        },
        {
            "name": "Invalid Method",
            "method": b"FETCH /bad HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n",
            "expected": b"HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 15\r\n\r\n400 Bad Request",
            "use_nc": True
        },
        {
            "name": "Empty Request",
            "method": b"\r\nConnection: close\r\n\r\n",
            "expected": b"HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 15\r\n\r\n400 Bad Request",
            "use_nc": True
        }
    ]

# ------------------- Main Runner -------------------


def main():
    """
    Main entry point: starts the server, runs all tests, exits with appropriate code.
    """
    start_server()
    try:
        all_passed = True
        for test in define_tests():
            all_passed &= run_test(
                test["name"],
                test["method"],
                test["expected"],
                test["use_nc"]
            )
        print("All integration tests passed." if all_passed else "Some integration tests failed.")
        sys.exit(0 if all_passed else 1)
    finally:
        stop_server()


if __name__ == "__main__":
    main()
