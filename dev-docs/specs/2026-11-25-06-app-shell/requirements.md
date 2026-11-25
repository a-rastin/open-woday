# Phase 3 — app-shell — Requirements

> **Spec:** `dev-docs/specs/2026-11-25-06-app-shell/`
> **Branch:** `phase-3-app-shell` · **Phase:** 3 (app-shell) · **Status:** planned
> **Authority:** `dev-docs/mission.md`, `dev-docs/tech-stack.md`,
> `dev-docs/roadmap.md`, `GLOSSARY.md`.
> **Produced by:** the `/to-spec` skill, adapted to the repo's three-file spec
> layout (`requirements.md` / `plan.md` / `validation.md`). No interview was
> conducted; this is a synthesis of the settled constitution and the current repo
> state, and it is a **proposal** awaiting maintainer confirmation (see the
> "Seam decision — please confirm" section of `validation.md`). Implementation
> decisions are stated in durable terms (no code snippets and no file paths, per
> the `/to-spec` skill); concrete file targets and commands live in `plan.md`,
> and the checks that prove success live in `validation.md`. No schedule or effort
> estimates are given (D7, D9); order is by dependency.

## Problem Statement

From the Target user's and contributor's perspective: **open-woday now launches,
remembers data, but shows no product surface.** Phase 1 delivered a buildable,
runnable Qt 6 Quick skeleton that opens one empty window; Phase 2 added the local
persistence foundation (a SQLite file created on first launch, with Item and
Block repositories) proven through the restart seam. Neither phase renders the
Reference UI. A Target user who opens the planner sees a single blank window:
there is no Sidebar, no TopBar, no RightRail, no visual language, no panel
tokens and no resize behaviour. Every later v0.1 increment — the Today page,
Task CRUD, the block editor, the Inbox rail, Quick Add and CSV export — needs a
window to render into and a shared set of visual tokens to inherit. Before any
page can be shown, the app must provide the three-panel frame and the tokens that
make it recognisably the open-woday window, using only original strings and
assets.

## Solution

Ship the **reference three-panel window and visual tokens** as the next runnable,
verifiable increment: a `Sidebar`, `TopBar` and `RightRail` QML composition on
the dark canvas, with white rounded centre and right-rail panels, a shared panel
token set (`canvas`, `sidebar`, `panel`, chips, radii, font), correct
resize/fold behaviour and native OS chrome. The increment is deliberately a
**shell only**: it renders the frame and its visual language, not the Today page,
Task/Event lines, the Day note, the block editor or the Inbox contents. All
strings, icons and assets are written fresh for open-woday; `logo/` and
`screenshots/` stay internal reference material and are never bundled. The
increment is proven through a single, high-level seam: build, launch and observe
the window.

## Context

- **Where this phase comes from.** `roadmap.md` §1 defines *the next phase* as the
  lowest-numbered phase whose status is not `done`, to be read with the roadmap's
  own intent ("update the status as work lands"; one spec → one branch → one
  mergeable increment). Phase 1 — project-skeleton and Phase 2 — data-layer have
  demonstrably landed: both were specified, ticketed, implemented, reviewed
  (`.code-review/`), documented (`CHANGELOG.md`) and merged to `main` (PR #1 and
  PR #4). The `Status:` fields in `roadmap.md` were **not** advanced (the
  `/bnvt-documentation` step maintains only `README.md` and `CHANGELOG.md`), so a
  purely mechanical reading of those stale fields would still name Phase 2, whose
  `Status:` remains `in-progress`. Re-speccing Phase 2 would duplicate a spec,
  five tickets, a review and merged code that already exist. Following the
  precedent set by the Phase 2 spec (its "Where this phase comes from" note), the
  next increment on intent is therefore **Phase 3 — app-shell**, `Branch:`
  `phase-3-app-shell`. **This is a proposal, not maintainer sign-off:** if the
  maintainer intends a strict status-field reading (advance Phase 2 to `done`
  first, or re-target), say so before `/bnvt-ticket`; otherwise this reading
  stands.
- **Current codebase state.** A `qmake6` Qt 6 Quick / QML + C++ skeleton exists
  (one C++ entry point, one empty `Window` titled `open-woday`, Qt resource
  embedding) plus the `src/store/` persistence layer from Phase 2. There is no
  panel composition, no token set, no sidebar/top bar/right rail, and no product
  UI. `resources.qrc` embeds the root QML component.
