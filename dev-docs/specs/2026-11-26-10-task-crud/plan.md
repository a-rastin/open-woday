# Phase 5 — task-crud — Plan

> **Spec:** `dev-docs/specs/2026-11-26-10-task-crud/` · **Branch:** `phase-5-task-crud`
> Numbered task groups in dependency order. `/bnvt-ticket` turns these into
> tickets. Uses `GLOSSARY.md` vocabulary; respects `mission.md`, `tech-stack.md`
> and the absorbed ADRs. **No schedule or effort estimates** (D7 / D9).
>
> Concrete targets below come from `roadmap.md` Phase 5 deliverables and
> `tech-stack.md` §§5.1–5.2, 6.2, 6.4, 7.2–7.3, 8.2–8.5, 9.1, 9.3, 9.8, 10.3.
> `validation.md` holds the checks that prove each group is done.

## 1. Store extensions for Folders, Tags and subtask cascade

1. Extend the Phase 2 store (`src/store/`) with Folder and Tag persistence
   operations (e.g., `FolderRepository` / `TagRepository` or cohesive store
   classes following `StoreHelpers.h` / `StoreRepository`) backed by the
   existing `folders`, `tags` and `item_tags` tables in `src/store/schema.sql`:
   - Create and list Folders (`id`, `name`, `icon`, `cover`, `sort`).
   - Create (or look up by unique `name`) and list Tags (`id`, `name`, `colour`,
     `icon`), attach a Tag to an Item (`item_tags`), detach a Tag from an Item,
     and read all Tags attached to a given Item.
2. Extend `ItemRepository::softDelete(qlonglong id)` (or add the cascading
   soft-delete path in `ItemRepository`) so soft-deleting a Task atomically
   sets `deleted_at` on the Task and all of its active descendant subtasks
   (`parent_id` chain) inside a database transaction (`tech-stack.md` §6.4).
   Keep `readById` / normal queries filtering `deleted_at IS NULL` and expose no
   Trash UI or hard delete (`tech-stack.md` §10.3).
3. Keep `src/store/schema.sql` and `Database` migration version completely
   unchanged (**no schema change, no migration**).

## 2. C++ ItemModel and Today synchronization

1. Add `ItemModel` (`src/app/ItemModel.{h,cpp}`) per `tech-stack.md` §§5.1–5.2,
   exposed to QML via `Q_PROPERTY` / `Q_INVOKABLE` / `QAbstractListModel` as
   needed, registered/bound cleanly from `src/main.cpp` with no product logic in
   `main.cpp`.
2. Support full Task lifecycle operations in `ItemModel`:
   - Select / open a Task by `id` into the detail pane, and clear selection to
     close the pane.
   - Create a Task (top-level or subtask with `parentId`, optionally due on a
     date) and rename any Task (`title`).
   - Toggle completion / reopen any Task or subtask (`completed_at`,
     `modified_at`).
   - Set, update or clear due date (`due_at`, `due_precision`: `'day'` for
     `YYYY-MM-DD`, `'time'` for ISO date-time with 24-hour `HH:mm`, or
     `std::nullopt` for undated).
   - Set or clear priority (`0` none, `1` low, `2` medium, `3` high), inline
     planned duration (`duration_min` in minutes, or cleared), Folder
     (`folder_id`, including creating a new Folder or clearing), and Tags
     (attach existing Tag, create-and-attach new Tag, detach Tag).
   - Create, rename, complete/reopen and soft-delete nested subtasks
     (`parent_id`), and expose subtask lists plus `done/total` progress counts.
   - Soft-delete the selected Task (cascading to its subtasks) and close the
     detail pane if the deleted Task was open.
   - Load and persist the selected Task's own plain-text Note scaffold as
     `blocks` rows under `item_id = task.id` via `BlockRepository` (using the
     same plain-text safety guard as Day notes so non-plain-text Blocks are not
     overwritten).
3. Keep `TodayModel` (`src/app/TodayModel.{h,cpp}`) synchronized with
   `ItemModel`:
   - Connect mutation signals so creating, renaming, rescheduling, prioritizing,
     retagging, filing, completing, adding/completing subtasks or deleting a
     Task immediately refreshes Today's `lines` and `summary` counts.
   - Enrich `TodayModel::lines()` entries for Tasks with set metadata needed by
     Today's Task rows: `priority`, `dueTime` (when `due_precision='time'`),
     `durationMin`, `folderId` / `folderName`, `tags` (name/colour),
     `subtaskDone` / `subtaskTotal`, and nested subtask data (or structured tree
     rows so Today reflects nested subtasks accurately without duplicating them
     as unrelated top-level Tasks).
4. Update `open-woday.pro` with the new C++ sources/headers; keep
   `QT += quick widgets sql`, `CONFIG += c++17`, `TARGET = open-woday`, and
   **no CMake**.

## 3. Shared QML components and upgraded Today Task rows

