# Phase 4 — today-page — Validation

> **Spec:** `dev-docs/specs/2026-11-25-13-today-page/` · **Branch:** `phase-4-today-page`
> How to know the Phase 4 implementation succeeded and can be merged. Holds the
> `/to-spec` "Testing Decisions" and the roadmap's "Verifiable when" gate. Uses
> `GLOSSARY.md` vocabulary; no schedule or effort estimates (D7 / D9).

## Verifiable when (the merge gate)

From `roadmap.md` Phase 4: **"Today renders a day's Task/Event lines and the
editable Day note persists across restarts."** A phase ships only if it is a
runnable / verifiable increment (`roadmap.md` §1). Phase 4 does **not** satisfy
the full v0.1 Definition of Done (`mission.md` §10 / `roadmap.md` §3) on its
own — it directly advances item 3 (Today as an editable Day note), while the
full 5-block editor and to-do→Task links (item 5) are completed in Phases 6–7.
Its gate is the condition above plus the checks below. Because this is the first
phase with live product data, the gate is proven through the launched window
**and** the persistence/restart seam together.

## Testing Decisions

### Seam decision — please confirm

Per the `/to-spec` skill, seams are sketched at the **highest possible level**,
existing seams preferred, and the ideal number is **one**. Phase 1 established
the build-and-launch seam; Phase 2 added the framework-free persistence seam
(`tests/verify-persistence.cpp`); Phase 3 kept the build-and-launch appearance
seam for the shell. Phase 4 reuses both rather than adding new lower ones.

- **The single seam:** the **externally observable Today behaviour across a
  restart** — `qmake6 && make` produces the binary; launching it opens the
  three-panel frame with the Today page, the day strip, summary line and the
  day's Task/Event lines with check-off; writing free text and checking off a
  Task, then restarting against the same SQLite file, shows the same Day note,
  lines and completion state rendered again unchanged. This is the highest seam
  available and needs no new seam around QML component internals, C++ model
  internals or individual SQL statements.
- **No new lower seams.** Do not add a unit-test framework, test doubles or
  seams around individual QML properties, token values, model methods or query
  strings. Testing whether a specific token constant was declared or a specific
  SQL string was issued would be testing implementation details; the observable
  contract is "Today renders the day's lines and the Day note survives a
  restart".
- **Reuse the one executable seam.** Extend the existing framework-free
  `tests/verify-persistence.cpp` (built from `tests/verify-persistence.pro`) to
  cover the Day-note-per-date wiring and Task/Event/Block round-trips; keep the
  verification at one seam with no test framework (`tech-stack.md` §12).
- **Local execution.** Build and run the executable verification on the
  developer's machine (`AGENTS.md`; `tech-stack.md` §12). From `tests/`, run
  `qmake6 verify-persistence.pro`, `make -j"$(nproc)"`, then
  `./verify-persistence`; verification passes only when all commands exit zero.
  No remote runtime, Google account or GPU is required.
- **Reviewer confirmation:** if the maintainer expects a different seam (for
  example a committed screenshot-diff harness, a UI automation job or a CI gate
  in this phase), say so before `/bnvt-ticket`; otherwise this single-seam
  decision stands.
- **Approval status (spec approval process).** This spec was synthesized
  without an interview and is a **proposal**. No maintainer confirmation of the
  seam decision, nor of the next-phase reading in `requirements.md` §"Where this
  phase comes from", has been recorded. Both are left **open before
  `/bnvt-ticket`**, following the project's spec approval process: the spec is
  published as a proposal awaiting confirmation rather than claiming a sign-off
  that was never given. This note is an explicit record of that open question —
  it is a judgement call about how the phase is verified, **not** maintainer
  sign-off.

### What makes a good test here

- Test **external behaviour**, not implementation details: "launching shows
  Today with the day's Task/Event lines and an editable Day note" and "the Day
  note, its free writing, its lines and check-off state read back unchanged
  after a restart", not "this QML symbol exists" or "this exact SQL ran".
- Assert on observable outcomes (a clean configure/compile, one visible native
  window, the three-panel frame with the Today page, day strip and summary line,
  Task/Event lines with badge chips, check-off persistence, one Day note per
  date, free writing persisted as Blocks, original strings and no bundled
  upstream assets), never on internal structure.
- Prefer the single launch → write → restart → read-back round trip over any
  harness that mocks the store; keep the seam count at one.
- Keep the test data canonical: use `GLOSSARY.md` terms and cover all three Item
  kinds (Task, Note, Event) and the Day-note-per-date rule.

### Modules in scope for validation

- The declarative Today QML surface (page, day strip, summary line, Task/Event
  lines, badge chips, checkbox) and the C++ `TodayModel` mediating it.
- The Phase 2 store as surfaced through the seam: `ItemRepository` (Day note,
  Task and Event Items), `BlockRepository` (Day-note free writing), `Database`
  and the `meta` schema version.
- `resources.qrc` embedding, and the set of original strings/icons shipped.
- **Prior art:** the Phase 1/3 build-and-launch merge-gate checks, Phase 2's
  framework-free `tests/verify-persistence.cpp` restart seam, and the
  `.code-review/` records for Phases 1–3.

## Validation checks

Run the executable verification locally from `tests/`:

```bash
qmake6 verify-persistence.pro
make -j"$(nproc)"
./verify-persistence
```

The build commands below describe what is configured as part of this phase.

Run on Linux with the checked toolchain (`tech-stack.md` §4): Qt 6.12
(`Qt6Quick`, `Qt6Widgets`, `Qt6Core`), the SQL/SQLite driver, `qmake6`, `g++`;
no CMake.

