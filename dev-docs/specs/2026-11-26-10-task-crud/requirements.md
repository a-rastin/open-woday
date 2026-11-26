# Phase 5 — task-crud — Requirements

> **Spec:** `dev-docs/specs/2026-11-26-10-task-crud/`
> **Branch:** `phase-5-task-crud` · **Phase:** 5 (task-crud) · **Status:** planned
> **Authority:** `dev-docs/mission.md`, `dev-docs/tech-stack.md`,
> `dev-docs/roadmap.md`, `GLOSSARY.md`.
> **Produced by:** the `/to-spec` skill, adapted to the repo's three-file spec
> layout (`requirements.md` / `plan.md` / `validation.md`). No interview was
> conducted; this is a synthesis of the settled constitution and the current repo
> state, and it is a **proposal** awaiting maintainer confirmation (see the
> "Seam decision — please confirm" section of `validation.md`). Implementation
> decisions are stated in durable terms (no code snippets and no file paths, per
> the `/to-spec` skill); concrete file targets and commands live in `plan.md`,
> and the checks that prove success live in `validation.md`. No schedule or effort
> estimates are given (D7, D9); order is by dependency.

## Problem Statement

From the Target user's and contributor's perspective: **open-woday can now show
Today with quick task creation, check-off and an editable Day note — but Tasks
cannot yet live their full lifecycle.** Phase 4 added the Today page (`TodayPage`,
`DayStrip`, `TodayModel`) with minimal Task creation for the selected date,
basic check-off and a plain-text Day note. A Target user who opens the planner
still cannot rename a Task, edit or delete it, set a due time or reschedule it,
assign a priority, attach Tags or file it into a Folder, set a planned duration,
break a Task down into nested subtasks, or inspect a Task's details and own Note
pane in the right rail. Today rows do not yet show priority-coloured checkboxes,
detail badge chips or subtask progress, and the right rail remains an empty
panel. Every later v0.1 increment — the 5-type block editor, to-do→Task linking,
the Inbox rail, Quick Add parsing and CSV export — depends on a complete Task
lifecycle and Task detail pane. Before the planner can manage real work, a user
must be able to create, schedule, tag, file, nest, edit, complete, reopen and
soft-delete Tasks, with every change reflected on Today and persisted across
restarts.

## Solution

Ship **the full Task lifecycle (`task-crud`)** as the next runnable, verifiable
increment. Extend the application model and declarative QML surface so a Target
user can create, rename inline, edit, complete, reopen and soft-delete Tasks
(with soft-delete cascading to subtasks); schedule a Task with a due date (day
plus optional 24-hour time, or cleared); assign priority (`none`, `low`,
`medium`, `high`) which colours the checkbox; attach and remove multiple Tags
and file into an optional Folder (creating Tags/Folders when needed, backed by
the existing `tags`, `item_tags` and `folders` tables); record an inline
planned duration; and nest subtasks via `parent_id` with visible subtask rows
and completion progress. Clicking a Task row opens a `TaskDetailPane` inside the
existing right rail showing the Task's title, checkbox, a badge row containing
only the details that are set, a `"+"` badge for adding unset details (and
Tags), and a scaffold for the Task's own Note pane persisted as plain-text
Blocks. Upgrade Today's Task rows to use the shared component vocabulary
(`TaskRow`, `Checkbox`, `Badge`, `TagChip`, `IconButton`), keeping all strings
and assets original and preserving the Phase 3 shell and native OS chrome. The
increment is proven through a single high-level seam: build and launch, create,
schedule, tag, file, nest, rename, complete/reopen and delete Tasks, confirm
Today reflects every change, restart against the same SQLite file and observe
everything unchanged — executed locally with the repository's `qmake6` / `make`
verification commands.

## Context

