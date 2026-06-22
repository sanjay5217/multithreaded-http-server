#!/usr/bin/env python3
"""
Stress / edge-case / race-condition test suite for src/obj/server.

Build first:
    cd src && make

Run:
    python3 tests/stress_test.py

This is a standalone driver (no pytest needed) so it can run the server
itself, watch its stdout/stderr for AddressSanitizer reports, and run
timing-sensitive concurrency scenarios that don't fit well into normal
assert-style unit tests.
"""

import os
import re
import socket
import subprocess
import sys
import threading
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SERVER_BINARY = os.path.join(ROOT, "src", "obj", "server")
HOST = "127.0.0.1"
PORT = 8080
CONNECT_TIMEOUT = 2.0

WORKER_COUNT = 10
PER_WORKER_CAPACITY = 10
TOTAL_CAPACITY = WORKER_COUNT * PER_WORKER_CAPACITY

results = []


def record(name, ok, detail=""):
    results.append((name, ok, detail))
    tag = "PASS" if ok else "FAIL"
    print(f"[{tag}] {name}" + (f" -- {detail}" if detail else ""))

# Helpers

def connect():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(CONNECT_TIMEOUT)
    s.connect((HOST, PORT))
    return s


def build_request(method, path, headers=None, body=""):
    headers = dict(headers or {})
    headers.setdefault("Host", f"{HOST}:{PORT}")
    lines = [f"{method} {path} HTTP/1.1"]
    lines += [f"{k}: {v}" for k, v in headers.items()]
    req = "\r\n".join(lines) + "\r\n\r\n" + body
    return req.encode()


def recv_all(sock, timeout=1.5):
    sock.settimeout(timeout)
    chunks = []
    try:
        while True:
            chunk = sock.recv(65536)
            if not chunk:
                break
            chunks.append(chunk)
    except (socket.timeout, ConnectionResetError, OSError):
        pass
    return b"".join(chunks)


def raw_request(method, path, headers=None, body="", timeout=1.5):
    with connect() as s:
        s.sendall(build_request(method, path, headers, body))
        return recv_all(s, timeout)


def status_code(resp: bytes):
    m = re.match(rb"HTTP/\S+\s+(\d+)", resp)
    return int(m.group(1)) if m else None


class ServerProcess:
    def __init__(self):
        self.proc = None
        self.stdout_lines = []
        self.stderr_data = []
        self._readers = []

    def start(self):
        if not os.path.exists(SERVER_BINARY):
            print(f"Server binary not found at {SERVER_BINARY}. Run `cd src && make` first.")
            sys.exit(1)

        self.proc = subprocess.Popen(
            [SERVER_BINARY],
            cwd=ROOT,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1,
        )

        # Must drain stdout/stderr continuously: hundreds of test connections
        # produce enough "Connected from ..." output to fill the OS pipe
        # buffer, which would otherwise block the server on write() and
        # deadlock the whole test run.
        def reader(stream, sink):
            for line in iter(stream.readline, ""):
                sink.append(line)

        for stream, sink in ((self.proc.stdout, self.stdout_lines),
                              (self.proc.stderr, self.stderr_data)):
            t = threading.Thread(target=reader, args=(stream, sink), daemon=True)
            t.start()
            self._readers.append(t)

        self._wait_for_port()

    def _wait_for_port(self, timeout=5.0):
        deadline = time.time() + timeout
        while time.time() < deadline:
            try:
                with socket.create_connection((HOST, PORT), timeout=0.2):
                    return
            except OSError:
                time.sleep(0.05)
        raise RuntimeError("server never started listening")

    def alive(self):
        return self.proc is not None and self.proc.poll() is None

    def stop(self):
        if not self.proc:
            return
        self.proc.terminate()
        try:
            self.proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            self.proc.kill()
            self.proc.wait(timeout=5)

    def stderr_text(self):
        return "".join(self.stderr_data)


def test_connectivity():
    try:
        with connect():
            record("connectivity: TCP handshake", True)
    except OSError as e:
        record("connectivity: TCP handshake", False, str(e))


