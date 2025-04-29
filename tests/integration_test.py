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

# ------------------- Configuration -------------------
TEST_PORT = 80
SERVER_BIN = "bin/server"
CONFIG_FILE_BASE_PATH = "../tests/integration_testcases/"

# Global state for server process
server_proc = None
log_file = None

# ------------------- Server Control -------------------


def start_server(config_file):
    """
    Starts the server as a background subprocess.
    Output is written to a temporary log file.
    """
    global server_proc, log_file
    log_file = tempfile.NamedTemporaryFile(delete=False)
    build_dir = os.path.abspath(os.getcwd())
    server_path = os.path.join(build_dir, SERVER_BIN)
    config_file = os.path.join(CONFIG_FILE_BASE_PATH, config_file)
    if not os.path.isfile(server_path):
        raise FileNotFoundError("Can't find server at {server_path}")
    server_proc = subprocess.Popen(
        [server_path, config_file],
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


def run_curl_test(uri="") -> bytes:
    """
    Sends a simple HTTP GET request using curl.
    """
    return subprocess.check_output([
        "curl", "-sS", f"http://localhost:{TEST_PORT}{uri}"
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
        actual = run_nc_test(method) if use_nc else run_curl_test(method)
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
        {"config": "simple_config",
         "tests": [
             {
                 "name": "Valid echo request with curl",
                 "method": "/echo",
                 "expected": b"GET /echo HTTP/1.1\r\nHost: localhost\r\nUser-Agent: curl/8.5.0\r\nAccept: */*\r\n\r\n",
                 "use_nc": False

             },
             {
                 "name": "Valid echo request",
                 "method": b"GET /echo HTTP/1.1\r\nHost: localhost\r\nUser-Agent: curl/8.5.0\r\nAccept: */*\r\n\r\n",
                 "expected": b"HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 76\r\n\r\nGET /echo HTTP/1.1\r\nHost: localhost\r\nUser-Agent: curl/8.5.0\r\nAccept: */*\r\n\r\n",
                 "use_nc": True
             },
             {
                 "name": "Valid static file request",
                 "method": b"GET /static/test1/test.txt HTTP/1.1\r\nHost: localhost\r\nUser-Agent: curl/8.5.0\r\nAccept: */*\r\n\r\n",
                 "expected": b"HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 18\r\n\r\nline1\nline2\n\nline4",
                 "use_nc": True
             },
             {
                 "name": "Invalid Method",
                 "method": b"FETCH /echo HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n",
                 "expected": b"HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 15\r\n\r\n400 Bad Request",
                 "use_nc": True
             },
             {
                 "name": "Unsupported request",
                 "method": b"GET /video HTTP/1.1\r\nHost: localhost\r\nUser-Agent: curl/8.5.0\r\nAccept: */*\r\n\r\n",
                 "expected": b"HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nContent-Length: 13\r\n\r\n404 Not Found",
                 "use_nc": True
             },
         ]},
        {"config": "static_config",
         "tests": [
             {
                 "name": "Valid static file request with alt file path",
                 "method": b"GET /file/test.txt HTTP/1.1\r\nHost: localhost\r\nUser-Agent: curl/8.5.0\r\nAccept: */*\r\n\r\n",
                 "expected": b"HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 18\r\n\r\nline1\nline2\n\nline4",
                 "use_nc": True
             },
         ]}
    ]

# ------------------- Main Runner -------------------


def main():
    """
    Main entry point: starts the server, runs all tests, exits with appropriate code.
    """
    all_passed = True
    tests = define_tests()

    for test in tests:
        config_file = test["config"]
        if not os.path.isfile(os.path.join(CONFIG_FILE_BASE_PATH, config_file)):
            print(f"Config file {config_file} not found.")
            continue
        print(f"Running tests for config: {config_file}")
        start_server(config_file)
        try:
            for test_case in test["tests"]:
                all_passed &= run_test(
                    test_case["name"],
                    test_case["method"],
                    test_case["expected"],
                    test_case["use_nc"]
                )
        finally:
            stop_server()

    print("All integration tests passed." if all_passed else "Some integration tests failed.")
    sys.exit(0 if all_passed else 1)


if __name__ == "__main__":
    main()
