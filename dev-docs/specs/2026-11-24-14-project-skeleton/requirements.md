# Phase 1 — project-skeleton — Requirements

> **Spec:** `dev-docs/specs/2026-11-24-14-project-skeleton/`
> **Branch:** `phase-1-project-skeleton` · **Phase:** 1 (project-skeleton) · **Status:** planned
> **Authority:** `dev-docs/mission.md`, `dev-docs/tech-stack.md`, `dev-docs/roadmap.md`, `GLOSSARY.md`.
> **Produced by:** the `/to-spec` skill, adapted to the repo's three-file spec layout
> (`requirements.md` / `plan.md` / `validation.md`).
>
> Implementation decisions below are stated in durable terms (no code snippets and no
> file paths, per the `/to-spec` skill). Concrete file targets and commands live in
> `plan.md` and `validation.md`. No schedule or effort estimates are given (D7, D9);
> order is by dependency.

## Problem Statement

From the Target user's and contributor's perspective: **open-woday cannot yet be
run at all.** The repository holds a settled design (the three constitution files,
the GLOSSARY, the behavioural/visual references) but no application code, no
build definition and no binary. A Target user who cannot run the macOS-only
upstream app still has nothing to launch on Linux; a contributor who clones the
repo cannot configure, compile or open anything; and none of the later v0.1
phases — the data layer, the three-panel shell, the Today page — has a runnable
home to build on. Before any part of the v0.1 main slice can be demonstrated,
there must exist a minimal, buildable application that opens a window on Linux.

## Solution

Ship the smallest runnable **vertical increment**: a `qmake6` project that
configures, compiles and launches a single, empty window on Linux, built with Qt 6
Quick / QML + C++ from one codebase, with no webview. This establishes three
things every later phase extends — the build, the C++/QML layer split, and the
launch seam — while deliberately shipping no product surface. The window is
intentionally empty: no populated pages, no placeholder destinations, no disabled
menu items ("no empty shells", D5 / D12). It uses native OS window decorations and
the original `open-woday` name only. Nothing is persisted, nothing is networked,
and no downstream phase's work is pulled forward.

## Context

- **Where this phase comes from.** `roadmap.md` §1 defines *the next phase* as the
  lowest-numbered phase whose status is not `done`. All twelve phases are
  `planned`, so the next phase is **Phase 1 — project-skeleton**, whose `Branch:`
  line is `phase-1-project-skeleton` and whose goal is "a buildable, runnable Qt 6
  Quick application skeleton".
- **Greenfield.** `tech-stack.md` §12 states there are no tests yet and no
  application code exists; `mission.md` confirms "No implementation has started".
- **Toolchain (factual, `tech-stack.md` §4).** `qmake6`, Qt 6.12
  (`Qt6Quick`, `Qt6Widgets`, `Qt6Core`), the SQL/SQLite driver and `g++` are
  present; `cmake` is **not** installed. The build must therefore use `qmake6`.
- **The three-file workflow.** `/bnvt-spec` (this step) records specs as files
  under `dev-docs/specs/`; `/bnvt-ticket` then breaks `plan.md` into tickets,
  followed by implement → review → revise → documentation → commit → merge.
  Specs are written but **not committed** in this step.
- **Vocabulary.** All text uses `GLOSSARY.md` terms. Avoid: "to-do" for a Task,
  "document/page/doc" for a Note, "appointment/meeting" for an Event, "daily
  note" for a Day note, and the upstream's "Woday"/"Wodie"/mascot names.

### Constitutional constraints governing this phase

| Constraint | Source |
|---|---|
| One codebase, **no webview** (no Electron/Tauri/HTML-rendered shell); Qt Quick uses its own scene graph, which is not a web engine | D11, D13, ADR-0002 (`tech-stack.md` §3) |
| Linux is the primary target; Windows/macOS are later build targets from the same source | D6, D11 |
| Local-first: no server, no account, no sync, **no runtime network access** | D3, D21 (constraint, not a deferred feature) |
| Build with `qmake6` + `make`; **no CMake**; Qt 6 Quick, Widgets and SQL modules | D13, `tech-stack.md` §5.3 / §11 |
| Native OS window chrome; no drawn macOS traffic lights on Linux; inner UI identical across platforms | D15 |
| Original strings/assets only; never bundle upstream logos, mascot, UI strings, the guide scrape or the screenshots | ADR-0001, ADR-0004, `mission.md` §5 / §9 |
| Only populated pages — no empty shells / disabled placeholders | D5, D12 |
| QML views are declarative (no business logic); C++ app/ controllers & models; store/ for persistence; no network layer, plugin system or scripting engine | `tech-stack.md` §5.1 |
| No schedule or effort estimates; order by dependency | D7, D9 |
| Use `GLOSSARY.md` vocabulary; do not re-litigate settled decisions or absorbed ADRs | `mission.md` §3 (principle 9), §13 |

## Scope

