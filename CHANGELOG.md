# Changelog

All notable changes to `open-woday` are documented here, following
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/). This file is
maintained by the `/bnvt-documentation` workflow step (`mission.md` §9): each
phase adds an entry for the work done since the last full-spec commit — currently
`a991433` (2026-11-24), which consolidated the `v0.1` spec and ADRs into the
three-file constitution.

No release has been tagged yet, so everything below is **Unreleased**. Earlier
commits (the initial `v0.1` spec, ADRs, `GLOSSARY.md`, reference material and
the constitution itself) predate this file; consult the git log for that
design-only history.

## [Unreleased] — 2026-11-26

### Added

- **Phase 1 — project-skeleton: the first runnable increment.** A `qmake6` Qt 6
  Quick / QML + C++ application that configures, compiles and opens a single
  empty window on Linux, with no webview, no network and no persistence.
  - `open-woday.pro` — the `qmake6` build definition, declaring the `quick`,
    `widgets` and `sql` modules, C++17, and targeting `open-woday`. No CMake.
  - `src/main.cpp` — one C++ entry point creating `QGuiApplication` and
    `QQmlApplicationEngine`; fails loudly to stderr and returns `1` if the root
    QML component cannot be created, otherwise exits cleanly. No business logic.
  - `src/qml/MainWindow.qml` — one declarative, empty `Window` titled
    `open-woday`, using native OS decorations; no placeholder pages, disabled
    destinations or fake content ("no empty shells", D5 / D12). All strings are
    original.
  - `resources.qrc` — embeds the root QML component so the binary is
    self-contained.
  - Build and run: `qmake6 open-woday.pro && make && ./open-woday`. Verified on
    Qt 6.12.0 with `g++`: clean configure/compile with no compiler warnings, and
    the root QML window loads without a startup error.
- **Phase 1 spec** at `dev-docs/specs/2026-11-24-14-project-skeleton/`
  (`requirements.md`, `plan.md`, `validation.md`) and two tracer-bullet
  implementation tickets at `.scratch/2026-11-24-14-project-skeleton/issues/`.
- **First code-review record** at
  `.code-review/2026-11-24-14-project-skeleton.md`: a two-axis (Standards /
  Spec) review of the phase branch against the merge-base.
- **Phase 2 — data-layer: local persistence.** Merged to `main` in PR #4.
  - `Database` creates the app-data directory and one SQLite file on first
    launch, enables foreign-key enforcement, applies versioned migrations and
    records the schema version in `meta`. Database-open failures stop startup
    with a clear non-zero exit.
  - `src/store/schema.sql` defines the seven-table schema (`items`, `blocks`,
    `folders`, `tags`, `item_tags`, `links`, `meta`), constraints, cascades and
    indexes, including the active-Day-note uniqueness rule.
  - `ItemRepository` provides create/read/update/soft-delete for Tasks, Notes
    and Events, round-trips the full Item model and supports the queries later
    phases need. `BlockRepository` provides Block CRUD, ordering and reordering.
  - `tests/verify-persistence.cpp` and `tests/verify-persistence.pro` add a
    framework-free persistence/restart verification target.
  - Phase 2 spec at `dev-docs/specs/2026-11-24-16-data-layer/` and review record
    at `.code-review/2026-11-24-16-data-layer.md`.
- **Phase 3 — app-shell: static three-panel frame.** Merged to `main` in PR #7.
  - `Theme.qml` centralizes the canvas, sidebar, panel, chip, accent, radius,
    spacing and typography tokens, resolving Inter with a system-font fallback.
  - `Sidebar.qml`, `TopBar.qml`, `CentrePanel.qml` and `RightRail.qml` compose a
    dark sidebar/top bar with white rounded centre and right-rail panels, using
    original strings and native OS decorations.
  - The Sidebar folds before the RightRail on resize. A minimum window width
    preserves a usable centre surface, and the TopBar elides its title and
    compacts its Quick Add label at narrow widths.
  - The shell initially showed a platform-correct `Ctrl` / `⌘` shortcut label
    without live planning pages or Quick Add behaviour.
  - Phase 3 spec at `dev-docs/specs/2026-11-25-06-app-shell/`.
- **Phase 4 — today-page: Today page and editable Day note.** Merged to `main`
  in PR #10.
  - A selectable ten-day strip, date label and summary show the selected day's
    Task and Event counts. Tasks and Events due or starting on that day appear
    on Today.
  - A plain-text Day note is created or loaded per date and saved locally via
    debounced persistence. Users can add Tasks to the selected date and complete
    or reopen them; completion and note content persist across restarts.
  - The framework-free persistence verification covers Today model, midnight
    rollover, local-day offset timestamps, formatted-Block protection, and
    restart survival. The Phase 4 spec is at
    `dev-docs/specs/2026-11-25-13-today-page/`.
