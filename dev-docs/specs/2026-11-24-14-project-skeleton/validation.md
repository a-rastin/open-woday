# Phase 1 — project-skeleton — Validation

> **Spec:** `dev-docs/specs/2026-11-24-14-project-skeleton/` · **Branch:** `phase-1-project-skeleton`
> How to know the Phase 1 implementation succeeded and can be merged. Holds the
> `/to-spec` "Testing Decisions" and the roadmap's "Verifiable when" gate.
> Uses `GLOSSARY.md` vocabulary; no schedule or effort estimates (D7 / D9).

## Verifiable when (the merge gate)

From `roadmap.md` Phase 1: **"the app configures, compiles and opens a window on
Linux."** A phase ships only if it is a runnable / verifiable increment
(`roadmap.md` §1). Phase 1 does **not** satisfy any full v0.1 Definition-of-Done
item (`mission.md` §10 / `roadmap.md` §3) on its own; those are completed in
Phases 2–12. Its gate is the condition above plus the checks below.

## Testing Decisions

### Seam decision — please confirm

Per the `/to-spec` skill, seams are sketched at the **highest possible level**,
existing seams preferred, and the ideal number is **one**. This is greenfield
(`tech-stack.md` §12: "There are no tests yet — this is greenfield"), so there are
no existing seams to reuse.

- **The single seam:** the application's **externally observable build-and-launch
  behaviour** — `qmake6 && make` produces a binary, and launching that binary
  opens a window on Linux. This is the highest seam available and needs no new
  seam to be introduced.
- **No new lower seams.** Phase 1 adds no unit-test framework, no test doubles and
  no seams around `main.cpp` internals. The C++ entry point has no business logic
  to test, and testing QML component internals would be testing implementation
  details. The first natural unit seam appears in **Phase 2**, when the repositories
  gain CRUD behaviour worth testing.
- **Reviewer confirmation:** if the maintainer expects a different seam (for
  example a committed smoke-test script or a CI job in this phase), say so before
  `/bnvt-ticket`; otherwise this single-seam decision stands.

### What makes a good test here

- Test **external behaviour**, not implementation details: "the build produces a
  runnable binary" and "a window appears", not "this C++ symbol exists".
- Assert on observable outcomes (exit codes, a visible native window, clean
  teardown), never on internal structure.
- Keep the seam count at one; prefer the build/run seam over any new harness.

### Modules in scope for validation

There are no behavioural modules yet. Validation covers the **build definition**,
the **C++ entry point** and the **QML root window** only as they surface through
the single launch seam. Prior art: none — this is the first code in the repo
(`tech-stack.md` §12).

## Validation checks

Run on Linux with the checked toolchain (`tech-stack.md` §4): Qt 6.12
(`Qt6Quick`, `Qt6Widgets`, `Qt6Core`, SQL/SQLite driver), `qmake6`, `g++`; no
CMake.

1. **Configure.** `qmake6 open-woday.pro` exits `0`, emits no errors, and produces
   a Makefile. No `CMakeLists.txt` exists.
2. **Compile.** `make` exits `0`, produces the `open-woday` binary, with no errors
   and no new compiler warnings.
3. **Launch.** Running the binary opens exactly **one visible window** on the
   Linux desktop, using **native OS decorations** (no drawn macOS traffic lights),
   titled `open-woday` only.
4. **Empty by design.** The window contains no placeholder pages, disabled
   destinations, badge chips, day strip, panels or fake content ("no empty
   shells", D5 / D12).
5. **Teardown & repeatability.** Closing the window exits the process with code
   `0`; relaunching opens the window again.
6. **Constraint review (diff inspection).** The change contains:
   - no webview of any kind (no `QWebEngineView`, no HTML/CSS/JS shell) — ADR-0002;
   - no network classes / `QNetworkAccessManager` and no runtime network access — D3;
   - no schema, migrations, `meta` version, repositories, SQLite file or
     `Item`/`Block` model code (Phase 2 boundary);
   - no three-panel shell, tokens or panel components (Phase 3 boundary);
   - no upstream logo, mascot, UI string, guide text or screenshot bundled —
     ADR-0001 / ADR-0004; all strings original;
   - no test framework, CI config or packaging scripts;
   - no CMake files;
   - canonical `GLOSSARY.md` vocabulary in identifiers/comments, with none of the
     `_Avoid_` terms used for domain concepts.
7. **Docs consistency.** The README's documented `qmake6 open-woday.pro && make`
   now works as written; the README is **not** edited in this phase.
8. **Spec hygiene.** No schedule or effort estimates anywhere in the spec (D7 /
   D9); no settled decision or absorbed ADR is re-litigated.

## Merge criteria

Phase 1 can be merged when **all** of the following hold:

1. Every numbered task group in `plan.md` (groups 1–4) is complete.
2. All validation checks above pass on Linux.
3. The "Verifiable when" condition is met: the app configures, compiles and opens
   a window.
4. Constitutional compliance: ADR-0002 (no webview), D3 (no network), D15 (native
   chrome), ADR-0001 / ADR-0004 (original assets & strings), D5 / D12 (no empty
   shells), D6 / D11 (one codebase, Linux primary), D7 / D9 (no estimates), and
   `GLOSSARY.md` vocabulary.
5. The increment is **runnable / demoable on its own** (`roadmap.md` §1) — it must
   not ship a non-demoable horizontal layer.
6. No out-of-scope item leaked in: none of Phases 2–12, none of the deferred
   backlog (`roadmap.md` §4), none of the permanent non-goals (`mission.md` §8).
7. `/bnvt-review` is clean and `/bnvt-ticket` can derive tracer-bullet tickets
   from `plan.md`.
8. Per the workflow, the spec files are written on the phase branch but **not
   committed in the spec step**; committing is the `/bnvt-commit` stage.
