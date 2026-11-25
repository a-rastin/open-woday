# 01: Establish the original visual foundation and native TopBar

**What to build:** A Target user launches open-woday and sees a native desktop
window with a dark canvas and a TopBar spanning the frame. The frame introduces
open-woday's original visual language through one shared set of panel, chip,
radius, spacing and typography tokens, with Inter declared once and a system-UI
fallback. Any modifier hint shown in the frame says `Ctrl` on Linux/Windows and
`⌘` on macOS. This is the visual foundation the Sidebar, centre panel and
RightRail inherit; it renders no live planner data or product page.

**Blocked by:** None (can start immediately)

**Status:** ready-for-agent

- [ ] The documented `qmake6 && make` build configures and compiles cleanly with
      no new warnings, no CMake and no new build tooling; any QML or original icon
      assets added by this ticket are embedded in the application resources.
- [ ] Launching opens exactly one window titled `open-woday`, using the OS's
      native decorations; no macOS-style traffic-light controls are drawn on
      Linux or Windows, and closing the window exits cleanly.
- [ ] The existing loud-failure guard is preserved: if the root QML component
      cannot be created, startup reports a clear error, exits non-zero and opens
      no half-working window.
- [ ] The window has a dark canvas and a TopBar spanning the frame, with original
      frame text and no upstream brand name, mascot or copied interface copy.
- [ ] One shared token set is defined once and contains `canvas`, `sidebar`,
      `sidebarText`, `sidebarMuted`, `panel`, `panelText`, `muted`, `chip`,
      `accent`, `radiusPanel`, `radiusChip`, `radiusControl`, `gap`, `font`,
      `bodySize`, `titleSize` and `dayStripCell`, using the specification's
      starting values.
- [ ] Every frame surface and control introduced here uses the shared tokens
      rather than duplicating colour, radius, spacing or type values.
- [ ] Inter with a system-UI fallback is declared once; body and title text use
      the shared type tokens consistently.
- [ ] Any modifier hint rendered by the frame is platform-correct: `Ctrl` on
      Linux/Windows and `⌘` on macOS. No shortcut binding or Quick Add behaviour
      is implemented in this phase.
- [ ] Every string and icon shipped by this ticket is original; no upstream logo,
      wordmark, mascot, screenshot, guide text or verbatim UI string is bundled,
      and internal reference assets remain reference-only.
- [ ] The C++ entry point stays free of business logic and QML stays declarative;
      the existing database startup path continues to work without changes to
      persistence, schema, migrations or repositories.
- [ ] No live Item, Task, Note, Event, Tag, Folder, Inbox or Day note data is
      rendered, and no Today page, Day strip, editor, Quick Add or other later
      phase feature is pulled forward.
- [ ] Identifiers and comments use canonical `GLOSSARY.md` vocabulary wherever
      domain concepts appear.
