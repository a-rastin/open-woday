# 03: Task and Event lines with badge chips

**What to build:** The selected day's Task and Event lines render on the Today
page from real Task and Event Items read through the store, so what the Target
user sees reflects stored data. Each line shows badge chips only for the details
that are set — due time, priority and Tag name when present — and priority
colours the Task line's checkbox. A minimal write path through the existing
repositories lets a Task or Event line be placed on a day so the day is demoable
on its own. Switching days in the day strip shows that day's lines. Events stay
lightweight first-class Items with a title, start/end and all-day flag — there
is no calendar grid, reminders or synced-calendar behaviour.

**Blocked by:** 01: Stand up the TodayModel and render the Today page in the centre panel

**Status:** ready-for-agent

- [ ] The selected day's Task and Event lines render from real Task/Event Items
      read through the existing repository, and switching to another day in the
      strip shows that day's lines.
- [ ] Each line shows badge chips only for details that are set — due time,
      priority and Tag name when present — using the shared chip and radius
      tokens; no chip appears for an unset detail.
- [ ] Priority colours a Task line's checkbox so important work stands out.
- [ ] Event lines render from lightweight first-class Event Items (title,
      start/end, all-day) with no calendar grid, reminders or synced-calendar
      behaviour.
- [ ] A minimal write path through the existing repositories can place a Task or
      Event line on a day, making this increment demoable on its own; it is
      deliberately not the full Task lifecycle.
- [ ] No full Task CRUD, rename/edit, scheduling or priority/Tag/Folder/duration
      assignment UI, nested subtasks, detail pane with a "+" badge or
      Task/Event Note pane is added (Phase 5).
- [ ] No `inline_json` formatting ranges (Phase 6), no to-do→Task linking via a
      Block's `task_id` and no embedding of the editor into Task/Event notes
      (Phase 7).
- [ ] The model exposes the day's lines to declarative QML; QML holds no business
      logic and the C++ entry point stays free of product logic.
- [ ] All lines and chips use the single shared token set, and every new string,
      placeholder, empty state and icon is original with no upstream asset,
      string, guide text or screenshot bundled.
- [ ] The build stays clean with no new warnings, no CMake and no new build
      tooling; launching still opens the three-panel frame with the Today page,
      native chrome, fold order and platform-correct modifier labels unchanged.
- [ ] No schema change, migration, new entity, kind, table or index is added;
      no webview, network layer, runtime network access, plugin system or
      scripting engine is added.
- [ ] Identifiers and comments use canonical `GLOSSARY.md` vocabulary, with none
      of the `_Avoid_` terms used for domain concepts.
