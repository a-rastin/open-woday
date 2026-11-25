# Phase 2 — data-layer — Validation

> **Spec:** `dev-docs/specs/2026-11-24-16-data-layer/` · **Branch:** `phase-2-data-layer`
> How to know the Phase 2 implementation succeeded and can be merged. Holds the
> `/to-spec` "Testing Decisions" and the roadmap's "Verifiable when" gate.
> Uses `GLOSSARY.md` vocabulary; no schedule or effort estimates (D7 / D9).

## Verifiable when (the merge gate)

From `roadmap.md` Phase 2: **"the schema is created on first launch, repositories
can CRUD Items/Blocks, and data survives an app restart."** A phase ships only if
it is a runnable / verifiable increment (`roadmap.md` §1). Phase 2 does **not**
satisfy any full v0.1 Definition-of-Done item (`mission.md` §10 / `roadmap.md`
§3) on its own; those are completed in Phases 3–12. Its gate is the condition
above plus the checks below. Because the phase renders no product surface, the
gate is proven through the persistence seam, not through the UI.

## Testing Decisions

### Seam decision — please confirm

Per the `/to-spec` skill, seams are sketched at the **highest possible level**,
existing seams preferred, and the ideal number is **one**. Phase 1 introduced no
tests (`tech-stack.md` §12: greenfield) and explicitly recorded that "the first
natural unit seam appears in Phase 2, when the repositories gain CRUD behaviour
worth testing".

- **The single seam:** the **externally observable persistence behaviour** of the
  store layer — create Items and Blocks through the repositories, close the
  database (or relaunch the app), reopen the same SQLite file, and read the same
  data back unchanged. This is the highest seam available and needs no new seam
  around C++ internals. It exercises `Database`, `ItemRepository` and
  `BlockRepository` together as one behaviour.
- **No new lower seams.** Do not add a unit-test framework, test doubles or seams
  around individual SQL statements, private helpers or query strings. Testing
  whether a specific SQL string was issued would be testing implementation
  details; the observable contract is "written data survives a restart and reads
  back correctly".
- **Reviewer confirmation:** if the maintainer expects a different seam (for
  example a committed smoke-test harness or a CI job in this phase), say so before
  `/bnvt-ticket`; otherwise this single-seam decision stands.
- **Revise-stage record (confirmation still outstanding).** The increment landed
  a committed, framework-free `tests/verify-persistence.cpp` (built from
  `tests/verify-persistence.pro`) as the executable verification of the merge
  gate. The code review flagged its size as borderline scope creep against the
  single-seam rule; it is retained because removing it would leave the
  persistence seam unverified, but it is recorded here as an unconfirmed
  judgement call, not as maintainer sign-off.

### What makes a good test here

- Test **external behaviour**, not implementation details: "an Item written
  before a restart reads back with the same fields" and "Blocks read back in the
  same order", not "this C++ symbol exists" or "this exact DDL ran".
- Assert on observable outcomes (round-tripped field values, Block order, the
  `meta` schema version, absence of data loss across restart, soft-deleted rows
  excluded from normal reads), never on internal structure.
- Prefer the create → restart → read-back round trip over any harness that mocks
  the database; keep the seam count at one.
- Keep the test data canonical: use `GLOSSARY.md` terms and cover all three Item
  kinds (Task, Note, Event) and all five Block types.

### Modules in scope for validation

`Database` (connection, first-launch schema creation, versioned migrations,
`meta` schema version), `ItemRepository` (Item CRUD and soft-delete) and
`BlockRepository` (Block CRUD, ordering and cascade) — only as they surface
through the single persistence seam. **Prior art:** none — Phase 1 added no tests
and no test framework (`tech-stack.md` §12); this is the first behavioural seam in
the codebase.

## Validation checks

Run on Linux with the checked toolchain (`tech-stack.md` §4): Qt 6.12
(`Qt6Quick`, `Qt6Widgets`, `Qt6Core`), the SQL/SQLite driver, `qmake6`, `g++`;
no CMake.

1. **Configure.** `qmake6 open-woday.pro` exits `0`, emits no errors, and produces
   a Makefile. No `CMakeLists.txt` exists; `QT += quick widgets sql` and C++17 are
   unchanged. The verification program is configured separately from `tests/`
   with `qmake6 verify-persistence.pro && make` — one additional qmake target,
   no CMake.
2. **Compile.** `make` exits `0`, produces the `open-woday` binary, with no
   errors and no new compiler warnings.
3. **Launch unaffected.** Running the binary still opens the single empty native
   window from Phase 1 (no regression); the app exits cleanly on close.
