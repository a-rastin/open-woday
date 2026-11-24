# open-woday — Grilling Session Record

> **Language note:** The conversation with the user is conducted in Persian. This
> file is the official record of the session and is kept in **English**, as are
> all downstream artifacts (`GLOSSARY.md`, `docs/adr/*.md`).
>
> **Method:** `/grill-with-docs` = `grilling` + `domain-modeling`. We work a
> **design tree** in **rounds**. Each round asks the current **frontier** (every
> decision whose prerequisites are settled), with a recommended answer for each.
> Settled decisions are recorded here as they crystallise; `GLOSSARY.md` and ADRs
> are created lazily, only when a term or a hard-to-reverse decision is resolved.

---

## 1. Known facts about the starting point (from the repo, 2026-11-23)

- `README.md`: one line — "Open source alternative to Woday".
- `LICENSE`: Apache License 2.0.
- `dev-docs/app-document.md`: a verbatim scrape of Woday's official docs —
  134 guides across 14 sections (7,812 lines). Covers the full product surface.
- `dev-docs/DESIGN.md`: a visual design system **derived from Woday's public
  homepage and changelog** — i.e. a *marketing website* design system, not an
  in-app UI specification.
- `logo/`: four image files that are Woday's own logos / wordmarks / mascot.
- `screenshots/`: three PNGs (Chromium captures).
- Git: single "Initial commit"; everything else untracked. No application code yet.

### What Woday is (high level)

- Proprietary, **paid** Mac app (one-time purchase, license key, 14-day refund).
- Requires **macOS 26 (Tahoe)**, built natively with Apple frameworks; iOS/iPad
  app exists; several features are explicitly Mac-only.
- **Local-first**: "everything stored locally on your Mac", no account needed.
- Optional **iCloud / CloudKit** sync across Macs and iPhone.
- Unified item model: Tasks, Events, Notes, Folders, Tags, Views, Habits, a
  per-day **day note**, Inbox, Trash. Every task/event also owns a block-based
  note. 17 block types, `@`-mentions, `#`-tags, live view blocks.
- Tasks: day/week/month/Someday scheduling, priority, reminders, subtasks,
  duration, location, attachments, repeats (habits + streaks), Waiting,
  In Progress, favorites, covers, convert/duplicate/history.
- **Quick Add**: natural-language parsing (dates, times, kinds, priority, tags,
  folders, habits), symbol menus, 5 languages, live preview, "Magic Lines".
- Calendar/Today/Upcoming, saved **Views**, list/board/table layouts,
  command palette, menu-bar panel, widgets, system-wide hotkeys, clipper, launcher.
- **AI & MCP**: BYOK providers, "Wodie" assistant, agent mode with approval,
  AI writing tools, a built-in **local MCP server** (~40 tools, port 24680).
- **Integrations**: Apple Calendar, Reminders, Notes, Google Calendar, Notion,
  Obsidian, Todoist, Linear, ICS/webcal feeds, Apple Weather.
- Import center (CSV/TSV/XLSX/MD/TXT/PDF), export-all-to-CSV, single-note export.

### Compiled technical facts

A fact-finding sub-agent read the entire `app-document.md` and reported on
2026-11-23. Its full findings are recorded in **Appendix A**. Facts are never
asked of the user. Two consequences for this session:

1. The "1:1 parity" option (Q1) is now concretely scoped: it means ~17 block
   types, 11 view templates, ~40 MCP tools, 10 integrations, a 5-language
   natural-language parser, Magic Lines, a menu-bar app, widgets, a clipper and
   an iOS app — i.e. a multi-year surface, not a weekend MVP.
2. The source docs are **internally contradictory** (see A.9), which surfaced a
   new branch in the tree (M: spec authority / contradiction resolution). It is
   logged below but belongs to a later round, not Round 1.

---

## 2. The design tree

```
open-woday
│
├── [R1 ✔] A. Vision & parity target ....................... SETTLED: Q1
├── [R1 ✔] B. Motivation & audience ........................ SETTLED: Q2
├── [R1 ✔] C. Resources & constraints ...................... SETTLED: Q3 + Q7, **timeline de-scoped as a design input by Q9**
├── [R1 ✔] H. Branding / IP & legal stance ................. SETTLED: Q4 → ADR-0001 (**amended** by Q10)
│   └── [R2 ✔] H1. Product & repo name ..................... SETTLED: Q8 + Q10 = keep `open-woday`
│
├── [R2 ✔] N. Design fidelity: meaning, source & boundary .. SETTLED: Q5 (option b)
├── [R2 ✔] D-a. Target operating systems ................... SETTLED: Q6 (Linux → Windows → macOS)
│
├── [R3 ✔] O. Definition of done & sprint scope ............ SETTLED: Q9 (option c — full main slice, ignore timeline)
├── [R3 ✔] H2. Name / ADR conflict ......................... SETTLED: Q10 (yes — ADR amended)
├── [R3 ✔] P. Linux-first meaning .......................... SETTLED: Q11 (one codebase) + **NO WEBVIEW**
│
├── [R4 ✔] I/O. Contents of the "main slice" ............... SETTLED: Q12 (option a)
├── [R4 ✔] D-b/c. App shell, language & tooling ........... SETTLED: Q13 (Qt Quick/QML + C++, qmake6) → ADR-0002
├── [R4 ✔] M. Spec authority & contradiction resolution .... SETTLED: Q14 (screenshots=visual, guides=behavioural)
│
├── [R5 ✔] D-d. Window chrome & ⌘→Ctrl adaptation ......... SETTLED: Q15 (native per-OS chrome; deliberate deviation)
├── [R5 ✔] E. Governance, maintenance & distribution ...... SETTLED: Q16 (maintained community project)
├── [R5 ✔] F1. Domain model shape & Event entity ........... SETTLED: Q17 (unified Item + lightweight Event) → `GLOSSARY.md`
├── [R5 ✔] L. Explicit non-goals ........................... SETTLED: Q18 (permanent vs deferred split) → ADR-0003
│
├── [R6 ✔] F2/F3. Block editor & editable day note ........ SETTLED: Q19 (in-house QML editor, 5 block types, Today = day note)
├── [R6 ✔] F4. Quick Add parsing scope .................... SETTLED: Q20 (minimal, English only)
├── [R6 ✔] G. Local storage engine & CSV export ........... SETTLED: Q21 (SQLite via Qt SQL, CSV export only)
├── [R6 ✔] Packaging / distribution formats ............... SETTLED: Q22 (Linux: AppImage + .deb)

**FRONTIER: EMPTY — all 22 design decisions settled.**
│
├── [CLOSED] J. AI & MCP details — out of the main slice (Q18)
├── [CLOSED] K. Integration details — out of the main slice (Q18)
└── [OUT OF TREE] implementation details below design granularity
    (exact DB schema, block serialization, per-widget CSS, build scripts,
    file paths) → handed to the written plan; explicitly not silently assumed
```

`[Rn]` = the round in which the decision first becomes askable. Dependencies
are deliberately pushed to later rounds; a question is never asked while one of
its prerequisites is still open in the same round. Branch **M** was added after
Round 1 was asked, because fact-mining revealed the source doc contradicts
itself; it will be put to the user in a later round once D is settled.

---

## 3. Round 1 — the frontier (roots, no prerequisites)

**Status: ANSWERED (2026-11-23).** All four questions were answered; see the
decision log in §4. Two answers added qualifications, one was only partially
answered, and one combination produced a genuine contradiction. Those are
recorded in §8 and put back to the user as Q5 (Round 2).

A "yes" to any question means "accept the recommended answer".

### Q1 — Vision & parity target
What does "open source alternative" actually mean: (a) a 1:1 full-parity clone
of all 134 guides, (b) a curated MVP (one vertical slice), or (c) a
reinterpretation inspired by the idea but free to diverge? Also sharpen the fuzzy
word "alternative": drop-in replacement (must import Woday data) vs. spiritual
successor (similar, independent)?

