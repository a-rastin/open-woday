# open-woday — Mission

> **Constitution, file 1 of 3.** The three constitution files — `mission.md`,
> `tech-stack.md`, `roadmap.md` — are the **single, self-contained authority**
> for what open-woday is, how it is built, and in what order.
>
> **Consolidation note (2026-11-24):** the former `docs/PLAN.md` and
> `docs/adr/0001`–`0004` were fully merged into these three files and then
> removed. Their decisions, rationale, considered options and consequences all
> live here; nothing was dropped. `dev-docs/grill-session.md` is the untouched
> historical design-session record and still refers to those old paths — those
> references are historical, not current. `GLOSSARY.md` is the untouched authority
> for canonical vocabulary.
>
> **Language:** English. The design interview was conducted in Persian, but every
> downstream artifact is kept in English. All three constitution files are in
> English.
>
> **Status:** agreed. 22 design decisions settled on 2026-11-23 after six
> grilling rounds; shared understanding confirmed (Q23/Q24). No implementation
> has started.

---

## 1. What this document is

This is the "why and what" layer of the constitution. It holds the mission, the
non-negotiable principles, the legal/IP boundary, the v0.1 scope, the non-goals,
the definition of done, and the traceability back to the settled decisions. It is
**guidance for spec and implementation work**: `/bnvt-spec` reads `mission.md` and
`tech-stack.md` for guidance and reads `roadmap.md` to find the next phase.

Authority map:

| Concern | Authoritative file |
|---|---|
| Mission, principles, scope, non-goals, legal, decisions | `dev-docs/mission.md` (this file) |
| Technology, architecture, data, visuals, build, engineering rules | `dev-docs/tech-stack.md` |
| Ordered phases of work, deferred backlog | `dev-docs/roadmap.md` |
| Canonical vocabulary (13 terms) | `GLOSSARY.md` (untouched, separate authority) |
| Historical design-session record | `dev-docs/grill-session.md` (untouched) |
| Behavioral reference (internal only) | `dev-docs/app-document.md` |
| Marketing-site design reference (internal only) | `dev-docs/DESIGN.md` |
| Visual source of truth (reference only) | `screenshots/`, `logo/` |

`GLOSSARY.md`, `grill-session.md`, `app-document.md`, `DESIGN.md`, `README.md`,
`logo/` and `screenshots/` were **not** modified by this consolidation.

---

## 2. Mission & product definition

**open-woday** is a non-commercial, open-source, **local-first desktop planner**.
Its purpose is to give users who cannot run a macOS-only proprietary app the same
kind of day-planning experience. It targets **Linux first, then Windows, then
macOS, from one codebase**.

- **Why it exists:** access. The upstream app is macOS-only; open-woday brings a
  comparable day-planning experience to other desktop users (D2).
- **Name:** `open-woday` (product and repository). May appear in the window title
  and About panel. Standalone "Woday", "Wodie", the mascot, upstream logos and
  upstream UI strings never appear in the app (D4, D8, D10).
- **License:** Apache 2.0. Unaffiliated, non-commercial; README carries a plain
  disclaimer (D10).
- **Governance:** maintained community project — GitHub issues/PRs, tagged
  releases, `v0.1` followed by a roadmap (D16). Not a throwaway sprint (C12).
- **Data:** entirely local. No server, no account, no cloud, no sync in v1 (D3,
  D21). "No cloud service" is a **constraint**, not a deferred feature.
- **Design priority:** fidelity to the reference app's visual language is the
  top priority of v1 (D1), bounded by D5 (original strings/assets, only
  populated pages).
- **Parity target:** a curated MVP — a vertical "main slice", not a 1:1 clone of
  all 134 upstream guides. A spiritual successor, not a drop-in import-of-upstream-
  data replacement (D1, Q1).

**Canonical vocabulary** lives in `GLOSSARY.md`: Upstream app, open-woday, Target
user, Reference UI; Item, Task, Note, Block, Event, Tag, Folder, Inbox, Day note.
Use those words, and only those words, in code, docs and issues. See §4 for a
compact restatement.

---

## 3. Core principles (non-negotiable)

These are the distilled, settled decisions. They are not to be re-litigated in
specs, tickets or reviews; a change to one requires an explicit amendment to this
constitution, not a per-feature exception.

