# Phase 4 — today-page — Requirements

> **Spec:** `dev-docs/specs/2026-11-25-13-today-page/`
> **Branch:** `phase-4-today-page` · **Phase:** 4 (today-page) · **Status:** planned
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

From the Target user's and contributor's perspective: **open-woday now launches,
remembers data, and shows the three-panel frame — but the centre panel is still
empty.** Phase 1 delivered a buildable Qt 6 Quick skeleton that opens one empty
window; Phase 2 added the local persistence foundation (a SQLite file created on
first launch, with Item and Block repositories); Phase 3 added the `Sidebar`,
`TopBar`, `CentrePanel` and `RightRail` with one shared set of visual tokens.
None of them shows a single piece of planning content. A Target user who opens
the planner sees a dark sidebar and a clean white centre panel with no Today
page: no day strip, no summary line, no Task or Event lines, no badge chips, no
check-off and no editable Day note. The app can store Items and Blocks, but no
surface reads them into a day view. Every later v0.1 increment — Task CRUD, the
block editor, the Inbox rail, Quick Add and CSV export — assumes the Today page
exists and that a Day note is wired to a date. Before the planner can be used
for its purpose, it must render Today as an editable Day note whose lines and
free writing survive restarts, using only original strings and assets.

## Solution

Ship the **Today page as an editable Day note** as the next runnable, verifiable
increment. Compose a `TodayPage` into the existing centre panel: a `DayStrip`
of ≈10 selectable days, a summary line, the day's Task and Event lines with
badge chips (due time, priority, Tag), check-off, and free writing around the
lines. Wire the page to exactly one Day note Item — a Note Item with a `day`
value, unique per date — through the Phase 2 store, reusing the existing schema,
the `one_day_note_per_day` index and the `blocks` table; free writing persists
as ordered Blocks. Events are lightweight first-class Items rendered as lines,
with no calendar grid. All strings and assets are original and the page inherits
the Phase 3 token set, native chrome and platform-correct modifiers. The
increment is proven through a single high-level seam: launch, see Today with the
day's lines and Day note, write, restart, and see the same data rendered
unchanged — executed on a GPU-backed Google Colab runtime via
`scripts/colab-gpu-test.sh`.

## Context