def test_basic_routes():
    cases = [
        ("GET", "/", None, "", 200),
        ("GET", "/health", None, "", 200),
        ("GET", "/stat", None, "", 200),
        ("GET", "/header", None, "", 200),
        ("GET", "/compute", None, "", 200),
        ("GET", "/does-not-exist", None, "", 404),
        ("PUT", "/health", None, "", 405),
        ("DELETE", "/", None, "", 405),
    ]
    for method, path, headers, body, expect in cases:
        resp = raw_request(method, path, headers, body)
        code = status_code(resp)
        record(f"basic route: {method} {path} -> {expect}", code == expect,
               f"got {code!r}, resp[:80]={resp[:80]!r}")

    # Custom header round trip via /header
    resp = raw_request("GET", "/header", {"X-Test-Marker": "abc123"})
    record("basic route: /header echoes custom header",
           b"X-Test-Marker" in resp and b"abc123" in resp, resp[:200].decode(errors="replace"))

    # POST /echo with a body
    resp = raw_request("POST", "/echo", {"Content-Length": "11"}, "hello world")
    record("basic route: POST /echo returns body",
           status_code(resp) == 200 and b"hello world" in resp, resp[:200].decode(errors="replace"))

    # GET /header with zero headers at all (regression test for the
    # pop_back()-on-empty-string crash fixed in handles.cpp)
    with connect() as s:
        s.sendall(b"GET /header HTTP/1.1\r\n\r\n")
        resp = recv_all(s)
    record("basic route: GET /header with no headers doesn't crash",
           status_code(resp) == 200, resp[:200].decode(errors="replace"))


def test_static_handler():
    resp = raw_request("POST", "/static", {"Content-Length": "9"}, "README.md")
    record("static: existing file in cwd -> 200", status_code(resp) == 200, resp[:120].decode(errors="replace"))

    body = "no_such_file_xyz.txt"
    resp = raw_request("POST", "/static", {"Content-Length": str(len(body))}, body)
    record("static: missing file -> 404", status_code(resp) == 404, resp[:120].decode(errors="replace"))

    # Regression test for the std::mismatch out-of-bounds read fixed in
    # handles.cpp: an existing file whose absolute path has fewer components
    # than the server's cwd used to read past target.end().
    body = "/etc/hosts"
    resp = raw_request("POST", "/static", {"Content-Length": str(len(body))}, body)
    record("static: path traversal outside cwd -> 403 (no crash)",
           status_code(resp) == 403, resp[:120].decode(errors="replace"))

    body = "src"  # a directory, not a regular file
    resp = raw_request("POST", "/static", {"Content-Length": str(len(body))}, body)
    record("static: directory path -> 404", status_code(resp) == 404, resp[:120].decode(errors="replace"))


def test_malformed_requests():
    cases = {
        # A single whitespace-free token: `method >> path >> version` can't
        # extract 3 tokens, so extract_string() should bail out with an
        # empty map. (Earlier draft used a payload with spaces in it, which
        # parses into method/path/version tokens just fine and legitimately
        # gets routed as an unknown method -> 405; that wasn't malformed.)
        "garbage bytes": b"\x00\x01\x02\xff\xfe\r\n\r\n",
        "missing HTTP version": b"GET /\r\n\r\n",
        "single token": b"ONLYONEWORD\r\n\r\n",
        "empty line only": b"\r\n\r\n",
    }
    for name, raw in cases.items():
        with connect() as s:
            s.sendall(raw)
            resp = recv_all(s, timeout=1.0)
        record(f"malformed request closes cleanly: {name}", resp == b"", f"got {resp!r}")


def test_content_length_edge_cases():
    resp = raw_request("POST", "/echo", {"Content-Length": "abc"}, "ignored")
    record("content-length: non-numeric treated as no body -> 200",
           status_code(resp) == 200, resp[:120].decode(errors="replace"))

    resp = raw_request("POST", "/echo", {"Content-Length": "0"}, "")
    record("content-length: explicit 0 -> 200", status_code(resp) == 200, resp[:120].decode(errors="replace"))

    resp = raw_request("POST", "/echo", {"Content-Length": "-5"}, "x")
    record("content-length: negative treated as no body -> 200",
           status_code(resp) == 200, resp[:120].decode(errors="replace"))

    # Client claims more body than it actually sends, then disconnects.
    with connect() as s:
        s.sendall(build_request("POST", "/echo", {"Content-Length": "1000"}, "short"))
        s.shutdown(socket.SHUT_WR)
        resp = recv_all(s, timeout=1.5)
    record("content-length: short body + disconnect -> connection closes, no response",
           resp == b"", f"got {resp!r}")


