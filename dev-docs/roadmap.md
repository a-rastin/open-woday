# open-woday — Roadmap

> **Constitution, file 3 of 3.** Companion to `mission.md` (why/what) and
> `tech-stack.md` (how). This is the **high-level implementation order in small
> phases of work**.
>
> **Consolidation note (2026-11-24):** this file absorbs the build order (§13),
> the deferred roadmap (§15) and the definition of done (§12) of the former
> `docs/PLAN.md`, which was removed. Nothing was dropped.
>
> **Language:** English. **No time or effort estimates** (D7, D9) — order is by
> dependency, not calendar.

---

## 1. How this file drives the workflow

`/bnvt-spec` finds **the next phase** here, creates a git branch `phase-X-TITLE`
(X = this phase's number, TITLE = this phase's branch-safe title), and writes a
spec (`plan.md`, `requirements.md`, `validation.md`) under `dev-docs/specs/`.
`/bnvt-ticket` then breaks that spec into tickets, followed by implement →
review → revise → documentation → commit → merge.

**Conventions**

- Phases are numbered `1..N` in dependency order. Each phase is one spec → one
  branch → one mergeable increment.
- Each phase has a **branch-safe title** (lowercase, hyphenated). The exact branch
  name is given on the `Branch:` line so there is no ambiguity.
- **The next phase = the lowest-numbered phase whose status is not `done`.**
  Status values: `planned` → `in-progress` → `done`. All phases start `planned`;
  update the status as work lands.
- Each phase ends in a **runnable / verifiable increment** (`Verifiable when`).
  No phase ships a non-demoable horizontal layer on its own.
- Phases cover **v0.1 only**. Deferred features are an uncommitted backlog with
  **no phase numbers** (§4); permanent non-goals are excluded entirely
  (`mission.md` §8).
- A deferred backlog item, when picked up, gets its own design round and, if hard
  to reverse, an amendment to this constitution.

---

## 2. The v0.1 phases

Derived from the dependency-ordered build order of the former `PLAN.md` §13.

---

### Phase 1 — project-skeleton

- **Branch:** `phase-1-project-skeleton`
- **Status:** done
- **Goal:** a buildable, runnable Qt 6 Quick application skeleton.
- **Deliverables:** `open-woday.pro` (Qt 6 Quick / Widgets / SQL modules),
  `src/main.cpp`, `src/qml/MainWindow.qml` (empty window), build via
  `qmake6 && make`.
- **Verifiable when:** the app configures, compiles and opens a window on Linux.

---

### Phase 2 — data-layer

- **Branch:** `phase-2-data-layer`
- **Status:** in-progress
- **Goal:** the local persistence foundation.
- **Deliverables:** `Database` (schema creation, versioned migrations), the full
  schema (`items`, `blocks`, `folders`, `tags`, `item_tags`, `links`, `meta`),
  `ItemRepository`, `BlockRepository`, `meta` schema version. See
  `tech-stack.md` §7.2.
- **Verifiable when:** the schema is created on first launch, repositories can
  CRUD Items/Blocks, and data survives an app restart.

---

### Phase 3 — app-shell

- **Branch:** `phase-3-app-shell`
- **Status:** planned
- **Goal:** the reference three-panel window and visual tokens.
- **Deliverables:** `Sidebar`, `TopBar`, `RightRail` QML; panel tokens
  (`canvas`, `sidebar`, `panel`, chips, radii, font — `tech-stack.md` §8.3);
  resize/fold behaviour; native OS chrome; platform-correct `Ctrl`/`⌘` labels.
- **Verifiable when:** the window shows a dark sidebar, white rounded centre
  panel and white rounded right rail in the reference visual language, with no
  macOS traffic lights on Linux.

---

### Phase 4 — today-page

- **Branch:** `phase-4-today-page`
- **Status:** planned
- **Goal:** the Today page as an editable Day note.
- **Deliverables:** `DayStrip`, summary line, Task/Event lines with badge chips,
  check-off, day-note wiring (one `items` row with `day`, unique per date), free
  writing around the lines. `tech-stack.md` §§6.3, 9.2.
- **Verifiable when:** Today renders a day's Task/Event lines and the editable
  Day note persists across restarts.

---

### Phase 5 — task-crud

- **Branch:** `phase-5-task-crud`
- **Status:** planned
- **Goal:** the full Task lifecycle.
- **Deliverables:** create / rename / edit / complete / reopen; due date (day +
  optional time), priority, Tags, Folder, nested subtasks (`parent_id`), inline
  duration; detail pane with badge row and "+" badge; scaffold for each Task's own
  Note pane. `tech-stack.md` §§9.3, 6.4.
- **Verifiable when:** a Task can be created, scheduled, tagged, nested and
  completed, and is reflected on Today.

---

### Phase 6 — block-editor

- **Branch:** `phase-6-block-editor`
- **Status:** planned
- **Goal:** the in-house 5-type QML block editor.
- **Deliverables:** `text`, `heading`, `to-do`, `bullet`, `code` blocks; inline
  bold / italic / link; Return splits, Backspace merges, Tab indents, six-dot
  handle (drag / turn-into / duplicate / delete), Markdown shortcuts;
  `inline_json` formatting ranges. `tech-stack.md` §§9.4, 7.3.
- **Verifiable when:** a Note can be written and reordered with all 5 block types
  and inline formatting, persisted as `blocks` rows.

---

### Phase 7 — task-note-link

