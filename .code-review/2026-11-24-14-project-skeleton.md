# Code review — 2026-11-24-14-project-skeleton

Two-axis review (`/code-review`) of the Phase 1 *project-skeleton* increment.

- **Fixed point:** `main` — `9f0db6274a6763bb9905748ae748227c396e8a20` (the merge-base of `main` and `HEAD`).
  Interpretation of "the latest code-review commit": no prior code-review commit exists in
  this repository — no commit message, tag, git note or unreachable/dangling commit marks a
  review, and no `.code-review/` report has ever been committed. This is therefore the first
  review, so it covers every change on the phase branch since it diverged from `main`.
- **Diff command:** `git diff main...HEAD` (three-dot, against the merge-base) — 12 files,
  +514 / −8801.
- **Commits under review** (`git log main..HEAD --oneline`):
  - `7216ff1` feat: add runnable Qt application skeleton
  - `e8f1042` docs: add phase 1 implementation tickets
  - `9193612` docs: add phase 1 spec and remove legacy design docs
- **Spec source:** `dev-docs/specs/2026-11-24-14-project-skeleton/requirements.md`,
  `plan.md`, `validation.md`, plus the tracer-bullet tickets
  `.scratch/2026-11-24-14-project-skeleton/issues/01-*.md` and `02-*.md`.
- **Standards sources:** `README.md` (§Contributing is the interim contributing guide; no
  `CODING_STANDARDS.md` or `CONTRIBUTING.md` exists), `GLOSSARY.md`,
  `dev-docs/mission.md`, `dev-docs/tech-stack.md`, `dev-docs/roadmap.md`, plus the Fowler
  (*Refactoring*, ch.3) smell baseline.
- The two axes were reviewed in parallel by independent sub-agents with fresh contexts and
  are reported separately below — findings are neither merged nor reranked.
- **No git commits were made** as part of this review.

## Standards

### (a) Documented-standard violations
**No hard violations found in the added files.** Concretely checked:

- **README §Build from source** (`qmake6 open-woday.pro && make`; "CMake is not used and should not be added"), **tech-stack §5.3/§11**, **roadmap Phase 1**: `open-woday.pro` is `TEMPLATE = app` / `QT += quick widgets sql` / `CONFIG += c++17`, no CMake file anywhere in the diff. ✓
- **tech-stack §5.1** ("QML views are declarative (no business logic)… no network layer, plugin system or scripting engine"; **mission §3.2** no webview): `MainWindow.qml` is pure declaration; `src/main.cpp` is bootstrap only — no `QNetwork*`/`QWebEngine*`/`QSql*`/schema. ✓
- **tech-stack §5.2** source tree: `src/main.cpp`, `src/qml/MainWindow.qml`, `open-woday.pro` at root ✓. `resources.qrc` at root is not in the tree, but §5.2 is labelled "**Proposed** file tree" and roadmap Phase 1 names only three deliverables — a **borderline judgement**, not hard.
- **GLOSSARY.md `_Avoid_`** + **README §Contributing** / **mission §13 / §3.9**: no `_Avoid_` term is used *for a domain concept*. "to-do" occurs only inside requirements.md's explicit avoid-list quote; "page"/"doc"/"object"/"entry point"/"Woday"(=`open-woday`) are non-domain uses. ✓
- **README §Contributing / mission §13 / tech-stack §12** ("No schedule or effort estimates"): none present; specs restate the rule. ✓ **tech-stack §12** ("no tests yet") — no test framework added. ✓ **roadmap §1** (next phase = lowest not `done`) — Phase 1 is correct; `validation.md` carries the "Verifiable when" gate. ✓

**Borderline judgement (standards consistency only — not spec intent):** the diff DELETES `dev-docs/grill-session.md` and `dev-docs/app-document.md` without updating the docs that cite them: README §Project documentation table lists `grill-session.md` and says `app-document.md` is "kept here only as behavioural / design references"; mission §1 authority map, §6, and tech-stack §8.1/§12 call them "separate, untouched authorities" and the behavioural source of truth. Those references now dangle. Whether removal was *intended* is the spec axis's call.

