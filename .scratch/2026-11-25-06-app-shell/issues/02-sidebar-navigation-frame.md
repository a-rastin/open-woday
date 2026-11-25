# 02: Add the Sidebar navigation frame

**What to build:** A Target user sees a dark Sidebar integrated into the
tokenized window, with static original headings for Today, Folders and Tags. The
Sidebar establishes the navigation frame in the Reference UI without presenting
fake Folder or Tag data, disabled destinations or working navigation. It inherits
the shared colours, typography, radii and spacing introduced by the previous
ticket.

**Blocked by:** 01: Establish the original visual foundation and native TopBar

**Status:** ready-for-agent

- [ ] The documented `qmake6 && make` build remains clean with no new warnings,
      no CMake and no new build tooling.
- [ ] Launching still opens exactly one natively decorated window with the
      TopBar spanning the frame and no drawn macOS-style traffic lights.
- [ ] A dark Sidebar appears on the left of the window beneath the TopBar,
      sitting on the dark canvas and using the shared `sidebar`, `sidebarText`
      and `sidebarMuted` tokens.
- [ ] The Sidebar shows static, original section headings for Today, Folders and
      Tags in a clear navigation hierarchy, using canonical `GLOSSARY.md` terms.
- [ ] Sidebar spacing and typography use the shared `gap`, `font`, `bodySize` and
      `titleSize` tokens rather than local one-off values.
- [ ] The Sidebar contains no live Folder, Tag or Item data and no disabled or
      placeholder destinations for deferred pages; it does not imply that
      navigation or filing already works.
- [ ] Any icon used in the Sidebar is original; no upstream asset or verbatim
      interface string is bundled or shipped.
- [ ] The Sidebar remains declarative QML, with no repository access, business
      logic, persistence change or data write.
- [ ] Native chrome, platform-correct modifier hints and the root-QML
      loud-failure guard from ticket 01 remain intact.
- [ ] No Today page, Day strip, Day note, Task/Event lines, Inbox contents, Quick
      Add, block editor or other later-phase feature is added.
- [ ] Identifiers and comments use canonical `GLOSSARY.md` vocabulary.
