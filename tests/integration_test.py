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
import shutil
import json

# ------------------- Configuration -------------------
TEST_PORT = 80
SERVER_BIN = "bin/server"
CONFIG_FILE_BASE_PATH = "../"
CONFIG_FILE_INTEGRATION_PATH = "../tests/integration_testcases/"
TEMP_FILE_PATH = "../tests/temp/"


# Global state for server process
server_proc = None
log_file = None

# ------------------- Server Control -------------------


def start_server(config_file, integration_test_folder_exists):
    """
    Starts the server as a background subprocess.
    Output is written to a temporary log file.
    """
    global server_proc, log_file
    create_test_suite()
    log_file = tempfile.NamedTemporaryFile(delete=False)
    build_dir = os.path.abspath(os.getcwd())
    server_path = os.path.join(build_dir, SERVER_BIN)
    if integration_test_folder_exists:
        config_file = os.path.join(CONFIG_FILE_INTEGRATION_PATH, config_file)
    else:
        config_file = os.path.join(CONFIG_FILE_BASE_PATH, config_file)
    if not os.path.isfile(server_path):
        raise FileNotFoundError("Can't find server at {server_path}")
    server_proc = subprocess.Popen(
        [server_path, config_file],
        cwd=build_dir,
        stdout=log_file,
        stderr=subprocess.STDOUT
    )
    time.sleep(0.5)  # Wait a moment for server to start


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


def cleanup_test_suite():
    """
    Deletes the current test directory, if it exists.
    """
    if os.path.isdir(TEMP_FILE_PATH):
        shutil.rmtree(TEMP_FILE_PATH)

def create_test_suite():
    """
    Creates a new test directory, removing previous contents if it exists.
    """
    cleanup_test_suite()
    os.mkdir(TEMP_FILE_PATH)


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
        return True
    else:
        print(f"{name}: Actual vs Expected do not match.")
        with tempfile.NamedTemporaryFile(mode="wb", delete=False) as af:
            af.write(actual)
            actual_path = af.name
        with tempfile.NamedTemporaryFile(mode="wb", delete=False) as ef:
            ef.write(expected)
            expected_path = ef.name
        subprocess.run(["diff", "-u", expected_path, actual_path])
        sys.stdout.flush()
        return False


def run_test(name, method, expected_bytes, use_nc=False, crud_args=None):
    """
    Executes a single test case, using either curl or nc depending on `use_nc`.
    'crud': expects crud type as a string: GET, POST, PUT, LIST,DELETE,
    """
    print("======================================")
    print(f"Running test: {name}...")
    sys.stdout.flush()
    response_correct = False
    try:
        actual = run_nc_test(method) if use_nc else run_curl_test(method)
        response_correct = compare_output(name, actual, expected_bytes)
    except subprocess.TimeoutExpired:
        print(f"{name}: TIMED OUT")
        response_correct = False
    except subprocess.CalledProcessError as e:
        print(f"{name}: FAILED with subprocess error")
        print(e.output.decode())
        response_correct = False

    # Verify File Contents in CRUD
    if not crud_args:
        if response_correct:
            print(f"{name}: PASSED")
        else:
            print(f"{name}: FAILED: Invalid Response Content")
        return response_correct

    test_result = False
    if crud_args["type"] == "POST":
        test_result = validate_POST(response_correct,crud_args)
    elif crud_args["type"] == "GET":
        test_result = validate_GET(response_correct,crud_args) 
    elif crud_args["type"] == "PUT":
        test_result = validate_PUT(response_correct,crud_args)
    elif crud_args["type"] == "DELETE":
        test_result = validate_DELETE(response_correct,crud_args)
    elif crud_args["type"] == "LIST":
        test_result = validate_LIST(response_correct,crud_args,name,actual,expected_bytes)


    if not response_correct:
        print(f"{name}: FAILED: Invalid Response Content")

    if test_result:
        print(f"{name}: PASSED")
    else:
        print(f"{name}: FAILED")
    return test_result

