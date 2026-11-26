# 05: Nested subtasks and cascading soft-delete

**What to build:** A Target user can break any Task down into nested subtasks (`parent_id`), supporting arbitrary nesting depth in the store and clear nested display on Today and in the detail pane. On Today, parent Tasks show a `done/total` subtask progress badge and can be expanded/collapsed inline to view, complete, open, or add child subtasks—without duplicating nested subtasks as unrelated top-level Today lines. Inside the detail pane, a subtasks section lets the user add, rename, complete/reopen, open, and soft-delete subtasks. Soft-deleting any parent Task atomically sets `deleted_at` across the Task and all of its active descendant subtasks inside a SQLite transaction so orphaned subtasks never remain behind.

**Blocked by:** 01: Task selection, detail pane shell, inline rename, and soft-delete

**Status:** ready-for-agent

- [ ] `ItemRepository` soft-delete atomically sets `deleted_at` on the target Task and all of its active descendant subtasks (`parent_id` chain at arbitrary depth) inside a single database transaction; normal reads exclude all soft-deleted subtasks and no Trash UI is exposed.
- [ ] The Item model supports creating child subtasks (`parent_id` at arbitrary nesting depth), renaming, completing/reopening, navigating into a subtask in the detail pane, and soft-deleting subtasks, synchronized immediately with the Today model.
- [ ] Today renders top-level due Tasks with accurate `done/total` subtask progress badges when subtasks exist, supports inline expand/collapse of nested subtasks indented beneath their parent and inline subtask creation, and does not duplicate nested subtasks as unrelated top-level root rows.
- [ ] The Task detail pane includes a subtasks section showing child subtasks with completion checkboxes, inline rename, click-to-open navigation into any subtask, subtask soft-delete, and an inline input to add a new subtask.
- [ ] Soft-deleting an ancestor Task closes the detail pane if that ancestor or any of its descendants was open, removes the entire subtree from Today immediately, and leaves no active descendant subtasks in SQLite across restarts.
- [ ] Drag-and-drop reordering or re-parenting remains out of scope for Phase 5.
- [ ] The `qmake6 && make` build remains clean with no new warnings; all strings are original, canonical `GLOSSARY.md` vocabulary is used, and no schema change, webview, network call, or Phase 6–12 feature is added.