- **Toolchain (factual, `tech-stack.md` §4).** `qmake6`, Qt 6.12 (`Qt6Quick`,
  `Qt6Widgets`, `Qt6Core`), the SQL/SQLite driver and `g++` are present; `cmake`
  is **not** installed. The build stays `qmake6 open-woday.pro && make`.
- **Spec authority (`mission.md` §6, ADR-0004).** The three in-app `screenshots/`
  are the visual source of truth; `dev-docs/app-document.md` (the 134 guides) is
  the behavioural source; `dev-docs/DESIGN.md` is the marketing site's reference,
  **not** this app's UI spec. Contradictions are resolved here. None of these
  are copied verbatim into the shipped product.

### Constitutional constraints governing this phase

| Constraint | Source |
|---|---|
| Original name & assets only; never bundle upstream logos, wordmark, mascot, UI strings, guide text or screenshots | `mission.md` §5 (ADR-0001), `tech-stack.md` §8.4 |
| Visual similarity is reached through layout and visual language, not copied brand elements or strings | `mission.md` §5, §6 (ADR-0004) |
| Use `GLOSSARY.md` vocabulary; do not re-litigate settled decisions or absorbed ADRs | `mission.md` §3 (principle 9), §13 |
| Native OS chrome; no drawn macOS traffic lights on Linux; platform-correct `Ctrl`/`⌘` labels | `tech-stack.md` §8.5 (D15) |
| No webview, no web-based editor, no network layer | `tech-stack.md` §3, §5.1 (ADR-0002, D3) |
| No schedule or effort estimates | D7 / D9 |
| Each phase is one runnable / verifiable increment | `roadmap.md` §1 |
| `logo/`, `screenshots/`, `app-document.md`, `DESIGN.md` remain reference material only | `mission.md` §5, §6 |

## Scope

In scope: the three-panel frame (`Sidebar`, `TopBar`, `RightRail`), the shared
panel token set, resize/fold behaviour, native chrome and platform-correct
modifier labels, and original placeholder/empty-state strings and icons for the
frame itself. The shell renders no live Item, Task, Note, Event, Tag, Folder or
Inbox data.

## User Stories

1. As a Target user, I want the app to open a window with a dark Sidebar, a white rounded centre panel and a white rounded RightRail, so that the window reads as the open-woday Reference UI.
2. As a Target user, I want a TopBar spanning the window, so that the frame matches the reference layout.
3. As a Target user, I want panels to sit on a dark canvas with consistent gaps and rounded corners, so that the visual language is coherent.
4. As a Target user, I want the Sidebar to show section headings (Today, Folders, Tags) as static, original labels, so that the navigation frame is present without shipping a fake, empty shell.
5. As a Target user, I want the centre panel to be a clean white rounded surface, so that later pages have a place to render.
6. As a Target user, I want the RightRail to be a white rounded card, so that the Inbox/detail surface has a frame to fill later.
7. As a Target user, I want the window to use native OS decorations, so that it behaves like a normal Linux desktop app.
8. As a Target user, I want no drawn macOS traffic-light controls on Linux, so that the deliberate deviation (D15) holds.
9. As a Target user, I want `Ctrl`-based labels on Linux/Windows and `⌘`-based labels on macOS, so that modifier hints are platform-correct.
10. As a Target user, I want the window to resize by folding the Sidebar first and then the RightRail, so that the centre panel stays usable.
11. As a Target user, I want every string, placeholder and empty state in the frame to be written fresh for open-woday, so that no upstream UI string is reused.
12. As a reviewer, I want no upstream logo, wordmark, mascot, screenshot or guide text bundled or shipped, so that the legal/IP boundary (ADR-0001 / ADR-0004) holds as the UI grows.
13. As a reviewer, I want the visual language reached through layout and tokens rather than copied brand elements, so that the design is visibly distinct and original.
14. As a reviewer, I want `DESIGN.md` treated as a marketing-site reference only, so that it is never mistaken for the app's UI spec.
15. As a reviewer, I want a single shared token set (canvas, sidebar, panel, chips, radii, font), so that later phases inherit one coherent palette.
16. As a reviewer, I want chips/pills with the documented radii and neutral/pastel surfaces, so that later badge chips have a defined base.
17. As a reviewer, I want the Inter font with a system-UI fallback declared once, so that typography is consistent.
18. As a reviewer, I want no schedule or effort estimates anywhere in the spec, so that D7 / D9 are respected.
19. As a reviewer, I want canonical `GLOSSARY.md` vocabulary in identifiers and comments wherever domain concepts appear, so that later code stays consistent.
20. As a reviewer, I want no Today page, Day note, Task/Event lines, check-off, block editor, Inbox contents, Quick Add parsing or CSV export in this phase, so that Phases 4–10 own their work cleanly.
21. As a reviewer, I want no new persistence behaviour, schema change or repository change in this phase, so that Phase 2 owns the data layer cleanly.
22. As a reviewer, I want the C++ entry point to stay free of business logic and the QML to stay declarative, so that `tech-stack.md` §5.1 holds.
23. As a reviewer, I want no webview, network layer, plugin system or scripting engine added, so that ADR-0002 / D3 / D11 hold.
24. As a reviewer, I want no CMake and no rework of the build baseline, so that `tech-stack.md` §4 / §11 hold.
25. As a reviewer, I want no new unit-test framework or low-level seams introduced prematurely, so that the single-seam rule (`tech-stack.md` §12) is kept.
26. As a reviewer, I want the existing empty-window launch behaviour to remain the fallback if the root QML cannot be created, so that the Phase 1 loud-failure guard is preserved.
27. As a maintainer, I want the spec to state plainly that this is a proposal awaiting confirmation, so that no maintainer sign-off is implied.

