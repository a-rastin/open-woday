# 05: Restart survival and phase-boundary verification (merge gate)

**What to build:** A reviewer can confirm the Phase 2 increment is mergeable.
Exercising the single high-level persistence seam shows that a representative set
of Items of all kinds and Blocks of all five types survives a restart unchanged;
the schema is created on first launch and left intact on reopen; the `meta` schema
version survives; no state lives only in memory; and soft-deleted rows are
excluded from normal reads but remain in the file. The build stays green, the
empty window still opens, and a diff inspection confirms every constitutional and
phase boundary held with canonical domain vocabulary. The roadmap's "Verifiable
when" gate is met and nothing out of scope leaked in.

**Blocked by:** 03: Full Item persistence; 04: Block persistence (and transitively 01 and 02)

**Status:** ready-for-agent

- [ ] Write a representative set of Items (all three kinds) and Blocks (all five
      types), close the connection or relaunch, reopen the same file, and confirm
      everything reads back unchanged — data survives an app restart.
- [ ] Confirm the schema is created on first launch and left intact on reopen, and
      the `meta` `schema_version` survives the restart; already-applied migrations
      are skipped and no destructive rebuild occurs.
- [ ] Confirm no state lives only in memory and soft-deleted rows are excluded
      from normal reads while remaining in the file.
- [ ] From a clean state, `qmake6` and `make` exit `0` with no errors, no new
      warnings and no CMake files; the documented build command still works as
      written.
- [ ] Launching the binary still opens the single empty native window from Phase 1
      and exits cleanly on close (no regression).
- [ ] Diff inspection finds no webview of any kind, no network classes and no
      runtime network access — ADR-0002 / D3.
- [ ] Diff inspection finds no three-panel shell, tokens, pages, Today page, Day
      note rendering, Task CRUD UI, detail pane, block editor, inline-formatting
      UI, to-do→Task linking, Inbox rail, Folders/Tags UI, Quick Add or CSV
      export — the Phases 3–10 boundaries.
- [ ] Diff inspection finds no upstream logo, mascot, UI string, guide text or
      screenshot bundled; all shipped strings and assets are original —
      ADR-0001 / ADR-0004.
- [ ] Diff inspection finds no test framework, CI config, low-level unit seams or
      packaging scripts beyond the single persistence seam, and no CMake files.
- [ ] Identifiers and comments use canonical `GLOSSARY.md` vocabulary, with none
      of the `_Avoid_` terms used for domain concepts.
- [ ] README, CHANGELOG and roadmap are not edited in this phase; the spec
      contains no schedule or effort estimates and re-litigates no settled
      decision or absorbed ADR — D7 / D9.
- [ ] No out-of-scope item leaked in: none of Phases 3–12, none of the deferred
      backlog, none of the permanent non-goals.
- [ ] The "Verifiable when" gate is met (schema created on first launch,
      repositories can CRUD Items/Blocks, data survives an app restart), and the
      increment is runnable / verifiable on its own rather than a non-demoable
      horizontal layer.
