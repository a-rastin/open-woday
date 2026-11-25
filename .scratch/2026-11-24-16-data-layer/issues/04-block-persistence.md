# 04: Block persistence — CRUD, ordering and cascade

**What to build:** A contributor can persist a Note's content as ordered Blocks
and trust them across a restart. Through the Block repository, Blocks of all five
types can be created, read, updated, deleted and reordered; they read back in the
same order within their owning Item after a restart, with inline formatting
ranges, an optional to-do→Task id and indentation intact; deleting the owning
Item removes its Blocks. Blocks live as rows rather than a JSON blob, so
per-block editing and the future to-do→Task foreign key stay simple. No editor or
inline-formatting UI is built here.

**Blocked by:** 02: Create and read an Item across an app restart

**Status:** ready-for-agent

- [ ] The Block repository creates, reads, updates, deletes and reorders Blocks,
      stored as rows in their own table rather than a JSON blob on the Item.
- [ ] All five Block types are supported with the `CHECK` constraint; a to-do
      Block is a Block type and is not used as a synonym for a Task.
- [ ] `position`, `type`, `content`, `inline_json` (bold/italic/link formatting
      ranges), `task_id` and `indent` round-trip unchanged.
- [ ] Blocks are kept ordered by `position` within their owning Item, and
      positions are re-sequenced on insert, delete and reorder.
- [ ] Deleting an Item cascades to its Blocks; no orphan Block rows remain.
- [ ] A Note's Blocks written and reordered before a restart read back in the
      same order after the connection is reopened — the single persistence seam.
- [ ] All writes use bound values; foreign-key enforcement stays on.
- [ ] The build stays green (`qmake6 && make`, no new warnings, no CMake) and the
      empty window still opens.
- [ ] No block editor, inline-formatting UI, Markdown shortcuts or to-do→Task
      linking UI is built (later phases); no product surface, webview or network
      is added.
- [ ] Canonical `GLOSSARY.md` vocabulary in identifiers and comments; no new test
      seams beyond the single persistence seam.
