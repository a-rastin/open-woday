# 03: Full Item persistence — every kind, field, query and soft-delete

**What to build:** A contributor can work with any Item through one consistent
persistence API. Any Item — Task, Note or Event — can be created, read, updated
and soft-deleted with every field intact, queried by the dimensions later phases
need (kind, day, folder, parent, tag), and read back unchanged after a restart.
Deletion is a soft-delete: soft-deleted rows disappear from normal reads but
remain in the file, and the one-Day-note-per-date rule is enforced with a clear
error rather than silently. All of this lands as one slice and is demonstrable
through the same write → restart → read-back seam; no Inbox grouping, Today page
or Task detail pane is pulled forward.

**Blocked by:** 02: Create and read an Item across an app restart

**Status:** ready-for-agent

- [ ] The Item repository creates, reads, updates and soft-deletes Items of all
      three kinds (Task, Note, Event) through one Item table with a `kind`
      discriminator — no separate Task, Note, DayNote, View or Habit entity.
- [ ] Every Item column round-trips unchanged: timestamps, completion,
      soft-delete, scheduling (`due_at`, `due_precision`, `start_at`, `end_at`,
      `all_day`), `priority`, `duration_min`, `parent_id` (subtasks), `folder_id`,
      `day` and `sort_order`.
- [ ] Items can be read by id, kind, `day`, folder, parent and tag — the query
      shapes later phases need — while staying plain CRUD, with no Inbox grouping,
      Today page or Task detail pane yet.
- [ ] Deletion is a soft-delete via `deleted_at`; no hard delete and no Trash UI
      are exposed.
- [ ] Soft-deleted rows are excluded from normal reads while remaining in the
      file, so a later CSV export can honour "soft-deleted rows excluded".
- [ ] Inserting a second Day note for the same `day` is rejected with a clear
      error through the partial unique index.
- [ ] Soft-delete is not cascaded to subtasks in this phase (that rule is
      behavioural and belongs to a later phase).
- [ ] All of the above hold across a restart: a representative set of Items of
      all three kinds, with all fields set, reads back unchanged after the
      connection is reopened.
- [ ] All writes use bound values; foreign-key enforcement stays on; no SQL is
      built by concatenating user text.
- [ ] The build stays green (`qmake6 && make`, no new warnings, no CMake) and the
      empty window still opens.
- [ ] Canonical `GLOSSARY.md` vocabulary in identifiers and comments; no product
      UI, webview, network, or later-phase work pulled forward; no new test seams
      beyond the single persistence seam.
