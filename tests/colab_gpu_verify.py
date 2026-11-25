"""VM-side driver for the mandatory GPU-Colab test route.

Executed on the Colab VM by ``scripts/colab-gpu-test.sh`` through
``colab exec -s <session> -f tests/colab_gpu_verify.py``. Any failure raises, so
``colab exec`` exits non-zero and the runner reports a broken route.

Implements the project rule (``AGENTS.md`` / ``dev-docs/tech-stack.md`` §12):
tests must be run on a GPU from Google Colab using colab-cli. This file is the
VM half of that route — it refuses to run the verification unless a *usable*
NVIDIA GPU is proven, then builds and runs the framework-free
``verify-persistence`` executable on that same GPU VM.
"""

from __future__ import annotations

import os
import subprocess
import tarfile

TARBALL = "/content/open-woday-src.tar.gz"
WORKDIR = "/content/open-woday"
TESTS = os.path.join(WORKDIR, "tests")


def step(title: str) -> None:
    print(f"\n==> {title}", flush=True)


def run(cmd: str, *, cwd: str | None = None, env: dict | None = None) -> None:
    """Run a command, echo its output into the kernel, and raise on failure.

    Child-process stdout is not captured by `colab exec` by itself, so capture
    it explicitly and print it: the verification PASS/FAIL line and any build
    diagnostics must be visible in the route's output.
    """
    print(f"+ {cmd}", flush=True)
    result = subprocess.run(
        cmd,
        shell=True,
        cwd=cwd,
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if result.stdout:
        print(result.stdout, end="" if result.stdout.endswith("\n") else "\n", flush=True)
    if result.returncode != 0:
        raise subprocess.CalledProcessError(
            result.returncode, cmd, output=result.stdout
        )


def verify_gpu() -> None:
    """Fail loudly unless this runtime really has a working NVIDIA GPU."""
    step("Proving a usable NVIDIA GPU is present on this runtime")
    probe = subprocess.run(
        ["nvidia-smi", "--query-gpu=name,memory.total", "--format=csv,noheader"],
        capture_output=True,
        text=True,
    )
    if probe.returncode != 0 or not probe.stdout.strip():
        raise RuntimeError(
            "nvidia-smi found no NVIDIA GPU - refusing to run the test on a "
            "CPU runtime (the GPU Colab rule would be violated)"
        )
    print("nvidia-smi:", probe.stdout.strip(), flush=True)

    try:
        import torch
    except ImportError as exc:
        raise RuntimeError("PyTorch is required for the CUDA compute check") from exc

    if not torch.cuda.is_available():
        raise RuntimeError("torch.cuda.is_available() is False on this runtime")
    print("torch CUDA device:", torch.cuda.get_device_name(0), flush=True)

    # A real CUDA computation proves the accelerator works, not just that a
    # device node is visible.
    a = torch.randn(512, 512, device="cuda")
    b = torch.randn(512, 512, device="cuda")
    c = a @ b
    torch.cuda.synchronize()
    print("GPU compute smoke test: OK", tuple(c.shape), flush=True)


def unpack() -> None:
    step("Unpacking the uploaded source tree")
    os.makedirs(WORKDIR, exist_ok=True)
    with tarfile.open(TARBALL, "r:gz") as archive:
        try:
            archive.extractall(WORKDIR, filter="data")
        except TypeError:  # Python < 3.12
            archive.extractall(WORKDIR)
    print("unpacked into", WORKDIR, flush=True)


def install_qt() -> None:
    step("Ensuring a Qt 6 qmake6 toolchain is available on the VM")
    env = dict(os.environ, DEBIAN_FRONTEND="noninteractive")
    try:
        run(
            "apt-get update -qq && "
            "apt-get install -y -qq qt6-base-dev libqt6sql6-sqlite",
            env=env,
        )
    except subprocess.CalledProcessError:
        # Some Ubuntu releases ship the QSQLITE plugin inside qt6-base-dev.
        run("apt-get install -y -qq qt6-base-dev", env=env)
    run("qmake6 --version")


def build_and_test() -> None:
    step("Building verify-persistence with qmake6")
    run('qmake6 verify-persistence.pro && make -j"$(nproc)"', cwd=TESTS)

    step("Running verify-persistence on the GPU VM")
    run("./verify-persistence", cwd=TESTS)


def main() -> None:
    verify_gpu()
    unpack()
    install_qt()
    build_and_test()
    print(
        "\n==> PASS - verify-persistence passed on a GPU-backed Google Colab "
        "runtime via colab-cli",
        flush=True,
    )


# Called unconditionally: `colab exec` sends the file to the kernel, where
# __name__ is not guaranteed to be "__main__".
main()