def test_trickle_slow_client():
    raw = build_request("GET", "/health")
    with connect() as s:
        for b in raw:
            s.sendall(bytes([b]))
            time.sleep(0.001)
        resp = recv_all(s, timeout=2.0)
    record("slow trickle: byte-by-byte GET still completes",
           status_code(resp) == 200, resp[:120].decode(errors="replace"))

    body = "y" * 500
    raw = build_request("POST", "/echo", {"Content-Length": str(len(body))}, body)
    with connect() as s:
        for i in range(0, len(raw), 7):
            s.sendall(raw[i:i + 7])
            time.sleep(0.001)
        resp = recv_all(s, timeout=2.0)
    record("slow trickle: chunked POST body reassembles correctly",
           status_code(resp) == 200 and body.encode() in resp, f"len(resp)={len(resp)}")


def test_large_body():
    body = "X" * 250_000
    resp = raw_request("POST", "/echo", {"Content-Length": str(len(body))}, body, timeout=5.0)
    ok = status_code(resp) == 200 and body.encode() in resp
    record("large body: 250KB POST /echo round-trips intact", ok, f"len(resp)={len(resp)}")


def test_no_keepalive_pipelining():
    req = build_request("GET", "/health")
    with connect() as s:
        s.sendall(req + req)  # two requests back-to-back, same connection
        resp = recv_all(s, timeout=1.5)
    # Note: every response echoes the parsed request "version" back as a
    # response header (e.g. "version: HTTP/1.1"), so a single response
    # legitimately contains the substring "HTTP/" twice. Count status-line
    # starts (anchored to the start of a line) instead of raw substring hits.
    status_lines = re.findall(rb"^HTTP/\S+ \d+ ", resp, re.MULTILINE)
    record("pipelining: only first request is answered, then connection closes",
           len(status_lines) == 1, f"got {len(status_lines)} responses, resp={resp!r}")


def test_rapid_connect_disconnect_churn(server):
    failures = 0
    n = 200
    for _ in range(n):
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.settimeout(CONNECT_TIMEOUT)
            s.connect((HOST, PORT))
            s.close()
        except OSError:
            failures += 1
    record("churn: 200 rapid connect/disconnect cycles",
           failures == 0 and server.alive(), f"{failures} failed connects, server_alive={server.alive()}")


def test_concurrent_load(server):
    N_THREADS = 150
    REQS_PER_THREAD = 5
    errors = []
    lock = threading.Lock()

    def worker():
        for _ in range(REQS_PER_THREAD):
            try:
                choice = threading.get_ident() % 3
                if choice == 0:
                    resp = raw_request("GET", "/health")
                elif choice == 1:
                    resp = raw_request("GET", "/stat")
                else:
                    resp = raw_request("POST", "/echo", {"Content-Length": "4"}, "ping")
                if status_code(resp) != 200:
                    with lock:
                        errors.append(f"bad status {status_code(resp)!r}")
            except OSError as e:
                with lock:
                    errors.append(str(e))

    threads = [threading.Thread(target=worker) for _ in range(N_THREADS)]
    start = time.time()
    for t in threads:
        t.start()
    for t in threads:
        t.join(timeout=20)
    elapsed = time.time() - start

    total = N_THREADS * REQS_PER_THREAD
    ok = len(errors) == 0 and server.alive()
    record(f"concurrent load: {total} requests across {N_THREADS} threads",
           ok, f"{len(errors)} errors in {elapsed:.2f}s, server_alive={server.alive()}, sample_errors={errors[:5]}")


def test_worker_capacity_backpressure():
    """
    Fills every worker slot (WORKER_COUNT * PER_WORKER_CAPACITY connections),
    then proves a request beyond capacity is held (not served) until a slot
    is freed -- exercising the reserve_slot()/condition_variable handoff
    in ThreadPool::push / Worker::remove_client.
    """
    held = []
    try:
        for _ in range(TOTAL_CAPACITY):
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.settimeout(CONNECT_TIMEOUT)
            s.connect((HOST, PORT))
            held.append(s)  # connected, but deliberately sends nothing

        overflow = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        overflow.settimeout(CONNECT_TIMEOUT)
        overflow.connect((HOST, PORT))
        overflow.sendall(build_request("GET", "/health"))

        stalled = recv_all(overflow, timeout=2.0)
        record("backpressure: request beyond full capacity is not served immediately",
               stalled == b"", f"got {stalled!r}")

        held.pop().close()  # free exactly one slot

        resumed = recv_all(overflow, timeout=5.0)
        record("backpressure: freeing one slot lets the queued request complete",
               status_code(resumed) == 200, f"got {resumed!r}")
    finally:
        for s in held:
            s.close()
        try:
            overflow.close()
        except Exception:
            pass


