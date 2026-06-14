"""
Unit tests for extract_string() in src/utils.cpp.

Requires the shared driver binary to be compiled first. The session-scoped
fixture below handles this automatically when you run pytest.

Build manually:
    g++ -std=c++17 -o tests/driver tests/driver.cpp

Run tests:
    pytest tests/test_utils.py -v
"""

import subprocess

from conftest import DRIVER_BIN

# HELPERS

def extract_string(raw_http: str) -> dict[str, str]:
    """Run the driver with a raw HTTP message and return the parsed dict."""
    proc = subprocess.run(
        [DRIVER_BIN, "utils"],
        input=raw_http.encode(),
        capture_output=True,
    )
    result = {}
    for line in proc.stdout.decode().splitlines():
        if "\t" in line:
            key, value = line.split("\t", 1)
            result[key] = value
    return result


# TESTS

# Tests method "string_dict extract_string(std::string msg);""
class TestExtractString:
    def test_get_root_request_line(self):
        """Parses method, path, and version from a basic GET request."""
        raw = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n"
        parsed = extract_string(raw)
        assert parsed["method"] == "GET"
        assert parsed["path"] == "/"
        assert parsed["version"] == "HTTP/1.1"

    def test_post_request_line(self):
        """Parses a POST request with a non-root path."""
        raw = "POST /submit HTTP/1.1\r\nHost: localhost\r\n\r\n"
        parsed = extract_string(raw)
        assert parsed["method"] == "POST"
        assert parsed["path"] == "/submit"
        assert parsed["version"] == "HTTP/1.1"

    def test_header_parsing(self):
        """Headers are extracted and keys are lowercased."""
        raw = (
            "GET /data HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "Content-Type: application/json\r\n"
            "\r\n"
        )
        parsed = extract_string(raw)
        assert parsed.get("Host") == "example.com"
        assert parsed.get("Content-Type") == "application/json"

    def test_invalid_request_returns_empty(self):
        """A malformed first line returns an empty map."""
        raw = "NOTVALID\r\n"
        parsed = extract_string(raw)
        assert parsed == {}
