# 05: Fold the Sidebar first, then the RightRail, on resize

**What to build:** A Target user can narrow the open-woday window and the frame
adapts without sacrificing the centre surface: the Sidebar folds first, then the
RightRail folds as the window narrows further, and the centre panel remains
usable. Widening the window restores a coherent frame without changing the
static-shell scope or visual language.

**Blocked by:** 04: Complete the three-panel frame with the RightRail

**Status:** ready-for-agent

- [ ] The documented `qmake6 && make` build remains clean with no new warnings,
      no CMake and no new build tooling.
- [ ] At the normal launch size, the complete three-panel frame remains visible
      and correctly composed.
- [ ] As the window narrows, the Sidebar folds or hides before the RightRail.
- [ ] As the window narrows further, the RightRail folds or hides while the
      centre panel remains visible and usable.
- [ ] At each resize stage, the TopBar continues to span the frame and panels do
      not overlap, clip or leave the centre surface unusable.
- [ ] Widening the window restores the panels coherently without losing the
      Sidebar-then-RightRail folding behaviour.
- [ ] Folding preserves the shared tokens, typography, native OS decorations,
      absence of drawn traffic lights and platform-correct modifier hints.
- [ ] Resize behaviour is declarative presentation logic; it adds no repository
      access, business logic, persistence state or live planner data.
- [ ] No new navigation, rail toggle, Quick Add, editor or other later-phase
      behaviour is introduced merely to support folding.
- [ ] Manual build-and-launch checks at representative wide and narrow window
      sizes confirm the fold order and centre-panel usability without adding a
      UI test framework or lower-level seam.
- [ ] The root-QML loud-failure guard and clean window-close behaviour remain
      intact.
- [ ] Identifiers and comments use canonical `GLOSSARY.md` vocabulary wherever
      domain concepts appear.
