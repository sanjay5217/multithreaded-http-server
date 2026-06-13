"""
Unit tests for Socket::send_message() and Socket::get_message() in src/socket.cpp.

Uses an AF_UNIX socketpair internally (via the shared driver binary) so no
network access is required.

Build manually:
    g++ -std=c++17 -o tests/driver tests/driver.cpp

Run tests:
    pytest tests/test_socket.py -v
"""

import subprocess

from conftest import DRIVER_BIN


# HELPERS

def _run(subcmd: str, stdin: str, *args: str) -> dict[str, str]:
    """Invoke `driver socket <subcmd> [args...]` with the given stdin and return output as a dict."""
    proc = subprocess.run(
        [DRIVER_BIN, "socket", subcmd, *args],
        input=stdin.encode(),
        capture_output=True,
    )
    result = {}
    for line in proc.stdout.decode().splitlines():
        if "\t" in line:
            key, value = line.split("\t", 1)
            # Driver escapes \r and \n so values fit on one line; undo that here.
            result[key] = value.replace("\\r", "\r").replace("\\n", "\n")
    return result


# TESTS

class TestSendMessage:
    def test_returns_true_on_success(self):
        """send_message() returns true when the peer socket is open."""
        out = _run("send_recv", "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n")
        assert out["send_ok"] == "true"


class TestGetMessage:
    def test_round_trip_simple_request(self):
        """get_message() returns the full message up to and including \\r\\n\\r\\n."""
        raw = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n"
        out = _run("send_recv", raw)
        assert out["message"] == raw

    def test_round_trip_with_headers(self):
        """get_message() preserves all header lines verbatim."""
        raw = (
            "POST /submit HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: 0\r\n"
            "\r\n"
        )
        out = _run("send_recv", raw)
        assert out["message"] == raw

    def test_stops_at_first_terminator(self):
        """get_message() does not include data that follows \\r\\n\\r\\n."""
        first  = "GET /first HTTP/1.1\r\nHost: localhost\r\n\r\n"
        second = "GET /second HTTP/1.1\r\nHost: localhost\r\n\r\n"
        out = _run("send_recv", first + second)
        assert out["message"] == first

    def test_returns_empty_without_terminator(self):
        """get_message() returns '' when the connection closes before \\r\\n\\r\\n."""
        out = _run("no_term", "incomplete request without terminator")
        assert out["message"] == ""

    def test_returns_empty_on_empty_input(self):
        """get_message() returns '' when the peer closes immediately."""
        out = _run("no_term", "")
        assert out["message"] == ""

    def test_on_length(self):
        """get_message(int length) reads appropriate length"""
        msg = "hello"
        out = _run("send_recv_length", msg, "5")
        assert out["message"] == "hello"

    def test_large_length(self):
        """get_message(int length) reads up to 4096 characters"""
        msg = "0" * 5000
        out = _run("send_recv_length", msg, "4096")
        assert len(out["message"]) == 4096
        assert out["message"] == "0" * 4096