### (b) Fowler baseline smells — all judgement calls
- **Duplicated Code (doc-level):** the same constraint checklist recurs in ≥5 files. E.g. `validation.md` §Validation 6 "no webview of any kind … no network classes … no schema, migrations … no CMake files" vs. ticket 02 "Diff inspection finds no webview of any kind … no network classes … no schema … no CMake files" vs. requirements.md constraints table, `plan.md` §4.4, ticket 01. → extract one shared list, reference it. Corollary **Shotgun Surgery**: one constraint change forces edits to all five.
- **Considered & SUPPRESSED — Speculative Generality:** `QT += sql`/`widgets` "up front … so later phases never have to rework the build" (requirements.md §Implementation Decisions) is a hook for a need the skeleton lacks — but **tech-stack §5.3** ("Qt 6 Quick, Widgets, SQL modules") and roadmap Phase 1 deliverables mandate it, so the documented standard overrides the baseline. Likewise the startup guard and `qrc` embedding are mandated by `plan.md` §2.2/§1.3.
- **No other smells observed** (Mysterious Name, Feature Envy, Data Clumps, Primitive Obsession, Repeated Switches, Message Chains, Middle Man, Refused Bequest, Divergent Change): one translation unit, no classes/inheritance/switch, one concern per new file. Tooling-enforced items skipped.

## Spec

**(a) Missing / partial requirements**

No gaps in the skeleton itself. One partial: story 25, "so that the docs are accurate the moment this phase lands — without editing the README in this phase." (requirements.md:113). README is correctly untouched, but deleting files it links to leaves it inaccurate (see c). Static review only; I did not run the `qmake6 && make` / launch gate ("the app configures, compiles and opens a window on Linux.", validation.md:10), so that merge gate is unverified though the code matches.

**(b) Scope creep**

1. Commit `9193612` deletes `dev-docs/app-document.md` (7812 lines) and `dev-docs/grill-session.md` (989). Nothing asks for this: "**In scope (from `roadmap.md` Phase 1 deliverables):** a `qmake6` project file declaring the Qt 6 Quick / Widgets / SQL modules; a single C++ entry point; one empty QML main window; a clean `qmake6 && make` build; a window that opens on Linux." (requirements.md:75–78); Out of Scope lists no doc deletions (requirements.md:171–191). The spec/constitution treat them as *kept*: "Screenshots are the visual source of truth and the 134 guides the behavioural source of truth (ADR-0004)" (requirements.md:201); mission.md:156 "`dev-docs/app-document.md` stays an internal behavioural reference"; mission.md:10 "`dev-docs/grill-session.md` is the untouched historical design-session record". Removing them destroys sources Phases 4+ need; 8801 deletions dwarf 376 additions.
2. `.gitignore` added though never mentioned in plan.md groups 1–4 (cf. "Test frameworks, CI configuration, unit seams and packaging scripts.", requirements.md:190). Benign build hygiene, but unrequested.

**(c) Wrongly implemented**

The deletion looks aimed at "Original strings/assets only; never bundle upstream logos, mascot, UI strings, the guide scrape or the screenshots" (requirements.md:67) and "No upstream asset, wordmark, mascot, UI string, guide text or screenshot is bundled or published." (requirements.md:152) — but those govern the *shipped binary*, not repo reference docs, which the spec keeps: "`logo/` and `screenshots/` remain reference material only." (requirements.md:153). It re-litigates settled ADR-0001/0004 material, contrary to "do not re-litigate settled decisions or absorbed ADRs" (requirements.md:71; "no settled decision or absorbed ADR is re-litigated.", validation.md:86). Because the README is intentionally "not modified" (requirements.md:168–169), README.md:200/204, mission.md:41/42 and tech-stack.md:304/492 now link deleted files — dangling references, so story 25's docs are not "accurate".

**Correctly implemented**: single C++ entry point `QGuiApplication`+`QQmlApplicationEngine` (plan.md:28); loud guard + `return 1` (plan.md:30–31); qrc-embedded root `qrc:/qml/MainWindow.qml` matching `resources.qrc` prefix/alias (requirements.md:144–146); `QT += quick widgets sql`, `c++17`, `TARGET=open-woday`, `RESOURCES` (plan.md:14–20); one empty `Window { visible:true; title:"open-woday" }` with default native chrome (plan.md:40–45); no CMake/webview/network/schema/panel/test scaffolding (validation.md:71–82); README unmodified; spec files + 2 tracer tickets present per workflow (requirements.md:51–52); no estimates (validation.md:85).

## Summary

**Standards:** 3 findings (0 hard documented-standard violations; 3 judgement calls — 1 borderline consistency note, 1 Duplicated Code, 1 Shotgun Surgery corollary), worst: the duplicated constraint checklist (Duplicated Code / Shotgun Surgery) recurring across five docs. **Spec:** 4 findings (1 partial requirement, 2 scope creep, 1 wrongly-implemented) plus an unverified build/launch merge gate, worst: commit `9193612`'s unrequested deletion of 8,801 lines of reference docs, which re-litigates settled ADR-0001/0004 and leaves dangling links in README/mission/tech-stack.