1. Add the Phase 5 reusable QML primitives under `src/qml/components/`
   (`tech-stack.md` §5.2) inheriting `Theme.qml`:
   - `Checkbox.qml`: priority-coloured border/fill (neutral muted for None,
     subtle accent/blue/amber/red tones harmonizing with `Theme.qml` for
     Low/Medium/High; exact screenshot polish stays Phase 11) and checked
     state.
   - `Badge.qml`: rounded pill/control chip for set Task metadata (due
     date/time, priority, duration, Folder, subtask count, and `"+"` add badge).
   - `TagChip.qml`: pastel pill chip displaying a Tag name (and optional remove
     affordance in the detail pane).
   - `IconButton.qml`: compact accessible action button with original glyphs/icons
     (no upstream assets).
2. Add `src/qml/TaskRow.qml` (`tech-stack.md` §5.2) and replace the bare
   `CheckBox` delegate in `src/qml/TodayPage.qml`:
   - Renders the priority-coloured `Checkbox`, Task title (with double-click or
     explicit inline rename trigger), set detail chips (`dueTime`, `durationMin`,
     `TagChip`s, `subtaskDone/subtaskTotal` progress badge), and subtle
     hover/focus actions (open detail, add subtask, delete).
   - Clicking a Task row opens that Task in the right rail's `TaskDetailPane`.
   - Clicking the `done/total` subtask badge expands/collapses nested subtasks
     indented below the parent row, with inline subtask creation, completion
     and click-to-open.
3. Update `resources.qrc` to embed all new QML files and components.

## 4. Task detail pane in the right rail

1. Add `src/qml/TaskDetailPane.qml` (`tech-stack.md` §§5.2, 9.3) and host it in
   `src/qml/RightRail.qml` when a Task is selected in `ItemModel`:
   - Header with close button (`×` / `IconButton`) to clear selection and return
     the right rail to its default empty state (ready for Phase 8's Inbox rail),
     plus a soft-delete action.
   - Priority-coloured `Checkbox` paired with an inline-editable Task title
     field that commits directly to SQLite.
   - **Badge row:** renders **only the details that are actually set** (due
     date/time badge, priority badge when priority > 0, duration badge when set,
     Folder badge when filed, and `TagChip`s for attached Tags). Clicking any set
     badge allows editing or clearing that detail.
   - **`"+"` badge** at the end of the badge row (`tech-stack.md` §9.3): opens a
     menu/picker listing only the currently unset details (`Due date`,
     `Priority`, `Duration`, `Folder`) plus `Add tag` (always available so
     additional Tags can be attached or new Tags created).
   - Due date/time editor supporting picking/entering a calendar day, toggling
     or entering an optional 24-hour time (`HH:mm`), and clearing the due date.
   - Inline duration editor supporting minute presets or numeric minute entry
     and clearing duration.
   - **Subtasks section:** lists the Task's subtasks (with completion checkboxes,
     inline rename, click-to-navigate into a subtask, delete, and an inline
     "Add subtask" input creating child Tasks with `parent_id`).
   - **Task Note pane scaffold:** below the badges/subtasks, an editable
     plain-text Note area wired to the Task's own `blocks` rows via
     `ItemModel`/`BlockRepository`, with clear error feedback and save-on-blur /
     debounced persistence matching `TodayModel`'s Day-note durability rules.
2. Preserve `MainWindow.qml`'s three-panel layout, Sidebar-then-RightRail fold
   thresholds, native OS window decorations, `Ctrl`/`⌘` modifier labels, and
   original strings (`tech-stack.md` §§8.4–8.5). Keep all deferred UI (5-type
   block editor, `blocks.task_id`, Inbox rail, Sidebar Folder/Tag trees, Quick
   Add parser, CSV export) out of this phase.

## 5. Persistence verification and build/hygiene checks

1. Extend the single framework-free executable seam
   (`tests/verify-persistence.cpp` and `tests/verify-persistence.pro`) to cover
   the Phase 5 observable Task lifecycle across a database restart:
   - Create, rename, complete and reopen a Task; set and clear due date +
     24-hour time; set priority; create/attach/detach Tags; create/assign/clear
     Folder; set/clear inline duration; nest multi-level subtasks (`parent_id`)
     and observe `done/total` progress; write and save the Task's own Note
     scaffold; observe `TodayModel` reflecting scheduled/updated/completed/deleted
     Tasks immediately; soft-delete a parent Task and verify cascading
     soft-delete of all descendant subtasks.
   - Close and reopen `Database` against the same local SQLite file and confirm
     all surviving Tasks, attributes, Tags, Folders, subtasks, Task Notes and
     Today state read back unchanged, and soft-deleted Tasks/subtasks remain
     excluded from normal reads.
2. Run local verification from `tests/`:
   `qmake6 verify-persistence.pro && make -j"$(nproc)" && ./verify-persistence`
   and confirm both build and executable exit `0`.
3. Run `qmake6 open-woday.pro && make` from a clean build and launch
   `./open-woday` to verify the live UI flow (create, schedule, tag, file, nest,
   rename, complete, reopen, Task Note edit, delete, restart).
4. Audit the diff against constitutional constraints: no webview, no runtime
   network access, no schema/migration edits, no CMake, no test framework, no
   upstream strings/assets, canonical `GLOSSARY.md` vocabulary, no Phase 6–12
   features, no estimates, and spec files uncommitted on `phase-5-task-crud`.