**In scope (from `roadmap.md` Phase 1 deliverables):** a `qmake6` project file
declaring the Qt 6 Quick / Widgets / SQL modules; a single C++ entry point; one
empty QML main window; a clean `qmake6 && make` build; a window that opens on
Linux. **Verifiable when:** "the app configures, compiles and opens a window on
Linux."

Everything else is out of scope (below). In particular, Phase 1 does **not**
create the SQLite file, the schema, migrations or repositories — those are
Phase 2 — and it does **not** draw the three-panel shell — Phase 3. Each phase
is one spec → one branch → one mergeable, runnable increment (`roadmap.md` §1);
Phase 1 must not ship a non-demoable horizontal layer.

## User Stories

1. As a Target user, I want to launch open-woday on Linux and see a window, so that I can confirm the planner actually runs on my machine.
2. As a Target user, I want that window to use my OS's native title bar and decorations, so that it looks like a normal Linux application rather than an imitation of macOS.
3. As a Target user, I want the window title to read `open-woday` and nothing else, so that the product is clearly identified as an unaffiliated, original app.
4. As a Target user, I want the app to open the same way on every launch, so that the baseline is dependable before features are added.
5. As a Target user, I want the app to close cleanly when I close the window, so that launching and quitting are correct from the start.
6. As a contributor, I want to clone the repo and build it with one documented command, so that I can get a running app without setup guesswork.
7. As a contributor, I want the build to use `qmake6` and `make` only, so that it works with the toolchain on the maintainer's machine where CMake is not installed.
8. As a contributor, I want `qmake6` configuration to succeed with no errors, so that the build baseline is clean.
9. As a contributor, I want `make` to compile with no errors and no new warnings, so that later phases start from a clean baseline.
10. As a contributor, I want the project to declare Qt 6 Quick, Widgets and SQL modules up front, so that later phases can add QML views, native dialogs and SQLite without reworking the build.
11. As a contributor, I want the QML root component bundled into the binary via the Qt resource system, so that the built app is self-contained and later packaging has nothing external to gather.
12. As a contributor, I want a single C++ entry point that loads one root QML component, so that the application startup path is proven before Phase 3 adds panels.
13. As a contributor, I want startup to fail loudly with a clear message if the root QML component cannot be created, so that a broken QML load is never a silent blank app.
14. As a maintainer, I want the C++ entry point to contain no business logic, so that the layered architecture (QML views / C++ app / store) is respected from day one.
15. As a maintainer, I want no network, plugin, scripting or webview initialisation in the entry point, so that D3, D11 and the "no network layer / no plugin system / no scripting engine" rule hold from the first build.
16. As a maintainer, I want no webview of any kind anywhere in the skeleton, so that ADR-0002 is honoured and never "fixed" later by introducing an HTML/CSS/JS shell.
17. As a maintainer, I want zero runtime network access, so that the local-first constraint is not violated before the data layer even exists.
18. As a maintainer, I want no upstream strings, logos, mascot, screenshots or guide text bundled, so that the legal/IP boundary (ADR-0001 / ADR-0004) holds from the first build.
19. As a maintainer, I want all shipped strings to be written fresh, so that we never copy the upstream app's UI text.
20. As a reviewer, I want an empty window with no placeholder pages, disabled destinations or fake content, so that "no empty shells" (D5 / D12) is respected and we never ship a non-demoable shell.
21. As a reviewer, I want no drawn macOS traffic lights on Linux, so that the deliberate native-chrome deviation (D15) is correct.
22. As a reviewer, I want canonical GLOSSARY vocabulary in code identifiers and comments wherever domain concepts appear, so that later code stays consistent.
23. As a reviewer, I want no schema, migrations, repositories or SQLite file creation in this phase, so that Phase 2 owns the data layer cleanly and Phase 1 stays a single increment.
24. As a reviewer, I want no three-panel shell, tokens or panel components yet, so that Phase 3 owns the visual foundation.
25. As a reviewer, I want the README's already-documented build command (`qmake6 open-woday.pro && make`) to become true, so that the docs are accurate the moment this phase lands — without editing the README in this phase.
26. As a packager, I want `qmake6 && make` to produce a predictable binary, so that Phase 12's AppImage and `.deb` have something to package.
27. As a packager, I want the project to avoid Linux-only build hacks, so that the same codebase can later target Windows and macOS (D6 / D11).
28. As a reviewer, I want no test framework or low-level unit seams introduced prematurely, so that we keep the ideal single seam and only test external behaviour once there is behaviour to test.
29. As a maintainer, I want this spec to contain no schedule or effort estimates, so that D7 / D9 are respected.
30. As a maintainer, I want the spec recorded as uncommitted files on the phase branch, so that the commit step remains a separate, reviewable workflow stage.

## Implementation Decisions

Stated in durable terms; concrete file/command targets are in `plan.md` and
`validation.md`. No prototype was produced, so no snippet is inlined.