- **Where this phase comes from.** `roadmap.md` §1 defines *the next phase* as the
  lowest-numbered phase whose status is not `done`, read together with the
  roadmap's own rule ("update the status as work lands"; one spec → one branch
  → one mergeable increment). Phases 1 (`project-skeleton`), 2 (`data-layer`),
  3 (`app-shell`) and 4 (`today-page`) have all landed: each was specified,
  ticketed, implemented, reviewed (`.code-review/`), documented (`CHANGELOG.md`
  §Unreleased) and merged to `main` (PR #1, PR #4, PR #7 and PR #10), and each
  has its own spec directory under `dev-docs/specs/`. The `Status:` fields in
  `roadmap.md` were **not** updated after Phase 2 (`in-progress`), Phase 3
  (`planned`) or Phase 4 (`planned`), because the `/bnvt-documentation` step
  maintains `README.md` and `CHANGELOG.md`. Following the precedent set by the
  Phase 2, Phase 3 and Phase 4 specs, the next increment on intent is therefore
  **Phase 5 — task-crud**, `Branch:` `phase-5-task-crud`. **This is a proposal,
  not maintainer sign-off:** if the maintainer intends a strict status-field
  reading (advance Phases 2–4 to `done` first, or re-target), say so before
  `/bnvt-ticket`; otherwise this reading stands.
- **Current codebase state.** A `qmake6` Qt 6 Quick / QML + C++ app exists:
  - Phase 1's entry point (`src/main.cpp`) and root `Window` (`MainWindow.qml`).
  - Phase 2's store (`src/store/`): `Database`, `StoreHelpers`, `ItemRepository`,
    `BlockRepository` and `schema.sql`, plus `tests/verify-persistence.cpp`. The
    `items` table already stores `title`, `completed_at`, `deleted_at`, `due_at`,
    `due_precision`, `priority`, `duration_min`, `parent_id`, `folder_id` and
    `sort_order`; the `folders`, `tags`, `item_tags` and `blocks` tables already
    exist. `ItemRepository` already implements `readTasksDueOn`, `readByFolder`,
    `readByParent` and `readByTag`, and `softDelete` explicitly defers subtask
    cascading to a later phase. There is not yet any repository API for CRUD on
    `folders`, `tags` or `item_tags`.
  - Phase 3's three-panel shell (`Theme.qml`, `Sidebar.qml`, `TopBar.qml`,
    `CentrePanel.qml`, `RightRail.qml`) with resize fold thresholds and
    platform-correct `Ctrl`/`⌘` modifier labels; `RightRail` is still an empty
    white rounded rectangle.
  - Phase 4's Today page (`TodayModel`, `TodayPage.qml`, `DayStrip.qml`):
    renders a 10-day strip, summary line, minimal inline Task creation (`addTask`
    with `due_precision='day'`), Task check-off (`setTaskCompleted`), Event
    titles, and a plain-text Day note persisted via `BlockRepository`. Task
    lines in `TodayPage.qml` are plain `CheckBox` delegates without badge chips,
    priority checkbox colours, inline rename, subtask display or detail-pane
    opening, and there is no `ItemModel`, `TaskRow`, `TaskDetailPane` or
    `components/` directory yet.
- **Toolchain (factual, `tech-stack.md` §4).** `qmake6`, Qt 6.12 (`Qt6Quick`,
  `Qt6Widgets`, `Qt6Core`), the SQL/SQLite driver and `g++` are present; `cmake`
  is **not** installed. The build stays `qmake6 open-woday.pro && make`.
- **Test-execution rule (`AGENTS.md`; `tech-stack.md` §12).** Every run of the
  verification suite happens on the developer's machine. From `tests/`, run
  `qmake6 verify-persistence.pro && make -j"$(nproc)" && ./verify-persistence`.
  The build and executable must both exit zero; no remote runtime, Google
  account or GPU is required.
- **Spec authority (`mission.md` §6, ADR-0004).** The three in-app `screenshots/`
  are the visual source of truth; `dev-docs/app-document.md` (the 134 guides) is
  the behavioural source for Task lifecycle, the Task detail pane, subtasks,
  due dates, priority and duration; `dev-docs/DESIGN.md` is the marketing site's
  reference, **not** this app's UI spec. Contradictions are resolved by the
  constitution. None of these are copied verbatim into the shipped product.
- **The three-file workflow.** `/bnvt-spec` (this step) records specs as files
  under `dev-docs/specs/` on branch `phase-5-task-crud`; `/bnvt-ticket` then
  breaks `plan.md` into tickets, followed by implement → review → revise →
  documentation → commit → merge. Specs are written but **not committed** in
  this step.
- **Vocabulary.** All text uses `GLOSSARY.md` terms (Item, Task, Note, Block,
  Event, Tag, Folder, Inbox, Day note; Upstream app, open-woday, Target user,
  Reference UI). Avoid: "to-do" for a Task, "document/page/doc" for a Note,
  "appointment/meeting" for an Event, "daily note" for a Day note,
  "entry/record/object/card" for an Item, "node/paragraph/chunk" for a Block,
  "label/category/keyword" for a Tag, "project/notebook/collection" for a
  Folder, and the upstream's "Woday"/"Wodie"/mascot names.