4. **First launch.** On a fresh app-data directory, exactly one SQLite file is
   created in the platform app-data location, the seven v0.1 tables are created
   (`items`, `blocks`, `folders`, `tags`, `item_tags`, `links`, `meta`), and a
   `schema_version` row exists in `meta`.
5. **Schema shape.** The schema matches `tech-stack.md` §7.2: column names and
   types, `CHECK` constraints on `items.kind`, `items.due_precision`,
   `blocks.type` and `links.kind`; foreign keys with the specified cascade
   deletes; composite primary keys on `item_tags` and `links`;
   `blocks_by_item` index; and the partial unique index
   `one_day_note_per_day` (covering active rows only, `deleted_at IS NULL`).
   Inserting a second active Day note for the same `day` is rejected with a
   clear error, while the date of a soft-deleted Day note can be reused by a
   replacement.
6. **Migrations.** Migrations are versioned and applied in order; on a second
   launch, already-applied versions are skipped, the schema is not destructively
   rebuilt, and the recorded version is unchanged.
7. **Item CRUD.** `ItemRepository` creates, reads, updates and soft-deletes Items
   of all three kinds; every field (timestamps, completion, scheduling, priority,
   duration, parent, folder, day, sort order) round-trips unchanged; deletion sets
   `deleted_at` (no hard delete) and soft-deleted rows are excluded from normal
   reads while remaining in the file.
8. **Block CRUD.** `BlockRepository` creates, reads, updates, deletes and reorders
   Blocks; `position`, `type`, `content`, `inline_json`, `task_id` and `indent`
   round-trip; Blocks read back ordered by `position` within their owning Item;
   deleting an Item cascades to its Blocks.
9. **Restart survival (the seam).** Write a representative set of Items and
   Blocks, close the connection or relaunch, reopen the same file, and confirm all
   data reads back unchanged — this is the roadmap's "data survives an app
   restart" condition and the single highest seam.
10. **Query safety.** All writes use bound values; foreign-key enforcement is on;
   no SQL is assembled by concatenating user text.
11. **Constraint review (diff inspection).** The change contains:
    - no webview of any kind (no `QWebEngineView`, no HTML/CSS/JS shell) — ADR-0002;
    - no `QNetwork*` classes and no runtime network access — D3;
    - no three-panel shell, tokens, pages or panel components (Phase 3 boundary);
    - no Today page / Day note rendering (Phase 4), Task CRUD / detail pane (Phase 5),
      block editor / inline-formatting UI (Phase 6), to-do→Task linking (Phase 7),
      Inbox rail / Folders / Tags UI (Phase 8), Quick Add (Phase 9) or CSV export
      (Phase 10);
    - no upstream logo, mascot, UI string, guide text or screenshot bundled —
      ADR-0001 / ADR-0004; all strings original;
    - no test framework, CI config, low-level unit seams or packaging scripts
      beyond the single persistence seam;
    - no CMake files;
    - canonical `GLOSSARY.md` vocabulary in identifiers/comments, with none of the
      `_Avoid_` terms used for domain concepts.
12. **Docs consistency.** README / CHANGELOG / roadmap are **not edited** in this
    phase; the documented `qmake6 open-woday.pro && make` command still works as
    written.
13. **Spec hygiene.** No schedule or effort estimates anywhere in the spec (D7 /
    D9); no settled decision or absorbed ADR is re-litigated.

## Merge criteria

Phase 2 can be merged when **all** of the following hold:

1. Every numbered task group in `plan.md` (groups 1–8) is complete.
2. All validation checks above pass on Linux.
3. The "Verifiable when" condition is met: the schema is created on first launch,
   repositories can CRUD Items/Blocks, and data survives an app restart.
4. Constitutional compliance: ADR-0002 (no webview), D3 / D21 (no network, local
   SQLite, restart survival), D17 (unified Item model), D15 (native chrome
   preserved), ADR-0001 / ADR-0004 (original assets & strings), D5 / D12 (no
   empty shells), D6 / D11 (one codebase, Linux primary), D7 / D9 (no estimates),
   and `GLOSSARY.md` vocabulary.
5. The increment is **runnable / verifiable on its own** through the single
   persistence seam (`roadmap.md` §1) — it must not ship a non-demoable horizontal
   layer.
6. No out-of-scope item leaked in: none of Phases 3–12, none of the deferred
   backlog (`roadmap.md` §4), none of the permanent non-goals (`mission.md` §8).
7. `/bnvt-review` is clean and `/bnvt-ticket` can derive tracer-bullet tickets
   from `plan.md`.
8. Per the workflow, the spec files are written on the phase branch but **not
   committed** in the spec step; committing is the `/bnvt-commit` stage.