def validate_POST(response_correct, crud_args):
    expect_write = crud_args["expect_write"]
    file_exists = os.path.isfile(crud_args["file_path"])

    if file_exists and crud_args["expect_write"]:
        file_content_valid = validate_file_content(crud_args["file_path"], crud_args["expected_file_contents"])
        file_is_json = is_valid_json(crud_args["file_path"])

    if expect_write and not file_exists:
        print(f"{name}: FAILED: file not created/found.")
    if not expect_write and file_exists:
        print(f"{name}: FAILED: file should not exist.")
    if expect_write and not file_content_valid:
        print(f"{name}: FAILED: file content invalid.")
    if expect_write and not is_valid_json:
        print(f"{name}: FAILED: file not valid json.")

    if expect_write:
        test_result = response_correct and file_exists and file_content_valid and file_is_json
    else:
        test_result = response_correct and not file_exists

    return test_result

def validate_GET(response_correct, crud_args):
    test_result = response_correct # so far, we aren't expecting anything else.
    return test_result

def validate_PUT(response_correct, crud_args):
    expect_write = crud_args["expect_write"]
    file_exists = os.path.isfile(crud_args["file_path"])

    if file_exists and expect_write:
        file_content_valid = validate_file_content(crud_args["file_path"], crud_args["expected_file_contents"])
        file_is_json = is_valid_json(crud_args["file_path"])

    if expect_write and not file_exists:
        print(f"{name}: FAILED: file not created/found.")
    if not expect_write and file_exists:
        print(f"{name}: FAILED: file should not exist.")
    if expect_write and not file_content_valid:
        print(f"{name}: FAILED: file content invalid.")
    if expect_write and not is_valid_json:
        print(f"{name}: FAILED: file not valid json.")

    if expect_write:
        test_result = response_correct and file_exists and file_content_valid and file_is_json
    else:
        test_result = response_correct and not file_exists

    return test_result

def validate_DELETE(response_correct, crud_args):
    file_does_not_exist = not os.path.isfile(crud_args["file_path"])
    test_result = file_does_not_exist
    return test_result

def validate_LIST(response_correct, crud_args, name, actual_bytes, expected_bytes):
    headers_actual, body_actual = split_HTTP_response(actual_bytes)
    headers_expected, body_expected = split_HTTP_response(expected_bytes)
    headers_match = compare_output(name, headers_actual, headers_expected)

    body_match = json_arrays_equal_unordered(body_actual, body_expected)
    # response doesn't need to match since we recheck headers anyway
    test_result = headers_match and body_match
    return test_result

def validate_file_content(file_path, expected_content):
    with open(file_path, 'rb') as f:
        actual = f.read()
        same_content = compare_output(file_path, actual, expected_content)
        return same_content

def is_valid_json(file_path):
    with open(file_path, 'rb') as f:
        content = f.read()
        try:
            json.loads(content)
            return True
        except json.JSONDecodeError:
            return False
    return False

def split_HTTP_response(response):
    """
    Splits a byte-string HTTP response into headers and body.
    """
    # HTTP headers and body are separated by b'\r\n\r\n'
    separator = b"\r\n\r\n"
    split_index = response.find(separator)

    if split_index == -1:
        return response, b""

    headers = response[:split_index]
    body = response[split_index + len(separator):]
    return headers, body

def json_arrays_equal_unordered(json_a, json_b):
    try:
        list_a = json.loads(json_a)
        list_b = json.loads(json_b)
    except json.JSONDecodeError:
        return False

    return set(list_a) == set(list_b)


# ------------------- Test Cases -------------------