- **Where this phase comes from.** `roadmap.md` §1 defines *the next phase* as the
  lowest-numbered phase whose status is not `done`, to be read with the roadmap's
  own intent ("update the status as work lands"; one spec → one branch → one
  mergeable increment). Phases 1 — project-skeleton, 2 — data-layer and 3 —
  app-shell have demonstrably landed: each was specified, ticketed, implemented,
  reviewed (`.code-review/`), documented (`CHANGELOG.md` §Unreleased) and merged
  to `main` (PR #1, PR #4 and PR #7), and each has its own spec directory under
  `dev-docs/specs/`. The `Status:` fields in `roadmap.md` were **not** advanced
  (the `/bnvt-documentation` step maintains only `README.md` and
  `CHANGELOG.md`), so a purely mechanical reading of those stale fields would
  still name Phase 2, whose `Status:` remains `in-progress` and whose spec, five
  tickets, review and merged code already exist. Re-speccing it would duplicate
  work. Following the precedent set by the Phase 2 and Phase 3 specs (their
  "Where this phase comes from" notes), the next increment on intent is
  therefore **Phase 4 — today-page**, `Branch:` `phase-4-today-page`. **This is
  a proposal, not maintainer sign-off:** if the maintainer intends a strict
  status-field reading (advance Phases 2–3 to `done` first, or re-target), say so
  before `/bnvt-ticket`; otherwise this reading stands.
- **Current codebase state.** A `qmake6` Qt 6 Quick / QML + C++ app exists:
  Phase 1's C++ entry point and root `Window`; Phase 3's `Theme.qml`,
  `Sidebar.qml`, `TopBar.qml`, `CentrePanel.qml` and `RightRail.qml` (a static
  three-panel frame with shared tokens and no live data); and Phase 2's
  `src/store/` layer (`Database`, `ItemRepository`, `BlockRepository`,
  `schema.sql`) with the framework-free `tests/verify-persistence.cpp` seam. The
  `items` table already carries `day`, `due_at`, `priority`, `start_at`, `end_at`,
  `all_day`, `completed_at`, `parent_id`, `folder_id` and `sort_order`; `blocks`
  already carries `position`, `type`, `content`, `inline_json`, `task_id` and
  `indent`; the `one_day_note_per_day` partial unique index already exists. There
  is no `TodayPage`, `DayStrip`, `TodayModel`, Task/Event line rendering, badge
  chips, check-off or editable Day note. README and CHANGELOG document all three
  landed phases.
- **Toolchain (factual, `tech-stack.md` §4).** `qmake6`, Qt 6.12 (`Qt6Quick`,
  `Qt6Widgets`, `Qt6Core`), the SQL/SQLite driver and `g++` are present; `cmake`
  is **not** installed. The build stays `qmake6 open-woday.pro && make`; the
  `sql` module was declared up front in Phase 1.
- **Test-execution rule (`AGENTS.md`; `tech-stack.md` §12).** Every run of the
  verification suite **must** be on a GPU from Google Colab using `colab-cli`.
  `scripts/colab-gpu-test.sh` is the single supported route (`colab new --gpu`
  → `colab upload` → `colab exec` → `colab stop`), with
  `tests/colab_gpu_verify.py` proving a usable NVIDIA GPU before building and
  running `verify-persistence`. A local, CPU-only or non-GPU run is **not** a
  valid verification. `colab-cli` authentication is a prerequisite; when a
  specific Google account is required, `COLAB_EXPECTED_EMAIL` blocks GPU
  allocation on a mismatch.
- **Spec authority (`mission.md` §6, ADR-0004).** The three in-app `screenshots/`
  are the visual source of truth; `dev-docs/app-document.md` (the 134 guides) is
  the behavioural source; `dev-docs/DESIGN.md` is the marketing site's reference,
  **not** this app's UI spec. Contradictions are resolved here. None of these
  are copied verbatim into the shipped product.
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
| A Day note is one `items` row with `day`, unique per date among active rows (soft-deleted rows excluded) | `tech-stack.md` §6.3 / §7.2 |
| Blocks live in their own table (not a JSON blob); exactly 5 block types are a Phase 6 deliverable | `tech-stack.md` §7.3 / §9.4 (D19) |
| Soft-delete via `deleted_at`; no Trash UI; all writes straight to SQLite; no undo-stack requirement in v0.1 | `tech-stack.md` §9.8 / §10.3 |
| Original strings/assets only; never bundle upstream logos, mascot, UI strings, the guide scrape or the screenshots | ADR-0001, ADR-0004, `mission.md` §5 / §9 |
| Only populated pages — no empty shells / disabled placeholders | D5, D12 |
| Native OS chrome and platform-correct modifiers; inner UI identical across platforms | D15 (`tech-stack.md` §8.5) |
| Every phase is one runnable / verifiable increment | `roadmap.md` §1 |
| Verification suite runs on a GPU from Google Colab via `colab-cli` | `AGENTS.md`; `tech-stack.md` §12 |
| No schedule or effort estimates; order by dependency | D7, D9 |
| Use `GLOSSARY.md` vocabulary; do not re-litigate settled decisions or absorbed ADRs | `mission.md` §3 (principle 9), §13 |

## Scope

**In scope (from `roadmap.md` Phase 4 deliverables):** `DayStrip`, a summary
line, Task/Event lines with badge chips, check-off, day-note wiring (one `items`
row with `day`, unique per date), free writing around the lines, and a
`TodayModel` connecting the page to the Phase 2 store — all rendered in the
Phase 3 centre panel with original strings and the shared tokens.
**Verifiable when:** *"Today renders a day's Task/Event lines and the editable
Day note persists across restarts."*

Everything else is out of scope (below). In particular, Phase 4 does **not**
build the full Task lifecycle / detail pane (Phase 5), the 5-type block editor /
inline formatting (Phase 6), to-do→Task linking / Task-Event Note panes (Phase
7), the Inbox rail / Folders / Tags UI (Phase 8), Quick Add parsing (Phase 9),
CSV export (Phase 10), visual tuning (Phase 11) or release packaging (Phase 12).
Each phase is one spec → one branch → one mergeable, runnable increment
(`roadmap.md` §1); Phase 4 must remain verifiable on its own without shipping a
non-demoable horizontal layer.

## User Stories

1. As a Target user, I want the centre panel to show a Today page instead of an empty surface, so that the planner is finally usable for day planning.
2. As a Target user, I want a day strip of roughly ten selectable days, so that I can move between dates.
3. As a Target user, I want today's cell accented in the day strip, so that I can see which day I am on.
4. As a Target user, I want a summary line with counts written in original open-woday strings, so that I get an at-a-glance view of the day.
5. As a Target user, I want the day's Task lines rendered from real Task Items, so that what I see reflects stored data.
6. As a Target user, I want the day's Event lines rendered from lightweight Event Items, so that events at a time appear on Today.
7. As a Target user, I want badge chips on lines for the details that are set (due time, priority, Tag), so that I can scan the day quickly.
8. As a Target user, I want priority to colour a Task line's checkbox, so that important work stands out.
9. As a Target user, I want a checkbox to check off a Task line, so that I can mark work done on Today.
10. As a Target user, I want check-off to survive closing and reopening the app, so that my progress is not lost.
11. As a Target user, I want the Day note to be editable with free writing around the lines, so that I can write notes for the day.
12. As a Target user, I want my free writing to persist across restarts, so that my Day note is durable.
13. As a Target user, I want exactly one Day note per date, so that my writing for a day is never split or duplicated.
14. As a Target user, I want a soft-deleted Day note's date to be reusable by a replacement, so that the one-note-per-day rule does not trap me.
15. As a Target user, I want all of this to stay in one local SQLite file with no network access, so that the local-first constraint (D3) holds.
16. As a Target user, I want the Phase 3 three-panel frame, fold order and native OS decorations preserved, so that the window still behaves like a normal Linux desktop app.
17. As a Target user, I want no drawn macOS traffic-light controls on Linux, so that the deliberate deviation (D15) holds.
18. As a Target user, I want `Ctrl`-based labels on Linux/Windows and `⌘`-based labels on macOS, so that modifier hints stay platform-correct.
19. As a reviewer, I want every new string, placeholder, empty state and icon written fresh for open-woday, so that no upstream UI string is reused.
20. As a reviewer, I want no upstream logo, wordmark, mascot, screenshot or guide text bundled or shipped, so that the legal/IP boundary (ADR-0001 / ADR-0004) holds as the UI grows.
21. As a reviewer, I want `DESIGN.md` treated as a marketing-site reference only, so that it is never mistaken for the app's UI spec.
22. As a reviewer, I want the page to inherit the single shared `Theme.qml` token set, so that later phases keep one coherent palette.
23. As a contributor, I want a C++ `TodayModel` mediating between declarative QML and the Phase 2 store, so that the layered architecture (`tech-stack.md` §5.1) holds.
24. As a contributor, I want the Day note to be a Note Item with a `day` value rather than a new entity, table, kind or migration, so that D17 / D19 / §10.2 are respected.
25. As a contributor, I want free writing persisted as ordered `blocks` rows through `BlockRepository`, so that the Phase 6 editor inherits the right storage shape.
26. As a contributor, I want a minimal write path through the existing repositories to place a Task/Event line on a day, so that this increment is demoable on its own.
27. As a reviewer, I want no full Task CRUD, rename/edit, scheduling UI, priority/tag/folder assignment, subtasks, detail pane or "+" badge in this phase, so that Phase 5 owns that work cleanly.
28. As a reviewer, I want no 5-type block editor, inline bold/italic/link, `inline_json` ranges, Markdown shortcuts or six-dot handle in this phase, so that Phase 6 owns that work cleanly.
29. As a reviewer, I want no `blocks.task_id` to-do→Task linking and no Task/Event Note panes in this phase, so that Phase 7 owns that work cleanly.
30. As a reviewer, I want no Inbox rail, Folders/Tags sections, drag-to-file, Quick Add parsing, CSV export, visual tuning or packaging in this phase, so that Phases 8–12 own their work cleanly.
31. As a reviewer, I want no In Progress / Waiting state, no cascading subtask soft-delete and no undo stack, so that v0.1's state model (`tech-stack.md` §6.4 / §9.8) is kept.
32. As a reviewer, I want no webview, network layer, plugin system or scripting engine added, so that ADR-0002 / D3 / D11 hold.
33. As a reviewer, I want no CMake, no schema rewrite, no new migration and no rework of the build baseline, so that `tech-stack.md` §4 / §7.2 / §11 hold.
34. As a reviewer, I want no new unit-test framework or low-level seams, so that the single-seam rule (`tech-stack.md` §12) is kept.
35. As a reviewer, I want the verification suite run on a GPU from Google Colab through `scripts/colab-gpu-test.sh`, so that the mandatory test-execution rule is honoured.
36. As a reviewer, I want canonical `GLOSSARY.md` vocabulary in identifiers and comments, so that later code stays consistent.
37. As a reviewer, I want no schedule or effort estimates anywhere in the spec, so that D7 / D9 are respected.
38. As a maintainer, I want the spec to state plainly that this is a proposal awaiting confirmation, so that no maintainer sign-off is implied.
39. As a maintainer, I want this spec recorded as uncommitted files on the phase branch, so that the commit step remains a separate, reviewable workflow stage.

## Implementation Decisions

Stated in durable terms; concrete file/command targets are in `plan.md` and
`validation.md`. No prototype was produced, so no snippet is inlined.

- **Layering.** The Today page is a declarative QML composition rendered into the
  existing centre panel; a C++ `TodayModel` controller/model (per
  `tech-stack.md` §5.1) mediates between QML and the Phase 2 store via
  `QAbstractListModel` / `Q_PROPERTY` / `Q_INVOKABLE`. QML holds no business
  logic and the C++ entry point stays free of product logic.
- **Day note identity.** The Day note is exactly one Note Item per date
  (`kind='note'`, `day='YYYY-MM-DD'`), created on first open of a date and read
  by `day` thereafter. Uniqueness is enforced by the existing
  `one_day_note_per_day` partial unique index (active rows only), and a
  soft-deleted Day note frees its date for a replacement. There is **no** new
  entity, table, kind, index or migration (D17, D19; `tech-stack.md` §6.3 /
  §7.2 / §10.2).
- **Task and Event lines.** Lines render from real Task/Event Items read through
  the store. Events remain lightweight first-class Items (title, start/end,
  all-day) with **no calendar grid, reminders or synced-calendar behaviour**
  (`tech-stack.md` §9.7). A minimal write path through the existing repositories
  makes the increment demoable; the full Task lifecycle, scheduling / priority /
  Tag / Folder / duration / subtask UI and the detail pane with a "+" badge are
  Phase 5 (`tech-stack.md` §9.3).
- **Badge chips and check-off.** Chips show only the details that are set (due
  time, priority, Tag name when present); priority colours the checkbox
  (`tech-stack.md` §6.4 / §9.2). Check-off sets `completed_at` through the store
  and survives restarts. v0.1 keeps no In Progress / Waiting state, no cascading
  soft-delete of subtasks and no undo stack; those behavioural rules belong to
  Phase 5 (`tech-stack.md` §6.4 / §9.8). The optional completion sound and the
  "done today" progress ring are visual polish, not gate items for this phase.
- **Free writing.** The editable Day note persists free writing as ordered
  `blocks` rows (plain UTF-8 `content`, `position`) through `BlockRepository`,
  reusing the Phase 2 `blocks` table (`tech-stack.md` §7.3). Phase 4 provides a
  minimal plain-text surface only: the 5-type editor, inline bold/italic/link,
  Return/Backspace/Tab, the six-dot handle, Markdown shortcuts and `inline_json`
  ranges are Phase 6 (D19; `tech-stack.md` §9.4); `blocks.task_id` to-do→Task
  linking and Task/Event Note panes are Phase 7.
- **Persistence.** All writes go straight to SQLite with no in-memory-only state;
  soft-delete is the only deletion path and no Trash UI is exposed (D21;
  `tech-stack.md` §9.8 / §10.3).
- **Strings and assets.** All strings, placeholders, empty states and icons are
  original. Reference material (`logo/`, `screenshots/`, `app-document.md`,
  `DESIGN.md`) is read for layout and behaviour only and never bundled
  (ADR-0001 / ADR-0004; `tech-stack.md` §8.1 / §8.4).
- **Visual language.** The page inherits the single shared token set from
  `Theme.qml` (`tech-stack.md` §8.3); exact hues, radii and typography are tuned
  in Phase 11 against the three screenshots, never borrowed from `DESIGN.md`
  (§10.6). Native OS chrome and platform-correct `Ctrl`/`⌘` labels are
  preserved and the inner UI stays identical across platforms (D15; §8.5).
- **Constraints.** No webview, network layer, plugin system, scripting engine,
  CMake, test framework, CI job, low-level unit seams or packaging script is
  introduced (ADR-0002, D3, `tech-stack.md` §12).
- **Testing.** The single highest seam (launch → observe Today → write → restart
  → observe unchanged data), its prior art and the mandatory GPU Colab route are
  in `validation.md`; the executable seam stays framework-free and is run
  through `scripts/colab-gpu-test.sh`.
- **Documentation.** README / CHANGELOG / roadmap are maintained by the
  `/bnvt-documentation` step and are intentionally **not modified** in this
  phase; the full docs are a Phase 12 deliverable.
- **Explicit phase boundary.** No full Task CRUD, 5-type editor, to-do→Task
  links, Inbox grouping, Quick Add, CSV export, visual tuning or packaging is
  built here. No settled decision or absorbed ADR is re-litigated; a constitution
  change needs an explicit amendment (`mission.md` §13). No schedule or effort
  estimates (D7 / D9).

## Out of Scope

- **Phases 5–12** in full: the full Task lifecycle, rename/edit, scheduling /
  priority / Tag / Folder / duration / subtask UI, detail pane and Task/Event
  Note panes (Phase 5); the 5-type block editor and inline formatting (Phase 6);
  to-do→Task linking via `blocks.task_id` (Phase 7); the Inbox rail, Folders /
  Tags sections, drag-to-file and tag-chip assignment UI (Phase 8); Quick Add
  (Phase 9); CSV export (Phase 10); visual tuning (Phase 11); release packaging
  and full documentation (Phase 12).
- **Behavioural rules, not persistence:** the Inbox "no date AND no folder" query
  as a UI grouping, completing a Task clearing In Progress state, cascading
  soft-delete to subtasks, the completion sound, the "done today" progress ring
  and any undo stack.
- **The deferred backlog** (`roadmap.md` §4, no phase numbers): Calendar/Upcoming
  pages, Habits/streaks, saved Views, Settings, Analytics, AI assistant, local
  MCP server, third-party integrations, full CSV import, Trash UI, search /
  command palette, standalone list pages, the remaining 12 block types, symbol
  menus, live Quick Add preview, Magic Lines, multi-language parsing, Windows
  and macOS binaries.
- **Permanent non-goals** (`mission.md` §8, ADR-0003): Apple-ecosystem features,
  the macOS-only shell and the iOS app — never built on this architecture.
- Any schema rewrite, new migration, new entity / kind / table / index, or new
  database connection path; any webview, network call, plugin system, scripting
  engine, cloud/account/sync or external migration framework.
- Any populated page or disabled placeholder beyond the Today page; any fake
  content ("no empty shells", D5 / D12).
- A full unit-test framework, CI configuration, low-level unit seams and
  packaging scripts beyond the single extended persistence seam; a local,
  CPU-only or non-GPU test run as a substitute for the GPU Colab route.
- README / CONTRIBUTING / CHANGELOG / roadmap edits; a tagged release.
- Any schedule or effort estimate (D7 / D9) or settled-decision / ADR
  re-litigation.

## Further Notes

- This is the **fourth** mergeable increment of v0.1 and the first to render
  live product data. It directly advances `mission.md` §10 item 3 ("Shows Today
  as an editable Day note with Task and Event lines, a day strip and check-off");
  it does **not** by itself satisfy the full Definition of Done — the 5-block
  editing experience and to-do→Task links (item 5) are completed in Phases 6–7.
  Its merge gate is the Phase 4 "Verifiable when" condition.
- Phase 4 reuses the Phase 2 schema deliberately: `items.day`, `blocks`,
  `item_tags`, `links` and the `one_day_note_per_day` index were defined up front
  so this phase needs no migration. This is a documented standard
  (`tech-stack.md` §5.3 / §7.2), not speculative generality.
- Visual tokens are starting values; exact hues, radii and typography are tuned
  in Phase 11 against the three screenshots, still using only original strings
  and assets.
- Screenshots are the visual source of truth and the 134 guides the behavioural
  source of truth (ADR-0004); the day strip, summary line, Task/Event lines and
  check-off follow that authority without copying guide text or upstream strings.
- **Approval status.** This spec is a synthesis, not an interview, and carries no
  maintainer sign-off. The seam decision and the next-phase reading are open
  questions for the maintainer to confirm before `/bnvt-ticket` (see
  `validation.md` §"Seam decision — please confirm"). Following the project's
  spec approval process means leaving that confirmation outstanding rather than
  recording an approval that was never given; Phases 1–3 were specified the same
  way.
- **No issue-tracker configuration was provided to the `/to-spec` skill**, so per
  the repo's established layout the spec is published as the three local files
  in `dev-docs/specs/` rather than to an external tracker; no `ready-for-agent`
  label is applied.
- `/bnvt-ticket` breaks `plan.md` into tracer-bullet vertical slices; the spec
  is written to local files and is **not committed** in this step (committing is
  the `/bnvt-commit` stage).
