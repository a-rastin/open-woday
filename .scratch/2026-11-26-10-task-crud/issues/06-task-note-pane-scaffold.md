# 06: Task Note pane scaffold

**What to build:** A Target user inspecting a Task in the right-rail detail pane sees a dedicated Note pane scaffold below the badge row and subtasks where they can write and edit plain-text context for that Task. Content is stored as ordered plain-text `blocks` rows on the Task's own `item_id` through `BlockRepository`, using debounced auto-save and flush-on-blur/selection-change/quit persistence plus the same `isPlainText` safety guard as Today's Day note so non-plain-text Blocks are never silently overwritten. Every Task Note edit survives restarting against the same local SQLite file.

**Blocked by:** 01: Task selection, detail pane shell, inline rename, and soft-delete

**Status:** ready-for-agent

- [ ] Selecting a Task in the Item model loads its own Note content from `blocks` rows under `item_id = task.id` via `BlockRepository` and exposes text, editability (`isPlainText` guard), and error feedback properties to `TaskDetailPane`.
- [ ] Editing the Task's Note scaffold inside `TaskDetailPane` persists changes as plain-text `blocks` rows via `BlockRepository` using debounced save and explicit flush on focus loss, Task switch, detail-pane close, and application quit.
- [ ] If a Task's `blocks` contain any non-plain-text Block (`!BlockRepository::isPlainText`), the Note scaffold becomes read-only with a clear original diagnostic so formatted Block metadata cannot be silently stripped.
- [ ] Task Note content persists across closing and reopening the Task detail pane and across restarting against the same SQLite database file, while keeping Day note persistence on Today unaffected.
- [ ] No 5-type block editor, inline bold/italic/link formatting, `inline_json` ranges, Return/Backspace/Tab block splitting, six-dot handle, Markdown shortcuts (Phase 6), or `blocks.task_id` to-do→Task linking (Phase 7) is added.
- [ ] The `qmake6 && make` build remains clean with no new warnings; all strings and placeholders are original, canonical `GLOSSARY.md` vocabulary (Task, Note, Block, Day note) is used, and no schema change, webview, or network call is added.