- **Build system.** A single `qmake6` project (`.pro`) is the build definition.
  `qmake6` generates the Makefile and `make` compiles. **No CMake** is used or
  added (it is not installed; `tech-stack.md` §4, §11; README explicitly says so).
  The project declares Qt 6 **Quick**, **Widgets** (for native dialogs in later
  phases) and **SQL** (the SQLite driver) modules up front, sets a C++ standard
  compatible with Qt 6.12, and targets `open-woday`, so later phases never have to
  rework the build to add those modules.
- **Language / UI.** C++ + Qt Quick / QML, one codebase, Linux primary. Qt Quick's
  GPU scene graph satisfies "no webview"; no Electron/Tauri/HTML-rendered shell
  and no web-based editor/view library are introduced (ADR-0002).
- **Layer split.** QML views are declarative and hold no business logic; C++ holds
  the entry point and (in later phases) controllers/models; a store layer is
  reserved for persistence but is **not populated** in this phase. There is no
  network layer, no plugin system and no scripting engine.
- **Startup.** A single C++ entry point creates a GUI application, loads the root
  QML component through `QQmlApplicationEngine`, verifies the root object was
  constructed (failing loudly otherwise), and exits cleanly. It contains no
  product logic.
- **QML delivery.** The root QML component is embedded via the Qt resource system
  (`qrc`) so the built binary is self-contained — this also removes a class of
  "works from the source tree, fails from the binary" bugs and helps Phase 12
  packaging.
- **The window.** One empty QML window using the OS's **native** decorations. No
  custom chrome, no drawn traffic lights, no placeholder pages, no disabled
  sidebar destinations, no fake content. The window title uses `open-woday` only.
  This is the deliberate "no empty shells" and native-chrome (D15) behaviour.
- **Branding & legal hygiene.** Every string is original. No upstream asset,
  wordmark, mascot, UI string, guide text or screenshot is bundled or published.
  `logo/` and `screenshots/` remain reference material only.
- **Runtime constraints.** No network access, no account, no cloud, no sync, no
  I/O beyond launching the window. "No cloud" is a constraint, not a deferred
  feature (D3).
- **Cross-platform readiness.** Code avoids Linux-only assumptions; native chrome
  is delegated to the OS; Windows and macOS remain later build targets from the
  same source (D6 / D11), not separate ports.
- **Explicit phase boundary.** No schema, migrations, `meta` schema version,
  repositories, SQLite file, `Item`/`Block` models, panels, tokens, Today page,
  Task/Event lines, editor, Quick Add, Inbox or CSV export are built here. Those
  belong to Phases 2–12. Phase 1 ships no non-demoable horizontal layer.
- **Testing.** The seam decision, what makes a good test, and prior art are in
  `validation.md`: a single highest seam (build-and-launch), no unit-test
  scaffolding yet (greenfield).
- **Documentation.** The README already records the build command and prereqs
  ("planned for v0.1"); it is intentionally **not modified** in Phase 1 — the
  full build/run/packaging docs are a Phase 12 deliverable.

## Out of Scope

- **Phases 2–12** in full: the data layer and schema/migrations/repositories
  (Phase 2); the three-panel shell and visual tokens (Phase 3); the Today page and
  Day note (Phase 4); Task CRUD (Phase 5); the block editor (Phase 6);
  Task↔Note/Block links (Phase 7); Inbox and organisation (Phase 8); Quick Add
  (Phase 9); CSV export (Phase 10); visual tuning (Phase 11); release packaging
  (Phase 12).
- **The deferred backlog** (`roadmap.md` §4, no phase numbers): Calendar/Upcoming
  pages, Habits/streaks, saved Views, Settings, Analytics, AI assistant, local
  MCP server, third-party integrations, full CSV import, Trash UI, search /
  command palette, standalone list pages, the remaining 12 block types, symbol
  menus, live Quick Add preview, Magic Lines, multi-language parsing, Windows and
  macOS binaries.
- **Permanent non-goals** (`mission.md` §8, ADR-0003): Apple-ecosystem features,
  the macOS-only shell and the iOS app — never built on this architecture.
- Any populated page, placeholder destination, disabled UI element or fake
  content ("no empty shells", D5 / D12).
- Any webview, network call, plugin system, scripting engine or cloud/account/sync.
- Test frameworks, CI configuration, unit seams and packaging scripts.
- README / CONTRIBUTING / CHANGELOG edits; a tagged release.

## Further Notes

- This is the **first mergeable increment** of v0.1. It does **not** by itself
  satisfy any full v0.1 Definition-of-Done item (`mission.md` §10 / `roadmap.md`
  §3); its merge gate is the Phase 1 "Verifiable when" condition. Later phases
  complete those items.
- No schedule or effort estimates appear anywhere in this spec (D7 / D9); the
  order of work is by dependency.
- Screenshots are the visual source of truth and the 134 guides the behavioural
  source of truth (ADR-0004), but Phase 1 renders no product surface, so neither
  is consulted for content here; the empty window is a deliberate boundary.
- `/bnvt-ticket` breaks `plan.md` into tracer-bullet vertical slices; the spec is
  written to local files and is **not committed** in this step (committing is the
  `/bnvt-commit` stage).
