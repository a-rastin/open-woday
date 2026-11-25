# Phase 3 — app-shell — Plan

> **Spec:** `dev-docs/specs/2026-11-25-06-app-shell/` · **Branch:** `phase-3-app-shell`
> Numbered task groups in dependency order. `/bnvt-ticket` turns these into
> tickets. Uses `GLOSSARY.md` vocabulary; respects `mission.md`, `tech-stack.md`
> and the absorbed ADRs. **No schedule or effort estimates** (D7 / D9).
>
> Concrete targets below come from `roadmap.md` Phase 3 deliverables and
> `tech-stack.md` §§8.2–8.5, 9.1. `validation.md` holds the checks that prove
> each group is done. Phase 3 is the **three-panel shell and visual tokens** and
> deliberately ships no live product data: it is a runnable / verifiable
> increment proven through the build-and-launch appearance of the window, not
> through Item or Task behaviour.

## 1. Build wiring and QML layout

1. Keep `open-woday.pro` building with `qmake6 && make`; keep `QT += quick widgets
   sql`, `CONFIG += c++17`, `TARGET = open-woday`, `RESOURCES += resources.qrc`.
   **No CMake** is added (`tech-stack.md` §4 / §11). Extend `resources.qrc` only
   to embed any new QML or original icon assets this phase adds.
2. Keep the C++ entry point free of business logic and the views declarative
   (`tech-stack.md` §5.1). The root `Window` keeps native OS decorations and the
   Phase 1 loud-failure guard if the root QML component cannot be created.
3. Add the `Sidebar`, `TopBar`, `RightRail` and centre-panel QML composition on a
   dark `canvas`, matching the `tech-stack.md` §8.2 window structure. No product
   pages are added.

## 2. Shared visual tokens

1. Define one shared token set (`tech-stack.md` §8.3): `canvas`, `sidebar`,
   `sidebarText`, `sidebarMuted`, `panel`, `panelText`, `muted`, `chip`, `accent`,
   `radiusPanel`, `radiusChip`, `radiusControl`, `gap`, `font`, `bodySize`,
   `titleSize`, `dayStripCell`. Use the §8.3 starting values.
2. Declare Inter with a system-UI fallback once; apply `bodySize` / `titleSize`
   consistently across the frame. Pastel tag-chip surfaces and exact hues are
   tuned in Phase 11, not borrowed from `DESIGN.md`.
3. Give the centre panel and right rail `radiusPanel`, chips/pills
   `radiusChip`, controls `radiusControl`, and panels the `gap` spacing.

## 3. Original strings and assets

1. Write every frame string, section heading, placeholder, empty state and icon
   fresh for open-woday (`tech-stack.md` §8.4). No upstream UI string, logo,
   wordmark, mascot, screenshot or guide text is reused, embedded or shipped.
2. Keep `logo/`, `screenshots/`, `dev-docs/app-document.md` and
   `dev-docs/DESIGN.md` as internal reference material only (ADR-0001 / ADR-0004,
   `mission.md` §5 / §6). Reach visual similarity through layout and tokens, not
   copied brand elements.
3. The product name `open-woday` appears in the window title / About only; no
   standalone upstream brand name or mascot appears in the app.

## 4. Chrome, modifiers and resize behaviour

1. Use each OS's native window decorations; do **not** draw macOS traffic-light
   controls on Linux or Windows (`tech-stack.md` §8.5, D15). Record it as a
   deliberate deviation.
2. Show `Ctrl` on Linux/Windows and `⌘` on macOS in any modifier label; keep the
   inner app UI identical across platforms.
3. Implement resize/fold behaviour: fold the `Sidebar` first, then the
   `RightRail`, keeping the centre panel legible (`tech-stack.md` §8.2 / §9.1).

## 5. Static frame, no data

1. Render the frame and its visual language only: static section headings in the
   `Sidebar`, a clean white centre panel and a white rounded `RightRail`.
2. Add no Day strip, Day note, Task/Event lines, check-off, block editor, Inbox
   contents, Quick Add parsing or CSV export.
3. Add no Item/Task/Note/Event/Tag/Folder/Inbox read or write, no schema,
   migration, repository or `Database` change (Phase 2 owns these). Add no
   network, sync, account, cloud, webview, plugin, scripting or web-editor code.

## 6. Verification of the build-and-launch seam

1. Verify `qmake6 open-woday.pro && make` configures and compiles cleanly with no
   new warnings and no new build tooling.
2. Launch the binary and confirm a single window appears with the dark sidebar,
   white rounded centre panel and white rounded right rail, native chrome, no
   traffic lights on Linux and platform-correct modifier labels.
3. Confirm the frame uses the shared token set and only original strings/assets;
   confirm no upstream asset or verbatim string is bundled.
4. Confirm the resize/fold order (Sidebar, then RightRail).

## 7. Build, constraint and spec-hygiene verification

1. Confirm no CMake, no new test framework, no CI configuration, no unit seams
   and no packaging scripts were added (`tech-stack.md` §12).
2. Confirm `GLOSSARY.md` vocabulary is used and no settled decision or absorbed
   ADR is re-litigated; any constitution change would need an explicit amendment
   (`mission.md` §13).
3. Confirm the spec carries no schedule or effort estimates (D7 / D9) and that
   this phase is a single runnable / verifiable increment (`roadmap.md` §1).
4. Confirm the spec is recorded as a proposal with maintainer confirmation still
   outstanding (spec approval process), and that no maintainer sign-off is
   fabricated.
