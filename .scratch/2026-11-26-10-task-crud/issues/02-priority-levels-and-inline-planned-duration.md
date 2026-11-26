# 02: Priority levels and inline planned duration

**What to build:** A Target user can assign, edit, or clear a Task's priority (`None`, `Low`, `Medium`, `High`) and inline planned duration (in positive minutes) from the Task detail pane's `"+"` badge menu or by clicking an existing badge. Priority colours the Task's checkbox on both Today rows and the detail pane so urgent work stands out at a glance, while non-zero priority and set duration appear as detail badges on Today rows and in the detail pane's set-only badge row. Clearing priority back to `None` or clearing duration removes the corresponding badge from the row and detail pane and reveals it again under `"+"`, with every change written straight to SQLite and surviving restarts.

**Blocked by:** 01: Task selection, detail pane shell, inline rename, and soft-delete

**Status:** ready-for-agent

- [ ] Priority supports the four integer levels (`0` none, `1` low, `2` medium, `3` high), persisted directly to `items.priority` and `modified_at` in SQLite via the Item model and store.
- [ ] Priority colours the Task checkbox across Today rows and the detail pane using theme-harmonized tones, while `None` uses the neutral muted styling.
- [ ] Non-zero priority renders as a priority badge on Today rows and in the detail pane's badge row; priority `None` renders no priority badge.
- [ ] Inline planned duration supports setting, editing, and clearing a positive integer number of minutes (`items.duration_min`), rendered as a duration badge on Today rows and in the detail pane only when set.
- [ ] The detail pane's `"+"` badge menu offers `Priority` and `Duration` only when they are currently unset on the selected Task, and clicking a set priority or duration badge lets the user change or clear it.
- [ ] Changing priority or duration immediately refreshes Today's Task row via model synchronization and persists across closing and reopening the database.
- [ ] The `qmake6 && make` build remains clean with no new warnings; all new strings and icons are original, canonical `GLOSSARY.md` terms are used, and no out-of-scope Phase 6–12 feature, schema change, webview, or network call is added.