➡️ **Recommended:** (b) a curated MVP — a vertical slice of Task + Note + Today
view + simple Quick Add, with calendar, AI, and integrations explicitly deferred.
Reason: full parity is a multi-year effort and "alternative" need not mean
"everything". Treat it as a spiritual successor, not a drop-in replacement, for v1.

### Q2 — Motivation & audience
Why are you building this, and for whom? (a) a personal daily-driver you
dogfood, plus a small FOSS community; (b) a community-driven product meant to
compete for Woday's users; (c) a learning / portfolio project; (d) the foundation
of a future commercial product. Also sharpen "open source": code-only, or open
governance / outside contributions / strictly non-commercial?

➡️ **Recommended:** (a) — a local-first, privacy-focused daily planner for
yourself and power users / developers. This is the root that steers platform,
sync, and contribution model.

### Q3 — Resources & constraints
How much time and manpower are actually available — solo on weekends,
full-time, or a small team — and what is the realistic first-release horizon?

➡️ **Recommended:** solo, part-time, target a genuinely usable v0.1 in roughly
3 months, with no budget for hosted cloud services (which rules out any
server-dependent architecture).

### Q4 — Branding, naming & IP / legal stance
Woday is a paid, proprietary product (license keys, trademark, copyrighted
docs). The repo currently contains the name "open-woday", Woday's actual logo
files, a verbatim scrape of its copyrighted documentation, and a `DESIGN.md`
derived from its marketing site. Do we (a) keep the name/logos/look for
familiarity, or (b) adopt a fully original name and assets, treat the scraped
docs as internal reference only (no redistribution), and treat `DESIGN.md` as a
website-marketing reference rather than the app's UI spec?

➡️ **Recommended:** (b). Reusing a trademark and redistributing verbatim
copyrighted material carries real legal risk, and an open-source alternative
does not need brand imitation to be useful. Candidate ADR (hard to reverse,
surprising without context, genuine trade-off).

---

## 4. Decision log

