# 04: Check-off persistence and a live summary

**What to build:** A Target user checks off a Task line on the Today page and the
completion survives closing and reopening the app. The summary line's counts now
derive from the day's Items in original open-woday strings, so the at-a-glance
view reflects real data. Check-off goes straight through the store, and v0.1's
state model is kept: no In Progress / Waiting state, no cascading subtask
soft-delete and no undo stack. The optional completion sound and "done today"
progress ring are visual polish, not part of this slice.

**Blocked by:** 03: Task and Event lines with badge chips

**Status:** ready-for-agent

- [ ] Each Task line has a checkbox; checking it off sets the Item's
      `completed_at` through the existing repository and the checked state is
      rendered back on the line.
- [ ] Check-off survives a restart against the same local SQLite file — the
      completion state reads back unchanged on relaunch.
- [ ] The summary line's counts are derived from the selected day's Items and
      written in original open-woday strings; the inner UI stays identical
      across platforms.
- [ ] No In Progress / Waiting state, no cascading soft-delete to subtasks and
      no undo stack are introduced; those behavioural rules stay out of scope
      (Phase 5).
- [ ] No completion sound or "done today" progress ring is required for this
      slice (visual polish, Phase 11).
- [ ] The build stays clean with no new warnings, no CMake and no new build
      tooling; launching still opens the three-panel frame with the Today page,
      native chrome, fold order and platform-correct modifier labels unchanged.
- [ ] All new strings are original and all new surfaces inherit the single
      shared token set; the C++ entry point stays free of product logic and QML
      stays declarative.
- [ ] No schema change, migration, new entity, kind, table or index is added;
      no webview, network layer, runtime network access, plugin system,
      scripting engine, test framework or CI configuration is added.
- [ ] Identifiers and comments use canonical `GLOSSARY.md` vocabulary, with none
      of the `_Avoid_` terms used for domain concepts.