### Constitutional constraints governing this phase

| Constraint | Source |
|---|---|
| Local-first, single device: no server, no account, no sync, **no runtime network access**; data survives restarts in one local SQLite file | D3, D21, ADR-0002 (`mission.md` §3.1; `tech-stack.md` §7.1 / §7.5) |
| One codebase, **no webview** (no Electron/Tauri/HTML-rendered shell); Qt Quick's scene graph is not a web engine | D11, D13, ADR-0002 (`tech-stack.md` §3) |
| Build with `qmake6` + `make`; **no CMake**; Qt 6 Quick, Widgets and SQL modules | D13, `tech-stack.md` §5.3 / §11 |
| Unified domain model: one `Item` with `kind='task'`, `parent_id`, `folder_id`, `due_at`, `due_precision`, `priority`, `duration_min`; no separate Task/DayNote/View/Habit table | D17, D19 (`tech-stack.md` §6.2 / §7.2 / §10.2) |
| Deleting a Task soft-deletes it (`deleted_at`) and cascades soft-delete to its subtasks; no Trash UI; no undo stack requirement in v0.1 | `tech-stack.md` §6.4 / §9.8 / §10.3 |
| v0.1 keeps no Waiting state; completing a Task leaves no active In Progress state | `tech-stack.md` §6.4 |
| Clicking a Task row opens its detail pane in the right rail; detail pane shows title, checkbox, badge row (only set details), `"+"` badge for unset details, and Task's own Note scaffold | `tech-stack.md` §§9.1, 9.3 |
| Exactly 5 block types + inline formatting belong to Phase 6; `blocks.task_id` to-do→Task linking belongs to Phase 7 | `tech-stack.md` §§9.4, 7.3 (D19) |
| Original strings/assets only; never bundle upstream logos, mascot, UI strings, the guide scrape or the screenshots | ADR-0001, ADR-0004, `mission.md` §5 / §9 |
| Only populated pages — no empty shells / disabled placeholders | D5, D12 |
| Native OS chrome and platform-correct modifiers; inner UI identical across platforms | D15 (`tech-stack.md` §8.5) |
| Every phase is one runnable / verifiable increment | `roadmap.md` §1 |
| Verification suite runs locally with `qmake6` / `make` | `AGENTS.md`; `tech-stack.md` §12 |
| No schedule or effort estimates; order by dependency | D7, D9 |
| Use `GLOSSARY.md` vocabulary; do not re-litigate settled decisions or absorbed ADRs | `mission.md` §3 (principle 9), §13 |

## Scope

**In scope (from `roadmap.md` Phase 5 deliverables & `tech-stack.md` §§6.4, 9.1,
9.3):**
- Full Task CRUD lifecycle: create, inline rename, edit, complete, reopen, and
  soft-delete (cascading soft-delete to all descendant subtasks via `parent_id`).
- Task attributes and scheduling: due date (calendar day plus optional 24-hour
  time, or cleared to undated), priority (`none`, `low`, `medium`, `high`),
  Tags (many-to-many via `tags` and `item_tags`, including creating/selecting/removing
  Tags), Folder (optional single Folder via `folder_id`, including
  creating/selecting/clearing a Folder), inline planned duration (`duration_min`),
  and nested subtasks (`parent_id`, arbitrary depth supported in the store and
  rendered/managed in the UI).
- Store and C++ model layer: Folder and Tag persistence (reusing the existing
  `folders`, `tags` and `item_tags` tables with **no schema change or new
  migration**), cascading subtask soft-delete, and a C++ `ItemModel` mediating
  between QML and the store, synchronized with `TodayModel` so every Task edit
  is immediately reflected on Today.
