# Phase 4 — today-page — Plan

> **Spec:** `dev-docs/specs/2026-11-25-13-today-page/` · **Branch:** `phase-4-today-page`
> Numbered task groups in dependency order. `/bnvt-ticket` turns these into
> tickets. Uses `GLOSSARY.md` vocabulary; respects `mission.md`, `tech-stack.md`
> and the absorbed ADRs. **No schedule or effort estimates** (D7 / D9).
>
> Concrete targets below come from `roadmap.md` Phase 4 deliverables and
> `tech-stack.md` §§5.1–5.2, 6.3–6.4, 7.2–7.3, 8.2–8.5, 9.2, 9.7, 9.8.
> `validation.md` holds the checks that prove each group is done. Phase 4 is the
> **Today page as an editable Day note** — the first increment to render live
> product data into the Phase 3 shell. It is a runnable / verifiable increment
> proven through the launched window and the restart seam, not a non-demoable
> horizontal layer.

## 1. Build wiring and the C++ model layer

1. Add a `TodayModel` controller/model in `src/app/` per `tech-stack.md` §5.1,
   exposed to QML through `QAbstractListModel` / `Q_PROPERTY` / `Q_INVOKABLE`.
   QML views stay declarative (no business logic) and the C++ entry point stays
   free of product logic.
2. Wire the Phase 2 store (`Database`, `ItemRepository`, `BlockRepository`) into
   the model. Reuse the existing `items`, `blocks` and `meta` tables and the
   `one_day_note_per_day` index from `tech-stack.md` §7.2 — **no schema change,
   no migration, no new entity**.
3. Extend `open-woday.pro` with the new C++/QML sources; keep
   `QT += quick widgets sql`, `CONFIG += c++17`, `TARGET = open-woday`,
   `RESOURCES += resources.qrc`. **No CMake** is added (`tech-stack.md` §4 /
   §11). Extend `resources.qrc` only to embed the new QML and any original
   assets this phase adds.
4. Add no network layer, plugin system, scripting engine, webview or web-based
   editor library (D3, D11, ADR-0002; `tech-stack.md` §5.1 / §12).

## 2. The Day note — one Item per date

1. The Today page is backed by exactly one **Day note** Item: `kind='note'`
   with `day='YYYY-MM-DD'`, created on first open of a date through
   `ItemRepository` (`tech-stack.md` §6.3 / §10.2). The Day note is **not** a new
   entity, table or kind (D17, D19).
2. Enforce one active Day note per date through the existing partial unique
   index (`one_day_note_per_day`), which covers only `deleted_at IS NULL` rows;
   a soft-deleted Day note frees its date for a replacement.
3. Read the Day note back by `day` on later launches; never create a duplicate
   active Day note for the same date. Soft-delete stays the only deletion path
   (`tech-stack.md` §10.3); no Trash UI is exposed.

## 3. Today page composition in the centre panel

1. Add `TodayPage.qml`, composed into the Phase 3 `CentrePanel`, inheriting the
   shared `Theme.qml` token set; keep the dark canvas/sidebar, white rounded
   centre and right rail, native OS decorations, no drawn macOS traffic lights
   on Linux and platform-correct `Ctrl`/`⌘` labels (`tech-stack.md` §8.5, D15).
2. Add the `DayStrip` (≈10 selectable day cells using `dayStripCell` 44×56 and
   `accent` on today), the page title and a **summary line** with original
   strings (`tech-stack.md` §8.3 / §8.4 / §9.2).
3. Write every new string, placeholder, empty state, icon and label fresh for
   open-woday; no upstream UI string, logo, wordmark, mascot, screenshot or
   guide text is reused, embedded or shipped (ADR-0001 / ADR-0004,
   `mission.md` §5 / §6). `logo/`, `screenshots/`, `dev-docs/app-document.md`
   and `dev-docs/DESIGN.md` remain internal reference material only.
4. Keep the Phase 1 root-QML loud-failure guard and the Phase 3 resize/fold
   order (Sidebar first, then RightRail) in `MainWindow.qml` intact.

## 4. Task and Event lines with badge chips

1. Render the selected day's **Task** and **Event** lines from real Task/Event
   Items read through `ItemRepository` (`tech-stack.md` §6.2 / §6.4 / §9.7).
   Events are lightweight first-class Items (title, start/end, all-day) —
   **no calendar grid, reminders or synced-calendar behaviour** (§9.7).
