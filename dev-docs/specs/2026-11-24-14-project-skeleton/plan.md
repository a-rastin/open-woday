# Phase 1 — project-skeleton — Plan

> **Spec:** `dev-docs/specs/2026-11-24-14-project-skeleton/` · **Branch:** `phase-1-project-skeleton`
> Numbered task groups in dependency order. `/bnvt-ticket` turns these into
> tickets. Uses `GLOSSARY.md` vocabulary; respects `mission.md`, `tech-stack.md`
> and the absorbed ADRs. **No schedule or effort estimates** (D7 / D9).
>
> Concrete targets below come from `roadmap.md` Phase 1 deliverables and
> `tech-stack.md` §5.2 / §5.3. `validation.md` holds the checks that prove each
> group is done.

## 1. Project and build definition

1. Add `open-woday.pro`: a `qmake6` project targeting `open-woday` with a C++
   standard suitable for Qt 6.12.
2. Declare the Qt 6 modules up front: `quick`, `widgets`, `sql` (Quick for QML,
   Widgets for later native dialogs, SQL/SQLite driver for Phase 2) — per
   `tech-stack.md` §5.3. Add `core`/`gui` transitively as needed.
3. Embed the QML root component through the Qt resource system (`qrc`) so the
   binary is self-contained; list the resource in the `.pro`.
4. Verify `qmake6 open-woday.pro` configures with no errors and produces a
   Makefile. **No CMake** is added (not installed; README §"Build from source").
5. Verify `make` compiles cleanly (no errors, no new warnings) and produces the
   `open-woday` binary.

## 2. C++ entry point

1. Add `src/main.cpp`: construct `QGuiApplication`, load the root QML component
   via `QQmlApplicationEngine`.
2. Guard startup: if the root object was not created, fail with a clear error and
   a non-zero exit code rather than a silent blank app.
3. Set the application/window identity to `open-woday` only; no upstream strings.
4. Keep the entry point free of business logic (QML views stay declarative;
   `tech-stack.md` §5.1).
5. Do **not** initialise any network, plugin, scripting or webview component; do
   not add a network layer.

## 3. Empty QML main window

1. Add `src/qml/MainWindow.qml` as a single root window (`ApplicationWindow` /
   `Window`) with `visible: true`.
2. Use **native OS decorations** only — no custom chrome, no drawn macOS traffic
   lights on Linux (D15).
3. Keep the content empty: no placeholder pages, disabled destinations, badge
   chips, day strip, panels or fake content ("no empty shells", D5 / D12).
4. Use original strings only; bundle no upstream logo, mascot, UI string, guide
   text or screenshot (ADR-0001 / ADR-0004).
5. Ensure the window closes cleanly and the process exits on close.

## 4. Build, launch and constraint verification

1. From a clean state, run the documented `qmake6 open-woday.pro && make` command
   on Linux with the installed toolchain (Qt 6.12, `qmake6`, `g++`; no CMake).
2. Launch the produced binary and confirm exactly one visible window opens with
   native decorations and the `open-woday` title.
3. Close and relaunch to confirm repeatable launch and clean teardown.
4. Review-check the diff against the constraints: no webview import, no network
   classes, no schema/SQL/repository code, no panel/token code, no placeholder
   UI, no bundled upstream assets, no CMake files, no test framework.
5. Confirm the README's already-documented build command is now accurate; do not
   edit the README in this phase.
6. Confirm every item in `validation.md` passes; confirm no out-of-scope item
   (Phases 2–12, the deferred backlog, permanent non-goals) leaked in.
