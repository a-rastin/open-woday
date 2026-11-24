# Phase 2 — data-layer — Plan

> **Spec:** `dev-docs/specs/2026-11-24-16-data-layer/` · **Branch:** `phase-2-data-layer`
> Numbered task groups in dependency order. `/bnvt-ticket` turns these into
> tickets. Uses `GLOSSARY.md` vocabulary; respects `mission.md`, `tech-stack.md`
> and the absorbed ADRs. **No schedule or effort estimates** (D7 / D9).
>
> Concrete targets below come from `roadmap.md` Phase 2 deliverables and
> `tech-stack.md` §§5.2, 6.2, 7.1–7.3. `validation.md` holds the checks that
> prove each group is done. Phase 2 is the **local persistence foundation** and
> deliberately ships no product surface: it is a runnable / verifiable increment
> proven through the repository/database seam, not through the UI.

## 1. Build wiring and store-layer layout

1. Extend `open-woday.pro` with the new C++ store sources/headers so the project
   still configures and compiles with `qmake6 && make`; keep `QT += quick widgets
   sql`, `CONFIG += c++17`, `TARGET = open-woday`, `RESOURCES += resources.qrc`.
   **No CMake** is added (not installed; `tech-stack.md` §4 / §11).
2. Add the `src/store/` layer per `tech-stack.md` §5.2: `Database.{h,cpp}`,
   `ItemRepository.{h,cpp}`, `BlockRepository.{h,cpp}` and `schema.sql`. Keep
   QML views declarative and the C++ entry point free of business logic
   (`tech-stack.md` §5.1); the store layer is C++ only.
3. Keep the Phase 1 launch behaviour intact: the app still opens its single empty
   window. Do **not** add panels, tokens, pages or any QML product UI in this
   phase.
4. Add no network layer, plugin system, scripting engine, webview or web-based
   editor library (D3, D11, ADR-0002; `tech-stack.md` §5.1 / §12).

## 2. Database connection and first-launch creation

1. Open a `QSqlDatabase` connection using the **SQLite** driver, backed by one
   file in the platform app-data directory (e.g.
   `$XDG_DATA_HOME/open-woday/open-woday.db` on Linux; `tech-stack.md` §7.1).
2. Create the app-data directory and database file when absent; open the
   connection on startup. If the database cannot be opened or created, fail
   loudly with a clear error and a non-zero exit code rather than a silent,
   half-working app (mirroring the Phase 1 QML load guard).
3. Enable SQLite foreign-key enforcement and use bound values in `QSqlQuery`
   for all writes; never build SQL by string-concatenating user text.
4. Confirm there is **no runtime network access**, no account, no cloud and no
   sync — "no cloud" is a constraint, not a deferred feature (D3, D21).

## 3. The full schema

1. Define all seven tables from `tech-stack.md` §7.2 in `schema.sql`:
   `items`, `blocks`, `folders`, `tags`, `item_tags`, `links`, `meta`. Keep the
   schema in one versioned SQL source rather than scattering DDL through C++.
2. `items`: `id`, `kind` with `CHECK (kind IN ('task','note','event'))`, `title`,
   `created_at`, `modified_at`, `completed_at`, `deleted_at`, `due_at`,
   `due_precision` with its `CHECK`, `start_at`, `end_at`, `all_day`, `priority`
   (0–3), `duration_min`, `parent_id`, `folder_id`, `day`, `sort_order` — with
   the self- and cross-table foreign keys as specified.
3. `blocks`: `id`, `item_id` with `ON DELETE CASCADE`, `position`, `type` with
   `CHECK (type IN ('text','heading','todo','bullet','code'))`, `content`,
   `inline_json`, `task_id`, `indent` — plus `CREATE INDEX blocks_by_item ON
   blocks(item_id, position)`.
4. `folders`, `tags`, `item_tags` (many-to-many, composite primary key, cascade
   deletes), `links` (composite primary key plus its `kind` `CHECK`), and `meta`
   (`key` primary key, `value NOT NULL`).
5. Add `CREATE UNIQUE INDEX one_day_note_per_day ON items(day) WHERE day IS NOT
   NULL` so a Day note is unique per date (`tech-stack.md` §6.3 / §7.2).