1. **Local-first, single device.** No server, no account, no sync, no network at
   runtime. Data survives restarts in a local SQLite file. One-way CSV export is
   the only data-egress path (D3, D21, ADR-0002).
2. **No webview.** No Electron, Tauri, or any HTML/CSS/JS-rendered shell. Qt Quick
   / QML + C++ from one codebase (D11, D13, ADR-0002). Do not "fix" fidelity
   problems later by introducing a webview.
3. **One codebase, Linux primary.** Linux builds and is tested for v0.1;
   Windows/macOS are later build targets from the same source, not separate ports
   (D6, D11).
4. **Design fidelity is v1's top priority**, bounded: clone the visual language
   and layout with original strings and assets, and build only pages the main
   slice actually populates — no empty shells (D1, D5, D12).
5. **Original branding & legal hygiene.** Keep the `open-woday` name, but ship no
   upstream assets/strings and never redistribute the scraped docs (D4, D10,
   ADR-0001, ADR-0004).
6. **Spec authority.** Screenshots = visual source of truth; the 134 guides =
   behavioural source of truth; our own decisions resolve contradictions; guides
   are never copied verbatim (D14, ADR-0004).
7. **Native OS chrome, platform-correct modifiers.** Native window decorations,
   `Ctrl` on Linux/Windows and `⌘` on macOS; the inner app UI is identical
   everywhere (D15).
8. **Unified domain model.** One `Item` with a `kind` discriminator plus a
   lightweight first-class `Event`; no separate DayNote/View/Habit entities in
   v0.1 (D17, D19).
9. **Canonical vocabulary only.** Use `GLOSSARY.md` terms; respect the accepted
   decisions recorded here and their ADR rationale (ADR-0004).
10. **Maintained, non-commercial, community-run.** Outside contributions welcome;
    settled decisions and ADRs are not re-opened without an amendment (D2, D16).

---

## 4. Canonical vocabulary (compact restatement)

Full definitions and the opinionated `_Avoid_` lines are in `GLOSSARY.md`. This
restatement exists so the constitution is self-contained.

**Scope / identity**
- **Upstream app** — the proprietary, macOS-only product whose public behaviour we
  reimplement; its docs and brand are reference material, never shipped.
- **open-woday** — the shipped app; descriptive, unaffiliated FOSS alternative.
  Never presented as official/endorsed.
- **Target user** — a desktop user who cannot run the upstream app (macOS-only).
- **Reference UI** — the look/layout of the upstream app's own windows (dark
  sidebar, white rounded panels, right-hand rail). **Not** the marketing website.

**Product domain**
- **Item** — the single thing a user creates; everything is one Item of a kind.
- **Task** — an Item that can be done (due date, priority, Tags, subtasks).
- **Note** — an Item made of Blocks, used for writing; a Task/Event can own one.
- **Block** — one line of a Note; a to-do Block is also a real Task.
- **Event** — an Item that happens at a time (start/end, or all-day).
- **Tag** — a label attachable to any Item, crossing Folders.
- **Folder** — the single optional place an Item is filed.
- **Inbox** — Items with no due date and no Folder ("not dealt with yet").
- **Day note** — the editable Note belonging to a single day; holds that day's
  Tasks/Events as lines and is what the Today page shows.

Never say "to-do" for a Task, "document" for a Note, "appointment" for an Event,
or "daily note" for a Day note.

---

## 5. Name, assets & legal hygiene (was ADR-0001, accepted, amended 2026-11-23)

**Decision.** Keep `open-woday` as the product and repository name, but ship with
original visual assets: no upstream logo, wordmark or mascot files, no verbatim
UI strings ("Welcome to Woday", "Ask Wodie"), and no redistribution of the
scraped documentation. `dev-docs/app-document.md` stays an internal behavioural
reference; `dev-docs/DESIGN.md` is a marketing-site reference, not our app's UI
spec. The README must state plainly that this is an unaffiliated, non-commercial
community project.

**Considered options (rejected — do not re-litigate).**
- *Adopt a fully original name as well* (original Q4(b)). Rejected in Q8/Q10:
  `open-woday` is a descriptive, nominative handle for an unaffiliated FOSS
  alternative — low risk and aids discoverability. The legal exposure came from
  assets and verbatim copy, not the slug.
