"""
Concurrency tests for ThreadPool, exercised via tests/driver.cpp's `threadpool` subcommand.

Each "client" is a real connected socketpair: one end is pushed into a real ThreadPool
exactly like an accepted connection would be, and the other end is driven by a worker
thread inside the driver process that controls exactly when the request is sent. This
lets us prove genuine parallel dispatch (by timing) and verify the pool drains a backlog
without deadlocking, using the real ThreadPool/handle_client code path -- not a mock.

Build manually:
    g++ -std=c++17 -o tests/driver tests/driver.cpp

Run tests:
    pytest tests/test_threadpool.py -v
"""

import subprocess
from conftest import DRIVER_BIN


def run_threadpool(num_workers: int, num_clients: int, delay_ms: int, timeout: float = 10.0) -> dict[str, str]:
    proc = subprocess.run(
        [DRIVER_BIN, "threadpool", str(num_workers), str(num_clients), str(delay_ms)],
        capture_output=True,
        timeout=timeout,
    )
    assert proc.returncode == 0, (
        f"driver crashed or hung (exit {proc.returncode}): {proc.stderr.decode()}"
    )
    result = {}
    for line in proc.stdout.decode().splitlines():
        if "\t" in line:
            key, value = line.split("\t", 1)
            result[key] = value
    return result

# This verifies the concurrency of server based on the threadpool logic. 
class TestThreadPoolConcurrency:

    def test_less_than_pool_size(self):
        out = run_threadpool(num_workers=8, num_clients=4, delay_ms=200)
        assert out["all_ok"] == "true"
        max_complete_ms = float(out["max_complete_ms"])
        assert max_complete_ms < 350, (
            f"expected ~200ms if parallel, got {max_complete_ms:.0f}ms "
            f"(fully serial would be ~{8 * 200}ms)"
        )

    def test_clients_equal_pool_size_run_in_parallel(self):
        """
        Number of clients = pool size

        Every client delays sending by the same amount, so if
        the pool truly runs jobs in parallel, the slowest should finish shortly after that
        one delay, not after N delays stacked sequentially.
        """
        out = run_threadpool(num_workers=8, num_clients=8, delay_ms=200)
        assert out["all_ok"] == "true"
        max_complete_ms = float(out["max_complete_ms"])
        assert max_complete_ms < 350, (
            f"expected ~200ms if parallel, got {max_complete_ms:.0f}ms "
            f"(fully serial would be ~{8 * 200}ms)"
        )

    def test_single_client_baseline(self):
        out = run_threadpool(num_workers=4, num_clients=1, delay_ms=100)
        assert out["all_ok"] == "true"
        assert float(out["max_complete_ms"]) < 250

    def test_backlog_drains_without_deadlock(self):
        """Number of clients > Number of worker threads
        """
        out = run_threadpool(num_workers=4, num_clients=20, delay_ms=0, timeout=10.0)
        assert out["all_ok"] == "true"
        assert out["num_clients"] == "20"
