#!/usr/bin/env bash
# Enforce the project's testing rule (AGENTS.md / dev-docs/tech-stack.md §12):
# tests must be run on a GPU from Google Colab using colab-cli.
#
# Route: colab new --gpu ... -> colab upload -> colab exec -> colab stop.
# The VM-side driver (tests/colab_gpu_verify.py) refuses to run the test unless
# a usable NVIDIA GPU is proven, then builds and runs verify-persistence.
#
# Usage: scripts/colab-gpu-test.sh
#   COLAB_SESSION  session name       (default: open-woday-test)
#   COLAB_GPU      GPU variant        (default: T4; T4/L4/G4/H100/A100)
#   COLAB_KEEP=1   leave the runtime running instead of stopping it
#   COLAB_AUTH     oauth2 or adc      (default: oauth2)
#   COLAB_EXPECTED_EMAIL  require this Google account before allocating a GPU
set -Eeuo pipefail

SESSION="${COLAB_SESSION:-open-woday-test}"
GPU="${COLAB_GPU:-T4}"
KEEP="${COLAB_KEEP:-0}"
AUTH="${COLAB_AUTH:-oauth2}"
EXPECTED_EMAIL="${COLAB_EXPECTED_EMAIL:-}"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DRIVER="$ROOT/tests/colab_gpu_verify.py"
REMOTE_TARBALL="/content/open-woday-src.tar.gz"
CLI=(colab --auth="$AUTH")

die() {
    echo "colab-gpu-test: ERROR - $*" >&2
    exit 1
}

command -v colab >/dev/null 2>&1 || \
    die "colab-cli is not installed (uv tool install google-colab-cli)"
[[ -f "$DRIVER" ]] || die "missing VM-side driver $DRIVER"
case "$AUTH" in oauth2|adc) ;; *) die "COLAB_AUTH must be oauth2 or adc" ;; esac
case "$GPU" in T4|L4|G4|H100|A100) ;; *) die "unsupported COLAB_GPU: $GPU" ;; esac
[[ "$SESSION" =~ ^[a-zA-Z0-9][a-zA-Z0-9_-]*$ ]] || \
    die "COLAB_SESSION must start with a letter or digit and contain only letters, digits, hyphens, and underscores"

# Check the account before `sessions`, which can prune local state for the
# currently authenticated account. Leave interactive OAuth to the operator.
if [[ -n "$EXPECTED_EMAIL" ]]; then
    if ! identity="$("${CLI[@]}" whoami </dev/null)"; then
        die "could not check the Google account; authenticate colab-cli first"
    fi
    email="$(awk '$1 == "Email:" {print $2}' <<<"$identity")"
    [[ "$email" == "$EXPECTED_EMAIL" ]] || \
        die "expected Google account $EXPECTED_EMAIL, but colab-cli reports ${email:-no email}; no runtime was allocated"
fi

# Close stdin so the CLI cannot hang waiting for an OAuth code.
if ! sessions="$("${CLI[@]}" sessions </dev/null)"; then
    die "could not list Colab sessions; check the error above and your login with 'colab --auth=$AUTH whoami'"
fi
if awk -v label="[$SESSION] " 'index($0, label) == 1 {found = 1} END {exit !found}' <<<"$sessions"; then
    die "session '$SESSION' is already active; set COLAB_SESSION to a different name"
fi
ORPHANS="$(grep '^\[?\]' <<<"$sessions" || true)"
if [[ -n "$ORPHANS" ]]; then
    echo "colab-gpu-test: WARNING - server-side assignment(s) without a local record:" >&2
    echo "$ORPHANS" >&2
    echo "These can block allocation (HTTP 412). Release them first if new fails." >&2
fi

TEMP_ROOT="${TMPDIR:-/tmp/opencode}"
[[ -d "$TEMP_ROOT" ]] || TEMP_ROOT=/tmp
WORK="$(mktemp -d "$TEMP_ROOT/colab-gpu-test.XXXXXX")"
TARBALL="$WORK/open-woday-src.tar.gz"
CREATED=0
ENDPOINT=""

release_runtime() {
    echo "==> Releasing the Colab runtime (session: $SESSION)"
    "${CLI[@]}" stop -s "$SESSION" </dev/null || return 1
    local remaining
    remaining="$("${CLI[@]}" sessions </dev/null)" || return 1
    ! awk -v endpoint="$ENDPOINT" -v label="[$SESSION] " \
        '(endpoint != "" && $2 == endpoint) || index($0, label) == 1 {found = 1} END {exit !found}' \
        <<<"$remaining"
}

cleanup() {
    local status=$?
    if [[ "$CREATED" == "1" && "$KEEP" != "1" ]] && ! release_runtime; then
        echo "colab-gpu-test: could not confirm runtime release; check 'colab --auth=$AUTH sessions' and disconnect the runtime if it is still active (endpoint: ${ENDPOINT:-unknown})" >&2
        status=1
    fi
    rm -rf -- "$WORK"
    if [[ "$status" == "0" ]]; then
        echo "==> PASS - verify-persistence passed on a GPU-backed Google Colab runtime via colab-cli (requested: $GPU)"
    fi
    exit "$status"
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

echo "==> Allocating a $GPU GPU runtime via colab-cli (session: $SESSION)"
if ! "${CLI[@]}" new -s "$SESSION" --gpu "$GPU" </dev/null; then
    die "GPU allocation failed; check your Colab GPU quota, capacity, and active sessions. CPU fallback is not allowed."
fi
CREATED=1
status="$("${CLI[@]}" status -s "$SESSION" </dev/null)"
ENDPOINT="$(awk -v label="[$SESSION] " 'index($0, label) == 1 {print $2; exit}' <<<"$status")"
[[ -n "$ENDPOINT" ]] || die "could not identify the allocated Colab runtime"
echo "==> GPU runtime endpoint: $ENDPOINT"
if [[ "$KEEP" == "1" ]]; then
    echo "==> COLAB_KEEP=1: stop the runtime with: colab --auth=$AUTH stop -s $SESSION"
fi

# A freshly READY runtime's proxy can take a few seconds to become routable;
# establish the kernel before transferring files, then retry transient errors.
echo "==> Waiting for the runtime kernel to be reachable"
ready=0
for attempt in 1 2 3 4 5 6; do
    if "${CLI[@]}" exec -s "$SESSION" <<< 'print("kernel ready")' >"$WORK/kernel.log" 2>&1; then
        ready=1
        break
    fi
    sleep 5
done
if [[ "$ready" != 1 ]]; then
    cat "$WORK/kernel.log" >&2
    die "the runtime kernel never became reachable"
fi

echo "==> Packing the framework-free verification sources"
tar -czf "$TARBALL" -C "$ROOT" \
    src \
    resources.qrc \
    tests/verify-persistence.cpp \
    tests/verify-persistence.pro

echo "==> Uploading the source tree to the GPU runtime"
uploaded=0
for attempt in 1 2 3 4 5; do
    if "${CLI[@]}" upload -s "$SESSION" "$TARBALL" "$REMOTE_TARBALL" </dev/null; then
        uploaded=1
        break
    fi
    echo "colab-gpu-test: upload attempt $attempt failed; retrying..." >&2
    sleep 8
done
[[ "$uploaded" == 1 ]] || die "colab upload failed after 5 attempts"

echo "==> Proving the GPU, then building and running verify-persistence on the VM"
# --timeout 0: apt-get / qmake6 can be quiet for longer than the default 30 s.
"${CLI[@]}" exec -s "$SESSION" --timeout 0 -f "$DRIVER" </dev/null || \
    die "the GPU Colab verification failed on the VM"