- *Keep upstream logos/mascot/exact strings for familiarity.* Rejected:
  trademark/copyright exposure; undercuts independence.

**Consequences.**
- `dev-docs/app-document.md` is never published as project documentation.
- `open-woday` may appear as the product name in the window title / About, but
  standalone "Woday" / "Wodie" and the mascot never appear in the app.
- `logo/` and `screenshots/` are reference material only, never shipped assets.
- Visual similarity is reached through layout and visual language, not copied
  brand elements or strings.

---

## 6. Spec authority (was ADR-0004, accepted)

**Decision.** The three in-app screenshots are the **visual** source of truth and
the 134 guides are the **behavioural** source of truth for any screen without a
screenshot; anything contradictory or unspecified is decided in the grilling
record and promoted to this constitution only when hard to reverse. The guides
are never copied verbatim into our product or our documentation.

**Considered options (rejected).**
- *Treat the 134 guides as a literal specification.* Rejected: they contain three
  internal contradictions (Mac-only vs. a shipping iOS section; consent-based vs.
  silent auto-update; macOS 26 alongside GPT-6.1-class model names) and are
  copyrighted verbatim text.
- *Screenshots only.* Rejected: they cover three of 12+ pages, so the Notes editor
  and task detail pane would have no behavioural basis.
- *Treat `DESIGN.md` as the app UI spec.* Rejected: it is the marketing site's
  design system (black canvas, 80 px display type, 60 px radii), a different
  surface from the app UI (dark sidebar, white rounded panels, compact type).

**Consequences.**
- `app-document.md` stays internal reference material, never published; `logo/`
  and `screenshots/` are reference only, never shipped.
- Screens with no screenshot are designed by extrapolating the screenshots'
  visual language, never by copying upstream strings or assets.
- Future contradictions are resolved and recorded here (and in an ADR-equivalent
  amendment only if hard to reverse). The full contradiction log C1–C17 is in
  `dev-docs/grill-session.md` §8; all are resolved, dissolved, or routed to the
  roadmap.

---

## 7. Scope: the v0.1 "main slice"

### 7.1 In scope

1. **Three-panel window shell:** dark sidebar, top bar, centre page, right rail;
   Back/forward, a Quick Add field, a New button, a right-rail toggle.
2. **Today page** as an editable **Day note**: day strip, summary line, Task and
   Event lines with badge chips, check-off, free writing around them.
3. **Tasks:** create, rename, edit, complete/reopen; due date (day, optional
   time), priority (none/low/medium/high), Tags, Folder, nested subtasks, inline
   duration, and each Task's own Note pane.
4. **Events:** lightweight first-class Items (title, start/end, all-day) so Today
   can render event lines. No calendar grid.
5. **Inbox rail:** Items with no due date and no Folder.
6. **Notes:** an in-house QML block editor with exactly 5 block types — text,
   heading, to-do (creates a real Task), bullet list, code — plus inline
   bold / italic / link. Used for the Day note, standalone Notes, and each Task's
   own note.
7. **Quick Add parsing** in the top-bar field (minimal, English only).
8. **Local persistence** via SQLite, surviving restarts.
9. **CSV export** (one-way, Woday-compatible column shape).
10. **Linux packaging:** AppImage + `.deb`.

### 7.2 Explicit deviations from the reference UI

- Native OS window decorations instead of macOS traffic lights (D15).
- Platform-correct modifier labels: `Ctrl` on Linux/Windows, `⌘` on macOS.
- A shorter sidebar — only real, populated destinations (see `tech-stack.md`
  §10), no empty shells.
- All strings original: e.g. the top-bar placeholder is "Add a task, note or
  event…", not "Search, or create anything…"; no "Welcome to Woday", "Ask Wodie"
  or mascot.
- No AI panel in the right rail; the rail shows Inbox only.

---

## 8. Non-goals: permanent vs deferred (was ADR-0003, accepted)

**Decision.** We split the upstream feature set into **permanent non-goals** and
**deferred roadmap items** rather than calling everything "later".

- **Permanent non-goals:** iCloud / CloudKit, Apple Calendar, Reminders, Notes,
  Weather and Apple Intelligence; the macOS-only shell (menu bar, widgets, Save
  for Later, ⌥Space launcher); the iOS app. These are impossible on a
  Linux-first, no-webview Qt app.
