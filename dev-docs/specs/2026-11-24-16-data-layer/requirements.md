# Phase 2 — data-layer — Requirements

> **Spec:** `dev-docs/specs/2026-11-24-16-data-layer/`
> **Branch:** `phase-2-data-layer` · **Phase:** 2 (data-layer) · **Status:** planned
> **Authority:** `dev-docs/mission.md`, `dev-docs/tech-stack.md`, `dev-docs/roadmap.md`, `GLOSSARY.md`.
> **Produced by:** the `/to-spec` skill, adapted to the repo's three-file spec layout
> (`requirements.md` / `plan.md` / `validation.md`). No interview was conducted;
> this is a synthesis of the settled constitution and the current repo state.
>
> Implementation decisions below are stated in durable terms (no code snippets and no
> file paths, per the `/to-spec` skill). Concrete file targets and commands live in
> `plan.md`; the checks that prove success live in `validation.md`. No schedule or
> effort estimates are given (D7, D9); order is by dependency.

## Problem Statement

From the Target user's and contributor's perspective: **open-woday can be launched
but nothing survives a restart.** Phase 1 delivered a buildable Qt 6 Quick
skeleton that opens one empty window, yet the app has no local persistence at all
— there is no SQLite file, no schema, no way to store an Item, a Task, a Note, an
Event, a Tag, a Folder or a Block, and no way to read any of it back. A Target
user who cannot run the macOS-only upstream app can open the planner but cannot
keep a single piece of planning data across sessions, which makes the product
useless as a day planner. Every later v0.1 increment — the three-panel shell, the
Today page and its Day note, Task CRUD, the block editor, the Inbox rail, Quick
Add and CSV export — needs a durable, local foundation to read from and write to.
Before any product surface can be shown, the app must be able to create its
database on first launch, keep it across restarts, and let the code above it
create, read, update and soft-delete the core domain Items.

## Solution

Ship the **local persistence foundation** as the next runnable, verifiable
increment: a single local SQLite file (via Qt SQL) whose full v0.1 schema is
created on first launch, governed by simple versioned migrations recorded in a
`meta` schema-version row, and exposed to C++ through two repositories — one for
Items and one for Blocks — that can create, read, update and soft-delete
Items and Blocks. All writes go straight to SQLite; there is no network, no
account, no cloud, no sync and no in-memory-only state. The increment is proven
through a single, high-level seam: write data, close the app (or the connection),
reopen it, and read the same data back unchanged. Deliberately, no product UI is
added: the window stays empty, the layers stay clean, and no downstream phase's
work is pulled forward. This is the first phase where the app becomes capable of
*remembering* anything, which is what the local-first principle (D3, D21) requires.

## Context

