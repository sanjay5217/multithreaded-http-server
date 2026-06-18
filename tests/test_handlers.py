"""
Unit tests for all Handlers in src/handles.cpp.

Build manually:
    g++ -std=c++17 -o tests/driver tests/driver.cpp

Run tests:
    pytest tests/test_handlers.py -v
"""

import subprocess
import re

from conftest import DRIVER_BIN

# HELPERS

def fib(n: int):
    if n <= 1: return 1
    prev, curr = 1, 1
    for _ in range(2, n + 1):
        prev, curr = curr, prev + curr
    return curr

def invoke_handler(method: str, path: str, body: str = "") -> dict[str, str]:
    """Run `driver handles <method> <path>` with optional stdin body."""
    proc = subprocess.run(
        [DRIVER_BIN, "handles", method, path],
        input=body.encode(),
        capture_output=True,
    )
    result = {}
    for line in proc.stdout.decode().splitlines():
        if "\t" in line:
            key, value = line.split("\t", 1)
            result[key] = value.replace("\\n", "\n").replace("\\r", "\r")
    return result

def invoke_handler_seq(requests: list[tuple[str, str, str]]) -> list[dict[str, str]]:
    """Run multiple (method, path, body) requests through one driver process.

    Uses `driver handles_seq` so num_req accumulates across all calls.
    Returns a list of dicts with 'status' and 'body' keys, one per request.
    """
    lines = [f"{m}\t{p}\t{b}" for m, p, b in requests]
    proc = subprocess.run(
        [DRIVER_BIN, "handles_seq"],
        input="\n".join(lines).encode(),
        capture_output=True,
    )
    raw: dict[str, str] = {}
    for line in proc.stdout.decode().splitlines():
        if "\t" in line:
            key, value = line.split("\t", 1)
            raw[key] = value.replace("\\n", "\n").replace("\\r", "\r")

    n = len(requests)
    results: list[dict[str, str]] = [{} for _ in range(n)]
    for key, value in raw.items():
        if ":" in key:
            idx_str, field = key.split(":", 1)
            results[int(idx_str)][field] = value
    return results


# TESTS

class TestHealthHandler:
    def test_returns_200(self):
        out = invoke_handler("GET", "/health")
        assert "200 OK" in out["status"]

    def test_body_contains_status_ok(self):
        out = invoke_handler("GET", "/health")
        assert out["body"] == "{ Status: OK }"

    def test_wrong_method_rejected(self):
        """Health is GET-only; POST should not return 200."""
        out = invoke_handler("POST", "/health")
        assert "200 OK" not in out["status"]

class TestStatHandler:
    def test_returns_200(self):
        out = invoke_handler("GET", "/stat")
        assert "200 OK" in out["status"]

    def test_single_stat_call_counts_itself(self):
        results = invoke_handler_seq([("GET", "/stat", "")])
        assert results[0]["body"] == "{ Requests Served: 1 }"

    def test_n_stat_calls_accumulate(self):
        n = 5
        results = invoke_handler_seq([("GET", "/stat", "")] * n)
        for i in range(n):
            assert results[i]["body"] == f"{{ Requests Served: {i + 1} }}", \
                f"call {i}: got {results[i]['body']}"

    def test_n_non_stat_requests_counted(self):
        # 4 health calls then stat, thus total is 5 requests
        n = 4
        requests = [("GET", "/health", "")] * n + [("GET", "/stat", "")]
        results = invoke_handler_seq(requests)
        assert results[-1]["body"] == f"{{ Requests Served: {n + 1} }}"

    def test_mixed_stat_and_non_stat(self):
        # 6 total requests
        requests = [
            ("GET", "/health", ""), ("GET", "/stat", ""),
            ("GET", "/health", ""), ("GET", "/stat", ""),
            ("GET", "/health", ""), ("GET", "/stat", ""),
        ]
        results = invoke_handler_seq(requests)
        assert results[1]["body"] == "{ Requests Served: 2 }"
        assert results[3]["body"] == "{ Requests Served: 4 }"
        assert results[5]["body"] == "{ Requests Served: 6 }"

    def test_invalid_requests_not_counted(self):
        # Ensures invalid requests are not counted 
        requests = [
            ("DELETE", "/stat", ""),
            ("GET",    "/stat", ""),
        ]
        results = invoke_handler_seq(requests)
        assert results[1]["body"] == "{ Requests Served: 1 }"
    
class TestEchoHandler:
    def test_returns_200(self):
        out = invoke_handler("POST", "/echo", body="hello")
        assert "200 OK" in out["status"]

    def test_body_is_echoed(self):
        out = invoke_handler("POST", "/echo", body="echo this back")
        assert out["body"] == "echo this back"

    def test_empty_body_echoed(self):
        out = invoke_handler("POST", "/echo", body="")
        assert "200 OK" in out["status"]

    def test_wrong_method_rejected(self):
        """Echo is POST-only; GET should not return 200."""
        out = invoke_handler("GET", "/echo")
        assert "200 OK" not in out["status"]