- **Deferred (roadmap, not v0.1):** Calendar page, Upcoming, Habits / streaks,
  saved Views, Settings, Analytics, the AI assistant, the local MCP server,
  third-party integrations (Notion, Obsidian, Todoist, Linear, Google Calendar,
  ICS feeds), full CSV import, Trash UI, search / command palette, standalone
  list pages, 12 further block types, symbol menus, live Quick Add preview,
  Magic Lines calculator, multi-language parsing, Windows and macOS binaries.
  These are listed (uncommitted, no phase numbers) in `roadmap.md`.

**Considered options (rejected).**
- *Mark the entire upstream surface as "deferred".* Rejected: Apple-ecosystem
  items are impossible without Apple frameworks on a Linux-first Qt app; calling
  them deferred would mislead contributors into waiting for them.
- *Keep no explicit non-goals list.* Rejected: the reference screenshots and
  134-guide docs keep generating "why isn't X here?" work already answered.

**Consequences.**
- Requests for Apple-only features are answered by pointing here, not by
  re-opening the platform decision.
- The README must carry this list so new contributors see the boundary before
  proposing work (a phase-12 deliverable).
- Deferred items stay on the roadmap; nothing here forbids a later amendment that
  promotes one of them.

---

## 9. Governance & legal hygiene

- **README.md:** one-line description; the unaffiliated / non-commercial
  disclaimer (D10); the permanent non-goals list (ADR-0003); install and build
  instructions; a link to `CONTRIBUTING.md`; a note that `app-document.md` and
  `DESIGN.md` are internal references and are not project documentation.
- **CONTRIBUTING.md:** issues/PRs welcome, Apache 2.0, pointer to `GLOSSARY.md`
  for vocabulary, pointer to this constitution for decisions, and a request not
  to re-litigate settled decisions.
- **CHANGELOG.md:** maintained by the `/bnvt-documentation` workflow step.
- **Never publish or bundle:** the verbatim guide scrape, upstream logos, the
  mascot, upstream UI strings, or the screenshots.
- **Decision record:** the 22 settled decisions and 17 contradictions remain
  documented in `dev-docs/grill-session.md` (untouched); the authoritative
  restatement is §11 below and the four absorbed ADR decisions in §§5, 6, 8 and
  `tech-stack.md` §3.
- **License:** Apache License 2.0 (`LICENSE`). Unaffiliated, non-commercial.

---

## 10. Definition of done for `v0.1`

A tagged Linux release that:

1. Opens a three-panel window whose inner UI matches the screenshots' visual
   language (dark sidebar, white rounded panels, day strip, summary line, badge
   chips, right rail), using only original strings and assets.
2. Uses native OS window chrome and `Ctrl` labels on Linux.
3. Shows Today as an editable Day note with Task and Event lines, a day strip and
   check-off.
4. Can create, edit, tag, schedule, nest and complete Tasks; each Task has its
   own Note pane.
5. Provides the 5-block QML editor with inline formatting and to-do Blocks that
   create real Tasks.
6. Shows an Inbox rail driven by the "no date and no folder" rule.
7. Parses the minimal Quick Add syntax and shows parsed chips.
8. Persists everything in SQLite across restarts, with no network access.
9. Exports a Woday-shaped CSV.
10. Ships as AppImage + `.deb`, with README, CONTRIBUTING and the disclaimer.

---

## 11. Settled decisions (traceability, D1–D22)

The "why" behind the principles above. Full text and the contradiction log C1–C17
are in `dev-docs/grill-session.md` §4 and §8.