- Task detail pane in the right rail: clicking a Task row opens its detail pane
  in the right rail (with a close action to return the rail to its idle state),
  displaying editable title, priority-coloured checkbox, a badge row showing
  **only set details**, a `"+"` badge revealing unset details (plus adding Tags),
  subtask management, soft-delete action, and a **scaffold for each Task's own
  Note pane** (plain-text `blocks` persisted under the Task's own `id` via
  `BlockRepository`, analogous to Phase 4's plain-text Day note).
- Upgraded Today Task rows (`TaskRow`) and reusable UI primitives (`Badge`,
  `TagChip`, `Checkbox`, `IconButton`): priority-coloured checkbox, inline
  rename, badge chips for set details (due time, priority, duration, Folder,
  Tag chips, subtask `done/total` count), subtask expansion/indentation and
  inline subtask creation, and click-to-open in the right rail.

**Verifiable when:** *"a Task can be created, scheduled, tagged, nested and
completed, and is reflected on Today."*

Everything else is out of scope (below). In particular, Phase 5 does **not**
build the 5-type block editor / inline formatting / Markdown shortcuts (Phase 6),
to-do→Task linking via `blocks.task_id` or embedding the full block editor into
Task/Event notes (Phase 7), the Inbox rail / Sidebar Folders & Tags lists /
drag-to-file (Phase 8), Quick Add parsing / `Ctrl+K` / `Ctrl+N` (Phase 9),
CSV export (Phase 10), visual tuning (Phase 11) or release packaging (Phase 12).

## User Stories

1. As a Target user, I want to create a Task on Today or from a clear New Task action, so that I can capture work directly in the planner.
2. As a Target user, I want to rename a Task inline on its row or in the detail pane, so that I can refine a Task title without recreating it.
3. As a Target user, I want to complete and reopen any Task or subtask, so that I can track progress and undo accidental check-offs.
4. As a Target user, I want to soft-delete a Task from its detail pane or row action, so that unwanted Tasks disappear from Today immediately.
5. As a Target user, I want deleting a parent Task to also soft-delete all of its nested subtasks, so that orphaned subtasks never remain behind.
6. As a Target user, I want to click any Task row on Today to open its detail pane in the right rail, so that I can inspect and edit its properties without leaving the page.
7. As a Target user, I want to close the Task detail pane when I am done editing, so that the right rail returns cleanly to its default state.
8. As a Target user, I want the Task detail pane's badge row to show only the details currently set on that Task, so that the pane stays uncluttered.
9. As a Target user, I want a `"+"` badge at the end of the detail pane's badge row, so that I can add any unset detail (due date, priority, duration, Folder) or attach Tags from one place.
10. As a Target user, I want to set, change or clear a Task's due date (calendar day), so that I can schedule work onto any day or return it to undated.
11. As a Target user, I want to add, edit or clear an optional 24-hour due time on a scheduled Task, so that time-sensitive Tasks show when they are due.
12. As a Target user, I want Today to update immediately whenever I change a Task's due date or time, so that moving a Task to another day removes it from the current day's list and places it on the target day.
13. As a Target user, I want to set a Task's priority to None, Low, Medium or High, so that urgent work stands out.
14. As a Target user, I want priority to colour the Task's checkbox on Today and in the detail pane, so that priority is scannable at a glance.
15. As a Target user, I want a priority badge displayed when priority is above None, so that I can also change or clear priority directly from the badge.
16. As a Target user, I want to attach one or more Tags to a Task, create a new Tag when needed, and detach Tags from a Task, so that I can categorize work across Folders.
17. As a Target user, I want Tag chips to appear on Today's Task rows and in the detail pane, so that a Task's Tags are visible without opening an extra menu.
18. As a Target user, I want to file a Task into a single optional Folder (creating a Folder if needed) or clear its Folder, so that Tasks can be organized by area.
19. As a Target user, I want to set, edit or clear an inline planned duration in minutes on a Task, so that I can estimate how long a Task will take.
20. As a Target user, I want to create subtasks under a Task (and nest subtasks under subtasks), so that I can break complex work into smaller steps.
21. As a Target user, I want to see subtask progress (done/total) on a parent Task row and expand or collapse its subtasks inline on Today, so that nested work stays manageable.
22. As a Target user, I want to manage subtasks inside the Task detail pane as well, so that I can add, rename, complete, reopen and delete subtasks while focusing on the parent Task.
23. As a Target user, I want each Task to have a scaffold for its own Note pane inside the detail pane, so that I can write plain-text context for a Task before the full block editor arrives.
24. As a Target user, I want every Task attribute, Tag association, Folder assignment, subtask relationship and Task Note edit to persist in the local SQLite database across restarts, so that no work is lost.
25. As a Target user, I want all of this to operate offline with zero network access, so that the local-first constraint (D3) holds.
26. As a Target user, I want the Phase 3 shell, resize fold order (Sidebar then RightRail), native window chrome and platform-correct `Ctrl`/`⌘` labels preserved, so that window behaviour stays consistent.
27. As a reviewer, I want all strings, placeholders, badge labels and icons written fresh for open-woday, so that no upstream UI string or asset is reused.
28. As a reviewer, I want no schema rewrite or new SQL migration, so that the Phase 2 seven-table schema (`items`, `blocks`, `folders`, `tags`, `item_tags`, `links`, `meta`) continues to serve unmodified.
29. As a contributor, I want a clean C++ `ItemModel` controller/model mediating Task CRUD and Folder/Tag operations between declarative QML and the store, so that QML contains no SQL or business logic.
30. As a reviewer, I want no 5-type block editor, no `blocks.task_id` to-do→Task linking, no Inbox rail grouping, no Sidebar Folder/Tag tree navigation, no Quick Add parser, no CSV export, no Waiting/In Progress states and no undo stack in this phase, so that Phases 6–12 own their designated work cleanly.
31. As a reviewer, I want canonical `GLOSSARY.md` vocabulary in all identifiers, UI copy and comments, so that domain terms stay consistent.
32. As a reviewer, I want no schedule or effort estimates anywhere in the spec, so that D7 / D9 are respected.
33. As a maintainer, I want the spec recorded as uncommitted files on `phase-5-task-crud` as a proposal awaiting confirmation, so that maintainer sign-off and commit remain separate steps.

## Implementation Decisions

Stated in durable terms; concrete file/command targets are in `plan.md` and
`validation.md`. No prototype was produced, so no snippet is inlined.

- **Layering and model ownership.** Introduce a C++ `ItemModel` controller/model
  mediating active Task selection, full Task CRUD, subtask management, Folder/Tag
  assignment and creation, and the selected Task's own plain-text Note pane,
  leaving declarative QML free of business logic and persistence code. Keep
  `TodayModel` synchronized whenever `ItemModel` mutates, schedules, retags,
  files, completes or deletes a Task so Today's lines and summary counts stay
  accurate immediately without requiring a date switch or app restart.
- **Store reuse without schema changes.** Reuse the existing seven-table schema
  (`items`, `blocks`, `folders`, `tags`, `item_tags`, `links`, `meta`) with **no
  DDL changes and no new migration**. Add store-layer repository operations for
  `folders`, `tags` and `item_tags` (create/read/list Folders and Tags, attach
  and detach Tags on an Item, read Tags for an Item), following the existing
  bound-value query and error-reporting patterns.
- **Task lifecycle & cascading soft-delete.** Create, rename, edit, complete,
  reopen and soft-delete Tasks directly against SQLite (`modified_at`,
  `completed_at`, `deleted_at`) with no in-memory-only buffer and no undo stack.
  Soft-deleting a Task sets `deleted_at` atomically on the Task and all of its
  descendant subtasks (via `parent_id`), ensuring partial failures do not leave
  orphaned active subtasks. Keep v0.1's simplified state model: no Waiting state,
  no In Progress state, and no Trash UI.
- **Due date, time, priority and duration.** Support due date as an ISO 8601
  calendar day (`due_precision='day'`) or day plus 24-hour time
  (`due_precision='time'`), or clearing both to `null` (making the Task undated).
  Week/month/Someday precision UI and reminders are deferred. Support the four
  integer priority levels (0 none, 1 low, 2 medium, 3 high), colouring the
  checkbox across rows and detail pane and showing a priority badge when
  non-zero. Support inline planned duration as a positive integer number of
  minutes (`duration_min`), editable and clearable.
- **Subtasks (`parent_id`).** Any Task may parent subtasks via `parent_id`,
  supporting arbitrary nesting depth in the store and clear nested display on
  Today and in the detail pane. Today shows top-level due Tasks with a
  `done/total` subtask progress indicator and inline expand/collapse of subtasks
  plus inline subtask creation. Subtasks are not duplicated as top-level root
  lines when shown nested under a due parent, while top-level Today task counts
  remain accurate and consistent.
- **Detail pane in the right rail & Task Note scaffold.** Selecting a Task row
  opens the `TaskDetailPane` inside the existing right rail; closing it returns
  the rail to its idle state (ready for Phase 8's Inbox rail). The detail pane
  shows the editable title, priority-coloured checkbox, a badge row rendering
  **only set details** (due date/time, priority, duration, Folder, Tags), a `"+"`
  badge offering unset details plus Tag selection/creation, a subtask section, a
  soft-delete action, and a **scaffold for the Task's own Note pane** below the
  badges. The Note scaffold reads and saves plain-text `blocks` rows on the
  Task's `item_id` using `BlockRepository`, preserving existing formatted-block
  safety guards until Phase 6 supplies the 5-type editor and Phase 7 wires
  to-do→Task linking.
- **Reusable QML components & Today Task rows.** Extract reusable declarative
  primitives (`Checkbox`, `Badge`, `TagChip`, `IconButton`) and `TaskRow` using
  the single `Theme` token object (`Theme.qml`). Upgrade Today's Task lines to
  render priority-coloured checkboxes, inline rename, set detail badges (due
  time, duration, Tag chips, subtask progress) and row click to open the detail
  pane.
- **Strings, assets and platform behaviour.** All labels, placeholders, empty
  states and icons are original to open-woday. Native OS window decorations,
  `Ctrl`/`⌘` modifier labels and the Sidebar-then-RightRail fold order remain
  unchanged.
- **Constraints and testing.** No webview, no runtime network access, no plugin
  or scripting engine, no CMake, no test framework or low-level unit seams.
  Verification continues through the single high-level local seam (build,
  launch, full Task CRUD lifecycle across Today and detail pane, restart against
  the same SQLite file, verify persistence) executed locally with `qmake6` and
  `make`.

## Out of Scope

- **Phases 6–12:** the 5-type QML block editor, inline bold/italic/link,
  `inline_json` formatting ranges, Return/Backspace/Tab, six-dot handle and
  Markdown shortcuts (Phase 6); `blocks.task_id` to-do→Task linking and
  embedding the full block editor into Task/Event notes (Phase 7); the Inbox
  rail grouping (Overdue/Inbox/Today), Sidebar Folders/Tags lists, Folder/Tag
  pages and drag-to-file (Phase 8); Quick Add parsing and `Ctrl+K`/`Ctrl+N`
  global shortcuts (Phase 9); CSV export (Phase 10); visual tuning against
  screenshots (Phase 11); Linux AppImage/`.deb` packaging and full documentation
  (Phase 12).
- **Task features deferred beyond v0.1 or later phases:** week/month/Someday
  scheduling pickers, recurring habits, task reminders/notifications, In Progress
  and Waiting states, completion sound, "done today" progress ring, Trash UI,
  permanent deletion, undo stack, attachments/location/linked-event/linked-note
  badges, and drag-and-drop task reordering/re-parenting.
- **Deferred backlog (`roadmap.md` §4):** Calendar page, Upcoming page, Saved
  Views, Settings, Analytics, AI assistant, local MCP server, third-party
  integrations, full CSV import, search / command palette, standalone list pages,
  Windows/macOS binaries.
- **Permanent non-goals (`mission.md` §8, ADR-0003):** Apple-ecosystem
  integrations, macOS-only shell features, iOS app.
- Any schema change, new SQL migration, webview, runtime network call, CMake
  file, test framework or CI configuration.
- Feature-driven edits to `README.md`, `CHANGELOG.md`, `CONTRIBUTING.md` or
  `roadmap.md` during this spec step (`/bnvt-documentation` owns doc updates).

## Further Notes

- This is the **fifth** mergeable increment of v0.1 and directly advances
  `mission.md` §10 item 4 (*"Can create, edit, tag, schedule, nest and complete
  Tasks; each Task has its own Note pane"*). Its merge gate is the Phase 5
  "Verifiable when" condition.
- Note that `TodayModel`'s existing plain-text Day-note guard (`isPlainText`)
  should be respected in the Task Note pane scaffold so that if a Task Note
  ever holds non-plain-text Blocks (or later receives them in Phases 6–7), plain
  text editing does not silently strip formatted Block metadata.
- **Approval status.** This spec is a synthesis, not an interview, and carries no
  maintainer sign-off. The seam decision and the next-phase reading are open
  questions for the maintainer to confirm before `/bnvt-ticket` (see
  `validation.md` §"Seam decision — please confirm").
- **No issue-tracker configuration was provided to the `/to-spec` skill**, so per
  the repo's established layout the spec is published as the three local files
  in `dev-docs/specs/2026-11-26-10-task-crud/` rather than to an external
  tracker; no `ready-for-agent` label is applied.
- `/bnvt-ticket` breaks `plan.md` into tracer-bullet vertical slices; the spec
  is written to local files on `phase-5-task-crud` and is **not committed** in
  this step.