def define_tests():
    """
    Returns a list of test case definitions.
    Each test is a dictionary with: name, method, expected, use_nc
    """
    return [
        {"config": "simple_config",
         "integration_test_folder": True,
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
         "integration_test_folder": True,
         "tests": [
             {
                 "name": "Valid static file request with alt file path",
                 "method": b"GET /file/test.txt HTTP/1.1\r\nHost: localhost\r\nUser-Agent: curl/8.5.0\r\nAccept: */*\r\n\r\n",
                 "expected": b"HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 18\r\n\r\nline1\nline2\n\nline4",
                 "use_nc": True
             },
         ]},
        {"config": "my_config",
         "integration_test_folder": False,
         "tests": [
             {
                 "name": "Valid echo request on deploy config",
                 "method": b"GET /echo HTTP/1.1\r\nHost: localhost\r\nUser-Agent: curl/8.5.0\r\nAccept: */*\r\n\r\n",
                 "expected": b"HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 76\r\n\r\nGET /echo HTTP/1.1\r\nHost: localhost\r\nUser-Agent: curl/8.5.0\r\nAccept: */*\r\n\r\n",
                 "use_nc": True
             },
             {
                 "name": "Valid static file request on on deploy config",
                 "method": b"GET /static/test1/test.txt HTTP/1.1\r\nHost: localhost\r\nUser-Agent: curl/8.5.0\r\nAccept: */*\r\n\r\n",
                 "expected": b"HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 18\r\n\r\nline1\nline2\n\nline4",
                 "use_nc": True
             },
             {
                 "name": "Invalid request",
                 "method": b"GET /notfound HTTP/1.1\r\nHost: localhost\r\nUser-Agent: curl/8.5.0\r\nAccept: */*\r\n\r\n",
                 "expected": b"HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nContent-Length: 13\r\n\r\n404 Not Found",
                 "use_nc": True
             }
         ]},
        {"config": "crud_config",
         "integration_test_folder": True,
         "tests": [
             {
                 "name": "POST API Request Creates a File with Content",
                 "method": b"POST /api/Shoes HTTP/1.1\r\nHost: localhost\r\nContent-Type: application/json\r\nContent-Length: 26\r\n\r\n{\"message\":\"creeper test\"}",
                 "expected": b"HTTP/1.1 201 Created\r\nContent-Type: application/json\r\nContent-Length: 9\r\n\r\n{\"id\": 1}",
                 "use_nc": True,
                 "crud_args": {
                     "type": "POST",
                     "file_path": TEMP_FILE_PATH + "Shoes/1",
                     "expected_file_contents": b'{"message":"creeper test"}',
                     "expect_write": True
                 }
             },
             {
                 "name": "POST API Request w/ non-JSON content fails",
                 "method": b"POST /api/Shoes HTTP/1.1\r\nHost: localhost\r\nContent-Type: application/json\r\nContent-Length: 20\r\n\r\n{\"invalid\" \"content\"}", # no ':'
                 "expected": (
                     b"HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 17\r\n\r\n"
                     b"Invalid JSON body"
                 ),
                 "use_nc": True,
                 "crud_args": {
                     "type": "POST",
                     "file_path": TEMP_FILE_PATH + "Shoes/2",
                     "expect_write": False
                 }
             },
             {
                 "name": "Second POST API Request Creates + Returns Correct ID",
                 "method": b"POST /api/Shoes HTTP/1.1\r\nHost: localhost\r\nContent-Type: application/json\r\nContent-Length: 29\r\n\r\n{\"message\":\"creeper test 2\"}",
                 "expected": b"HTTP/1.1 201 Created\r\nContent-Type: application/json\r\nContent-Length: 9\r\n\r\n{\"id\": 2}",
                 "use_nc": True,
                 "crud_args": {
                     "type": "POST",
                     "file_path": TEMP_FILE_PATH + "Shoes/2",
                     "expected_file_contents": b'{"message":"creeper test 2"}',
                     "expect_write": True
                 }
             },
             {
                 "name": "GET API Request Retrieves a File",
                 "method": b"GET /api/Shoes/1 HTTP/1.1\r\nHost: localhost\r\nContent-Type: application/json\r\n\r\n",
                 "expected": b"HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: 26\r\n\r\n{\"message\":\"creeper test\"}",
                 "use_nc": True,
                 "crud_args": {
                     "type": "GET",
                 }
             },
             {
                "name": "GET API Request: Invalid Page returns 404",
                "method": b"GET /api/Shoes/DNE HTTP/1.1\r\nHost: localhost\r\nContent-Type: application/json\r\n\r\n",
                "expected": b"HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nContent-Length: 12\r\n\r\nID not found",
                 "use_nc": True,
                 "crud_args": {
                     "type": "GET",
                 }
             },
             {
                 "name": "LIST API Request returns a list of Valid IDs",
                 "method": b"GET /api/Shoes/ HTTP/1.1\r\nHost: localhost\r\nContent-Type: application/json\r\n\r\n",
                 "expected": b"HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: 9\r\n\r\n[\"1\",\"2\"]",
                 "use_nc": True,
                 "crud_args": {
                     "type": "LIST",
                 }
             },
             {
                 "name": "PUT Request Updates Current File",
                 "method": b"PUT /api/Shoes/1 HTTP/1.1\r\nHost: localhost\r\nContent-Type: application/json\r\nContent-Length: 29\r\n\r\n{\"message\":\"creeper test 3\"}",
                 "expected": b"HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n",
                 "use_nc": True,
                 "crud_args": {
                     "type": "PUT",
                     "file_path": TEMP_FILE_PATH + "Shoes/1",
                     "expected_file_contents": b'{"message":"creeper test 3"}',
                     "expect_write": True
                 }
             },
             {
                 "name": "PUT Request Creates New File",
                 "method": b"PUT /api/Shoes/4 HTTP/1.1\r\nHost: localhost\r\nContent-Type: application/json\r\nContent-Length: 29\r\n\r\n{\"message\":\"creeper test 4\"}",
                 "expected": b"HTTP/1.1 201 Created\r\nContent-Length: 0\r\n\r\n",
                 "use_nc": True,
                 "crud_args": {
                     "type": "PUT",
                     "file_path": TEMP_FILE_PATH + "Shoes/4",
                     "expected_file_contents": b'{"message":"creeper test 4"}',
                     "expect_write": True
                 }
             },
             {
                 "name": "DELETE API Request Deletes a File",
                 "method": b"DELETE /api/Shoes/2 HTTP/1.1\r\nHost: localhost\r\n\r\n",
                 "expected": b"HTTP/1.1 204 No Content\r\nContent-Length: 0\r\n\r\n",
                 "use_nc": True,
                 "crud_args": {
                     "type": "DELETE",
                     "file_path": TEMP_FILE_PATH + "Shoes/2",
                 }
             },
             {
                 "name": "DELETE API Request with invalid ID should 404",
                 "method": b"DELETE /api/Shoes/DNE HTTP/1.1\r\nHost: localhost\r\n\r\n",
                 "expected": b"HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nContent-Length: 12\r\n\r\nID not found",
                 "use_nc": True,
                 "crud_args": {
                     "type": "DELETE",
                     "file_path": TEMP_FILE_PATH + "Shoes/DNE",
                 }
             },
         ]
        }
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
        integration_test_folder_exists = test["integration_test_folder"]
        print(f"Running tests for config: {config_file}")
        start_server(config_file, integration_test_folder_exists)
        try:
            for test_case in test["tests"]:
                all_passed &= run_test(
                    test_case["name"],
                    test_case["method"],
                    test_case["expected"],
                    test_case["use_nc"],
                    test_case.get("crud_args", None)
                )
        finally:
            stop_server()
            cleanup_test_suite()

    print("All integration tests passed." if all_passed else "Some integration tests failed.")
    sys.exit(0 if all_passed else 1)


if __name__ == "__main__":
    main()