- **Phase 5 — task-crud: full Task lifecycle and detail pane.** In progress on
  `phase-5-task-crud`.
  - Store repositories `FolderRepository` and `TagRepository` back Folders,
    Tags and `item_tags`, and `ItemRepository::softDelete` cascades atomically
    to active descendant Task subtasks without touching non-Task Items.
  - `ItemModel` and `NoteBuffer` mediate Task selection, inline rename,
    completion/reopening, due date and optional 24-hour time scheduling,
    priority, inline planned duration, Folder filing, multi-Tag attachment,
    multi-level nested subtasks, and each Task's plain-text Note scaffold.
  - `TodayPage`, `TaskRow`, `TaskDetailPane` and reusable components (`Checkbox`,
    `Badge`, `TagChip`, `IconButton`, `ItemBadges`) render Task and Event lines
    with metadata chips and host the Task detail pane in `RightRail`.
  - `tests/verify-persistence.cpp` covers the Phase 5 Task lifecycle, subtask
    cascading soft-delete, DST-safe scheduling, Event metadata projection, and
    restart survival.
  - Phase 5 spec at `dev-docs/specs/2026-11-26-10-task-crud/` and review record
    at `.code-review/2026-11-26-10-task-crud.md`.

### Changed

- **Testing rule — local verification.** `AGENTS.md` and the constitution
  (`dev-docs/tech-stack.md` §12) now require every verification run to happen
  on the developer's machine with `qmake6`, `make` and
  `tests/verify-persistence`. The GPU Colab runner and VM-side driver were
  removed; no Google account, remote runtime or GPU is required. Documented in
  `README.md`.
- `README.md` was expanded (`9f0db62`) with a project overview, the
  unaffiliated / non-commercial disclaimer, `v0.1` scope, permanent non-goals,
  quick start, planned install/build/configuration, roadmap status, a project
  documentation index and an interim contributing guide.
- `README.md` now reflects that a buildable application skeleton exists: the
  status banner, quick start, build-from-source and roadmap sections were
  updated, while install and packaging remain marked as planned for `v0.1`.
- `README.md` now distinguishes the merged Phase 1/2 state from the Phase 3
  branch, documents startup database creation and the static app-shell frame,
  and separates implemented local-data behaviour from planned shortcuts,
  Quick Add and CSV export.
- `README.md` and `dev-docs/roadmap.md` were updated for the Phase 4 merge
  (PR #10, marking Phases 2–4 `done`) and Phase 5 (`in-progress` on
  `phase-5-task-crud`).
- Phase 2 review cleanup extracts shared store helpers in `StoreHelpers.h` and
  deduplicates repository/database query and binding code.
- Phase 5 helper extractions move shared debounced plain-text note persistence
  into `NoteBuffer` (used by `TodayModel` and `ItemModel`), Task/Event badge
  chip rendering into `ItemBadges.qml`, Task completion timestamps into
  `ItemRepository::setCompleted`, and calendar day/time parsing into
  `StoreHelpers.h`.

### Fixed

- Restored the internal reference documents `dev-docs/app-document.md` (the
  upstream 134-guide scrape) and `dev-docs/grill-session.md` (the historical
  design-session record), which the Phase 1 spec commit had deleted. A code
  review flagged the removal as out of scope; both remain internal reference
  material only — never published as project documentation and never bundled
  with a release.
- Phase 2 review fixes isolate each `Database` instance's connection, roll back
  failed commits, preserve a Block's owner and position on update, normalize
  null Item titles to empty strings, and allow a soft-deleted Day note's date to
  be reused.
- Phase 3 review fixes correct the Inter/system-font fallback and keep the shell
  and TopBar usable at narrow window widths.
- Phase 4 follow-up fixes (merged in PR #10) query Tasks and Events by the
  selected local calendar day (matching offset/local timestamps in
  `StoreHelpers`), refresh `TodayModel` at local midnight and after clock
  changes, keep the selected day reachable across midnight rollover, preserve
  partial read results on query failures, separate Task errors from Day notes,
  debounce Day note persistence, scroll Today tasks and the Day note together,
  prevent formatted Day notes from blocking date navigation, and handle index
  gaps in `BlockRepository::replaceText`.
- Phase 5 review fixes ensure `ItemRepository::softDelete` cascades strictly
  through Task subtasks without retiring linked Notes or Events, move
  explicitly rescheduled subtasks out of their parent's old-day subtree onto the
  target day as root lines, preserve naive wall-clock due/start times across
  daylight-saving gaps in `StoreHelpers`, project Event metadata (`allDay`,
  `startTime`, `endTime`, `priority`, `durationMin`, `folderName`, `tags`) for
  Today badge chips, and commit inline Task title edits to the edited Task
  rather than a newly selected Task.