- **Branch:** `phase-7-task-note-link`
- **Status:** planned
- **Goal:** to-do Blocks create real Tasks; wire the editor into Task/Event notes.
- **Deliverables:** to-do Block creates/links a Task Item (`blocks.task_id`),
  day-note to-dos are due that day, the Task detail pane embeds the editor,
  subtasks arise from to-do lines in a Task's note. `tech-stack.md` §§6.3, 9.3.
- **Verifiable when:** writing a to-do line in the Day note produces a real Task
  on Today, and a Task's own Note is editable with the same editor.

---

### Phase 8 — inbox-and-organization

- **Branch:** `phase-8-inbox-and-organization`
- **Status:** planned
- **Goal:** capture and filing via Inbox, Folders and Tags.
- **Deliverables:** Inbox rail (the "no date AND no folder" rule, Overdue / Inbox
  / Today groups, quick-add line), Folders section, Tags section (many-to-many,
  pastel chips), drag-to-file, quick-add from the rail. `tech-stack.md` §9.5.
- **Verifiable when:** unfiled/undated Items appear in the Inbox rail and dragging
  one onto a Folder or Tag files it correctly.

---

### Phase 9 — quick-add

- **Branch:** `phase-9-quick-add`
- **Status:** planned
- **Goal:** type-to-create in the top-bar field.
- **Deliverables:** `QuickAddParser` (kind prefixes/words, today/tomorrow/weekday
  names, `HH:mm`, `#tag` create-if-missing, `!`/`!!`/`!!!` priority), parsed
  tokens removed from the title and shown as chips; wired into the top-bar field;
  `Ctrl+K`/`⌘K` focuses it, `Ctrl+N`/`⌘N` creates a Task. `tech-stack.md` §9.6.
- **Verifiable when:** e.g. `"Call Anna tomorrow 10:00 #work !!"` creates a
  correctly parsed Task with chips, in English only.

---

### Phase 10 — csv-export

- **Branch:** `phase-10-csv-export`
- **Status:** planned
- **Goal:** the one-way data exit path.
- **Deliverables:** `CsvExporter`, one UTF-8 file, one row per Item (Notes →
  Tasks → Events), the 18-column Woday-compatible shape with empty non-v0.1
  columns, soft-deleted rows excluded. `tech-stack.md` §7.4.
- **Verifiable when:** export produces a valid CSV that opens cleanly in a
  spreadsheet; no import mapper is present.

---

### Phase 11 — visual-tuning

- **Branch:** `phase-11-visual-tuning`
- **Status:** planned
- **Goal:** design fidelity to the Reference UI.
- **Deliverables:** tune tokens against the three screenshots, tag-chip hues,
  day-strip cells, panel radii and typography; verify native chrome and
  platform-correct modifiers; confirm all strings/assets are original.
  `tech-stack.md` §8.
- **Verifiable when:** the inner UI matches the screenshots' visual language
  (dark sidebar, white rounded panels, day strip, badge chips, right rail),
  using only original strings and assets.

---

### Phase 12 — release

- **Branch:** `phase-12-release`
- **Status:** planned
- **Goal:** a shippable, tagged `v0.1`.
- **Deliverables:** AppImage (`linuxdeployqt`) + `.deb` (`dpkg-deb`); README
  (unaffiliated/non-commercial disclaimer, permanent non-goals list, build/run
  instructions), `CONTRIBUTING.md`, `CHANGELOG.md`; tagged GitHub release.
  `mission.md` §9, `tech-stack.md` §11.
- **Verifiable when:** the AppImage and `.deb` build cleanly and every item of the
  **Definition of done** (`mission.md` §10 / §3 below) passes.

---

## 3. Definition of done for v0.1 (summary)

Full list in `mission.md` §10. In short: a tagged Linux release that opens the
three-panel window in the reference visual language (original strings/assets,
native chrome, `Ctrl` labels), shows Today as an editable Day note with Task and
Event lines and check-off, supports full Task CRUD with a 5-block editor and
to-do→Task links, shows the Inbox rail, parses minimal Quick Add, persists in
SQLite with no network, exports a Woday-shaped CSV, and ships as AppImage +
`.deb` with README, CONTRIBUTING and the disclaimer.

---

## 4. Backlog — deferred, uncommitted (no phase numbers)

From D18 / ADR-0003 (`mission.md` §8), in **no committed order**. These are NOT
v0.1 phases. Each, when picked up, gets its own design round and, if hard to
reverse, its own constitutional amendment. **Do not** assign them phase numbers
until promoted.

- Calendar page
- Upcoming page
- Habits and streaks
- Saved Views
- Settings
- Analytics
- AI assistant
- Local MCP server
- Third-party integrations (Notion, Obsidian, Todoist, Linear, Google Calendar,
  ICS/webcal feeds)
- Full CSV import (column mapper)
- Trash UI
- Search / command palette
- Standalone list pages (Tasks / Notes / Events)
- The remaining 12 block types
- Symbol menus
- Live Quick Add preview
- Magic Lines calculator
- Multi-language Quick Add parsing
- Windows and macOS binaries

---

## 5. Permanent non-goals (excluded from the roadmap)

Not deferred — these will **never** be built on this architecture. See
`mission.md` §8 (ADR-0003): iCloud / CloudKit, Apple Calendar, Reminders, Notes,
Weather and Apple Intelligence; the macOS-only shell (menu bar, widgets, Save for
Later, ⌥Space launcher); the iOS app.