| # | Decision | Source |
|---|---|---|
| D1 | Curated MVP (main slice), desktop-only, no cloud, design fidelity = v1 top priority | Q1 |
| D2 | Non-commercial; purpose = access for non-macOS desktop users | Q2 |
| D3 | No cloud service is possible (constraint, stated twice) | Q3 |
| D4 | Original name & assets; scraped docs internal reference only; `DESIGN.md` = marketing site, not UI spec | Q4 |
| D5 | Design fidelity = clone visual language/layout from screenshots with original strings/assets; build only populated pages | Q5 |
| D6 | OS order: Linux → Windows → macOS | Q6 |
| D7 | Execution = intensive few-day sprint; later de-scoped as a design input (no schedule in the plan) | Q7, Q9 |
| D8 | Product & repo name stays `open-woday` | Q8 |
| D9 | Full main slice stands; timeline is not used to trim scope | Q9 |
| D10 | `open-woday` retained; original assets, no verbatim strings, no doc redistribution, README disclaimer; ADR-0001 amended | Q10 |
| D11 | One codebase, Linux primary; Windows/macOS deferred build targets; **no webview** | Q11 |
| D12 | Main slice = three-panel shell, Today, Task CRUD, Inbox rail, Quick Add, bounded Notes editor, task note pane; textual guides for behaviour, screenshots' visual language for appearance | Q12 |
| D13 | Qt Quick / QML + C++ with qmake6, no webview; pixel fidelity costs more, per-OS tuning deferred | Q13 |
| D14 | Spec authority: screenshots = visual, 134 guides = behavioural, contradictions resolved here, guides never copied verbatim | Q14 |
| D15 | Native per-OS chrome + platform-correct modifier labels; inner UI identical; deliberate deviation | Q15 |
| D16 | Maintained community project; outside contributions; tagged releases; v0.1 + roadmap | Q16 |
| D17 | Unified `Item` + lightweight first-class `Event`; Tag many-to-many; Folder optional/single; View/Habit/day-note-as-entity deferred | Q17 |
| D18 | Split non-goals into permanent vs deferred | Q18 |
| D19 | In-house QML block editor, exactly 5 block types + inline bold/italic/link; Today = editable day note | Q19 |
| D20 | Minimal Quick Add parsing (kind, today/tomorrow/weekday, HH:mm, #tag, !priority), English only | Q20 |
| D21 | SQLite via Qt SQL, one file in app-data dir; CSV export only (no import mapper in v0.1) | Q21 |
| D22 | Linux-only v0.1 distribution: AppImage + `.deb`; Windows/macOS binaries deferred | Q22 |

All contradictions C1–C17 are resolved, dissolved, or explicitly routed to the
roadmap. Two results shape the plan directly: **C1/C2** → visual language cloned,
brand strings/assets not, only populated pages built; **C13** → no webview means
pixel fidelity costs more, accept it and tune per OS.

---

## 12. Risks & known trade-offs

| Risk | Mitigation | Source |
|---|---|---|
| Pixel fidelity is costlier without a webview | Hand-tuned QML tokens + a dedicated visual pass; accept the cost | C13, ADR-0002 |
| Font rendering / native chrome differ per OS | Platform-correct modifiers, per-OS tuning; inner UI identical | D15, C10 |
| Copyright / trademark exposure | Original strings/assets; docs reference-only; disclaimer | ADR-0001, ADR-0004 |
| The 134 guides contradict themselves | Written spec-authority rule; contradictions logged | A.9, ADR-0004 |
| No cloud means single-device data | One-way CSV export as an exit path | D3, D21 |
| "Completely similar" vs. a bounded slice | Only populated pages built; deviations recorded | C1, C2, §7.2 |

---

## 13. How this constitution is used

The project follows a spec-driven workflow (`.opencode/commands/bnvt-*.md`):

1. `/bnvt-constitution` created these three files.
2. `/bnvt-spec` finds the **next phase** in `roadmap.md`, creates a
   `phase-X-TITLE` branch, and writes a spec (`plan.md`, `requirements.md`,
   `validation.md`) under `dev-docs/specs/`, using `mission.md` and
   `tech-stack.md` for guidance.
3. `/bnvt-ticket` → `/bnvt-implement` (TDD) → `/bnvt-review` → `/bnvt-revise`
   → `/bnvt-documentation` → `/bnvt-commit` → `/bnvt-merge`.

Rules for spec and implementation work:

- Use `GLOSSARY.md` vocabulary throughout; respect the decisions recorded here
  and in `tech-stack.md`.
- Do not re-litigate settled decisions or absorbed ADRs; propose an amendment to
  this constitution instead.
- Work from `roadmap.md` phases in order; each deferred backlog item, when picked
  up, gets its own design round and, if hard to reverse, its own constitutional
  amendment.
- No schedule or effort estimates (D7, D9): order is by dependency, not calendar.
