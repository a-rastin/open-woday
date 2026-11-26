# 01: Task selection, detail pane shell, inline rename, and soft-delete

**What to build:** A Target user clicks any Task row on Today and opens its Task detail pane inside the existing right rail, then closes the pane to return the right rail cleanly to its idle state. Today's Task lines upgrade from bare check-boxes to reusable Task rows built from shared declarative primitives (checkbox, badge, icon button) using the single shared theme token object. From either a Today row or the detail pane, the Target user can rename a Task inline, complete or reopen it, and soft-delete it (removing it immediately from Today and closing the detail pane when the deleted Task was selected). A C++ Item model mediates active Task selection, rename, completion, and soft-delete between declarative QML and the local SQLite store, staying synchronized with the Today model so every change updates Today's lines and summary counts immediately. The detail pane includes a set-only badge row and a `"+"` trigger shell ready for the attribute slices that follow.

**Blocked by:** None (can start immediately)

**Status:** ready-for-agent

- [ ] The documented `qmake6 && make` build configures and compiles cleanly with no errors, no new warnings, and no CMake or new build tooling; `QT += quick widgets sql` and C++17 stay unchanged, and all new QML components/surfaces are embedded in application resources.
- [ ] A C++ Item model mediates active Task selection, inline rename, completion/reopening, and soft-delete against the Phase 2 store, with QML holding no SQL or business logic and the C++ entry point containing only wiring.
- [ ] The Today model stays synchronized with Item model mutations so renaming, completing, reopening, or soft-deleting a Task updates Today's rendered lines and summary counts immediately without switching dates or restarting.
- [ ] Clicking a Task row on Today selects that Task and opens the detail pane inside the right rail; activating its close action clears selection and returns the right rail to its idle empty state.
- [ ] A Task can be renamed inline on its Today row and in the detail pane, completed and reopened from both surfaces, and soft-deleted (`deleted_at` set in SQLite) with immediate removal from Today and automatic detail-pane dismissal if open.
- [ ] The detail pane displays the Task's editable title, checkbox, a badge row reserved for set details (empty until attributes are set), and a `"+"` badge trigger; no empty placeholder chips appear for unset details.
- [ ] Reusable declarative primitives (checkbox, badge, icon button) and the Task row inherit the single shared theme token set; every string, placeholder, empty state, and icon is original to open-woday with no upstream asset or copy.
- [ ] Soft-delete remains the only deletion path with no Trash UI, no undo stack, and no Waiting or In Progress state; the Phase 3 three-panel frame, native OS chrome, Sidebar-then-RightRail fold order, and platform-correct `Ctrl`/`⌘` labels remain intact.
- [ ] No schema change, migration, webview, runtime network access, plugin/scripting engine, test framework, or Phase 6–12 feature (5-type block editor, `blocks.task_id` links, Inbox rail, Quick Add, CSV export) is introduced.
- [ ] Identifiers and comments use canonical `GLOSSARY.md` vocabulary (Item, Task, Note, Block, Event, Tag, Folder, Day note), with none of the `_Avoid_` terms used for domain concepts.
