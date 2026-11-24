# 01: Stand up the durable local database

**What to build:** A Target user who launches open-woday on a fresh device gets a
local planner that can remember things: on first launch the app automatically
creates its single local SQLite file in the platform app-data directory, with the
complete v0.1 schema and a recorded schema version. Reopening the app reuses
exactly the same file, leaves the schema intact, and does not re-run migrations
destructively. If the database cannot be created or opened, startup fails loudly
with a clear diagnostic and a non-zero exit code rather than leaving a silently
half-working planner. The existing empty window still opens and closes cleanly,
and at no point is a network, account, cloud or sync involved.

For a contributor, the full seven-table schema lives in one versioned SQL source
and migrations are simple, ordered and tracked in `meta` — this is the foundation
every later slice builds on, and it is verifiable on its own through the
launch/restart behaviour without shipping any product UI.

**Blocked by:** None (can start immediately)

**Status:** ready-for-agent

- [ ] The documented `qmake6 && make` build still configures and compiles with no
      errors and no new warnings, reusing the existing Qt Quick, Widgets and SQL
      module declarations and C++ standard; no CMake build files are introduced.
- [ ] The new store-layer C++ sources are part of the build; QML views stay
      declarative and the C++ entry point stays free of product logic, with no
      network layer, plugin system, scripting engine or webview added.
- [ ] Launching on a fresh app-data location creates exactly one SQLite file in
      the platform app-data directory, creating its parent directory when absent.
- [ ] The seven v0.1 tables are created on first launch — `items`, `blocks`,
      `folders`, `tags`, `item_tags`, `links` and `meta` — with the specified
      columns, `CHECK` constraints on `items.kind`, `items.due_precision`,
      `blocks.type` and `links.kind`, foreign keys with the specified cascade
      deletes, composite primary keys, the Blocks-by-Item index and the partial
      unique index enforcing one Day note per date.
- [ ] The full schema is defined once in a single versioned SQL source; C++ does
      not scatter DDL, and no separate Task, Note, DayNote, View or Habit entity
      is introduced.
- [ ] A `meta` row records the current schema version; migrations are versioned
      and applied in ascending order on first launch.
- [ ] On a second launch, already-applied versions are skipped, the schema is not
      destructively rebuilt, and the recorded version is unchanged; no migration
      drops or rebuilds a table holding user data.
- [ ] If the database cannot be opened or created, startup exits non-zero with a
      clear diagnostic rather than a silently degraded app, and the empty window
      does not appear half-working.
- [ ] SQLite foreign-key enforcement is enabled and all writes use bound values;
      no SQL is assembled by concatenating user text.
- [ ] There is no runtime network access, no account, no cloud and no sync —
      "no cloud" is a constraint, not a deferred feature.
- [ ] Launching the binary still opens the single empty native window from Phase 1
      and exits cleanly on close, with no regression.
- [ ] The increment is verifiable on its own (file, schema and version survive a
      restart) without shipping a non-demoable horizontal layer or any product
      surface: no panels, tokens, pages or QML product UI.
- [ ] Identifiers and comments use canonical `GLOSSARY.md` vocabulary (Item, Task,
      Note, Event, Block, Tag, Folder, Day note), with none of the `_Avoid_` terms
      used for domain concepts.
