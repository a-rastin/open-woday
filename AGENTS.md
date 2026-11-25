# Repository rules

## Testing — local execution

Run the verification suite on this machine before reporting verification complete:

```bash
cd tests
qmake6 verify-persistence.pro
make -j"$(nproc)"
./verify-persistence
```

Verification passes only when all commands exit zero. Keep the executable seam
framework-free; no remote runtime or GPU is required. The engineering rule is also
recorded in `dev-docs/tech-stack.md` §12, and setup is documented in
[README.md § Testing](README.md#testing).
