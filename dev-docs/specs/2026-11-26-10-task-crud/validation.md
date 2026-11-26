# Phase 5 — task-crud — Validation

> **Spec:** `dev-docs/specs/2026-11-26-10-task-crud/` · **Branch:** `phase-5-task-crud`
> How to know the Phase 5 implementation succeeded and can be merged. Holds the
> `/to-spec` "Testing Decisions" and the roadmap's "Verifiable when" gate. Uses
> `GLOSSARY.md` vocabulary; no schedule or effort estimates (D7 / D9).

## Verifiable when (the merge gate)

From `roadmap.md` Phase 5: **"a Task can be created, scheduled, tagged, nested
and completed, and is reflected on Today."** A phase ships only if it is a
runnable / verifiable increment (`roadmap.md` §1). Phase 5 advances `mission.md`
§10 item 4 (*"Can create, edit, tag, schedule, nest and complete Tasks; each Task
has its own Note pane"*), while the 5-type block editor and `blocks.task_id`
to-do→Task links (item 5) remain Phases 6–7. Its gate is the condition above plus
the checks below, proven through the launched window and the local persistence
restart seam together.

## Testing Decisions

### Seam decision — please confirm

Per the `/to-spec` skill, seams are sketched at the **highest possible level**,
existing seams preferred, and the ideal number is **one**. Phases 1–4
established the build-and-launch window check and the single framework-free
persistence seam (`tests/verify-persistence.cpp`). Phase 5 reuses both rather
than adding new lower seams.

- **The single seam:** the **externally observable Task lifecycle across Today,
  the detail pane, and a restart** — `qmake6 && make` produces the binary;
  launching it opens the three-panel frame; creating, renaming, scheduling (day
  and optional time), prioritizing, tagging, filing into a Folder, setting
  duration, nesting subtasks, editing the Task's Note scaffold, completing,
  reopening and deleting Tasks updates Today and the right-rail detail pane
  immediately; restarting against the same SQLite file shows every surviving
  Task, attribute, Tag, Folder, subtask and Task Note unchanged (and
  soft-deleted Tasks/subtasks absent).
- **No new lower seams.** Do not add a unit-test framework, mock repositories,
  or seams testing internal SQL statements, QML private helper bindings or
  internal signals in isolation.
- **Reuse the one executable seam.** Extend the existing framework-free
  `tests/verify-persistence.cpp` (built from `tests/verify-persistence.pro`) to
  exercise the Phase 5 `ItemModel` / `TodayModel` / repository lifecycle across
  a closed-and-reopened `Database` (`tech-stack.md` §12).
- **Local execution.** Build and run the executable verification on the
  developer's machine (`AGENTS.md`; `tech-stack.md` §12). From `tests/`, run
  `qmake6 verify-persistence.pro`, `make -j"$(nproc)"`, then
  `./verify-persistence`; verification passes only when all commands exit zero.
  No remote runtime, Google account or GPU is required.
- **Reviewer confirmation:** if the maintainer expects a different seam (for
  example an automated QML UI test runner or CI job in this phase), say so
  before `/bnvt-ticket`; otherwise this single-seam decision stands.
- **Approval status (spec approval process).** This spec was synthesized
  without an interview and is a **proposal**. No maintainer confirmation of the
  seam decision, nor of the next-phase reading in `requirements.md` §"Where this
  phase comes from", has been recorded. Both are left **open before
  `/bnvt-ticket`**, following the project's spec approval process rather than
  claiming an unrecorded sign-off.

### What makes a good test here

- Test **external behaviour**, not implementation details: "creating,
  scheduling, tagging, nesting, completing and deleting a Task updates Today
  and persists across a restart", not "this internal SQL string ran" or "this
  private method was called".
- Assert on observable outcomes (clean build, live Today & detail-pane state,
  priority/tag/duration/due-time badges, subtask progress and cascading
  soft-delete, round-trip survival across `Database` close/reopen, original
  strings and no bundled upstream assets).
- Keep the test data canonical using `GLOSSARY.md` terms (Task, Note, Block,
  Tag, Folder, Day note).

### Modules in scope for validation

- Store layer (`src/store/`): Folder and Tag persistence (`folders`, `tags`,
  `item_tags`), `ItemRepository` (including cascading subtask `softDelete`),
  `BlockRepository` (Task Note plain-text Blocks), `Database` (unchanged schema).
- Application model layer (`src/app/`): `ItemModel` and its synchronization with
  `TodayModel`.
- Declarative QML layer (`src/qml/`): `TaskRow`, `TaskDetailPane`, `RightRail`,
  `TodayPage`, and `components/` (`Checkbox`, `Badge`, `TagChip`, `IconButton`),
  plus `resources.qrc` and `open-woday.pro`.
- **Prior art:** Phases 1–4 build/launch checks and `tests/verify-persistence.cpp`
  restart round-trips.

## Validation checks

Run the executable verification locally from `tests/`:

```bash
qmake6 verify-persistence.pro
make -j"$(nproc)"
./verify-persistence
```

Run on Linux with the checked toolchain (`tech-stack.md` §4): Qt 6.12
(`Qt6Quick`, `Qt6Widgets`, `Qt6Core`), the SQL/SQLite driver, `qmake6`, `g++`;
no CMake.

1. **Configure & Compile.** `qmake6 open-woday.pro && make` exits `0`, produces
   `open-woday`, with no errors and no new compiler warnings. No `CMakeLists.txt`
   exists.
2. **Local verification.** From `tests/`,
   `qmake6 verify-persistence.pro && make -j"$(nproc)" && ./verify-persistence`
   exits `0` and prints `verify-persistence: PASS`.
3. **Shell & navigation intact.** Launching `./open-woday` opens the
   three-panel frame with native OS decorations, dark sidebar, white rounded
   centre panel and right rail, Sidebar-then-RightRail fold order, and
   platform-correct `Ctrl`/`⌘` labels; no macOS traffic lights on Linux.
4. **Task CRUD & inline rename.** A Task can be created, renamed inline on its
   row and in `TaskDetailPane`, completed, reopened, and soft-deleted; Today
   reflects every change immediately.
5. **Detail pane & badge row.** Clicking a Task row opens `TaskDetailPane` in
   the right rail; closing it returns the rail to idle. The badge row shows
   **only set details**; the `"+"` badge offers unset details (`Due date`,
   `Priority`, `Duration`, `Folder`) plus `Add tag`.
6. **Scheduling (day + optional time).** Setting, editing or clearing a Task's
   due day and optional 24-hour time (`HH:mm`) updates `due_at`/`due_precision`
   and immediately adds/updates/removes the Task on Today's selected day and
   renders a due-time chip when timed.
7. **Priority, Tags, Folder, Duration.**
   - Priority (`0..3`) colours the checkbox on Today and in the detail pane, and
     shows a priority badge when non-zero.
   - Multiple Tags can be created, attached and detached (`tags` / `item_tags`),
     rendering `TagChip`s on Today rows and in the detail pane.
   - An optional single Folder can be created, assigned and cleared (`folder_id`).
   - Inline planned duration (`duration_min`) can be set, edited and cleared.
8. **Nested subtasks & cascading soft-delete.** Subtasks (`parent_id`) can be
   created at multiple nesting depths, completed/reopened, expanded/collapsed on
   Today with `done/total` progress badges, and managed in the detail pane.
   Soft-deleting a parent Task cascades `deleted_at` to all descendant subtasks.
9. **Task Note pane scaffold.** Each Task's detail pane includes an editable
   plain-text Note scaffold backed by `blocks` rows on the Task's `item_id`,
   without overwriting non-plain-text Blocks.
10. **Restart survival (the seam).** Create, schedule, prioritize, tag, file,
    nest, rename, edit Note, complete and soft-delete Tasks, restart against the
    same SQLite file, and confirm all surviving state renders and reads back
    unchanged while soft-deleted Tasks/subtasks remain excluded.
11. **Constraint review (diff inspection).**
    - No webview (`QWebEngineView` or HTML/CSS/JS shell) — ADR-0002.
    - No `QNetwork*` classes or runtime network access — D3.
    - No schema rewrite or new SQL migration (`schema.sql` unchanged).
    - No Phase 6–12 scope: no 5-type block editor / `inline_json` formatting /
      Markdown shortcuts (Phase 6), no `blocks.task_id` links (Phase 7), no
      Inbox rail grouping / Sidebar Folder-Tag trees / drag-to-file (Phase 8),
      no Quick Add parser / `Ctrl+K` / `Ctrl+N` (Phase 9), no CSV export
      (Phase 10), no visual tuning pass (Phase 11), no packaging (Phase 12).
    - No Waiting / In Progress state, no Trash UI, no undo stack.
    - Original strings and assets only; `DESIGN.md` not used as app UI spec;
      canonical `GLOSSARY.md` vocabulary used throughout.
    - No CMake, no test framework, no low-level unit seams, no CI config.
12. **Spec hygiene.** No schedule or effort estimates (D7 / D9); no settled
    decision re-litigated; proposal awaiting confirmation; spec files remain
    uncommitted on `phase-5-task-crud`.

## Merge criteria

Phase 5 can be merged when **all** of the following hold:

1. Every numbered task group in `plan.md` (groups 1–5) is complete.
2. All validation checks above pass, including `qmake6 open-woday.pro && make`
   and local `tests/verify-persistence` exiting zero.
3. The "Verifiable when" condition is met: a Task can be created, scheduled,
   tagged, nested and completed, and is reflected on Today.
4. Constitutional compliance: ADR-0002, D3 / D21, D17 / D19, D15, ADR-0001 /
   ADR-0004, D5 / D12, D6 / D11, D7 / D9, local verification rule (`AGENTS.md` /
   `tech-stack.md` §12), and `GLOSSARY.md` vocabulary.
5. No out-of-scope item from Phases 6–12, deferred backlog (`roadmap.md` §4), or
   permanent non-goals (`mission.md` §8) leaked in.
6. `/bnvt-review` is clean and `/bnvt-ticket` can derive tracer-bullet tickets
   from `plan.md`.
7. Per `.opencode/commands/bnvt-spec.md`, the spec files are written on
   `phase-5-task-crud` and **not committed** in the spec step.
