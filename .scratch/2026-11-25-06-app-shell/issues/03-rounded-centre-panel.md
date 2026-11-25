# 03: Add the rounded centre panel

**What to build:** A Target user sees a clean white rounded centre panel beside
the Sidebar, giving later pages a coherent surface to render into. The panel sits
on the dark canvas with the shared spacing and radius, while the TopBar continues
to span the frame. It deliberately contains no Today page or other product
content.

**Blocked by:** 02: Add the Sidebar navigation frame

**Status:** ready-for-agent

- [ ] The documented `qmake6 && make` build remains clean with no new warnings,
      no CMake and no new build tooling.
- [ ] Launching still opens exactly one natively decorated window with the
      TopBar spanning the frame and no drawn macOS-style traffic lights.
- [ ] A white centre panel appears beside the Sidebar, uses the shared `panel`
      and `panelText` tokens, and has the shared `radiusPanel` corner radius.
- [ ] The centre panel sits on the dark `canvas` with the shared `gap` spacing
      from the Sidebar and window edges; it does not overlap adjacent surfaces.
- [ ] The panel uses the shared typography tokens and contains no duplicated
      colour, radius, spacing or font values.
- [ ] The centre surface expands to use the available window space at the normal
      launch size while keeping the Sidebar and TopBar correctly positioned.
- [ ] The panel is intentionally clean: no Day strip, summary line, Day note,
      Task/Event lines, check-off, badge chips with live data, block editor or
      fake placeholder content appears.
- [ ] Any text or icon added for the frame is original; no upstream asset or
      verbatim interface string is bundled or shipped.
- [ ] The centre panel remains declarative QML, with no repository access,
      business logic, persistence change or data write.
- [ ] Native chrome, platform-correct modifier hints, shared tokens and the
      root-QML loud-failure guard remain intact.
- [ ] No Item, Task, Note, Event, Tag, Folder, Inbox or Day note data is read or
      rendered.
- [ ] Identifiers and comments use canonical `GLOSSARY.md` vocabulary wherever
      domain concepts appear.
