# 04: Complete the three-panel frame with the RightRail

**What to build:** A Target user sees the complete Reference UI frame: a dark
Sidebar, a spanning TopBar, a white rounded centre panel and a white rounded
RightRail on the dark canvas. The RightRail provides the future Inbox/detail
surface but remains static and empty of live Items. At this point the window
reads as the open-woday shell in its original visual language.

**Blocked by:** 03: Add the rounded centre panel

**Status:** ready-for-agent

- [ ] The documented `qmake6 && make` build remains clean with no new warnings,
      no CMake and no new build tooling; new QML and original assets are embedded
      in the application resources.
- [ ] Launching opens exactly one window showing all four frame regions: a
      spanning TopBar, dark Sidebar, white rounded centre panel and white rounded
      RightRail on the dark canvas.
- [ ] The RightRail uses the shared `panel` and `panelText` tokens and the shared
      `radiusPanel` corner radius, with consistent shared `gap` spacing from the
      centre panel and window edges.
- [ ] The centre panel remains legible and correctly sized between the Sidebar
      and RightRail at the normal launch size, with no overlap or clipped frame.
- [ ] All frame surfaces continue to use the single shared token and typography
      foundation; no panel introduces one-off colours, radii, spacing or fonts.
- [ ] The RightRail contains no live Inbox Items, grouping, quick-add line, Task
      detail or Note content; any static heading or empty-state copy is original
      and clearly frame-only.
- [ ] Native OS decorations remain in use, no macOS-style traffic lights are
      drawn on Linux, and modifier hints remain `Ctrl` on Linux/Windows and `⌘`
      on macOS.
- [ ] Every string and icon is original; no upstream logo, wordmark, mascot,
      screenshot, guide text or verbatim UI string is bundled or shipped.
- [ ] The frame remains declarative QML, with no repository access, business
      logic, persistence change or data write.
- [ ] No Today page, Day strip, Day note, Task/Event lines, check-off, block
      editor, Quick Add, CSV export or other later-phase feature is added.
- [ ] The root-QML loud-failure guard still fails clearly and non-zero without
      opening a half-working window.
- [ ] The visible frame meets the Phase 3 merge-gate appearance; exact visual
      tuning remains deferred to the visual-tuning phase.