class TestComputeHandler:
    def test_returns_200(self):
        out = invoke_handler("GET", "/compute")
        assert "200 OK" in out["status"]  
    
    def test_body_returns(self):
        out = invoke_handler("GET", "/compute")
        m = re.match(r'\{ N: (\d+), Result: (\d+), Duration: ([\d.]+) \}', out["body"])
        assert m is not None, f"BODY FORMAT UNEXPECTED: {out['body']}"
        n, result, duration = int(m.group(1)), int(m.group(2)), float(m.group(3))
        assert result == fib(n)
        assert duration > 0

class TestStaticHandler:
    def test_returns_200(self):
        out = invoke_handler("POST", "/static", body="tests/test_files/sample.txt")
        assert "200 OK" in out["status"]

    def test_sample_input(self):
        out = invoke_handler("POST", "/static",  body="tests/test_files/sample.txt")
        file_input = "Hello world!\nThis is a test file.\nUsed for StaticHandler testing.\nEnd of file."
        assert file_input == out["body"], f"BODY: {out['body']}"

    def test_http_input(self):
        out = invoke_handler("POST", "/static",  body="tests/test_files/http.txt")
        file_input = "HTTP/1.1 200 OK\nContent-Type: text/plain\n\nThis is the body of the response.\nIt spans multiple lines.\nStatic file served successfully."
        assert file_input == out["body"], f"BODY: {out['body']}"       

    def test_empty_input(self):
        out = invoke_handler("POST", "/static",  body="tests/test_files/empty.txt")
        assert "" == out["body"], f"BODY: {out['body']}"


class TestHeaderHandler:
    def test_returns_200(self):
        out = invoke_handler("GET", "/header")
        assert "200 OK" in out["status"]

    def test_body_is_empty(self):
        out = invoke_handler("GET", "/header")
        assert out.get("body", "") == ""

    def test_post_rejected(self):
        # POST is valid method but /header has no POST route (404)
        out = invoke_handler("POST", "/header")
        assert "200 OK" not in out["status"]
        assert "404" in out["status"]

    def test_delete_rejected(self):
        # DELETE is not a registered method at all (405)
        out = invoke_handler("DELETE", "/header")
        assert "200 OK" not in out["status"]
        assert "405" in out["status"]


class TestInvalidHandler:
    # Wrong method (not in method map at all (405)

    def test_delete_method(self):
        out = invoke_handler("DELETE", "/health")
        assert "405" in out["status"]

    def test_patch_method(self):
        out = invoke_handler("PATCH", "/echo")
        assert "405" in out["status"]

    def test_put_method(self):
        out = invoke_handler("PUT", "/stat")
        assert "405" in out["status"]

    def test_empty_method(self):
        out = invoke_handler("", "/health")
        assert "200 OK" not in out["status"]

    # Valid method, wrong path (404)

    def test_get_unknown_path(self):
        out = invoke_handler("GET", "/notfound")
        assert "404" in out["status"]

    def test_get_root_path(self):
        out = invoke_handler("GET", "/")
        assert "404" in out["status"]

    def test_get_case_mismatch(self):
        out = invoke_handler("GET", "/HEALTH")
        assert "404" in out["status"]

    def test_post_to_get_only_path(self):
        # POST is valid, but /health is not in POST routes (404)
        out = invoke_handler("POST", "/health")
        assert "404" in out["status"]

    def test_get_path_with_special_chars(self):
        out = invoke_handler("GET", "/he@lth!")
        assert "404" in out["status"]

    def test_get_path_traversal(self):
        out = invoke_handler("GET", "/../etc/passwd")
        assert "404" in out["status"]

    def test_get_very_long_path(self):
        out = invoke_handler("GET", "/" + "a" * 1000)
        assert "404" in out["status"]

    def test_get_path_with_spaces(self):
        out = invoke_handler("GET", "/hea lth")
        assert "404" in out["status"]

    def test_get_path_with_numbers(self):
        out = invoke_handler("GET", "/health123")
        assert "404" in out["status"]

    def test_get_double_slash(self):
        out = invoke_handler("GET", "//health")
        assert "404" in out["status"]

    # Invalid Static file (404) or path traversal (403)

    def test_static_nonexistent_file(self):
        out = invoke_handler("POST", "/static", body="does_not_exist.txt")
        assert "404" in out["status"]

    def test_static_empty_body(self):
        out = invoke_handler("POST", "/static", body="")
        assert "404" in out["status"]

    def test_static_directory_path(self):
        # is_regular_file check rejects directories (404)
        out = invoke_handler("POST", "/static", body="tests/test_files/")
        assert "404" in out["status"]

    def test_static_curl_formatted_body(self):
        out = invoke_handler("POST", "/static", body="field=value&other=thing")
        assert "404" in out["status"]

    def test_static_path_with_null_byte(self):
        out = invoke_handler("POST", "/static", body="tests/test_files/sample\x00.txt")
        assert "404" in out["status"]

    def test_static_absolute_path_outside_tree(self):
        # Path outside CWD is rejected (403)
        out = invoke_handler("POST", "/static", body="/etc/passwd")
        assert "403" in out["status"]