def test_repeated_capacity_cycles():
    """Round-trips the full capacity twice more to shake out missed-wakeup races."""
    for cycle in range(2):
        held = []
        ok = True
        detail = ""
        try:
            for _ in range(TOTAL_CAPACITY):
                s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                s.settimeout(CONNECT_TIMEOUT)
                s.connect((HOST, PORT))
                held.append(s)

            for s in held:
                s.close()
            held = []

            # After freeing everything, fresh requests should serve immediately.
            resp = raw_request("GET", "/health", timeout=3.0)
            ok = status_code(resp) == 200
            detail = f"got {resp[:80]!r}"
        finally:
            for s in held:
                s.close()
        record(f"backpressure: capacity drains and recovers (cycle {cycle + 1})", ok, detail)


def test_worker_starvation_via_compute():
    """
    Best-effort, informational-only demonstration that a CPU-bound handler
    blocks its entire worker's event loop (not just the one client occupying
    it), because Worker::handle_client is a single thread doing both I/O
    dispatch and handler execution. Fires one /compute request per worker so
    each of the 10 workers is synchronously busy, then checks whether an
    unrelated request stalls instead of being served.

    This does NOT count toward pass/fail: /compute's fibonacci range (20-26)
    is fast enough (single-digit ms) that all 10 workers can easily finish
    and free up before this check even runs, so a clean response here is
    expected and is not evidence the blocking behavior was fixed -- it's
    just a timing race this test isn't trying to win. The structural
    blocking (see Worker::handle_client / Worker::respond in worker.cpp) is
    real regardless of what this particular run observes.
    """
    busy_sockets = []
    try:
        for _ in range(WORKER_COUNT):
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.settimeout(30)
            s.connect((HOST, PORT))
            s.sendall(build_request("GET", "/compute"))
            busy_sockets.append(s)

        time.sleep(0.05)  # give the accept loop a moment to dispatch all of them

        resp = raw_request("GET", "/health", timeout=0.3)
        starved = resp == b""
        print(f"[INFO] worker starvation demo: {'reproduced' if starved else 'did not reproduce'} "
              f"this run (not a pass/fail check; see worker.cpp comment)")
    finally:
        for s in busy_sockets:
            try:
                s.close()
            except OSError:
                pass

def run_test(server, fn, *args):
    """Runs one test, isolating its exceptions so a crash (e.g. the server
    dying mid-test and refusing further connections) doesn't take down the
    whole suite before we get a chance to dump the server's stderr."""
    if not server.alive():
        record(f"{fn.__name__} (skipped)", False, "server already died before this test")
        return
    try:
        fn(*args)
    except Exception as e:
        record(fn.__name__, False, f"raised {type(e).__name__}: {e}")


def main():
    server = ServerProcess()
    server.start()
    try:
        run_test(server, test_connectivity)
        run_test(server, test_basic_routes)
        run_test(server, test_static_handler)
        run_test(server, test_malformed_requests)
        run_test(server, test_content_length_edge_cases)
        run_test(server, test_trickle_slow_client)
        run_test(server, test_large_body)
        run_test(server, test_no_keepalive_pipelining)
        run_test(server, test_rapid_connect_disconnect_churn, server)
        run_test(server, test_concurrent_load, server)
        run_test(server, test_worker_capacity_backpressure)
        run_test(server, test_repeated_capacity_cycles)
        run_test(server, test_worker_starvation_via_compute)
    finally:
        still_alive = server.alive()
        server.stop()

    stderr_text = server.stderr_text()
    crashed = bool(re.search(r"AddressSanitizer|Sanitizer|Segmentation fault|core dumped", stderr_text))

    passed = sum(1 for _, ok, _ in results if ok)
    failed = len(results) - passed

    print("\n" + "=" * 70)
    print(f"{passed}/{len(results)} checks passed")
    print(f"server stayed alive through the run: {still_alive}")
    if crashed:
        print("\n!!! Sanitizer/crash output detected in server stderr !!!")
        print(stderr_text)

    if failed or crashed or not still_alive:
        sys.exit(1)
    sys.exit(0)


if __name__ == "__main__":
    main()
