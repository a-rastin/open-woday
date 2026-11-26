# 04: Multi-Tag attachment and optional Folder filing

**What to build:** A Target user can attach one or more Tags to a Task (selecting existing Tags or creating new ones on the fly), detach Tags, and file a Task into a single optional Folder (selecting an existing Folder, creating a new Folder, or clearing it). Store-layer repositories back `folders`, `tags`, and `item_tags` using the existing Phase 2 schema with no DDL change or migration. Attached Tags render as pastel `TagChip`s on Today rows and in the detail pane (with a remove affordance in the detail pane), and an assigned Folder renders as a Folder badge when set, with `Folder` offered under `"+"` when unset and `Add tag` always available under `"+"`.

**Blocked by:** 01: Task selection, detail pane shell, inline rename, and soft-delete

**Status:** ready-for-agent

- [ ] Store-layer persistence supports creating and listing Folders (`folders`), creating or looking up by unique name and listing Tags (`tags`), attaching and detaching Tags on an Item (`item_tags`), and reading all Tags for an Item using bound SQL queries and existing error-reporting conventions—with **no schema rewrite or SQL migration**.
- [ ] The Item model exposes Folder and Tag listing, creation, assignment/clearing (`folder_id`), and Tag attachment/detachment (`item_tags`) to declarative QML, synchronized immediately with the Today model.
- [ ] A reusable `TagChip` primitive displays attached Tag names (inheriting the shared theme tokens) on Today Task rows and in the detail pane's badge row, with a clear remove affordance in the detail pane.
- [ ] The detail pane renders a Folder badge only when a Folder is assigned (clickable to change or clear `folder_id`), offers `Folder` in the `"+"` menu when unset, and always keeps `Add tag` available in the `"+"` menu so multiple Tags can be attached or created.
- [ ] Today Task rows reflect attached Tag chips and Folder metadata immediately upon edit and persist across restarts against the same SQLite file.
- [ ] Sidebar Folder/Tag navigation lists, dedicated Folder/Tag pages, Inbox rail grouping, and drag-to-file remain out of scope (Phase 8).
- [ ] The `qmake6 && make` build remains clean with no new warnings; all strings and icons are original, canonical `GLOSSARY.md` terms (Tag, Folder, Task, Item) are used without `_Avoid_` synonyms, and no webview, network access, or Phase 6–12 feature is added.
