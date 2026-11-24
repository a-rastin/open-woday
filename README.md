# open-woday

An open-source, non-commercial, local-first desktop planner for people who
cannot run the macOS-only upstream app. Linux first, then Windows and macOS —
from one Qt Quick / QML + C++ codebase, with no account, no cloud and no
webview.

> **Status: design complete, implementation under way.** Every decision for
> `v0.1` is settled in the [constitution](#project-documentation). Phase 1 has
> produced a buildable Qt Quick skeleton that opens one empty window on Linux —
> there is now application code you can compile and run, but no product features
> and no packaged release yet. The [build-from-source steps](#build-from-source)
> are live; the install steps still describe the **planned** `v0.1` Linux
> release. See [Roadmap and status](#roadmap-and-status) for what is next.

---

## Disclaimer

`open-woday` is an **unaffiliated, non-commercial community project**. It is not
affiliated with, endorsed by, sponsored by, or officially connected to the
proprietary macOS-only app whose public day-planning experience it reimplements
(the "upstream app"), or to that app's developers. The name is used
descriptively only, and the app is never presented as official or endorsed.

The upstream app's logos, wordmarks, mascot, UI strings and documentation are
**reference material only**: they are never copied into the shipped product,
never presented as our own documentation, and never bundled with a release. All
strings and assets in the shipped app are original. Source code is licensed
under [Apache 2.0](LICENSE).

---

## What it is and why it exists

The upstream app is macOS-only. `open-woday` exists for **access**: it brings a
comparable day-planning experience to desktop users on other systems, starting
with Linux.

| | |
|---|---|
| **Data** | Entirely local — one SQLite file; no server, no account, no sync, no runtime network access |
| **UI** | Native Qt Quick / QML + C++ — no Electron, Tauri, or any HTML/CSS/JS-rendered webview |
| **Platforms** | One codebase; Linux (`v0.1`) → Windows → macOS as later build targets |
| **Data exit** | One-way CSV export (no import mapper in `v0.1`) |
| **Governance** | Maintained community project: GitHub issues/PRs, tagged releases |

"No cloud" is a **constraint, not a deferred feature** — data lives on a single
device by design, and design fidelity to the Reference UI is `v0.1`'s top
priority.

Canonical terms (upstream app, open-woday, Target user, Reference UI, Item,
Task, Note, Block, Event, Tag, Folder, Inbox, Day note) are defined in
[GLOSSARY.md](GLOSSARY.md); please use them in code, docs and issues.

---

## What `v0.1` will include

A curated "main slice" — a working vertical slice, not a 1:1 clone of the
upstream app:

- **Three-panel window:** dark sidebar, top bar, centre page, right rail.
- **Today page** as an editable **Day note**: day strip, summary line, Task and
  Event lines with badge chips, check-off, free writing around them.
- **Tasks:** create, rename, edit, complete/reopen; due date (day + optional
  time), priority, Tags, Folder, nested subtasks, inline duration, and each
  Task's own Note pane.
- **Events:** lightweight first-class Items (title, start/end, all-day) shown
  as lines on Today. No calendar grid.
- **Notes:** an in-house QML block editor with exactly 5 block types (text,
  heading, to-do, bullet list, code) plus inline bold / italic / link. A to-do
  Block creates a real Task.
- **Inbox rail:** Items with no due date and no Folder.
- **Quick Add:** a minimal English parser in the top bar —
  `Call Anna tomorrow 10:00 #work !!` becomes a Task with parsed chips.
- **Persistence:** SQLite via Qt SQL, surviving restarts.
- **Export:** one-way CSV in an upstream-compatible column shape.
- **Packaging:** AppImage + `.deb` for Linux.

Deliberate deviations from the Reference UI — native OS window chrome,
platform-correct `Ctrl`/`⌘` labels, a shorter sidebar with no empty shells,
all-original strings, and an Inbox-only right rail — are recorded in
[`dev-docs/mission.md`](dev-docs/mission.md).

---

## Permanent non-goals

These will **never** be built on this architecture. They are impossible on a
Linux-first, no-webview Qt app — they are **not** "later" features:

- iCloud / CloudKit, Apple Calendar, Apple Reminders, Apple Notes, Weather and
  Apple Intelligence
- The macOS-only shell (system menu bar, widgets, Save for Later, ⌥Space
  launcher)
- The iOS app

Features that are **deferred, not excluded** (Calendar page, Upcoming, Habits,
Views, Settings, search, integrations, Windows/macOS binaries, and more) form an
uncommitted backlog in [`dev-docs/roadmap.md`](dev-docs/roadmap.md). Please read
both lists before proposing work.

---

## Quick start (5 minutes)

There is no packaged release to install yet, but the repository can already be
built and launched — see [Build from source](#build-from-source). To understand
the project and find the next unit of work:

1. Clone it:
   ```bash
   git clone https://github.com/a-rastin/open-woday.git
   cd open-woday
   ```
2. Read [`dev-docs/mission.md`](dev-docs/mission.md) — what/why, principles,
   scope, non-goals and the `v0.1` definition of done.
3. Read [`dev-docs/roadmap.md`](dev-docs/roadmap.md) — the 12 ordered `v0.1`
   phases. **The next phase is the lowest-numbered one not marked `done`.**
4. Read [`dev-docs/tech-stack.md`](dev-docs/tech-stack.md) before writing any
   code — stack, schema, visual tokens and engineering rules.

---

## Install (planned for `v0.1`)

> Not available yet — no release has been tagged.

`v0.1` ships **Linux only**, as two packages from a tagged GitHub release:

| Package | Run |
|---|---|
| AppImage | `chmod +x open-woday-x86_64.AppImage && ./open-woday-x86_64.AppImage` |
| `.deb` | `sudo dpkg -i open-woday_0.1.0_amd64.deb` |

Windows and macOS binaries come later, built from the same codebase and only
after they are tested.

---

## Build from source

The Phase 1 skeleton builds and runs **now**. Release packaging (AppImage /
`.deb`) is still planned for `v0.1`.

Prerequisites (checked on the maintainer's machine 2026-11-23):

- Qt **6.12** with `Qt6Quick`, `Qt6Widgets`, `Qt6Core` and the **SQL / SQLite
  driver** module (headers in `/usr/include/qt6`)
- `qmake6` and `g++`. **CMake is not used and should not be added.**

```bash
qmake6 open-woday.pro && make
./open-woday
```

This opens a single, intentionally empty window titled `open-woday` with native
OS decorations — no product UI, persistence or network yet (Phase 1 ships only
the runnable build/launch seam). For `v0.1`, the supported distribution will be
the AppImage above. Packaging uses `linuxdeployqt` to bundle Qt libraries into
the AppImage and `dpkg-deb` to build the `.deb`. See
[`dev-docs/tech-stack.md`](dev-docs/tech-stack.md) for the proposed source tree
and build details.

---

## Configuration & usage (planned behaviour)

- **No setup, no account, no configuration file.** On first launch the app
  creates a single SQLite database in the platform app-data directory, e.g.
  `$XDG_DATA_HOME/open-woday/open-woday.db` on Linux.
- **No network.** There is no server, no account and no sync; data lives on one
  device.
- **Back up or move your data** by copying that database file while the app is
  closed, or use the one-way UTF-8 CSV export (one row per Item).
- **Keyboard:** `Ctrl+K` / `⌘K` focuses Quick Add, `Ctrl+N` / `⌘N` creates a
  Task. Labels are platform-correct (`Ctrl` on Linux/Windows, `⌘` on macOS).
- **Quick Add** understands kind prefixes (`T:`, `N:`, `E:`), `today`,
  `tomorrow`, weekday names, `HH:mm`, `#tag` (created if missing) and
  `!` / `!!` / `!!!` for low / medium / high priority. English only.

Full behavioural detail is in [`dev-docs/tech-stack.md`](dev-docs/tech-stack.md).

---

## Roadmap and status

Design is complete: 22 decisions were settled on 2026-11-23, and on 2026-11-24
the former plan and ADRs were fully consolidated into the three constitution
files (nothing was dropped). **Implementation has started** with Phase 1 — a
buildable, runnable Qt Quick skeleton (`open-woday.pro`, one C++ entry point and
one empty QML window) that opens a native Linux window. It ships no product
surface yet. No phase has been marked `done` or merged, so all 12 `v0.1` phases
remain `planned` in [`dev-docs/roadmap.md`](dev-docs/roadmap.md), beginning with
`phase-1-project-skeleton`. No release has been tagged; releases start with
`v0.1` on GitHub. The full phase list, deferred backlog and definition of done
are in the roadmap.

---

## Project documentation

| File | What it is |
|---|---|
| [`dev-docs/mission.md`](dev-docs/mission.md) | Constitution 1/3 — mission, principles, scope, non-goals, legal boundary, settled decisions |
| [`dev-docs/tech-stack.md`](dev-docs/tech-stack.md) | Constitution 2/3 — stack, architecture, schema, visuals, build, engineering rules |
| [`dev-docs/roadmap.md`](dev-docs/roadmap.md) | Constitution 3/3 — ordered `v0.1` phases, deferred backlog, definition of done |
| [`GLOSSARY.md`](GLOSSARY.md) | Canonical vocabulary — use these terms everywhere |
| [`dev-docs/grill-session.md`](dev-docs/grill-session.md) | Historical design-session record (22 decisions, contradiction log) |
| `dev-docs/specs/` | Per-phase specs (`requirements.md` / `plan.md` / `validation.md`) |
| [`CHANGELOG.md`](CHANGELOG.md) | Notable changes per phase, maintained by `/bnvt-documentation` |
| `.opencode/commands/bnvt-*.md` | The spec-driven workflow commands |

**Internal reference material — not project documentation:**
[`dev-docs/app-document.md`](dev-docs/app-document.md) is a verbatim scrape of
the upstream app's 134 guides, and [`dev-docs/DESIGN.md`](dev-docs/DESIGN.md) is
derived from its marketing website. Both are kept here only as behavioural /
design references; neither is a spec for our app's UI, neither is published as
project documentation, and neither is bundled with a release. `screenshots/` is
the visual source of truth and `logo/` holds brand references; files in both
folders are **reference material only and are never shipped as assets**.

---

## Contributing

Contributions are welcome — this is a maintained community project, not a
throwaway sprint.

- Open an issue or a PR on [GitHub](https://github.com/a-rastin/open-woday).
- Use the [`GLOSSARY.md`](GLOSSARY.md) vocabulary in code, docs, tickets and
  specs.
- The three constitution files are the authority for what to build and how.
  Settled decisions and absorbed ADRs are **not re-litigated** per feature;
  propose a constitutional amendment instead.
- Work proceeds in ordered phases from [`dev-docs/roadmap.md`](dev-docs/roadmap.md)
  via a spec-driven workflow: `/bnvt-spec` → `/bnvt-ticket` →
  `/bnvt-implement` (TDD) → `/bnvt-review` → `/bnvt-revise` →
  `/bnvt-documentation` → `/bnvt-commit` → `/bnvt-merge` (see
  `.opencode/commands/`).
- No schedule or effort estimates: phases are ordered by dependency.

A full [`CONTRIBUTING.md`](CONTRIBUTING.md) is a `v0.1` deliverable (phase 12)
and is not written yet; until then, the points above are the contributing guide.

---

## License

[Apache License 2.0](LICENSE). `open-woday` is unaffiliated and non-commercial;
see the [Disclaimer](#disclaimer) above.