| # | Decision | Round | Status | Recorded in |
|---|----------|-------|--------|-------------|
| D1 | **Parity target = curated MVP**, but v1 is **desktop-only**, has **no cloud server**, and its **top priority is design fidelity** ("completely similar" design). | R1 / Q1 | Settled, with an open tension (§8, C1–C2) | this file |
| D2 | **Non-commercial**, no commercial goal. The project's purpose is **access**: the upstream app is macOS-only, so this lets other desktop users use it. Primary audience = non-macOS desktop users. | R1 / Q2 | Settled | `GLOSSARY.md` ("Upstream app", "Target user") |
| D3 | **No cloud service is possible** — a stated *constraint*, confirmed twice (also restated inside the Q1 answer). The timeline half was later settled by Q7: a few-day sprint (D7). | R1 / Q3 + R2 / Q7 | Settled | this file |
| D4 | **Original name & assets**; the scraped upstream docs are **internal reference only** (never redistributed); `DESIGN.md` is a **marketing-site reference, not the app UI spec**. | R1 / Q4 | Settled, now **contested** by D8 | `docs/adr/0001-original-branding-and-reference-only-upstream-assets.md` |
| D5 | **Design fidelity = option (b)**: clone the visual language / layout from the 3 screenshots with original strings and assets; build only the pages the MVP populates; defer undocumented screens rather than invent them. | R2 / Q5 | Settled | `GLOSSARY.md` ("Reference UI") |
| D6 | **Target OS order: Linux → Windows → macOS** (Linux first — reverses the recommended Windows-first order). | R2 / Q6 | Settled; "first/then" semantics unclear → Q11 | this file |
| D7 | **Timeline = an intensive sprint of a few days** (rejects the recommended ~3 months). Later explicitly **de-scoped as a design input** by Q9 — it describes how the user will execute, not what we may design. | R2 / Q7 | Settled (execution fact only) | this file |
| D8 | **Product & repo name stays `open-woday`** (rejects Q8's recommendation). Conflict with D4 / ADR-0001 resolved by Q10. | R2 / Q8 | Settled | this file |
| D9 | **Main slice, not a timeline-trimmed demo** — Q9 = option (c). The full Q1(b) scope (Task + Note + Today + Quick Add) stands; the user asked me to *plan* it and to ignore the few-day execution timeline. | R3 / Q9 | Settled | this file |
| D10 | **Q8 wins on the name**: `open-woday` stays as product & repo name; the rest of Q4(b) stands (original assets, no verbatim strings, no doc redistribution, README disclaimer). ADR-0001 amended; `GLOSSARY.md` gains an `open-woday` term. | R3 / Q10 | Settled | `docs/adr/0001-...md`, `GLOSSARY.md` |
| D11 | **One codebase**, Linux primary, Windows/macOS deferred build targets — **and no webview**. The no-webview constraint rules out Electron, Tauri and any browser-based shell. | R3 / Q11 | Settled | this file |
| D12 | **Main slice (option a)**: three-panel shell, Today, Task CRUD with due date/priority/tags, Inbox rail, Quick Add parsing, a bounded Notes editor plus each task's own note pane. For screens with no screenshot: textual guides for behaviour, screenshots' visual language for appearance, no copied strings. | R4 / Q12 | Settled | this file |
| D13 | **Qt Quick / QML + C++ with qmake6, no webview.** Accepts that pixel fidelity costs more than CSS and needs per-OS font/chrome tuning. | R4 / Q13 | Settled | `docs/adr/0002-qt-quick-no-webview.md` |
| D14 | **Spec authority rule**: screenshots = visual source of truth; the 134 guides = behavioural source of truth; anything contradictory or unspecified is decided here and logged (ADR only if hard to reverse); guides are never copied verbatim. | R4 / Q14 | Settled | this file |
| D15 | **Native OS window decorations + platform-correct modifier labels** (Ctrl on Linux/Windows, ⌘ on macOS); the inner app UI stays identical everywhere. Recorded as a deliberate deviation from the macOS screenshots. | R5 / Q15 | Settled | this file (C10 resolved) |
| D16 | **Maintained community project**: outside contributions accepted, GitHub issues/PRs, tagged releases; the main slice is `v0.1` with a roadmap after it. | R5 / Q16 | Settled | this file (C12 resolved) |
| D17 | **Unified `Item`** with a kind (task / note / event), each able to own a block-based note body; `Tag` many-to-many; `Folder` optional/single; `View`, `Habit` and day-note-as-entity deferred. **Plus a lightweight first-class `Event`** (title, start/end, all-day) so Today can render event lines. | R5 / Q17 | Settled | `GLOSSARY.md` (product-domain terms) |
| D18 | **Split non-goals** — permanent: iCloud/CloudKit, Apple Calendar/Reminders/Notes/Weather/Intelligence, macOS-only shell, iOS app. Deferred: Calendar page, Upcoming, Habits, Views, Settings, Analytics, AI assistant, MCP server, third-party integrations. | R5 / Q18 | Settled | `docs/adr/0003-permanent-non-goals.md` |
| D19 | **In-house QML block editor** with exactly 5 block types (text, heading, to-do → real Task, bullet list, code) plus inline bold/italic/link; the **Today page is an editable day note** built from the same editor. The other 12 block types are deferred. | R6 / Q19 | Settled | `GLOSSARY.md` ("Day note", "Block") |
| D20 | **Minimal Quick Add parsing**: kind (task/note/event), today/tomorrow/weekday names, `HH:mm`, `#tag`, `!priority`, English only. Magic Lines is out of the main slice (AI half permanent, calculator half deferred). | R6 / Q20 | Settled | this file (C17 parser half resolved) |
| D21 | **SQLite via Qt SQL** (`QSqlDatabase`), one file in the user's app-data directory; **CSV export only** (Woday-compatible column shape), no import mapper in v0.1. | R6 / Q21 | Settled | this file |
| D22 | **Linux-only v0.1 distribution**: AppImage + `.deb`; Windows and macOS binaries deferred until those platforms are tested. | R6 / Q22 | Settled | this file |

## 5. Glossary terms resolved

`GLOSSARY.md` created with the resolved scope / identity terms:
**Upstream app**, **open-woday** (added after Q10), **Target user**,
**Reference UI**. Product-domain terms (Task, Note, Event, View, day note,
Inbox, Habit) are deliberately still absent — they resolve in the F round.
Still flagged for sharpening: "alternative", "clone", "MVP", "completely
similar", "desktop", "open source", **"main slice" (برش اصلی)**, **"no webview"**.

**Round 5:** Q17 resolved the domain vocabulary, so `GLOSSARY.md` gained a
`## Product domain` section: **Item, Task, Note, Block, Event, Tag, Folder,
Inbox** — each with an opinionated `_Avoid_` line (e.g. "to-do" is a Block, not
a synonym for Task). Definitions are domain-level only, with no schema or
implementation detail. **View, Habit and day note are deliberately absent** —
Q18 deferred them, so they are not yet resolved terms.

**Round 6:** Q19 resolved **Day note**, now added to `GLOSSARY.md` with its own
`_Avoid_` line. `GLOSSARY.md` now holds 13 canonical terms (4 scope/identity +
9 product-domain). **View and Habit remain deliberately absent** — Q18 deferred
them, so they are not yet resolved terms.

**Still withheld:** "Main slice" is a scope decision, not domain vocabulary —
putting its feature list in `GLOSSARY.md` would turn the glossary into a spec,
which the format forbids.

## 6. ADRs offered

`docs/adr/0001-original-branding-and-reference-only-upstream-assets.md` created
(**Status: accepted**). Q4(b) meets all three criteria: hard to reverse,
surprising without context (the repo still holds Woday logos and a verbatim doc
scrape), and a real trade-off against Q1's demand for design similarity.

A second ADR (local-only / no-cloud architecture) is a candidate but is
**deferred** until the platform & storage rounds settle what "local-only"
concretely means.

**Amended (2026-11-23, Q10 = yes):** ADR-0001 now records the final naming
decision (`open-woday` retained, original assets elsewhere). The "contested"
note has been removed; the rejected "fully original name" option is preserved
under *Considered Options* so a future contributor does not re-litigate it.

`docs/adr/0002-qt-quick-no-webview.md` created (**Status: accepted**) for Q13.
It meets all three criteria: hard to reverse (a toolkit swap is a quarter of
work), surprising without context (a webview was the obvious path for pixel
fidelity and three-platform support), and a real trade-off that the user
explicitly decided. Its *Considered Options* records the rejected webview path
so nobody reintroduces it later.

`docs/adr/0003-permanent-non-goals.md` created (**Status: accepted**) for Q18.
It meets all three criteria: hard to reverse (the boundary is explicitly
permanent), surprising without context (a reader of a Woday alternative will
ask "where is iCloud / where are the integrations?"), and a real trade-off
(deferred vs. permanently out). Its *Consequences* requires the README to carry
the list so new contributors see the boundary before proposing work.

**Candidate, decision now due:** an ADR for D14 (spec authority / using
copyrighted guides as behavioural spec without copying them). The rule
survived all six rounds unchanged, so it now meets all three criteria: hard to
reverse (code is being designed against it), surprising without context (a
reader will find a verbatim scrape of Woday's docs in the repo and wonder why
it is neither shipped nor treated as the spec), and a real trade-off. Put to the
user alongside the shared-understanding check. If accepted it becomes
`docs/adr/0004-spec-authority.md`. Four ADRs for 22 settled decisions remains
the intended "sparingly" rate.

---

## 7. Round 2 — the frontier (recomputed after Round 1)

**Status: ANSWERED (2026-11-23).** Q5 was accepted; Q6, Q7 and Q8 each
**rejected** the recommended answer. Two of them introduced contradictions with
already-settled decisions (§8, C6–C12). See D5–D8 in §4.

Prerequisites now settled: A (Q1), B (Q2), the cloud half of C (Q3), H (Q4).
Open roots and newly-unblocked decisions form this frontier. A "yes" accepts
the recommended answer.

### Q5 (N) — Design fidelity: definition, source of truth & boundary with Q4(b)
"Completely similar" is still fuzzy and collides with two settled decisions:
Q4(b) forbids copying the name, assets, mascot and verbatim strings, yet the
three reference captures contain exactly those ("Welcome to Woday", "Everything
Woday can do", "Ask Wodie", the red mascot, "Search, or create anything…");
and Q1(b) is a curated MVP, whereas full design similarity implies that every
page and panel exists (otherwise they are empty shells). Three readings:
(a) pixel-identical clone of everything including strings and mascot;
(b) same visual language, layout and information architecture, with original
copy, name and assets; (c) same interaction model only.

➡️ **Recommended:** (b). Source of truth = the three in-app screenshots. Clone
the dark sidebar, white rounded panels, day strip, badge chips and panel
structure; replace every brand string, the mascot and the name with originals;
build only the pages the MVP slice actually populates, and treat undocumented
screens as future work rather than inventing them. Reason: (a) directly
contradicts Q4(b) and detonates the MVP scope; (c) discards the stated top
priority of v1. The macOS-window-chrome question depends on Q6, so it is
deliberately pushed to Round 3.

### Q6 (D-a) — Target operating systems
Q2 reframed the project's purpose as access for users who cannot run a
macOS-only app. Which desktops does v1 target: (a) Windows + Linux only,
(b) Windows + Linux + macOS from one codebase, (c) macOS first, then others?

➡️ **Recommended:** (b) — one cross-platform codebase, priority
Windows → Linux → macOS, with macOS best-effort (nearly free with a web-based
shell, and excluding the upstream app's own platform would be odd). App shell,
language and tooling depend on this answer → Round 3.

### Q7 (C-residual) — Manpower & release horizon
Q3 settled only "no cloud service"; time / team / horizon were never answered
and must not be silently assumed.

➡️ **Recommended:** solo, part-time, a genuinely usable v0.1 in roughly
3 months. Governance and contribution model depend on this answer → later.

### Q8 (H1) — Does "original name" cover the repository too?
Q4(b) was accepted, yet the directory, git repo and README are still
`open-woday` / "Open source alternative to Woday". Nominative, descriptive use
of a trademark in a repo slug is common and low-risk, but it is not literally
"fully original". (a) rename both product and repo now, keeping "alternative to
Woday" only as descriptive prose in the README; (b) keep the repo slug and give
only the shipped product an original name.

➡️ **Recommended:** (a) — rename now, before any UI or branding code exists,
to a short, non-derived name (no "Woday" / "Wodie"). Reason: it is consistent
with the decision already made, and it is cheap now and expensive later.
The concrete name is requested alongside the answer.

---

## 8. Open contradictions & fuzzy-language challenges (domain-modeling log)

| # | Conflict | Status |
|---|----------|--------|
| C1 | "Design completely similar" (Q1) vs. "original brand / no copied assets or strings" (Q4b). The reference screenshots embed the very strings and mascot Q4(b) excludes. | **Resolved by Q5 (b)**: visual language cloned, brand strings/assets replaced |
| C2 | "Design completely similar" (Q1) vs. "curated MVP" (Q1b). Full similarity implies ~12+ pages exist; a vertical slice leaves most of them as empty shells. | **Resolved by Q5 (b)**: only MVP-populated pages are built |
| C3 | "Desktop-only" + "non-Mac users" (Q1/Q2) vs. macOS-native window chrome, ⌘K, menu bar and widgets in the reference UI. | Q6 settled (Linux-first); superseded by **C10** → Round 4 |
| C4 | `DESIGN.md` (black canvas, 80 px Inter Display, 60 px radii) vs. the reference UI (dark sidebar, white rounded panels, compact type). These are **two different** design systems. | Factually confirmed (A.10); Q4(b) correctly excluded `DESIGN.md` |
| C5 | "No cloud service" is a **constraint**, not a choice (stated twice as an impossibility). Recorded as a constraint, not a preference. | Settled (D3) |
| C6 | "Ready in a few days" (D7) vs. "design fidelity is v1's top priority" (D1) + Q5(b)'s clone of a three-panel window. | **Dissolved by Q9**: the user explicitly removed the timeline as a design input; scope is not to be trimmed for time |
| C7 | "First Linux, then Windows, then macOS" (D6) — three sequential releases vs. one codebase with Linux as the primary target. | **Resolved by Q11 (b)**: one codebase |
| C8 | Keep `open-woday` (D8) vs. "fully original name" (D4) vs. ADR-0001 (`accepted`). | **Resolved by Q10**: name retained, ADR amended |
| C9 | Q1(b)'s MVP slice (Task + Note + Today + Quick Add) vs. a few-day timeline. | **Dissolved by Q9 (c)**: full main slice stands |
| C10 | macOS-native traffic-light chrome, ⌘K and menu-bar conventions in the Reference UI vs. Linux-first target (D6) and Qt Quick (D13). | **Resolved by Q15**: native per-OS chrome, platform-correct modifiers, deliberate deviation recorded |
| C11 | "No cloud" (D3) still requires **local persistence** — data must survive a restart. | **Resolved by Q21**: SQLite via Qt SQL |
| C12 | D2 frames this as a usable daily driver + small FOSS community; D7 frames it as a one-off intensive sprint. Maintained product or throwaway? | **Resolved by Q16**: maintained community project, `v0.1` + roadmap |
| C13 | **"No webview" (D11) vs. design fidelity as v1's top priority (D1) + Q5(b)'s clone of a CSS-shaped UI.** Rounded white panels, badge chips, custom typography and the day strip are cheap in a browser engine and costly in a native toolkit. Also vs. "one codebase for 3 platforms" — font rendering and window chrome differ per OS. | **Resolved by Q13**: Qt Quick chosen, higher fidelity cost accepted, per-OS tuning deferred to Q15 |
| C14 | **"No webview" vs. the Q11(b) rationale.** My Q11 recommendation justified "one codebase" by "a webview yields all three nearly free"; the user accepted (b) but removed that justification. | **Resolved by Q13 / ADR-0002**: the new foundation is a single C++ codebase, not a webview |
| C15 | **"Don't invent undocumented screens" (Q5b) vs. the main slice containing Notes (Q1b / Q9c).** The Notes page and block editor have **no screenshot** (A.10) but are documented in the 134 guides. | **Resolved by Q12 + Q14**: textual guides for behaviour, screenshots' visual language for appearance |
| C16 | A.9's three internal contradictions become live once the textual guides are used as the spec for screens without screenshots. | **Resolved by Q14**: written rule + log; ADR only if hard to reverse |
| C17 | **"Bounded" was undefined in two places**: Q12's "bounded Notes editor" (5 block types vs. the guides' 17) and Q12's "Quick Add parsing" (a full 5-language NLP parser vs. minimal kind/date parsing). Both were scope ambiguities, not time-driven cuts. | **Resolved by Q19** (exactly 5 block types, in-house QML editor) **and Q20** (minimal English-only parser) |

**All contradictions C1–C17 are now resolved, dissolved, or explicitly routed
to the roadmap.** Nothing remains silently assumed.

Fuzzy terms sharpened so far: "other users" → **Target user**; "the original
app" → **Upstream app**; "the design system" → **Reference UI** vs. marketing
site. Still fuzzy: "alternative", "completely similar", "desktop", "MVP".

New fuzzy terms flagged in Round 2: "a few days" (no number), "intensive
sprint" (who builds it?), "ready" (runnable / installable / released?),
"first… then…" (sequence or priority?), "keep the name" (name only, or also
README wording and in-app strings?).

New fuzzy terms flagged in Round 3: **"main slice" / برش اصلی** (what exactly is
in it?), **"no webview"** (no Electron/Tauri specifically, or no HTML/CSS
rendering of any kind? Does a custom Skia/OpenGL renderer count?), and
**"documented screen"** (screenshot-backed vs. text-documented).

---

## 9. Deliberately deferred (and why)

Round 6 **was the last round**. Q19–Q22 are answered; the frontier is empty.
The only things left are implementation details below the design tree's
granularity, explicitly handed to the written plan rather than silently assumed.

- **Already closed:** J (AI & MCP details) and K (integration details) are out
  of the main slice by Q18; `View`, `Habit` and day-note-as-entity are deferred
  roadmap items, so they need no v0.1 decision.
- **Explicitly out of the tree (not silently assumed):** exact DB schema, block
  serialization format, per-widget CSS values, build scripts, source file paths
  and the concrete README / CONTRIBUTING wording. These are implementation
  details, not design decisions; the final plan will settle them.
- **Deferred to the roadmap, not this session:** Calendar page, Upcoming,
  Habits, saved Views, Settings, Analytics, AI assistant, MCP server,
  third-party integrations (D18 / ADR-0003).

After Q19–Q22 are answered the frontier is empty and the session moves to the
shared-understanding check. **No plan is written until the user confirms it.**

---

## 10. Round 3 — the frontier (recomputed after Round 2)

**Status: ANSWERED (2026-11-23).** Q9 rejected the recommendation (option c +
"ignore the timeline"), Q10 accepted it, and Q11 accepted it **with a hard new
constraint: no webview**. See D9–D11 and the new tensions C13–C16.

The frontier is deliberately small: **Q9 is the dominant branch** and most other
decisions hang off it, so they must wait a round. A "yes" accepts the
recommended answer.

### Q9 (O) — "A few days": definition of done, concrete scope & who builds it
Q7 replaces a ~3-month horizon with an intensive sprint of a few days. That
collides with nearly everything else: D1 makes **design fidelity v1's top
priority**; Q5(b) commits to cloning a three-panel window (a sidebar with 12+
rows plus Favorites/Recent/Folders/Tags sections, a top bar, a Today page with a
day strip / summary pill / badge chips, and a right-rail Inbox); Q1(b)'s slice
was Task + Note + Today + Quick Add, whose block editor and natural-language
parser are the two most expensive pieces; and D3 still requires data to survive
a restart without a server. Even a **static, pixel-faithful reproduction** of
the three captured screens is days of CSS work. So "a few days" forces a choice
between fidelity and function. Options: (a) a demo/prototype — faithful shell,
sample data, no real persistence; (b) a thin vertical slice — faithful Today +
sidebar + right rail, real Task create/complete with local persistence, with the
Notes editor, Quick Add NLP, Calendar and AI panel explicitly deferred; (c) keep
the full Q1(b) slice and let the timeline become weeks.

Also sharpen: how many days exactly, who does the building (the user solo / a
team / the AI agent), what "ready" means (runnable locally / installable package
/ tagged release), and whether this is a throwaway sprint or the seed of a
maintained product (C12).

➡️ **Recommended:** (b), **3–5 days**, with a hard rule that **design fidelity
wins over feature count**. Ship one runnable Linux desktop build: faithful Today
view + sidebar + right rail, real Task create/complete with local persistence,
and everything else visibly deferred (not faked). Reason: it is the only option
that honours both D1 and D7 at once; (a) delivers a screenshot rather than
software; (c) silently breaks the stated timeline.

### Q10 (H2) — Resolve the name / ADR-0001 conflict
Q4(b) ("fully original name & assets") was accepted and ADR-0001 was recorded as
`accepted` on that basis. Q8 now says keep `open-woday`. Both cannot hold. Which
is authoritative, and what exactly is kept?

➡️ **Recommended:** Q8 wins **on the name**; the rest of Q4(b) stands. Keep
`open-woday` as the product and repository identifier (a descriptive,
nominative handle for an unaffiliated FOSS alternative — low risk), but keep:
no upstream logo / wordmark / mascot files shipped, no verbatim doc
redistribution, no verbatim UI strings ("Welcome to Woday", "Ask Wodie"), and a
plain README disclaimer that the project is unaffiliated and non-commercial.
**Amend ADR-0001** (created in this session, unpublished) to reflect this and
record the trade-off; add a `GLOSSARY.md` term for `open-woday` once settled.
Sharpen the boundary: `open-woday` may appear as the product name in the window
title / About, but never standalone "Woday" / "Wodie" and never the mascot.

### Q11 (P) — What does "first Linux, then Windows, then macOS" mean?
(a) Three **sequential releases** — a Linux build first, then a separate Windows
build, then a macOS build; or (b) **one codebase** with Linux as the primary
development/test target and the other two as later build targets from the same
source?

➡️ **Recommended:** (b). Only the Linux build is produced and tested during the
few-day sprint; Windows and macOS are deferred build targets, not separate
implementations. Reason: three implementations in a few days are impossible,
and a webview-based desktop shell yields all three nearly free. The macOS-chrome
/ ⌘ question (C10) depends on this and moves to Round 4.

---

## 11. Round 4 — the frontier (recomputed after Round 3)

**Status: ANSWERED (2026-11-23).** All three recommendations were accepted
(Q12, Q13, Q14 = option a). See D12–D14. C13–C16 are resolved; C17 is a newly
surfaced scope ambiguity.

Q9 de-scoped the timeline, Q10 closed the naming conflict, and Q11 settled the
platform shape while adding a hard constraint ("no webview"). Q12 is now the
dominant branch; Q13 and Q14 are independent of it and are asked alongside.
A "yes" accepts the recommended answer.

### Q12 (I/O) — What exactly is in the "main slice"?
Q9(c) kept the full Q1(b) slice (Task + Note + Today + Quick Add) and asked me
to plan a "main slice" (برش اصلی) without trimming it for time. But that phrase
is undefined, and **C15** bites: the Notes page / block editor and the task
detail pane have **no screenshot** (A.10), while Q5(b) said to build only the
pages the MVP populates and not to invent undocumented screens. At the same
time the 134 guides document those screens textually in detail (17 block types,
badge rows, day notes, @-mentions, #-tags, conversions…). So: what ships, and
for screens without a screenshot, what is the design source?

Options: (a) three-panel window shell + Today page + Task CRUD + Inbox right
rail + Quick Add parsing + a bounded Notes editor, using the textual guides as
the behavioural spec and extrapolating the screenshots' visual language;
(b) only screenshot-backed screens, deferring Notes (contradicts Q9(c));
(c) treat the 134 guides as the full spec for every screen in the slice and
design freely.

➡️ **Recommended:** (a). Concretely: the three-panel shell (sidebar, top bar,
right rail); Today (day strip, summary pill, task/event lines, check-off);
Task create / edit / complete with due date, priority and tags; the Inbox rail;
Quick Add parsing in the top-bar field; a bounded Notes editor (text, heading,
to-do, bullet, code — the blocks the slice actually needs) plus each task's own
note pane. For screens with no screenshot: **textual guides for behaviour,
screenshots' visual language for appearance**, and no copied strings. Reason:
it keeps Q9(c)'s full scope, honours Q5(b) by grounding un-photographed screens
in the textual spec rather than inventing them, and makes the slice finite.
This is a *definition*, not a time-driven cut; the user may widen it.

### Q13 (D-b/c) — Shell, language & tooling with no webview
Q11(b) = one codebase for Linux / Windows / macOS, Linux first; Q11 also says
**no webview**. That rules out Electron, Tauri, any HTML/CSS/JS-rendered shell
and any browser-based local app. Fact (A.11): this machine has Qt 6.12.0
(`qmake6`, Qt6Quick/Widgets/Core), `g++`, GTK4 4.24.1, Rust/`cargo` and
`dotnet`; **no Flutter/Dart, no cmake**. Candidates: (a) Qt Quick / QML + C++
(qmake6), (b) Flutter (Dart, Skia/Impeller — needs installing), (c) Rust +
Slint / egui / iced, (d) Avalonia (.NET / C#), (e) GTK4 (weak on Windows/macOS,
so it conflicts with "one codebase").

Also resolve **C13 / C14**: rounded white panels, badge chips, custom
typography and the day strip are cheap in a browser engine and materially more
work in a native toolkit; and "one codebase for 3 platforms" needs a new
justification now that the webview rationale is gone. Sharpen "no webview":
does it forbid Electron/Tauri specifically, or any HTML/CSS rendering of any
kind? (Qt Quick uses its own GPU scene graph, not a web engine.)

➡️ **Recommended:** (a) **Qt Quick / QML + C++ with qmake6**. Reason: already
installed and buildable here (A.11); one C++ codebase really does target all
three desktops; no webview; QML is declarative and well suited to
custom-designed UIs (rounded rects, chips, custom fonts, animations), which is
the best available answer to C13; and it matches how this environment builds
desktop apps. Accept that pixel fidelity will cost more than CSS and will need
per-OS font / window-chrome tuning (→ D-d, Round 5).

### Q14 (M) — Spec authority & contradiction resolution
Once the main slice includes screens with no screenshot, we need an explicit
source of truth (C15, C16). Three candidates: the 3 screenshots (visual, 3
screens), `dev-docs/app-document.md` (behavioural, full, verbatim-copyrighted,
and internally contradictory per A.9), and our own decisions. A.9 documents
three contradictions (Mac-only vs. a shipping iOS section; consent-based vs.
silent auto-update; macOS 26 alongside GPT-6.1-class model names).

Options: (a) screenshots win for visuals, textual guides win for behaviour,
contradictions resolved by a written rule and logged; (b) treat the docs as a
literal spec, contradictions included; (c) ignore the docs for anything not in
the screenshots.

➡️ **Recommended:** (a), with an explicit rule: **screenshots = visual source
of truth; the 134 guides = behavioural source of truth; anything contradictory
or unspecified is decided in this session and recorded here (and in an ADR only
if hard to reverse)**. The guides are never copied verbatim into our product or
our docs (D10). Reason: it is the only rule that lets us build Notes at all
while keeping fidelity and legal hygiene.

---

## 12. Round 5 — the frontier (recomputed after Round 4)

**Status: ANSWERED (2026-11-23).** All four recommendations were accepted
(Q15, Q16, Q18 = option a/b as recommended; Q17 = "yes" to (a)+(i)). See
D15–D18. C10, C12, C17 are resolved or routed; `GLOSSARY.md` gained its
product-domain section and ADR-0003 was created.

All four Round 4 decisions settled. Four branches are now unblocked and none
depends on another question still open in this round, so all four are asked
together. A "yes" accepts the recommended answer.

### Q15 (D-d) — Window chrome & modifier keys across three operating systems
Q13 chose Qt Quick (native, no webview), but the Reference UI screenshots show
**macOS traffic-light controls, ⌘K and ⌘-labelled shortcuts**. Linux-first means
those do not exist natively there. Options: (a) use each OS's native window
decorations and platform-correct modifier labels (Ctrl on Linux/Windows, ⌘ on
macOS), keeping the *inner* app UI (sidebar, top bar, panels, chips) identical
everywhere; (b) draw custom traffic-light controls and keep ⌘ labels on all
platforms to match the screenshots exactly; (c) a fully custom title bar on
every platform for consistency.

➡️ **Recommended:** (a). Reason: Q5(b) narrowed "design fidelity" to the app's
own visual language and layout, not the OS chrome; fake traffic lights on Linux
or Windows would look wrong and age badly; Qt supplies native decorations and
platform-correct key names for free; and it preserves the one-codebase decision.
Record this as a **deliberate deviation** from the screenshots so nobody
"fixes" it later. A "yes" accepts (a).

### Q16 (E) — Governance, maintenance & distribution (resolves C12)
Apache 2.0 is already in the repo; D2 says non-commercial with a small FOSS
community; C12 still asks whether this is a one-off build or a maintained
project. Options: (a) a maintained community project — accept outside
contributions, use the existing GitHub repo with issues/PRs and tagged
releases, treat the main slice as v0.1 with a roadmap after it; (b) a one-off
personal build with no contribution infrastructure and a single release;
(c) maintained but solo-controlled — issues welcome, no outside code.

➡️ **Recommended:** (a). Reason: D2 explicitly frames the project as helping
other users, which implies something people can rely on and contribute to;
Apache 2.0 already signals contribution-friendliness; and setting it up now
costs almost nothing. A "yes" accepts (a). Detailed packaging formats are
deferred to Round 6 (they depend on this answer).

### Q17 (F1) — Domain model shape, and is Event a first-class entity?
With the main slice fixed (Q12) and Qt/C++ chosen (Q13): (a) one `Item` entity
with a `kind` discriminator (task / note / event), each item able to own a
block-based note body, `Tag` many-to-many, `Folder` optional/single, and
`View` / `Habit` / day-note-as-entity deferred; (b) separate `Task` and `Note`
tables with explicit foreign keys and no unified item; (c) plain files
(Markdown/JSON per item) with no database.

A second decision hides here: the Today screenshots show event lines ("Dinner
with Pete", "Design review · Anna"), but Q1(b)'s slice named no Event. (i) yes —
a lightweight Event (title, start/end, all-day) so Today can actually render
them; (ii) no — event-looking lines are display-only sample data with no entity
behind them; (iii) defer Events entirely.

➡️ **Recommended:** (a) + (i). Reason: a unified item is what makes
@-mentions, type conversions and "every task/event owns a note" cheap later;
separate tables would force a rewrite once Notes link to Tasks; and a
lightweight Event is needed for the Today page to look like the screenshots at
all. A "yes" accepts (a)+(i).

### Q18 (L) — Explicit non-goals: deferred, or permanently out?
Q12 deferred the Calendar page, Upcoming, Habits, Views, Settings, Analytics,
the AI assistant, the MCP server, all integrations, iCloud sync, widgets, the
menu-bar panel, the clipper and iOS. But Woday's identity partly *is* AI/MCP and
its integrations, and Q13 (no webview, Qt, Linux-first) makes several of them
literally impossible. Options: (a) everything is deferred to a roadmap;
(b) split — a set of **permanent non-goals** vs. a set of **deferred** items;
(c) no explicit non-goals list at all.

➡️ **Recommended:** (b), with this split. **Permanent non-goals:** iCloud /
CloudKit, Apple Calendar, Reminders, Notes, Apple Weather, Apple Intelligence,
the macOS-only shell (menu bar, widgets, Save for Later, ⌥Space launcher) and
the iOS app — all impossible without Apple frameworks. **Deferred (roadmap):**
Calendar page, Upcoming, Habits, saved Views, Settings, Analytics, the AI
assistant, the local MCP server, and third-party integrations (Notion,
Obsidian, Todoist, Linear, Google Calendar, ICS feeds). Reason: calling the
Apple-ecosystem features "deferred" would mislead contributors, and writing the
explicit no-s now is exactly the kind of boundary worth recording. A "yes"
accepts the split.

---

## 13. Round 6 — the frontier (expected final round)

**Status: ANSWERED (2026-11-23).** Q19, Q20, Q21 = "yes" (accepted); Q22 =
option (a). See D19–D22. C17 fully resolved; the frontier is empty.

Q19–Q22 are the last four branches. None depends on another question open in
this round. A "yes" accepts the recommended answer. After this round the
frontier is empty and the session moves to the shared-understanding check.

### Q19 (F2/F3) — Block editor & the Today page (resolves the editor half of C17)
Q12 set a "bounded Notes editor"; C17 flagged that "bounded" is undefined. With
Qt Quick / QML and no webview, there is no web-based editor library to adopt.
Options: (a) build a minimal in-house QML block editor; (b) use Qt's
`QTextDocument` rich-text component and approximate blocks; (c) reduce Notes to
plain multi-line text with no block types at all. Also: is the Today page an
**editable day note** (free writing around the task/event lines, as the guides
describe) or an agenda with checkboxes only? And which block types — the guides
list 17, Q12 suggested 5.

➡️ **Recommended:** (a), with exactly **5 block types** (text, heading, to-do
[which creates a real Task], bullet list, code) plus inline bold / italic /
link, and the Today page built as an **editable day note** from the same editor.
Reason: a webview is out; `QTextDocument` would fight the block model; "every
day has a day note" is core to the reference and Q12's Today already has
task/event lines; and 5 types is enough for the documented screens without
pretending to implement all 17. The other 12 block types, saved Views and
Habits are explicitly deferred (Q18). A "yes" accepts.

### Q20 (F4) — Quick Add parsing scope (resolves the parser half of C17)
Q12 includes "Quick Add parsing in the top-bar field". The guides describe a
5-language parser: kind prefixes/words, weekday and month names, 12/24-hour
times, week/month/Someday periods, `#tag`, `$folder`, `!/!!/!!!` priority,
rhythm phrases for habits, symbol menus (`# @ ! ~ * $ /`), live preview
highlights, saved links and Magic Lines. Options: (a) minimal — kind
(task/note/event), today/tomorrow/weekday names, `HH:mm`, `#tag`, `!priority`,
English only; (b) medium — add periods, duration/reminder symbols, more date
forms, 2–3 languages; (c) full parity — all 5 languages, habits, symbol menus,
live preview.

➡️ **Recommended:** (a). Reason: it covers the documented "Call Anna tomorrow
10:00" example and the visible top-bar field, stays finite, and habits / symbol
menus belong to deferred features. Magic Lines is out of the main slice: its
AI half is a permanent non-goal (Q18) and its built-in calculator half is
deferred. A "yes" accepts.

### Q21 (G) — Local storage engine & CSV export
D3 forbids cloud; C11 requires data to survive a restart; Q13 (Qt/C++, no
webview) rules out IndexedDB; Q17 gives a unified Item with blocks, tags and
folders. Options: (a) SQLite via Qt SQL (`QSqlDatabase`), one file in the
user's app-data directory; (b) plain files (one JSON/Markdown per Item);
(c) a custom append-only log. And for import/export: Woday exports a single
CSV with a fixed column set (A.3) — (i) compatible CSV import **and** export,
(ii) export only, (iii) neither in the main slice.

➡️ **Recommended:** (a) + (ii). Reason: SQLite via Qt SQL is already available,
handles the relational parts (tags many-to-many, links between items) that flat
files make painful, and is the natural fit for a native Qt app. One-way CSV
export gives users an exit path — important for a FOSS tool — without building
an import column-mapper in v0.1. Full CSV import is deferred. A "yes" accepts.

### Q22 (Packaging) — Distribution formats for v0.1
Q15 = native per-OS chrome; Q16 = maintained project with tagged releases;
Q13 = Qt/C++; Q6 = Linux first, then Windows, then macOS. Options: (a) Linux
only — AppImage (single binary, distro-agnostic) plus a `.deb`, with
Windows/macOS binaries deferred; (b) all three from v0.1 (AppImage, MSI, DMG);
(c) source-only tarball plus build instructions, no binaries.

➡️ **Recommended:** (a). Reason: Q6 says Linux ships first; AppImage needs no
packaging infrastructure and works across distributions; a `.deb` covers the
largest desktop Linux audience; and shipping untested Windows/macOS binaries
would contradict "Linux primary". A "yes" accepts.

---

## 14. Session state — frontier empty (2026-11-23)

**Status: SHARED UNDERSTANDING CONFIRMED (2026-11-23) — SESSION CLOSED.**

All 22 decisions are settled (D1–D22). Every contradiction C1–C17 is resolved,
dissolved, or explicitly routed to the roadmap. The design tree's frontier is
empty; nothing remains silently assumed — the only undecided items are
implementation details below the tree's granularity, explicitly handed to the
written plan in §9.

Per the `grilling` skill, no plan or implementation was produced until the
user confirmed a shared understanding. Both closing confirmations were accepted
on 2026-11-23:

| # | Closing check | Answer |
|---|---|---|
| Q23 | "Have we reached a shared understanding?" | **Yes** |
| Q24 | "Create ADR-0004 for the spec-authority rule (D14)?" | **Yes** |

The session is now closed: the frontier is empty, every branch of the design
tree has been visited, and the final deliverable has been written.

### Settled decisions at a glance

| Round | Questions | Decisions |
|---|---|---|
| R1 | Q1–Q4 | D1–D4 (vision, audience, no cloud, branding) |
| R2 | Q5–Q8 | D5–D8 (design fidelity, OS order, timeline, name) |
| R3 | Q9–Q11 | D9–D11 (main slice, name conflict, no webview) |
| R4 | Q12–Q14 | D12–D14 (slice contents, Qt Quick, spec authority) |
| R5 | Q15–Q18 | D15–D18 (window chrome, governance, domain model, non-goals) |
| R6 | Q19–Q22 | D19–D22 (block editor, Quick Add, storage, packaging) |

### English artifacts produced

- `dev-docs/grill-session.md` — this record: design tree, six rounds, decision
  log, contradiction log, compiled facts (Appendix A).
- `GLOSSARY.md` — 13 canonical terms (4 scope/identity + 9 product-domain),
  each with an opinionated `_Avoid_` line. No implementation detail.
- `docs/adr/0001-original-branding-and-reference-only-upstream-assets.md` —
  accepted, amended after Q10.
- `docs/adr/0002-qt-quick-no-webview.md` — accepted.
- `docs/adr/0003-permanent-non-goals.md` — accepted.
- `docs/adr/0004-spec-authority.md` — accepted (D14: screenshots = visual
  source of truth, 134 guides = behavioural source of truth, nothing copied
  verbatim).
- `docs/PLAN.md` — **the final deliverable**.

### Final deliverable (delivered)

`docs/PLAN.md` — a single **English** plan/spec for the main slice, covering:
the settled design tree and traceability, the main-slice feature list and
non-goals, the visual specification and hand-tuned tokens, the domain model,
the Qt/QML architecture, the concrete SQLite schema and block serialization,
packaging, governance/legal hygiene, a definition of done, a build order,
risks and a roadmap. It also explicitly settles the implementation details
handed over in §9 (§2.3 "Interpretations settled here"), so nothing is
silently assumed.

**No implementation was started.** The user's role is builder; this session's
role was planning (D9).

---

## Appendix A — Compiled technical facts (sub-agent report, 2026-11-23)

Read from the full `dev-docs/app-document.md` (7,812 lines). These are Woday's
**documented behaviours**, not decisions for open-woday. Reference only.

### A.1 Platform & runtime
- macOS 26 (Tahoe)+, "built natively with Apple's latest frameworks"; `.app`
  downloaded from the website and dragged to Applications (not the App Store).
- Frameworks implied by features: EventKit, CloudKit, UserNotifications,
  WidgetKit, NSStatusItem, Quick Look, MapKit/Apple Maps, Core Location + Apple
  Weather, Keychain, Apple Intelligence, macOS Automation, Accessibility.
- Menu-bar app (4 panel styles, ⌃⌥Space), Today widget (S/M/L, check-off in
  widget), system-wide hotkeys (⌥Space launcher, ⌃⌥C "Save for Later" clipper),
  local notifications, completion sounds (Pop/Tick/Chime/Bell/Melody).
- Save for Later clips web pages / Apple Mail into tasks; Quick Search launcher
  (⌥Space) is a floating Spotlight-like panel; in-app palette is ⌘K.
- Proprietary **one-time purchase** (not subscription): license key, activation
  panel (Status / Key / Licensed to / This device / Expires), 1 person / 2 Macs,
  "lifetime updates", 14-day refund. Four-page onboarding tour, sample content
  with "Sample" badge.
- Explicitly **Mac-only**: notification settings, all integrations, import &
  export, AI assistant + MCP server, Analytics, ⌥Space launcher, menu-bar panel,
  Save for Later. Their data still syncs to iPhone.

### A.2 Data model
- Core: Task, Event, Note, Folder, Tag, View; plus Habit (repeating task), the
  per-day **day note**, Inbox, Trash, Favorites, Files, saved links, sample items.
- "Every task and event also has its own note." Notes are block-based; the same
  editor is used in notes, task/event notes and day notes.
- Shared attributes: title, created/modified/completed timestamps, favorite,
  cover, icon, tags, folder, linked items, sync-source badges.
- **Task**: due precision = day (±time) / week / month / Someday / none; priority
  None/Low/Medium/High; reminder separate from due date; nested subtasks (to-do
  lines in its note, ≤8 levels); duration (15 min…4 h, drives calendar card
  height); location (Apple Maps); attachments ≤100 MB; Waiting (return presets +
  follow-up badge); In Progress; convert to Note/Event (repeat rules dropped);
  duplicate (with subtasks); optional dated History log.
- **Habit**: repeat Every N day/week/month/year (1–99), weekday buttons, monthly
  "On the", yearly month+day, Ends Never/On date/After N completions, reminder
  time; streak, best streak, monthly rate, 7-day history. Cannot wait; converting
  drops the repeat rule.
- **Event**: one schedule badge (start/end or all-day), 15-min drag, Add Google
  Meet link, calendar, reminder, folder, tag, linked task/note; own note.
  Synced Apple/Google/feed events are **read-only** in Woday; Join links for
  Zoom/Meet/Teams/FaceTime/Slack/Discord/Webex/Whereby, RSVP, guests.
- **Note**: cover, icon picker (Icons/Emoji/3D Icons/Upload), badge row, dated
  notes (pinned to a day). Export Markdown/PDF/RTF/Print.
- **17 block types**: Text, Heading, Bullet, Numbered, Task (real task), Event
  (real event), Note (separate linked note), Quote, Divider, Image, Video
  (YouTube/Vimeo), Code (15 languages), Link (bookmark), File (≤100 MB), Table
  (3×3, header, row/col ops), View (live saved view). Markdown shortcuts,
  six-dot handle (Turn into / Select / Duplicate / Copy / Delete), inline
  formatting (bold/italic/underline/strike/link/code).
- `@` links tasks/events/notes (in a to-do line it sets the due date instead);
  `#` tags; to-do symbols `@` due / `!` priority / `~` duration / `$` folder /
  `#` tag. Task/Event/Note blocks create real linked items.
- Extensive drag-and-drop drop-target rules (reschedule, file, nest, link).
- **Folder**: item in at most one folder; unfiled + no date → Inbox. **Tag**:
  many per item, spans folders. **Inbox**: no date AND no folder.
- **View**: saved Layout (Cards/List/Table), Group by, Sort by, Types, Status,
  Due (relative), Priority, Source, Tags, Folders, Hide completed, Fields —
  AND-combined, live. Global / Folder / Tag / Favorites / Page kinds; 11
  templates; embeddable in notes as a live `/view` block.

### A.3 Storage & sync
- "Everything you create is stored locally on your Mac. There is no account."
- iCloud sync via **private CloudKit database**; takes effect on next app start;
  same container on iOS (on by default there).
- Syncs: all Woday content, including items imported from other services;
  sidebar folder/tag order; Tasks/Events page tabs.
- Does **not** sync: connection settings/tokens/vaults/feeds (per-Mac), chat
  history, uploaded video files, API keys (Keychain), Favorites reorder, samples.
- Reset local data erases everything and, with iCloud on, deletes data on other
  devices too; docs advise exporting CSV first.
- **Export all → CSV**: one row per item (notes → tasks → events, Trash excluded);
  columns Type, Title, Status, Start/Due, End, All Day, Duration (min), Priority,
  Repeat, Location, Tags, Folder, Parent, Linked Note, Details, Created,
  Modified, Completed; UTF-8.
- Import: CSV/TSV, XLSX (first sheet), MD, TXT, PDF text; ≤~128 MB; preview +
  column mapping; "Woday export detected" auto-rebuilds.

### A.4 Views & navigation
- Pages: Today ⌘1, Upcoming ⌘2, Calendar ⌘3, Notes ⌘4, Tasks ⌘5, Events ⌘6,
  Analytics ⌘7, Trash ⌘8; plus Habits, Folders, Tags, Views.
- Main window = sidebar / top bar / centre page / right rail. Top bar has ⌘K
  "Search, or create anything…", New, Inbox & views, AI assistant.
- Views as pill tabs; right rail scopes (Views / Time / Lists / Scope); ⌘K
  palette with commands, `>` command-only mode, quick answers, "Ask Wodie".
- Layouts: List / Board / Table; Calendar Day / Days(2–5) / Week / Month.

### A.5 Quick Add & natural language
- Kind prefixes `T:/task:/todo:`, `E:/event:`, `H:/habit:`, `N:/note:`; kind
  words; command sentences; auto-guess (rhythm → Habit, event word/time → Event,
  else Task).
- Dates: today/tomorrow/tonight(20:00), weekdays, month names, years 2000–2099,
  24-hour and on-hour 12-hour times. **Numeric dates (`10/23`, `2026-10-23`)
  are NOT recognised.** Periods: this/next week, month, `week 45`, someday.
- `#tag` (creates if missing), `$Folder` (existing only, never creates),
  `!/!!/!!!` priority. Rhythm phrases incl. `every mon, wed and fri`,
  `every other friday`, `every weekday`, `every N days`.
- Symbol menus `# @ ! ~ * $ /`; menu chips win over typed words. Live preview
  with colour-coded highlights; saved links become tasks.
- Multi-language (Spanish, German, French, Hungarian, 简体中文) for date/repeat
  words; kind prefixes/words and `tonight` are English-only.
- **Magic Lines ✦**: built-in calculator (arithmetic, units, currency, date
  math, no AI) + AI answers; per-note or everywhere; presets (Helper, Calories,
  Budget, Workout…); actions → Task/Event chips, Fix, Log time; sections/labels,
  subtotals, totals computed by Woday (not AI).

### A.6 AI & MCP
- BYOK: Anthropic, OpenAI, Google Gemini, xAI Grok; keys in macOS Keychain, sent
  only to their own provider; no Woday AI subscription. Model picker with Auto.
- Sent per request: the message + chat history + a small snapshot of today
  (≤8 events + ≤8 tasks, overdue count), remembered facts, attached files,
  results of lookups. No calls unless an AI feature is used. Chat history stays
  on Mac (not iCloud).
- **Wodie**: mascot assistant in the right pane; streaming, context picker,
  conversation management, capabilities = the MCP tool set; limits: no web
  search, ≤12 steps/message, read-only synced events, no permanent deletion.
- Approval card ("Ask before changes" default) + Preview for note edits;
  ⌘Z undo. **Agent mode**: four proactive read-only briefings/day.
- AI writing tools (summarize, improve, fix, shorten, lengthen, tone, translate
  to 30 languages); on-device Apple Intelligence fallback; quick answers in ⌘K.
- **Local MCP server**: off by default, `http://127.0.0.1:24680/mcp` (port
  1024–65535), Bearer token in Keychain, Streamable HTTP POST. Clients: Claude
  Desktop, Cursor, Claude Code, Windsurf/VS Code. ~40 tools across Your day &
  calendar, Tasks & habits, Notes, Search/organization/views, Trash/stats/more.

### A.7 Integrations
Everything optional; "Woday works fully on its own".

| Integration | Auth | Direction |
|---|---|---|
| iCloud | Apple ID | Two-way, all content |
| Apple Calendar | macOS TCC permission | Read + optional write-back |
| Apple Reminders | macOS TCC permission | Read + optional two-way |
| Apple Notes | macOS Automation | Read + optional write-back |
| Google Calendar | Google sign-in | One-way (default) or two-way per account |
| Notion | OAuth (browser) | Pages → notes (to-dos two-way); databases → tasks/notes two-way |
| Obsidian | Local vault folder | From files (checkboxes two-way); optional full two-way |
| Todoist | Personal API token | Two-way (default) or one-way |
| Linear | Personal API key | From Linear; completion flows back |
| ICS/webcal feeds | URL paste | Read-only |
| Apple Weather | Location permission | Read-only, on Today |

Connection settings/tokens are per-Mac and never synced. "Soon" integrations:
Things, Slack, Jira, Confluence.

### A.8 Legal / IP-bearing material
- Proprietary, paid, one-time licence, 14-day refund, `soltwagner@gmail.com`.
- Identifiers/copy: name **"Woday"**; assistant/mascot **"Wodie"** ("red fluffy
  mascot with glasses"); the "bright, furry W mascot"; logo/wordmark art; named
  themes (Liquid Glass Light/Dark, Classic, Midnight) and the "Default" black
  chrome with white cards; exact UI copy strings ("Write, or type / for
  blocks…", "Search, or create anything…", "Ask, plan, create…"); the entire
  134-guide documentation text, verbatim, including a verbatim doc-index
  appendix. Docs also reference a Privacy Policy and Terms of Use.

### A.9 Internal contradictions in the source docs (factual)
1. "An iOS app is on the roadmap. For now, Woday runs on Mac only" vs. a full,
   shipping **iPhone & iPad** section (tab bar, iCloud on by default, "same
   iCloud container").
2. Auto-updates: "Nothing installs without your consent" vs. "new versions
   download and install automatically in the background".
3. macOS 26 (Tahoe) requirement alongside model names such as GPT-6.1 /
   Gemini 3.1 Pro / Grok 4 (doc dated 2026-11-23).

**Implication for the design tree:** the scraped docs cannot be treated as a
single consistent specification. A future branch (**M**) must settle how
contradictions are resolved and what the actual source of truth is.

### A.10 Visual reference found in the repo (checked 2026-11-23)

The three `screenshots/*.png` are **in-app UI captures**, not the marketing
website:

1. **Today page** — dark sidebar (Today, Upcoming, Calendar, Notes, Tasks,
   Events, Favorites, Recent, Folders, Tags); white rounded centre panel with a
   day strip, a "Good evening, you have 2 events, 1 daily habit and 2 important"
   summary pill, and task/event lines with coloured badge chips; white rounded
   **Inbox** right rail.
2. **Upcoming page** — same chrome, "Next 30 days" list, Inbox right rail.
3. **Today page** — fuller sidebar (adds Habits, Folders, Tags, Views,
   Favorites, Notes), centre Today panel, and a right rail showing **AI
   Assistant / "Ask Wodie"** with the red mascot and suggestion chips.

Common to all three: macOS traffic-light window controls, a top bar with
"Search, or create anything…" (⌘K), a near-black sidebar, white panels with
large rounded corners, compact UI typography and coloured badge chips.

Factual consequences:

- `DESIGN.md` (black canvas, 80 px Inter Display, 60 px radii) describes a
  **different** surface — the marketing website. Q4(b)'s framing ("not the app
  UI spec") is confirmed; the screenshots are the only visual source of truth
  for the app.
- Visual coverage is thin: ~3 screens out of 12+ pages and many states. There
  is **no reference** for the Calendar grid, the Notes block editor, Tasks
  board/table layouts, Habits, Views, Settings, Analytics or Trash.
- The captures embed IP-bearing content verbatim: "Welcome to Woday",
  "Everything Woday can do", "Ask Wodie", the Wodie mascot, "Ask, plan,
  create…" and exact UI placeholder strings. A pixel-faithful reproduction
  would reproduce these — which Q4(b) excludes (§8, C1).

### A.11 Local toolchain inventory (checked 2026-11-23, for Q13)

| Tool | Present | Version / path |
|---|---|---|
| `qmake6`, `qmake` | yes | `/usr/bin/qmake6` |
| Qt6 (`Qt6Quick`, `Qt6Widgets`, `Qt6Core`) | yes | **6.12.0** via pkg-config; headers in `/usr/include/qt6` |
| `g++` | yes | `/usr/bin/g++` |
| GTK4 | yes | 4.24.1 |
| `rustc`, `cargo` | yes | `~/.cargo/bin/` |
| `dotnet` | yes | `/usr/bin/dotnet` |
| `node`, `npm` | yes | mise-managed |
| `flutter`, `dart` | **no** | — |
| `cmake` | **no** | — |

Implication (factual): a Qt Quick / QML + C++ project can be configured and
built here immediately with `qmake6`; Flutter and CMake-based paths would
require installing toolchains first. GTK4 is present but has weak
Windows/macOS support relative to the "one codebase, three desktops" decision.