1. **Configure.** `qmake6 open-woday.pro` exits `0`, emits no errors and
   produces a Makefile. No `CMakeLists.txt` exists; `QT += quick widgets sql`
   and C++17 are unchanged. The verification program is still configured
   separately from `tests/` with `qmake6 verify-persistence.pro && make` — one
   additional qmake target, no CMake.
2. **Compile.** `make` exits `0`, produces the `open-woday` binary, with no
   errors and no new compiler warnings.
3. **Local verification.** From `tests/`,
   `qmake6 verify-persistence.pro && make -j"$(nproc)" && ./verify-persistence`
   exits `0` and prints `verify-persistence: PASS`. No remote runtime or GPU is
   required.
4. **Launch unaffected.** Running the binary still opens the Phase 3
   three-panel frame with native chrome, the dark sidebar, white rounded centre
   and right rail, the Sidebar-then-RightRail fold order and platform-correct
   `Ctrl`/`⌘` labels; no macOS traffic lights on Linux. The centre panel now
   shows the Today page instead of an empty surface.
5. **Day strip and summary.** The Today page shows a day strip of ≈10
   selectable cells (today accented) and a summary line with original strings.
6. **Task/Event lines.** The selected day's Task and Event lines render from
   real Task/Event Items with badge chips for set details (due time, priority,
   Tag name when present); priority colours the checkbox. Events render from
   lightweight Event Items with no calendar grid.
7. **Day note wiring.** Exactly one active Day note Item exists per date
   (`kind='note'`, `day='YYYY-MM-DD'`); opening a new date creates one, opening
   it again reuses it, and a second active Day note for the same date is
   rejected. A soft-deleted Day note frees its date for a replacement. No new
   entity, kind, table, index or migration was added.
8. **Check-off.** Checking off a Task line sets `completed_at` through the store
   and the state survives a restart; no In Progress / Waiting state, no
   cascading subtask soft-delete and no undo stack are introduced.
9. **Free writing.** Free writing around the lines is editable and persists as
   ordered `blocks` rows (plain UTF-8 `content`, `position`) through
   `BlockRepository`. The 5-type editor, inline formatting, `inline_json`
   ranges, Markdown shortcuts and `blocks.task_id` links are absent (Phases 6–7).
10. **Restart survival (the seam).** Write a representative Day note, its free
    writing, and Task/Event lines, restart against the same SQLite file, and
    confirm all of it renders and reads back unchanged — the roadmap's "editable
    Day note persists across restarts" condition and the single highest seam.
11. **Constraint review (diff inspection).** The change contains:
    - no webview of any kind (no `QWebEngineView`, no HTML/CSS/JS shell) — ADR-0002;
    - no `QNetwork*` classes and no runtime network access — D3;
    - no full Task CRUD / detail pane / subtasks (Phase 5), no 5-type block
      editor / inline formatting (Phase 6), no to-do→Task linking / Task-Event
      Note panes (Phase 7), no Inbox rail / Folders / Tags UI (Phase 8), no
      Quick Add (Phase 9), no CSV export (Phase 10), no visual tuning (Phase 11)
      or packaging (Phase 12);
    - no schema rewrite, new migration, new entity/kind/table or new index;
    - no upstream logo, mascot, UI string, guide text or screenshot bundled —
      ADR-0001 / ADR-0004; all strings original; `DESIGN.md` is not used as a
      UI spec;
    - no test framework, CI config, low-level unit seams or packaging scripts
      beyond the single extended persistence seam;
    - no CMake files;
    - canonical `GLOSSARY.md` vocabulary in identifiers/comments, with none of
      the `_Avoid_` terms used for domain concepts.
12. **Docs consistency.** Today-page feature work leaves README / CHANGELOG /
    roadmap unchanged, apart from the separate local-testing rule amendment;
    the documented `qmake6 open-woday.pro && make` command still works as
    written.
13. **Spec hygiene.** No schedule or effort estimates anywhere in the spec (D7 /
    D9); no settled decision or absorbed ADR is re-litigated; the spec is
    recorded as a proposal with maintainer confirmation outstanding (spec
    approval process), and no maintainer sign-off is fabricated.

## Merge criteria

Phase 4 can be merged when **all** of the following hold:

1. Every numbered task group in `plan.md` (groups 1–8) is complete.
2. All validation checks above pass, including the local
   `verify-persistence` build and executable exiting zero.
3. The "Verifiable when" condition is met: Today renders a day's Task/Event
   lines and the editable Day note persists across restarts.
4. Constitutional compliance: ADR-0002 (no webview), D3 / D21 (no network,
   local SQLite, restart survival), D17 / D19 (unified Item model, Day note is
   not a new entity, exactly 5 editor types remain Phase 6), D15 (native chrome
   and platform-correct modifiers preserved), ADR-0001 / ADR-0004 (original
   assets & strings), D5 / D12 (no empty shells), D6 / D11 (one codebase, Linux
   primary), D7 / D9 (no estimates), the local test-execution rule
   (`AGENTS.md` / `tech-stack.md` §12), and `GLOSSARY.md` vocabulary.
5. The increment is **runnable / verifiable on its own** through the single
   launch → write → restart seam (`roadmap.md` §1) — it must not ship a
   non-demoable horizontal layer.
6. No out-of-scope item leaked in: none of Phases 5–12, none of the deferred
   backlog (`roadmap.md` §4), none of the permanent non-goals (`mission.md` §8).
7. `/bnvt-review` is clean and `/bnvt-ticket` can derive tracer-bullet tickets
   from `plan.md`.
8. Per the workflow, the spec files are written on the phase branch but **not
   committed** in the spec step; committing is the `/bnvt-commit` stage.
