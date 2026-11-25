# 02: One editable Day note per date, with free writing that persists

**What to build:** A Target user's Today page is backed by exactly one Day note
per date. Opening a date for the first time creates one Day note Item — a Note
with a `day` value — and reopening that date reuses it; a second active Day note
for the same date is rejected, and a soft-deleted Day note frees its date for a
replacement. The Target user can write free text around the lines in a minimal
plain-text editing surface, and that writing survives closing and reopening the
app as ordered Blocks. This slice cuts a complete path from the editing surface,
through the model, into the existing store and back out on restart, reusing the
Phase 2 schema with no new entity, kind, table, index or migration.

**Blocked by:** 01: Stand up the TodayModel and render the Today page in the centre panel

**Status:** ready-for-agent

- [ ] Exactly one active Day note Item exists per date: `kind='note'` with a
      `day='YYYY-MM-DD'` value, created on first open of a date through the
      existing repository and read back by day on later launches.
- [ ] Opening a date a second time reuses its existing Day note and never creates
      a duplicate active Day note for that date.
- [ ] The existing one-Day-note-per-date uniqueness rule rejects a second active
      Day note for the same date with a clear diagnostic; it covers only active
      rows, so a soft-deleted Day note frees its date for a replacement.
- [ ] The Day note is a Note Item with a `day` value, not a new entity, kind,
      table, index, migration or database connection path (D17 / D19).
- [ ] Soft-delete remains the only deletion path and no Trash UI is exposed;
      normal reads exclude soft-deleted Day notes.
- [ ] The Day note is editable: a minimal plain-text surface lets the Target user
      write free text around the lines, with no 5-type block editor, inline
      bold/italic/link, `inline_json` formatting ranges, Markdown shortcuts,
      Return/Backspace/Tab behaviour or six-dot handle.
- [ ] Free writing persists as ordered Blocks (plain UTF-8 content and
      contiguous positions) through the existing Block repository, reusing the
      Phase 2 `blocks` table; the note's content is not stored as a JSON blob.
- [ ] Day-note edits survive a restart against the same local SQLite file, and
      all writes go straight to SQLite with no in-memory-only state.
- [ ] No `task_id` to-do→Task linking and no Task/Event Note panes are added
      (Phase 7); the editor is not embedded into Task or Event lines.
- [ ] The build stays clean with no new warnings, no CMake and no new build
      tooling; launching still opens the three-panel frame with the Today page
      and no regression to the earlier slice.
- [ ] Every string, placeholder and empty state shipped by this ticket is
      original, and all new surfaces inherit the single shared token set.
- [ ] No schema change, migration, new entity, kind, table or index is added;
      no webview, network layer, runtime network access, plugin system,
      scripting engine, test framework or CI configuration is added.
- [ ] Identifiers and comments use canonical `GLOSSARY.md` vocabulary for Item,
      Task, Note, Block, Event and Day note, with none of the `_Avoid_` terms
      used for domain concepts.
