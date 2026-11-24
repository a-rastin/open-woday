# Changelog

All notable changes to `open-woday` are documented here, following
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/). This file is
maintained by the `/bnvt-documentation` workflow step (`mission.md` §9): each
phase adds an entry for the work done since the last full-spec commit — currently
`a991433` (2026-11-24), which consolidated the `v0.1` spec and ADRs into the
three-file constitution.

No release has been tagged yet, so everything below is **Unreleased**. Earlier
commits (the initial `v0.1` spec, ADRs, `GLOSSARY.md`, reference material and
the constitution itself) predate this file; consult the git log for that
design-only history.

## [Unreleased] — 2026-11-24

### Added

- **Phase 1 — project-skeleton: the first runnable increment.** A `qmake6` Qt 6
  Quick / QML + C++ application that configures, compiles and opens a single
  empty window on Linux, with no webview, no network and no persistence.
  - `open-woday.pro` — the `qmake6` build definition, declaring the `quick`,
    `widgets` and `sql` modules, C++17, and targeting `open-woday`. No CMake.
  - `src/main.cpp` — one C++ entry point creating `QGuiApplication` and
    `QQmlApplicationEngine`; fails loudly to stderr and returns `1` if the root
    QML component cannot be created, otherwise exits cleanly. No business logic.
  - `src/qml/MainWindow.qml` — one declarative, empty `Window` titled
    `open-woday`, using native OS decorations; no placeholder pages, disabled
    destinations or fake content ("no empty shells", D5 / D12). All strings are
    original.
  - `resources.qrc` — embeds the root QML component so the binary is
    self-contained.
  - Build and run: `qmake6 open-woday.pro && make && ./open-woday`. Verified on
    Qt 6.12.0 with `g++`: clean configure/compile with no compiler warnings, and
    the root QML window loads without a startup error.
- **Phase 1 spec** at `dev-docs/specs/2026-11-24-14-project-skeleton/`
  (`requirements.md`, `plan.md`, `validation.md`) and two tracer-bullet
  implementation tickets at `.scratch/2026-11-24-14-project-skeleton/issues/`.
- **First code-review record** at
  `.code-review/2026-11-24-14-project-skeleton.md`: a two-axis (Standards /
  Spec) review of the phase branch against the merge-base.

### Changed

- `README.md` was expanded (`9f0db62`) with a project overview, the
  unaffiliated / non-commercial disclaimer, `v0.1` scope, permanent non-goals,
  quick start, planned install/build/configuration, roadmap status, a project
  documentation index and an interim contributing guide.
- `README.md` now reflects that a buildable application skeleton exists: the
  status banner, quick start, build-from-source and roadmap sections were
  updated, while install and packaging remain marked as planned for `v0.1`.

### Fixed

- Restored the internal reference documents `dev-docs/app-document.md` (the
  upstream 134-guide scrape) and `dev-docs/grill-session.md` (the historical
  design-session record), which the Phase 1 spec commit had deleted. A code
  review flagged the removal as out of scope; both remain internal reference
  material only — never published as project documentation and never bundled
  with a release.
