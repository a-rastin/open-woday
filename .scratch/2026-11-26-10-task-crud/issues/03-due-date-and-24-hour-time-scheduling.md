# 03: Due date and 24-hour time scheduling

**What to build:** A Target user can set, change, or clear a Task's due date (calendar day) and optional 24-hour due time (`HH:mm`) from the Task detail pane. Setting a calendar day stores `due_precision='day'`; adding a 24-hour time stores `due_precision='time'`; clearing the due date resets both `due_at` and `due_precision` to unset (making the Task undated). Today updates immediately whenever a Task's due date or time changes—adding a Task scheduled onto the selected day, removing a Task moved to another day or cleared to undated, updating Today's summary counts, and displaying a due-time badge chip on Today rows and in the detail pane when timed.

**Blocked by:** 01: Task selection, detail pane shell, inline rename, and soft-delete

**Status:** ready-for-agent

- [ ] A Task's due date can be set to any ISO 8601 calendar day (`due_precision='day'`), edited to another day, or cleared to `null` (`due_at` and `due_precision` unset) through the Item model and persisted directly to SQLite.
- [ ] An optional 24-hour time (`HH:mm`, `due_precision='time'`) can be added to a scheduled Task, edited, or cleared back to day-only (`due_precision='day'`) with input validation.
- [ ] The detail pane shows a due date/time badge only when a due date is set (allowing edit or clear on click) and offers `Due date` in the `"+"` badge menu when the Task is undated.
- [ ] Timed Tasks (`due_precision='time'`) render a 24-hour due-time badge on their Today row and in the detail pane; day-only Tasks on Today omit redundant date text unless timed.
- [ ] Rescheduling a Task onto or off Today's selected date (or clearing its due date) immediately updates Today's Task list and summary counts via model synchronization, while leaving the detail pane open unless closed or deleted.
- [ ] Week, month, and Someday precision pickers, recurring schedules, and reminders/notifications remain out of scope for Phase 5.
- [ ] The `qmake6 && make` build remains clean with no new warnings; all strings are original, canonical `GLOSSARY.md` vocabulary is used, and no schema change, webview, network call, or Phase 6–12 feature is added.
