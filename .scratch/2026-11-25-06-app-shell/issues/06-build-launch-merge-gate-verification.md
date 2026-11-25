# 06: Verify the build-and-launch merge gate and phase boundaries

**What to build:** A reviewer can confirm that Phase 3 is a mergeable, runnable
increment. Building and launching the application shows the original
three-panel frame with its shared visual language, native chrome and required
resize behaviour. A diff and resource audit confirms that the phase stayed a
static shell: no live data, later-phase feature, persistence change, upstream
asset or new verification/tooling layer leaked in. The roadmap's Phase 3
"Verifiable when" gate is met through the single externally observable
build-and-launch seam.

**Blocked by:** 05: Fold the Sidebar first, then the RightRail, on resize

**Status:** ready-for-agent

- [ ] From a clean state, the documented `qmake6 && make` build configures and
      compiles with no errors, no new warnings, no CMake and no new build tooling.
- [ ] Launching the binary opens exactly one natively decorated window and
      closing it exits cleanly.
- [ ] In a controlled check where the root QML component cannot be created,
      startup reports a clear error, exits non-zero and opens no window; the
      Phase 1 loud-failure guard is preserved without a new test harness.
- [ ] The launched window shows a dark Sidebar, a TopBar spanning the frame, a
      white rounded centre panel and a white rounded RightRail on a dark canvas,
      matching the Phase 3 merge-gate appearance in an original visual language.
- [ ] The frame uses the one shared token set, including the shared panel/chip/
      control radii, panel gap and Inter typography with a system-UI fallback.
- [ ] Native OS decorations are used; no macOS-style traffic-light controls are
      drawn on Linux, and any modifier hint is `Ctrl` on Linux/Windows and `⌘`
      on macOS.
- [ ] Resizing folds the Sidebar first and the RightRail second while the centre
      panel stays visible and usable.
- [ ] Every frame string, placeholder, empty state and icon is original; no
      upstream logo, wordmark, mascot, screenshot, guide text or verbatim UI
      string is bundled or shipped, and internal reference material remains
      reference-only.
- [ ] Visual similarity comes from layout and shared tokens rather than copied
      brand elements, and the marketing-site design reference is not treated as
      the application's UI specification.
- [ ] The frame renders no live Item, Task, Note, Event, Tag, Folder, Inbox or Day
      note data, and contains no Day strip, Day note, Task/Event lines, check-off,
      block editor, Inbox contents, Quick Add parsing or CSV export.
- [ ] There is no persistence, schema, migration, repository or database change;
      the existing Phase 2 startup and restart behaviour is not regressed.
- [ ] There is no network, account, cloud, sync, webview, plugin system,
      scripting engine or web-based editor code.
- [ ] The C++ entry point remains free of business logic and QML remains
      declarative.
- [ ] No unit-test framework, UI automation harness, CI configuration,
      low-level seam or packaging script is added; verification remains at the
      single build-and-launch seam.
- [ ] Identifiers and comments use canonical `GLOSSARY.md` vocabulary, no
      settled decision or absorbed ADR is re-litigated, and the work contains no
      schedule or effort estimates.
- [ ] The phase remains one runnable/verifiable increment; exact hues, chip
      colours and typography tuning remain owned by the later visual-tuning
      phase.