2. Give each line badge chips for the details that are set — due time, priority
   and Tag name when present — using the `chip` / `radiusChip` / `radiusControl`
   tokens; priority colours the checkbox (`tech-stack.md` §6.4 / §9.2).
3. Provide the **minimal write path** through the existing repositories needed
   to place a Task/Event line on a day so the increment is demoable on its own.
   This is deliberately **not** the full Task lifecycle: no rename/edit, no
   scheduling or priority/tag/folder assignment UI, no nested subtasks, no
   detail pane with a "+" badge, no Task/Event Note pane (Phase 5 / §9.3).
4. Add no `blocks.task_id` to-do→Task linking and no embedding of the editor
   into Task/Event notes (Phase 7); no `inline_json` formatting ranges (Phase 6).

## 5. Check-off and the day summary

1. Add a `Checkbox` to each Task line; check-off toggles completion
   (`completed_at`) through `ItemRepository` and persists across restarts
   (`tech-stack.md` §9.2 / §9.8).
2. Derive the summary line's counts from the day's Items with original strings
   (e.g. event/task counts); keep the inner UI identical across platforms.
3. Keep v0.1's state model: no In Progress / Waiting state, no cascading
   soft-delete of subtasks, no undo stack (those are Phase 5 / `tech-stack.md`
   §6.4 / §9.8). The optional completion sound and the "done today" progress
   ring are visual polish, not part of this phase's merge gate.

## 6. Free writing in the editable Day note

1. Make the Day note **editable**: free writing around the lines is persisted as
   ordered `blocks` rows (`position`, plain UTF-8 `content`) through
   `BlockRepository`, reusing the Phase 2 `blocks` table (`tech-stack.md` §7.3).
2. Provide a minimal plain-text editing surface for the Day note. Do **not**
   build the 5-type block editor (`text`, `heading`, `to-do`, `bullet`, `code`),
   inline bold/italic/link, Return/Backspace/Tab behaviour, the six-dot handle,
   Markdown shortcuts or `inline_json` ranges — all of that is Phase 6 (§9.4).
3. Confirm Day-note edits survive a restart; all writes go straight to SQLite
   with no in-memory-only state (D21; `tech-stack.md` §9.8).

## 7. Persistence and the restart seam

1. Exercise the single highest seam (`validation.md`): build and launch, see
   Today with the day's Task/Event lines and the editable Day note, write/edit,
   restart, and see the same data rendered unchanged.
2. Extend the one framework-free executable seam
   (`tests/verify-persistence.cpp` / `verify-persistence.pro`) to cover the
   Day-note-per-date wiring and Task/Event/Block round-trips; add **no** test
   framework, no low-level unit seams, no test doubles (`tech-stack.md` §12).
3. Run the verification through `scripts/colab-gpu-test.sh` on a Google Colab
   **GPU** runtime (`AGENTS.md`; `tech-stack.md` §12). A local, CPU-only or
   non-GPU run does not satisfy the verification rule.

## 8. Build, constraint and spec-hygiene verification

1. From a clean state, `qmake6 open-woday.pro && make` configures and compiles
   cleanly with no errors and no new compiler warnings; no CMake and no new
   build tooling are added.
2. Launch the binary: the Phase 3 three-panel frame still opens with the Today
   page in the centre panel; fold order, native chrome and modifier labels are
   unchanged.
3. Review-check the diff against the constraints: no webview, no network
   classes/runtime network access, no plugin/scripting engine, no upstream
   assets/strings, no Phase 5–12 UI (Task CRUD detail pane, block editor,
   to-do→Task links, Inbox rail, Quick Add, CSV export, packaging), no schema
   rewrite or new migration, no test framework / CI / unit seams.
4. Confirm canonical `GLOSSARY.md` vocabulary (avoid "to-do" for a Task,
   "document/page/doc" for a Note, "appointment/meeting" for an Event,
   "daily note" for a Day note, "record/card" for an Item) and that no settled
   decision or absorbed ADR is re-litigated (`mission.md` §13).
5. Confirm the spec carries no schedule or effort estimates (D7 / D9) and that
   this phase is a single runnable / verifiable increment (`roadmap.md` §1).
6. Confirm the spec is recorded as a proposal with maintainer confirmation
   outstanding (spec approval process), that no maintainer sign-off is
   fabricated, and that the spec files are written on the phase branch but
   **not committed** in this step (`/bnvt-commit` owns that).
