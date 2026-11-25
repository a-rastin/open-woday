# Repository rules

## Testing — mandatory GPU Colab route

**Tests must be run on a GPU from Google Colab using `colab-cli`.** Running the
verification suite locally, in CI, or on a CPU-only runtime does **not** satisfy
the project's verification rule.

Use the repository runner as the single supported route:

```bash
scripts/colab-gpu-test.sh
```

Authenticate `colab-cli` before running. When a specific Google account is
requested, set `COLAB_EXPECTED_EMAIL` to that address so an account mismatch
blocks GPU allocation. See [README.md § Testing](README.md#testing) for setup
and runner options. Verification passes only when the runner exits zero.

Do not add a local test command, a CI test job, or a test framework as a
substitute for this route. The rule is also recorded as an engineering rule in
`dev-docs/tech-stack.md` §12.
