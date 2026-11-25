# 05: Prove the restart seam locally and verify the phase boundary (merge gate)

**What to build:** A reviewer can confirm Phase 4 is a mergeable, runnable
increment through the single highest seam: `qmake6 && make` produces the binary;
launching it opens the three-panel frame with the Today page, the day strip, the
summary line and the day's Task/Event lines with check-off; writing free text and
checking off a Task, then restarting against the same SQLite file, shows the same
Day note, lines, free writing and completion state rendered again unchanged. The
one framework-free executable persistence seam is extended to cover the
Day-note-per-date wiring and Task/Event/Block round-trips and run locally with
`qmake6` and `make`. A clean build, constraint/diff review, canonical vocabulary,
unedited feature docs (apart from the local-testing amendment) and spec hygiene
are confirmed, and no out-of-scope item leaked in. The roadmap's "Verifiable
when" condition is met.

**Blocked by:** 02: One editable Day note per date, with free writing that persists; 04: Check-off persistence and a live summary (and transitively 01 and 03)

**Status:** ready-for-agent

- [ ] From a clean state, the documented `qmake6 && make` build configures and
      compiles with no errors, no new warnings and no CMake files; `QT += quick
      widgets sql`, C++17, the target name and the resource embedding are
      unchanged.
- [ ] The single framework-free executable persistence seam is extended to cover
      the Day-note-per-date rule and Task/Event/Block round-trips, with no new
      test framework, test doubles, low-level unit seams, UI automation harness
      or CI configuration added; the seam count stays at one.
- [ ] The extended verification is run locally from `tests/` with
      `qmake6 verify-persistence.pro && make -j"$(nproc)" && ./verify-persistence`;
      the build and executable exit zero and the executable prints
      `verify-persistence: PASS`.
- [ ] The launch → observe Today → write → restart → read-back round trip shows
      the Day note, its free writing, the day's Task/Event lines and check-off
      state rendered and read back unchanged against the same local SQLite file.
- [ ] Launching still opens the Phase 3 three-panel frame with native chrome,
      the dark Sidebar, white rounded centre and RightRail, the
      Sidebar-then-RightRail fold order and platform-correct `Ctrl`/`⌘` labels,
      with no macOS traffic lights on Linux; the centre panel shows the Today
      page instead of an empty surface.
- [ ] The Today page shows a day strip of roughly ten selectable cells (today
      accented) and a summary line with original strings; Task and Event lines
      render from real Items with badge chips for set details and priority
      colouring the checkbox; Events render with no calendar grid.
- [ ] Exactly one active Day note Item exists per date; opening a new date
      creates one, reopening reuses it, a second active Day note for the same
      date is rejected, and a soft-deleted Day note frees its date — with no new
      entity, kind, table, index or migration.
- [ ] Checking off a Task sets `completed_at` through the store and survives a
      restart; free writing persists as ordered Blocks with plain UTF-8 content;
      no In Progress / Waiting state, cascading subtask soft-delete or undo stack
      is present.
- [ ] Diff inspection finds no webview of any kind, no `QNetwork*` classes and
      no runtime network access (ADR-0002 / D3), and no plugin system or
      scripting engine.
- [ ] Diff inspection finds no full Task CRUD / detail pane / subtasks
      (Phase 5), no 5-type block editor / inline formatting / `inline_json`
      ranges / six-dot handle (Phase 6), no to-do→Task linking / Task-Event Note
      panes (Phase 7), no Inbox rail / Folders/Tags UI (Phase 8), no Quick Add
      (Phase 9), no CSV export (Phase 10), no visual tuning (Phase 11) or
      packaging (Phase 12).
- [ ] Diff inspection finds no schema rewrite, new migration, new entity / kind
      / table / index or new database connection path, and no CMake files.
- [ ] Diff inspection finds no upstream logo, mascot, UI string, guide text or
      screenshot bundled; all shipped strings, placeholders, empty states and
      icons are original, and the marketing-site design reference is not used as
      the app's UI spec (ADR-0001 / ADR-0004).
- [ ] Diff inspection finds no test framework, CI config, low-level unit seams
      or packaging scripts beyond the single extended persistence seam.
- [ ] Identifiers and comments use canonical `GLOSSARY.md` vocabulary, with none
      of the `_Avoid_` terms used for domain concepts.
- [ ] README, CHANGELOG and roadmap are not edited by Today-page feature work,
      apart from the separate local-testing rule amendment; the documented
      build command still works as written.
- [ ] The spec contains no schedule or effort estimates, re-litigates no
      settled decision or absorbed ADR, and no maintainer sign-off is
      fabricated.
- [ ] No out-of-scope item leaked in: none of Phases 5–12, none of the deferred
      backlog, none of the permanent non-goals. The phase is one runnable /
      verifiable increment proven through the single restart seam rather than a
      non-demoable horizontal layer.