- **Where this phase comes from.** `roadmap.md` §1 defines *the next phase* as the
  lowest-numbered phase whose status is not `done`, and states that status should
  be updated "as work lands". Phase 1 — project-skeleton has demonstrably landed:
  it was implemented, reviewed (`.code-review/2026-11-24-14-project-skeleton.md`),
  documented (`CHANGELOG.md` §Unreleased), given its own spec
  (`dev-docs/specs/2026-11-24-14-project-skeleton/`) and merged to `main`
  (PR #1); the runnable skeleton is present in the working tree. The `Status:`
  fields in `roadmap.md` were **not** advanced (the `/bnvt-documentation` step
  maintains only `README.md` and `CHANGELOG.md`), so a purely mechanical reading
  of those stale fields would still name Phase 1. Per the roadmap's own intent
  ("update the status as work lands"; one spec → one branch → one mergeable
  increment), the next increment is therefore **Phase 2 — data-layer**, whose
  `Branch:` line is `phase-2-data-layer` and whose goal is "the local persistence
  foundation". This spec proceeds on that basis; if the maintainer intends a
  strict status-field reading instead, Phase 1 can be re-targeted before
  `/bnvt-ticket`.
- **Current codebase state.** A `qmake6` Qt 6 Quick / QML + C++ skeleton exists
  (one C++ entry point, one empty QML window, Qt resource embedding). There is no
  store layer, no schema and no tests (`tech-stack.md` §12: greenfield; the Phase
  1 spec noted "the first natural unit seam appears in Phase 2, when the
  repositories gain CRUD behaviour worth testing"). No application reads or
  writes data yet.
- **Toolchain (factual, `tech-stack.md` §4).** `qmake6`, Qt 6.12 (`Qt6Quick`,
  `Qt6Widgets`, `Qt6Core`), the SQL/SQLite driver and `g++` are present; `cmake`
  is **not** installed. The `sql` module was declared up front in Phase 1
  specifically so this phase can add persistence without reworking the build.
- **The three-file workflow.** `/bnvt-spec` (this step) records specs as files
  under `dev-docs/specs/`; `/bnvt-ticket` then breaks `plan.md` into tickets,
  followed by implement → review → revise → documentation → commit → merge.
  Specs are written but **not committed** in this step.
- **Vocabulary.** All text uses `GLOSSARY.md` terms (Item, Task, Note, Block,
  Event, Tag, Folder, Inbox, Day note; upstream app, open-woday, Target user,
  Reference UI). Avoid: "to-do" for a Task, "document/page/doc" for a Note,
  "appointment/meeting" for an Event, "daily note" for a Day note,
  "record/object/card" for an Item, "node/paragraph/chunk" for a Block, and the
  upstream's "Woday"/"Wodie"/mascot names. A "to-do Block" is a Block type and is
  fine; it is not a synonym for a Task.

### Constitutional constraints governing this phase

| Constraint | Source |
|---|---|
| Local-first, single device: no server, no account, no sync, **no runtime network access**; data survives restarts in one local SQLite file | D3, D21, ADR-0002 (`mission.md` §3.1; `tech-stack.md` §7.1 / §7.5) |
| One codebase, **no webview** (no Electron/Tauri/HTML-rendered shell); Qt Quick's scene graph is not a web engine | D11, D13, ADR-0002 (`tech-stack.md` §3) |
| Build with `qmake6` + `make`; **no CMake**; Qt 6 Quick, Widgets and SQL modules | D13, `tech-stack.md` §5.3 / §11 |
| Unified domain model: one `Item` with a `kind` discriminator plus lightweight first-class `Event`; no separate DayNote/View/Habit entity | D17, D19 (`tech-stack.md` §6.2 / §10.2) |
| Layered architecture: QML views declarative (no business logic), C++ app/ controllers & models, store/ for persistence; no network layer, plugin system or scripting engine | `tech-stack.md` §5.1 |
| Soft-delete via `deleted_at`; no Trash UI; all writes straight to SQLite; no undo-stack requirement in v0.1 | `tech-stack.md` §9.8 / §10.3 |
| Blocks live in their own table (not a JSON blob) so a to-do Block can carry a `task_id`; `inline_json` holds formatting ranges | `tech-stack.md` §7.3 |
| Original strings/assets only; never bundle upstream logos, mascot, UI strings, the guide scrape or the screenshots | ADR-0001, ADR-0004, `mission.md` §5 / §9 |
| Only populated pages — no empty shells / disabled placeholders; Phase 2 renders no product surface | D5, D12 |
| Native OS chrome and platform-correct modifiers; inner UI identical across platforms | D15 |
| No schedule or effort estimates; order by dependency | D7, D9 |
| Use `GLOSSARY.md` vocabulary; do not re-litigate settled decisions or absorbed ADRs | `mission.md` §3 (principle 9), §13 |

## Scope

**In scope (from `roadmap.md` Phase 2 deliverables):** `Database` (schema
creation, versioned migrations); the **full schema** — `items`, `blocks`,
`folders`, `tags`, `item_tags`, `links`, `meta` — exactly as specified in
`tech-stack.md` §7.2; `ItemRepository` and `BlockRepository` with create / read /
update / soft-delete of Items and Blocks; and a `meta` **schema version**.
**Verifiable when:** *"the schema is created on first launch, repositories can
CRUD Items/Blocks, and data survives an app restart."*

Everything else is out of scope (below). In particular, Phase 2 does **not** draw
the three-panel shell (Phase 3), show the Today page or Day note (Phase 4), add
Task CRUD or a detail pane (Phase 5), build the block editor (Phase 6), link
to-do Blocks to real Tasks (Phase 7), add the Inbox rail / Folders / Tags UI
(Phase 8), parse Quick Add (Phase 9) or export CSV (Phase 10). Each phase is one
spec → one branch → one mergeable, runnable increment (`roadmap.md` §1); Phase 2
must remain verifiable on its own through the persistence seam without shipping a
non-demoable horizontal layer.

## User Stories

1. As a Target user, I want the app to create its local database automatically on first launch, so that I never have to run a setup step or manage files by hand.
2. As a Target user, I want my planning data to survive closing and reopening the app, so that the planner is useful across sessions.
3. As a Target user, I want all my data to stay in one local file on my own device, so that no account, cloud or server is ever involved.
4. As a Target user, I want no network access at any time, so that the local-first constraint (D3) is honoured before any feature exists.
5. As a Target user, I want the app to fail clearly if its database cannot be opened, so that I am never left with a silently half-working planner.
6. As a Target user, I want the existing empty window to keep opening, so that the Phase 1 launch behaviour is not regressed by the data layer.
7. As a contributor, I want the full v0.1 schema defined in one versioned SQL source, so that there is a single authority for the tables and constraints.
8. As a contributor, I want the schema to include `items`, `blocks`, `folders`, `tags`, `item_tags`, `links` and `meta`, so that every later phase has its tables ready.
9. As a contributor, I want one `Item` table with a `kind` discriminator for Task, Note and Event, so that the unified domain model (D17) is respected.
10. As a contributor, I want the `items` columns to cover scheduling, priority, duration, parent, folder, day and sorting, so that Phase 4–9 features map to existing fields rather than a later schema rewrite.
11. As a contributor, I want the `items` `kind` and `due_precision` values constrained by `CHECK`, so that invalid data cannot be persisted silently.
12. As a contributor, I want a partial unique index enforcing one active Day note per date (soft-deleted rows excluded), so that two active Day notes for the same day cannot exist while a soft-deleted note's date can be reused.
13. As a contributor, I want `blocks` to have a `position`, a five-type `CHECK`, `inline_json`, `task_id` and `indent`, so that the Phase 6 editor and Phase 7 to-do→Task links have the right storage shape.
14. As a contributor, I want an index on Blocks by owning Item and position, so that loading a Note's Blocks in order stays cheap.
15. As a contributor, I want `item_tags` to be a many-to-many table with a composite primary key, so that a Tag can cross Folders as the glossary requires.
16. As a contributor, I want `links` to model mentions and linked notes/events, so that later @-mention and linking behaviour has a home.
17. As a contributor, I want foreign keys enforced with cascade deletes where specified, so that deleting an Item does not leave orphan Blocks or tag rows.
18. As a contributor, I want migrations to be simple, versioned SQL applied in order, so that the schema can evolve without a framework or an external dependency.
19. As a contributor, I want the current schema version stored in `meta`, so that the app can tell which migrations have already been applied.
20. As a contributor, I want already-applied migrations to be skipped on later launches, so that opening the app never re-runs DDL destructively.
21. As a maintainer, I want no migration to drop or rebuild tables holding user data, so that upgrading never destroys a user's planning history.
22. As a contributor, I want an `ItemRepository` that can create, read, update and soft-delete Items, so that later features have one consistent persistence API.
23. As a contributor, I want Item CRUD to work for all three kinds — Task, Note and Event — so that no phase needs a second persistence path.
24. As a contributor, I want every Item column (timestamps, completion, scheduling, priority, duration, parent, folder, day, sort order) to round-trip through the repository, so that no field is lost between write and read.
25. As a contributor, I want reads by id, kind, day, folder, parent and tag, so that Today, Folders, Tags and the Inbox can be built later without reshaping the store.
26. As a contributor, I want deletion implemented as a soft-delete via `deleted_at`, so that no hard delete or Trash UI is introduced prematurely (`tech-stack.md` §10.3).
27. As a contributor, I want soft-deleted rows excluded from normal reads while remaining in the file, so that CSV export (Phase 10) can later honour the "soft-deleted rows excluded" rule.
28. As a contributor, I want a clear error when a duplicate Day note date is inserted, so that the one-note-per-day rule is enforceable rather than silent.
29. As a contributor, I want a `BlockRepository` that can create, read, update, delete and reorder Blocks, so that the Phase 6 editor can persist its Notes.
30. As a contributor, I want `inline_json`, `task_id` and `indent` to round-trip, so that inline formatting, to-do→Task links and indentation survive persistence.
31. As a contributor, I want Blocks kept ordered by `position` within their owning Item, so that a Note reads back in the order it was written.
32. As a contributor, I want Blocks stored as rows rather than a JSON blob, so that per-block editing and the to-do `task_id` foreign key stay simple (`tech-stack.md` §7.3).
33. As a maintainer, I want QML to stay declarative and the C++ entry point to stay free of business logic, so that the layered architecture (`tech-stack.md` §5.1) holds as the store grows.
34. As a maintainer, I want no network layer, plugin system, scripting engine or webview added, so that the settled constraints are not eroded by a persistence phase.
35. As a maintainer, I want all writes to go straight to SQLite with no in-memory-only state, so that restart survival is a property of the design, not an afterthought.
36. As a maintainer, I want bound values in queries, so that user text can never be interpreted as SQL.
37. As a reviewer, I want a single high-level persistence seam (write → restart → read back), so that external behaviour is tested rather than internal SQL details.
38. As a reviewer, I want no test framework or low-level unit seams introduced beyond that one seam, so that the ideal single-seam rule (`tech-stack.md` §12) is kept.
39. As a reviewer, I want the build to remain `qmake6 && make` with no CMake, no new warnings and the SQL module declaration reused, so that Phase 1's build baseline is not reworked.
40. As a reviewer, I want no three-panel shell, tokens, pages, Task CRUD UI, editor, Quick Add, Inbox rail or CSV export in this phase, so that Phases 3–10 own their work cleanly.
41. As a reviewer, I want no upstream strings, logos, mascot, screenshots or guide text bundled, so that the legal/IP boundary (ADR-0001 / ADR-0004) holds as code grows.
42. As a reviewer, I want canonical `GLOSSARY.md` vocabulary in identifiers and comments, so that later phases inherit consistent domain language.
43. As a reviewer, I want no schedule or effort estimates anywhere in the spec, so that D7 / D9 are respected.
44. As a maintainer, I want this spec recorded as uncommitted files on the phase branch, so that the commit step remains a separate, reviewable workflow stage.

## Implementation Decisions

Stated in durable terms; concrete file/command targets are in `plan.md` and
`validation.md`. No prototype was produced, so no snippet is inlined.

- **Engine and location.** Persistence uses SQLite through Qt SQL
  (`QSqlDatabase`), a single file in the platform app-data directory (e.g.
  `$XDG_DATA_HOME/open-woday/open-woday.db` on Linux). The file and its parent
  directory are created on first launch. There is no server, no account, no cloud,
  no sync, no conflict resolution and no runtime network access — "no cloud" is a
  constraint, not a deferred feature (D3, D21; `tech-stack.md` §7.1 / §7.5).
- **Startup failure behaviour.** The database is opened at startup. If it cannot
  be opened or created, the app fails loudly with a clear diagnostic and a
  non-zero exit code rather than continuing in a silently degraded state. This
  mirrors the Phase 1 guard for a failed QML load.
- **Schema authority.** The full v0.1 schema is defined once, in versioned SQL,
  exactly as specified in `tech-stack.md` §7.2: seven tables (`items`, `blocks`,
  `folders`, `tags`, `item_tags`, `links`, `meta`), their columns, `CHECK`
  constraints, foreign keys, cascade deletes, the composite primary keys, the
  `blocks_by_item` index and the partial unique index that enforces one active
  Day note per date (soft-deleted rows excluded, so their dates can be reused).
  C++ does not embed scattered DDL.
- **Unified domain model.** One `Item` table with a `kind` discriminator
  (`task`, `note`, `event`) carries scheduling, priority, duration, parent,
  folder, day and sort-order fields. There is no separate Task, Note, DayNote,
  View or Habit entity (D17; `tech-stack.md` §6.2 / §10.2). A Day note is a Note
  Item with a `day` value, unique per date (`tech-stack.md` §6.3 / §10.2).
- **Blocks as rows.** Blocks live in their own table with `position`, one of five
  types, plain UTF-8 `content`, `inline_json` formatting ranges, an optional
  `task_id` and `indent`. Rows (not a JSON blob) keep per-block editing and the
  future to-do→Task foreign key simple (`tech-stack.md` §7.3). Blocks cascade on
  Item deletion.
- **Migrations.** A minimal versioned-migration mechanism reads and writes a
  `schema_version` row in `meta`, applies migrations in ascending order on first
  launch and skips already-applied versions thereafter. No migration framework,
  no external dependency, no down-migrations and no destructive rebuild of
  user-data tables in v0.1.
- **Repositories.** `ItemRepository` and `BlockRepository` expose plain C++ CRUD
  (create, read, update, soft-delete; plus Block reorder) over their tables.
  Item deletion is a soft-delete via `deleted_at`; soft-deleted rows are excluded
  from normal reads but remain in the file, so the Phase 10 CSV export can honour
  "soft-deleted rows excluded". No hard delete and no Trash UI are exposed.
- **Query safety and hygiene.** All writes use bound values; foreign-key
  enforcement is enabled. No SQL is assembled by concatenating user text.
- **Layering.** QML views stay declarative and hold no business logic; the C++
  entry point stays free of product logic; the store layer is C++ only. There is
  no network layer, plugin system, scripting engine or webview, and no QML
  product surface is added in this phase.
- **Build.** The existing `qmake6` project is extended with the new store
  sources; `QT += quick widgets sql` and C++17 are reused, **no CMake** is added.
  The build and the empty-window launch behaviour of Phase 1 must remain intact.
- **Explicit phase boundary.** No three-panel shell, visual tokens, Today page,
  Day-note rendering, Task CRUD or detail pane, block editor, inline-formatting
  UI, to-do→Task linking, Inbox grouping, Quick Add parsing, CSV export,
  packaging or documentation edits are built here. Those belong to Phases 3–12.
  Phase 2 ships no non-demoable horizontal layer; it is proven through the
  persistence seam alone.
- **Testing.** The seam decision, what makes a good test and prior art are in
  `validation.md`: a single highest seam (create → restart → read back), which is
  the first unit-grade seam in the codebase; no lower seams or test framework yet.
- **Documentation.** README and CHANGELOG are maintained by the
  `/bnvt-documentation` step and are intentionally **not modified** in this
  phase; the full build/run/packaging docs are a Phase 12 deliverable.

## Out of Scope

- **Phases 3–12** in full: the three-panel shell and visual tokens (Phase 3); the
  Today page, Day strip and Day note (Phase 4); Task CRUD, the detail pane and
  nested subtasks (Phase 5); the 5-type block editor and inline formatting UI
  (Phase 6); to-do→Task links and the Task/Event Note panes (Phase 7); the Inbox
  rail, Folders/Tags sections and drag-to-file (Phase 8); Quick Add (Phase 9);
  CSV export (Phase 10); visual tuning (Phase 11); release packaging (Phase 12).
- **Behavioural rules, not persistence:** the Inbox "no date AND no folder" query
  as a UI grouping, completing a Task clearing In Progress state, cascading
  soft-delete to subtasks, priority colouring, tag chips on rows, the completion
  sound, the done-today progress ring and any undo stack.
- **The deferred backlog** (`roadmap.md` §4, no phase numbers): Calendar/Upcoming
  pages, Habits/streaks, saved Views, Settings, Analytics, AI assistant, local
  MCP server, third-party integrations, full CSV import, Trash UI, search /
  command palette, standalone list pages, the remaining 12 block types, symbol
  menus, live Quick Add preview, Magic Lines, multi-language parsing, Windows and
  macOS binaries.
- **Permanent non-goals** (`mission.md` §8, ADR-0003): Apple-ecosystem features,
  the macOS-only shell and the iOS app — never built on this architecture.
- Any populated page, placeholder destination, disabled UI element or fake
  content ("no empty shells", D5 / D12).
- Any webview, network call, plugin system, scripting engine, cloud/account/sync,
  or external migration framework.
- A full unit-test framework, CI configuration, low-level unit seams and
  packaging scripts beyond the single persistence seam.
- README / CONTRIBUTING / CHANGELOG / roadmap edits; a tagged release.
- Any schedule or effort estimate (D7 / D9).

## Further Notes

- This is the **second** mergeable increment of v0.1 and the first that makes the
  app capable of remembering anything. It does **not** by itself satisfy any full
  v0.1 Definition-of-Done item (`mission.md` §10 / `roadmap.md` §3); its merge
  gate is the Phase 2 "Verifiable when" condition. Later phases complete those
  items.
- The `sql` Qt module and the SQL/SQLite driver were declared in Phase 1
  specifically so this phase adds persistence without reworking the build; this
  is a documented standard (`tech-stack.md` §5.3), not speculative generality.
- No schedule or effort estimates appear anywhere in this spec (D7 / D9); order
  of work is by dependency.
- Screenshots are the visual source of truth and the 134 guides the behavioural
  source of truth (ADR-0004), but Phase 2 renders no product surface, so neither
  is consulted for content here; the empty window is a deliberate boundary.
- `/bnvt-ticket` breaks `plan.md` into tracer-bullet vertical slices; the spec is
  written to local files and is **not committed** in this step (committing is the
  `/bnvt-commit` stage).
- No issue-tracker configuration was provided to the `/to-spec` skill, so per the
  repo's established layout the spec is published as the three local files in
  `dev-docs/specs/` rather than to an external tracker; no `ready-for-agent`
  label is applied.