6. Verify the schema is created **on first launch only** and is not re-applied
   destructively on later launches.

## 4. Versioned migrations and the `meta` schema version

1. Implement simple **versioned SQL migrations** (`tech-stack.md` §7.1): a
   `Database` component that reads and writes a `schema_version` row in `meta`.
2. Apply migrations in ascending order on first launch up to the current schema
   version; on later launches, skip versions already recorded.
3. Make the version record persistent and monotonically increasing; never drop
   or rebuild tables that hold user data when a later migration runs.
4. Keep the migration mechanism minimal — no migration framework, no external
   dependency, no down-migrations in v0.1.

## 5. ItemRepository — Item persistence

1. Provide create / read / update / soft-delete for **Items** of all three kinds
   (Task, Note, Event) — one `Item` table with a `kind` discriminator (D17;
   `tech-stack.md` §6.2), not separate Task/Note tables.
2. Round-trip every column: timestamps, completion/soft-delete, scheduling
   (`due_at`, `due_precision`, `start_at`, `end_at`, `all_day`), `priority`,
   `duration_min`, `parent_id` (subtasks), `folder_id`, `day`, `sort_order`.
3. Support reads by id and by the queries the later phases need (kind, `day`,
   folder, parent, tag) while keeping this phase to plain CRUD — no Inbox
   grouping, Today page or Task detail pane yet.
4. Implement deletion as a soft-delete via `deleted_at`; expose no hard delete
   and no Trash UI (`tech-stack.md` §10.3). Do not cascade soft-delete to
   subtasks in this phase (that rule is behavioural, Phase 5).
5. Enforce the one-Day-note-per-date rule through the unique index; surface a
   clear error when a duplicate `day` is attempted.

## 6. BlockRepository — Block persistence

1. Provide create / read / update / delete / reorder for **Blocks**, stored as
   rows (not a JSON blob) so a to-do Block can later carry a `task_id` foreign
   key and per-block editing stays simple (`tech-stack.md` §7.3).
2. Round-trip `position`, `type`, `content`, `inline_json` (bold / italic / link
   ranges), `task_id` and `indent`; keep the five-type `CHECK`.
3. Keep Blocks ordered by `position` within their owning Item and re-sequence
   positions on insert/delete/reorder.
4. Let `ON DELETE CASCADE` remove an Item's Blocks; do **not** build the block
   editor, inline formatting UI, Markdown shortcuts or to-do→Task linking here
   (Phases 6–7).

## 7. Persistence verification — the restart seam

1. Exercise the single highest seam (see `validation.md`): create Items and
   Blocks through the repositories, close the `QSqlDatabase` connection (or
   relaunch), reopen against the same file, and read everything back unchanged.
2. Confirm the schema is created on first launch, is left intact on reopen, and
   the `meta` `schema_version` survives the restart.
3. Confirm no state lives only in memory: data survives an app restart
   (`roadmap.md` Phase 2 "Verifiable when"; D21).
4. Confirm soft-deleted rows are excluded from normal reads but remain in the
   file.

## 8. Build, constraint and spec-hygiene verification

1. From a clean state, run the documented `qmake6 open-woday.pro && make` on
   Linux with the installed toolchain (Qt 6.12, `qmake6`, `g++`; SQL/SQLite
   driver; no CMake): no errors, no new warnings.
2. Launch the produced binary and confirm the Phase 1 empty window still opens
   cleanly (`tech-stack.md` §4 / §11).
3. Review-check the diff against the constraints: no webview, no network
   classes/runtime network access, no QML panels/tokens/pages, no Task CRUD UI,
   no block editor, no Quick Add, no Inbox rail, no CSV export, no upstream
   assets/strings, no CMake files, no test framework beyond the single seam.
4. Confirm canonical `GLOSSARY.md` vocabulary in identifiers/comments (avoid
   "to-do" for a Task, "document/page/doc" for a Note, "appointment/meeting"
   for an Event, "daily note" for a Day note, "record/card" for an Item).
5. Confirm every item in `validation.md` passes; confirm no out-of-scope item
   (Phases 3–12, the deferred backlog, permanent non-goals) leaked in.
6. Confirm the spec files are written on the phase branch but **not committed**
   in this step (committing is the `/bnvt-commit` stage).
