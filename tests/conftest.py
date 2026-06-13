import os
import subprocess
import pytest # type: ignore

DRIVER_SRC = os.path.join(os.path.dirname(__file__), "driver.cpp")
DRIVER_BIN = os.path.join(os.path.dirname(__file__), "driver")


@pytest.fixture(scope="session", autouse=True)
def compile_driver():
    """Compile the shared driver binary before any test runs."""
    result = subprocess.run(
        ["g++", "-std=c++17", "-o", DRIVER_BIN, DRIVER_SRC],
        capture_output=True,
        text=True,
    )
    assert result.returncode == 0, (
        f"driver compilation failed:\n{result.stderr}"
    )
