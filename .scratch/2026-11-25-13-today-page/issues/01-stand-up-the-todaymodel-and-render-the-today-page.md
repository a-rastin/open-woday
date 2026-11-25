# 01: Stand up the TodayModel and render the Today page in the centre panel

**What to build:** A Target user launches open-woday and sees a Today page in the
centre panel instead of an empty white surface. The page shows a day strip of
roughly ten selectable day cells with today accented, a page title and a summary
line written in original open-woday strings, all inheriting the single shared
token set. Selecting a day changes the selected date. A C++ `TodayModel`
mediates between declarative QML and the Phase 2 store through a list model,
properties and invokable methods, so QML holds no business logic and the C++
entry point stays free of product logic. This is the first complete end-to-end
path — build wiring, model, store and view — and is verifiable on its own: the
build stays clean, the Phase 3 three-panel frame still opens with native chrome,
the Sidebar-then-RightRail fold order and platform-correct modifier labels, and
the Phase 1 loud-failure guard is preserved.

**Blocked by:** None (can start immediately)

**Status:** ready-for-agent

- [ ] The documented `qmake6 && make` build configures and compiles cleanly with
      no errors, no new warnings and no new build tooling; no CMake is added and
      the Qt Quick, Widgets and SQL module declarations and C++ standard are
      unchanged. New QML and any original assets this ticket adds are embedded
      in the application resources.
- [ ] A C++ model/controller for the Today page is exposed to declarative QML
      through a list model, properties and invokable methods, wired to the
      existing Phase 2 store; QML stays declarative and the C++ entry point stays
      free of product logic.
- [ ] Launching opens the Phase 3 three-panel frame unchanged — dark Sidebar,
      spanning TopBar, white rounded centre panel and white rounded RightRail on
      the dark canvas — with native OS decorations, no drawn macOS traffic-light
      controls on Linux, and `Ctrl` labels on Linux/Windows and `⌘` labels on
      macOS.
- [ ] The centre panel now shows the Today page rather than an empty surface: a
      day strip of roughly ten selectable day cells (today accented with the
      shared accent token, cells sized from the shared day-strip token), a page
      title and a summary line.
- [ ] Selecting a day cell changes the selected date and the page reflects it;
      the day strip covers a contiguous range around today.
- [ ] The model reads the selected day's Items through the store so the summary
      line's counts are real derived values (reading zero until Task/Event lines
      exist); no Task/Event line rendering, badge chips, check-off or editable
      Day note is shipped in this slice.
- [ ] Every surface introduced here uses the one shared token set for colours,
      radii, spacing and typography rather than duplicating values.
- [ ] Every string, placeholder, empty state, icon and label shipped by this
      ticket is original; no upstream logo, wordmark, mascot, screenshot, guide
      text or verbatim UI string is bundled or shipped, and internal reference
      material (the screenshots, the guides and the marketing-site design
      reference) remains reference-only and is not treated as the app's UI spec.
- [ ] The Phase 1 root-QML loud-failure guard still fails clearly and non-zero
      without opening a half-working window, and the Phase 3 resize/fold order
      (Sidebar first, then RightRail) is unchanged.
- [ ] No Task/Event lines, badge chips, check-off, editable Day note, block
      editor, Inbox contents, Quick Add, CSV export or other later-phase feature
      is pulled forward; the page renders no fake content or disabled
      placeholder.
- [ ] No schema change, migration, new entity, new kind, new table or new index
      is added; the Phase 2 store, schema version and restart behaviour are not
      regressed.
- [ ] No webview, network layer or runtime network access, plugin system,
      scripting engine, test framework, CI configuration, low-level unit seam or
      packaging script is added.
- [ ] Identifiers and comments use canonical `GLOSSARY.md` vocabulary wherever
      domain concepts appear, with none of the `_Avoid_` terms used for domain
      concepts.