## Implementation Decisions

- The three-panel frame is a declarative QML composition: a `Sidebar` on the left, a `TopBar` across the top, a `RightRail` on the right and a centre panel between them, on a dark `canvas`. The root `Window` keeps native OS decorations.
- One shared token set is defined once and reused by all panels: `canvas`, `sidebar`, `sidebarText`, `sidebarMuted`, `panel`, `panelText`, `muted`, `chip`, `accent`, `radiusPanel`, `radiusChip`, `radiusControl`, `gap`, `font`, `bodySize`, `titleSize`, `dayStripCell` (`tech-stack.md` §8.3). Starting values are those in §8.3, to be verified in the visual-tuning phase (Phase 11), not borrowed from `DESIGN.md`.
- Resize behaviour folds the `Sidebar` first, then the `RightRail`, keeping the centre panel legible (`tech-stack.md` §8.2 / §9.1).
- Chrome and modifier labels follow §8.5 (D15): native decorations, `Ctrl`/`⌘` shown per platform, no drawn traffic lights on Linux.
- All frame strings, placeholders, empty-state text and icons are original. Reference assets in `logo/`, `screenshots/`, `app-document.md` and `DESIGN.md` are read for layout and visual language only and are never bundled, embedded or shipped.
- The shell is static: it renders the frame and visual language, not live Item/Task/Note/Event/Tag/Folder/Inbox data. No repository, schema, network or business-logic code is added.
- No settled decision or absorbed ADR is re-litigated; a change to the constitution would require an explicit amendment (`mission.md` §13).
- The single verification seam is the externally observable build-and-launch appearance of the window (see `validation.md`); no lower seams, test framework or test doubles are introduced.
- No schedule or effort estimates (D7 / D9).

## Out of Scope

- Today page, Day strip, Day note, Task/Event lines, badge chips with live data, check-off, summary line.
- Task CRUD, the 5-type block editor, to-do-Block→Task links, Notes, Task detail pane.
- Inbox rail contents, Folders/Tags data, drag-to-file, Quick Add parsing, CSV export.
- Any persistence, schema, migration, repository or `Database` change (Phase 2 owns these).
- Any upstream logo, wordmark, mascot, screenshot, guide text or verbatim UI string; `DESIGN.md` as a UI spec.
- Test frameworks, CI configuration, unit seams and packaging scripts (`tech-stack.md` §12).
- Schedule or effort estimates; settled-decision or ADR re-litigation; constitutional amendments.
- Network, sync, accounts, cloud, webview, plugin systems, scripting engines, web-based editors.
- Windows and macOS binaries; those remain later build targets from the backlog.

## Further Notes

- **Approval status.** This spec is a synthesis, not an interview, and carries no
  maintainer sign-off. The seam decision and the next-phase reading are open
  questions for the maintainer to confirm before `/bnvt-ticket` (see
  `validation.md` §"Seam decision — please confirm"). Following the project's
  spec approval process means leaving that confirmation outstanding rather than
  recording an approval that was never given; Phases 1 and 2 were specified the
  same way.
- Phase 3 does **not** satisfy any full v0.1 Definition-of-Done item
  (`mission.md` §10 / `roadmap.md` §3) on its own; those are completed in
  Phases 4–12. Its merge gate is the Phase 3 "Verifiable when" condition.
- Visual tokens are starting values; exact hues, radii and typography are tuned
  in Phase 11 (visual-tuning) against the three screenshots, still using only
  original strings and assets.
