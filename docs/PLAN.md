# open-woday — Main Slice Plan (v0.1)

> **Status:** agreed. Shared understanding confirmed on 2026-11-23 after six
> grilling rounds.
> **Language:** English. The interview itself was conducted in Persian; the
> living record is `dev-docs/grill-session.md`.
> **Companion documents:** `GLOSSARY.md`, `docs/adr/0001`–`0004`,
> `dev-docs/app-document.md` (internal reference only), `dev-docs/DESIGN.md`
> (marketing-site reference only), `screenshots/` (visual reference only).

---

## 0. What this document is

A single, buildable specification for the **main slice** — the `v0.1` desktop
release of open-woday. It is the deliverable of the grilling session: it turns
22 settled decisions (D1–D22), 17 resolved contradictions (C1–C17) and 13
canonical glossary terms into an implementation plan.

It also settles, explicitly, the details that the design tree deliberately left
below its granularity: the concrete database schema, block serialization,
visual values, build scripts, source paths and repository wording. Nothing here
is silently assumed; each such settlement is listed in §2.3.

No schedule or effort estimate appears here. The execution timeline is the
builder's concern (D7, D9).

---

## 1. Product definition

**open-woday** is a non-commercial, open-source, local-first desktop planner.
Its purpose is to give users who cannot run a macOS-only proprietary app the
same kind of day-planning experience. It targets Linux first, then Windows,
then macOS, from one codebase.

- **Name:** `open-woday` (product and repository). May appear in the window
  title and About panel. Standalone "Woday", "Wodie", the mascot, upstream
  logos and upstream UI strings never appear in the app (D4, D8, D10).
- **License:** Apache 2.0. Unaffiliated, non-commercial; README carries a plain
  disclaimer (D10, ADR-0001).
- **Governance:** maintained community project — GitHub issues/PRs, tagged
  releases, `v0.1` followed by a roadmap (D16).
- **Data:** entirely local. No server, no account, no cloud, no sync in v1
  (D3, D21).
- **Design priority:** fidelity to the reference app's visual language is the
  top priority of v1 (D1), bounded by D5.

**Canonical vocabulary** lives in `GLOSSARY.md`: Upstream app, open-woday,
Target user, Reference UI; Item, Task, Note, Block, Event, Tag, Folder, Inbox,
Day note. Use those words, and only those words, in code, docs and issues.

---

## 2. Traceability

### 2.1 Decisions → sections

| Decision | Subject | Plan section |
|---|---|---|
| D1, D9 | Curated main slice, no timeline trimming | §3 |
| D2, D16 | Audience, non-commercial, governance | §1, §11 |
| D3, D21 | No cloud; SQLite; CSV export | §8 |
| D4, D8, D10 | Name, original assets, reference-only docs | §1, §5.4, §11 |
| D5, D12 | Design fidelity, main-slice contents | §3.1, §5 |
| D6, D11, D22 | OS order, one codebase, no webview, packaging | §4, §10 |
| D13, ADR-0002 | Qt Quick / QML + C++ | §4, §9 |
| D14, ADR-0004 | Spec authority | §5.1, §7 |
| D15 | Native window chrome, modifier keys | §5.5 |
| D17 | Unified Item + lightweight Event | §6 |
| D18, ADR-0003 | Permanent non-goals vs deferred | §3.3, §15 |
| D19 | 5-block QML editor, editable day note | §6.3, §7.4 |
| D20 | Minimal Quick Add parsing | §7.6 |
| Q23/Q24 | Shared understanding, ADR-0004 | §0, §11 |

### 2.2 Contradictions

All of C1–C17 are resolved, dissolved or routed to the roadmap; see
`dev-docs/grill-session.md` §8. Two results shape this plan directly:

- **C1/C2** → visual language is cloned, but brand strings/assets are not, and
  only MVP-populated pages are built (no empty shells).
- **C13** → no webview means pixel fidelity costs more; accept it and tune
  per OS.

### 2.3 Interpretations settled *here* (implementation level)

These were handed over by §9 of the session record. They are recorded so they
are not silently assumed:

1. **Sidebar rows.** v0.1 renders only real, populated destinations: `Today`,
   a `Folders` section and a `Tags` section, plus an Inbox entry in the right
   rail. Deferred pages (Calendar, Upcoming, Habits, Views, Events list) are
   **absent**, not shown as disabled placeholders — consistent with D5/D12
   ("no empty shells"). Full-width page lists are roadmap items.
2. **Day note is not a new entity.** D17 deferred "day-note-as-entity"; D19
   requires an editable day note. It is therefore a `Note` Item with a `day`
   column (`items.day`), one per date — no separate table, no new kind.
3. **Deletion.** Soft-delete via `items.deleted_at`. No Trash UI in v0.1
   (Trash was not in D12's slice); permanent deletion is not exposed.
4. **Search.** The top-bar field is the **Quick Add** entry point (D12). A full
   search / command palette is not in v0.1; it is a roadmap item. `Ctrl+K`
   (`⌘K` on macOS) focuses that field.
5. **Standalone list pages.** No separate Tasks / Notes / Events pages in v0.1.
   Items are reached through Today, the Inbox rail, the Folders/Tags sections
   and the detail pane.
6. **Visual tokens** (§5.3) are hand-tuned from the screenshots, **not** copied
   from `DESIGN.md`, which is the marketing site's design system (D4/D14).

---

## 3. Scope

### 3.1 In scope (the main slice)

1. **Three-panel window shell:** dark sidebar, top bar, centre page, right
   rail. Back/forward, a Quick Add field, a New button, a right-rail toggle.
2. **Today page** as an editable **Day note**: day strip, a summary line,
   Task and Event lines with badge chips, check-off, and free writing around
   them.
3. **Tasks:** create, rename, edit, complete/reopen; due date (day, optional
   time), priority (none/low/medium/high), Tags, Folder, nested subtasks,
   inline duration, and each Task's own Note pane.
4. **Events:** lightweight first-class Items (title, start/end, all-day) so
   Today can render event lines. No calendar grid.
5. **Inbox rail:** Items with no due date and no Folder.
6. **Notes:** an in-house QML block editor with exactly 5 block types — text,
   heading, to-do (creates a real Task), bullet list, code — plus inline
   bold / italic / link. Used for the Day note, standalone Notes and each
   Task's own note.
7. **Quick Add parsing** in the top-bar field (minimal, English only).
8. **Local persistence** via SQLite, surviving restarts.
9. **CSV export** (one-way, Woday-compatible column shape).
10. **Linux packaging:** AppImage + `.deb`.

### 3.2 Deliberately deferred (roadmap, not v0.1)

Calendar page, Upcoming, Habits / streaks, saved Views, Settings, Analytics,
the AI assistant, the local MCP server, third-party integrations (Notion,
Obsidian, Todoist, Linear, Google Calendar, ICS feeds), full CSV import,
Trash UI, search / command palette, standalone list pages, 12 further block
types, symbol menus, live Quick Add preview, Magic Lines calculator,
multi-language parsing, Windows and macOS binaries.

### 3.3 Permanent non-goals (ADR-0003)

iCloud / CloudKit, Apple Calendar, Reminders, Notes, Weather and Apple
Intelligence; the macOS-only shell (menu bar, widgets, Save for Later, ⌥Space
launcher); the iOS app. These are impossible on a Linux-first, no-webview Qt
app. The README must carry this list.

### 3.4 Explicit deviations from the reference UI

- Native OS window decorations instead of macOS traffic lights (D15).
- Platform-correct modifier labels: `Ctrl` on Linux/Windows, `⌘` on macOS.
- A shorter sidebar (§2.3.1) — only real destinations.
- All strings original: the top-bar placeholder is e.g. "Add a task, note or
  event…", not "Search, or create anything…"; there is no "Welcome to Woday",
  "Ask Wodie" or mascot.
- No AI panel in the right rail; the rail shows Inbox only.

---

## 4. Platform & technology

| Choice | Value | Source |
|---|---|---|
| Language / UI | C++ + Qt Quick / QML | D13, ADR-0002 |
| Qt version | 6.12 (installed locally) | A.11 |
| Build system | `qmake6` (`.pro` file); `cmake` is not installed | A.11 |
| OS targets | Linux (primary, v0.1) → Windows → macOS | D6, D11 |
| Codebase | one; Windows/macOS are later build targets, not ports | D11 |
| Webview | **none** — no Electron, Tauri, HTML/CSS rendering | D11, ADR-0002 |
| Storage | SQLite via Qt SQL (`QSqlDatabase`) | D21 |
| Network | none at runtime | D3 |

Qt Quick uses its own GPU scene graph, which is not a web engine, so it
satisfies "no webview". The block editor, rich text and all panels are built in
QML/C++ — no web-based editor library (ADR-0002 consequences).

---

## 5. Visual specification

### 5.1 Source of truth (ADR-0004)

- **Visuals:** the three in-app screenshots in `screenshots/` (Today, Upcoming,
  Today + AI Assistant). They show a dark sidebar, white rounded content
  panels, a top bar and a rounded right rail.
- **Behaviour:** `dev-docs/app-document.md` (the 134 guides), for anything not
  visible in a screenshot — notably the Notes editor and the task detail pane.
- **Never:** copy guide text, upstream UI strings, logos, the mascot, or use
  `DESIGN.md` as the UI spec. Guides are internal reference only.
- Contradictions in the guides (three are documented in A.9) are resolved by
  the written rule in ADR-0004; new ones are logged in the grilling record.

### 5.2 Window structure

```
┌───────────────────────────────────────────────────────────────┐
│ [←] [→]   [ Add a task, note or event…        ⌘K ]  [+] [▤]   │  top bar
├──────────────┬────────────────────────────────┬───────────────┤
│ Today        │  Today                         │ Inbox         │
│ ── Folders   │  2026, November 23             │  Overdue      │
│    Work      │  [23][24][25][26]… day strip   │  Inbox        │
│    Personal  │  ┌ summary line ─────────────┐  │  Today        │
│ ── Tags      │  │ 2 events · 1 task · …     │  │  …            │
│    Health    │  └───────────────────────────┘  │               │
│    Work     │  ☐ Reply to Anna…   [Work]      │               │
│              │  • Design review · Anna 10:00   │               │
│              │  ☐ Pushups ×100    [Health]     │               │
│              │  (free writing / day note)       │               │
└──────────────┴────────────────────────────────┴───────────────┘
```

Rounded white panels sit on a dark canvas; the sidebar is dark; the right rail
is a white rounded card. Resize behaviour: the sidebar folds first, then the
right rail.

### 5.3 Tokens (hand-tuned from the screenshots, not from DESIGN.md)

These are starting values to be verified against the screenshots during the
visual-tuning pass; they are **not** the marketing site's 80 px display type or
60 px radii.

| Token | Value | Use |
|---|---|---|
| `canvas` | `#1C1C1E` | window background behind panels |
| `sidebar` | `#17181C` | sidebar background |
| `sidebarText` | `#E8E8E8` | sidebar labels |
| `sidebarMuted` | `#8A8A8A` | section headers, chevrons |
| `panel` | `#FFFFFF` | centre page, right rail |
| `panelText` | `#161616` | body text on panels |
| `muted` | `#757575` | metadata, timestamps |
| `chip` | `#F5F5F5` | neutral chips |
| `accent` | `#E8503A` | today cell, important badge, priority high |
| `radiusPanel` | `18px` | white panels, right rail |
| `radiusChip` | `999px` | pills / tag chips |
| `radiusControl` | `8px` | buttons, inputs |
| `gap` | `10–12px` | between panels |
| `font` | Inter (fallback: system UI) | all UI text |
| `bodySize` | `13–14px` | rows, labels |
| `titleSize` | `28px` | page title ("Today") |
| `dayStripCell` | `44×56px` | day strip cells |

Tag chips use pastel backgrounds (e.g. green for Health, blue for Work, purple
for Ideas) as seen in the screenshots; exact hues are tuned in the same pass.

### 5.4 Strings & branding

- Product name `open-woday` appears in the window title and About panel only.
- No upstream string is reused. All placeholders, button labels, menu items,
  summary text and empty states are written fresh.
- `logo/` and `screenshots/` are reference material, never bundled as assets.
  Original icons are drawn or sourced with a compatible license.

### 5.5 Window chrome & shortcuts (D15)

- Use each OS's native window decorations. Do not draw traffic-light controls
  on Linux or Windows. Record this as a deliberate deviation.
- Labels: `Ctrl` on Linux/Windows, `⌘` on macOS. Bind `Ctrl+K`/`⌘K` to focus
  the Quick Add field and `Ctrl+N`/`⌘N` to create a Task.
- The inner app UI is identical on all three platforms.

---

## 6. Domain model (D17, GLOSSARY.md)

### 6.1 Canonical terms

**Item** — the single thing a user creates; everything is one Item of a
particular kind. **Task**, **Note**, **Event** — the three kinds. **Block** —
one line of a Note; a to-do Block is also a real Task. **Tag** — a label
attachable to any Item, crossing Folders. **Folder** — the single optional
place an Item is filed. **Inbox** — Items with no due date and no Folder.
**Day note** — the editable Note belonging to a single day.

Never say "to-do" for a Task, "document" for a Note, "appointment" for an
Event, or "daily note" for a Day note.

### 6.2 Entities

One `Item` table with a `kind` discriminator (`task`, `note`, `event`), rather
than separate Task/Note tables. This is what makes @-mentions, type conversions
and "every Task/Event owns a Note" cheap later.

- **Item:** identity, kind, title, timestamps, completion, soft-delete,
  optional scheduling (`due_at`, `due_precision`, `start_at`, `end_at`,
  `all_day`), `priority` (0–3), `duration_min`, `parent_id` (subtasks),
  `folder_id`, `day` (for a Day note), `sort_order`.
- **Block:** belongs to an Item (a Note body), ordered, one of 5 types, with
  inline formatting ranges and an optional `task_id` for to-do Blocks.
- **Tag / Folder:** as above; `item_tags` is many-to-many.
- **Link:** @-mentions and linked notes/events between Items.

**Not entities in v0.1:** View, Habit, a separate DayNote type (§2.3.2).

### 6.3 The day note

`items` row with `kind='note'` and `day='YYYY-MM-DD'`, unique per date. The
Today page opens it; its Blocks include the day's Task and Event lines (to-do
Blocks point at real Task Items; Event lines point at real Event Items) plus
free writing. Writing a to-do line in it creates a Task due that day.

### 6.4 Rules

- An Item is in the **Inbox** when `due_at IS NULL AND folder_id IS NULL AND
  `completed_at IS NULL AND deleted_at IS NULL`. Computed by query, not stored.
- Completing a Task clears In Progress state; v0.1 has no Waiting state.
- Deleting a Task soft-deletes its subtasks.
- Priority colours the checkbox; tag chips appear on rows.

---

## 7. Behavioural specification

Behaviour for screens without screenshots comes from the 134 guides
(ADR-0004), reduced to what the main slice needs.

### 7.1 Shell & navigation
Sidebar: Today, Folders section, Tags section. Back/forward history. Right-rail
toggle. Clicking a Task row opens its detail pane in the right rail; the Inbox
is the rail's default scope. Window resizes by folding sidebar first, then
rail.

### 7.2 Today page
Day strip (≈10 days), a summary line, Task/Event lines with badge chips
(due time, tag, priority, countdown), check-off with the completion sound
optional, a "done today" progress ring. The page body is the editable Day note
(§6.3). A to-do written here is due that day; an Event line renders from a
lightweight Event Item.

### 7.3 Tasks
Create from the Quick Add field, the Today page, the Inbox rail or the New
button; rename inline; set due date (day, optional time), priority, tags,
folder, duration, subtasks; complete/reopen. Detail pane shows the title,
checkbox, badge row (only set details), a "+" badge for unset details, and the
Task's own Note editor. Subtasks nest via `parent_id` and via to-do Blocks in
the Task's note.

### 7.4 Notes & the block editor (D19)
In-house QML editor. Exactly 5 block types: **text**, **heading**, **to-do**
(creates and links a real Task), **bullet list**, **code** (plain, no syntax
highlighting in v0.1). Inline bold / italic / link. Return splits, Backspace
merges, Tab indents one level, six-dot handle for drag / turn-into / duplicate
/ delete, Markdown shortcuts on typing. No images, tables, views, videos,
files, quotes or dividers in v0.1.

### 7.5 Inbox rail
Lists Inbox Items grouped as Overdue / Inbox / Today, with a quick-add line at
top. Dropping an Item on a Folder or Tag files it; an Item with no date and no
folder lands here.

### 7.6 Quick Add (D20)
Minimal parser, English only, applied to the top-bar field:
- kind: `T:`/`task:`, `N:`/`note:`, `E:`/`event:` prefixes, plus kind words;
  default Task.
- dates: `today`, `tomorrow`, weekday names, `HH:mm` (24-hour).
- `#tag` (create if missing), `!`/`!!`/`!!!` for low/medium/high priority.
- parsed tokens are removed from the title and shown as chips.

No periods (week/month/Someday), no `$folder` creation, no rhythm phrases,
no symbol menus, no live preview, no multi-language, no Magic Lines.

### 7.7 Events
Lightweight only: title, `start_at`, `end_at`, `all_day`. Rendered as a line
on Today. No calendar grid, no reminders, no meeting links, no RSVP, no
synced-calendar behaviour.

### 7.8 Data lifecycle
All writes go straight to SQLite; there is no undo stack requirement in v0.1.
No Trash UI (§2.3.3). CSV export is one-way (§8.4).

---

## 8. Data layer (D21)

### 8.1 Engine & location
SQLite via `QSqlDatabase`, a single file in the platform app-data directory
(e.g. `$XDG_DATA_HOME/open-woday/open-woday.db` on Linux). No network access.
Schema is created on first launch; migrations are simple versioned SQL.

### 8.2 Schema

```sql
CREATE TABLE items (
  id             INTEGER PRIMARY KEY,
  kind           TEXT NOT NULL CHECK (kind IN ('task','note','event')),
  title          TEXT NOT NULL DEFAULT '',
  created_at     TEXT NOT NULL,
  modified_at    TEXT NOT NULL,
  completed_at   TEXT,
  deleted_at     TEXT,
  due_at         TEXT,                       -- ISO 8601
  due_precision  TEXT CHECK (due_precision IN ('day','time','week','month','someday')),
  start_at       TEXT,
  end_at         TEXT,
  all_day        INTEGER NOT NULL DEFAULT 0,
  priority       INTEGER NOT NULL DEFAULT 0, -- 0 none, 1 low, 2 medium, 3 high
  duration_min   INTEGER,
  parent_id      INTEGER REFERENCES items(id),
  folder_id      INTEGER REFERENCES folders(id),
  day            TEXT,                       -- 'YYYY-MM-DD', day note only
  sort_order     INTEGER NOT NULL DEFAULT 0
);
CREATE UNIQUE INDEX one_day_note_per_day ON items(day) WHERE day IS NOT NULL;

CREATE TABLE blocks (
  id           INTEGER PRIMARY KEY,
  item_id      INTEGER NOT NULL REFERENCES items(id) ON DELETE CASCADE,
  position     INTEGER NOT NULL,
  type         TEXT NOT NULL CHECK (type IN ('text','heading','todo','bullet','code')),
  content      TEXT NOT NULL DEFAULT '',
  inline_json  TEXT,                        -- bold/italic/link ranges
  task_id      INTEGER REFERENCES items(id), -- todo blocks create a real Task
  indent       INTEGER NOT NULL DEFAULT 0
);
CREATE INDEX blocks_by_item ON blocks(item_id, position);

CREATE TABLE folders (
  id     INTEGER PRIMARY KEY,
  name   TEXT NOT NULL,
  icon   TEXT,
  cover  TEXT,
  sort   INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE tags (
  id     INTEGER PRIMARY KEY,
  name   TEXT NOT NULL UNIQUE,
  colour TEXT,
  icon   TEXT
);

CREATE TABLE item_tags (
  item_id INTEGER NOT NULL REFERENCES items(id) ON DELETE CASCADE,
  tag_id  INTEGER NOT NULL REFERENCES tags(id) ON DELETE CASCADE,
  PRIMARY KEY (item_id, tag_id)
);

CREATE TABLE links (
  from_item_id INTEGER NOT NULL REFERENCES items(id) ON DELETE CASCADE,
  to_item_id   INTEGER NOT NULL REFERENCES items(id) ON DELETE CASCADE,
  kind         TEXT NOT NULL CHECK (kind IN ('mention','linked_note','linked_event')),
  PRIMARY KEY (from_item_id, to_item_id, kind)
);

CREATE TABLE meta (key TEXT PRIMARY KEY, value TEXT NOT NULL); -- schema version
```

### 8.3 Block serialization
Blocks live in their own table rather than a JSON blob on the Item. Reason: a
to-do Block needs a foreign key to a real Task, and per-block editing/reorder
is far simpler with rows. `inline_json` holds formatting ranges
(`[{start,end,type,href}]`). `content` is plain UTF-8 text.

### 8.4 CSV export
One file, UTF-8, one row per Item, ordered Notes → Tasks → Events, excluding
soft-deleted rows. Column shape matches the upstream export for compatibility:
`Type, Title, Status, Start/Due, End, All Day, Duration (min), Priority,
Repeat, Location, Tags, Folder, Parent, Linked Note, Details, Created,
Modified, Completed`. Columns with no v0.1 equivalent (`Repeat`, `Location`,
`Linked Note`) are emitted empty. No import mapper in v0.1.

### 8.5 No sync
There is no sync, no remote, no conflict resolution. Data lives on one device.
This is a constraint, not a deferred feature (D3).

---

## 9. Architecture & source layout

### 9.1 Layers
- **QML views** (`src/qml/`): declarative UI, no business logic.
- **C++ models/controllers** (`src/app/`): `ItemModel`, `TodayModel`,
  `InboxModel`, `QuickAddParser`, `CsvExporter`, exposed to QML via
  `QAbstractListModel` / `Q_PROPERTY` / `Q_INVOKABLE`.
- **Store** (`src/store/`): `Database` (schema, migrations, queries),
  `ItemRepository`, `BlockRepository`.
- No network layer, no plugin system, no scripting engine.

### 9.2 Proposed file tree

```
open-woday.pro
src/
  main.cpp
  app/
    AppController.{h,cpp}
    ItemModel.{h,cpp}
    TodayModel.{h,cpp}
    InboxModel.{h,cpp}
    QuickAddParser.{h,cpp}
    CsvExporter.{h,cpp}
  store/
    Database.{h,cpp}
    schema.sql
    ItemRepository.{h,cpp}
    BlockRepository.{h,cpp}
  qml/
    MainWindow.qml
    Sidebar.qml
    TopBar.qml
    RightRail.qml
    TodayPage.qml
    DayStrip.qml
    TaskRow.qml
    TaskDetailPane.qml
    NoteEditor.qml
    blocks/
      TextBlock.qml  HeadingBlock.qml  TodoBlock.qml
      BulletBlock.qml  CodeBlock.qml
    components/
      Badge.qml  TagChip.qml  Checkbox.qml  IconButton.qml
resources/
  icons/            (original assets only)
  fonts/            (Inter, if bundled with a compatible license)
packaging/
  AppImage/  deb/
README.md
CONTRIBUTING.md
```

### 9.3 Build
`qmake6 open-woday.pro && make`. Qt 6 Quick, Widgets (for native dialogs),
SQL (SQLite driver) modules. `linuxdeployqt` bundles Qt libs for the AppImage;
`dpkg-deb` builds the `.deb`. No CMake.

---

## 10. Distribution (D22)

`v0.1` ships **Linux only**: an AppImage (single binary, distro-agnostic) and
a `.deb`. Windows and macOS are build targets for later, produced from the
same codebase only after they are tested. Releases are tagged on GitHub
(D16). The README documents the build and the AppImage run command.

---

## 11. Documentation, governance & legal hygiene

- **README.md:** one-line description; the unaffiliated / non-commercial
  disclaimer (D10); the permanent non-goals list (ADR-0003); install and build
  instructions; a link to `CONTRIBUTING.md`; a note that
  `dev-docs/app-document.md` and `DESIGN.md` are internal references and are
  not project documentation.
- **CONTRIBUTING.md:** issues/PRs welcome, Apache 2.0, pointer to
  `GLOSSARY.md` for vocabulary, pointer to `docs/adr/` for decisions, and a
  request not to re-litigate settled ADRs.
- **Never publish or bundle:** the verbatim guide scrape, Woday logos, the
  mascot, upstream UI strings, or the screenshots (ADR-0001, ADR-0004).
- **ADRs:** `0001` branding (amended), `0002` Qt/no-webview, `0003`
  non-goals, `0004` spec authority. All accepted.

---

## 12. Definition of done for `v0.1`

A tagged Linux release that:

1. Opens a three-panel window whose inner UI matches the screenshots' visual
   language (dark sidebar, white rounded panels, day strip, summary line,
   badge chips, right rail), using only original strings and assets.
2. Uses native OS window chrome and `Ctrl` labels on Linux.
3. Shows Today as an editable Day note with Task and Event lines, a day strip
   and check-off.
4. Can create, edit, tag, schedule, nest and complete Tasks; each Task has its
   own Note pane.
5. Provides the 5-block QML editor with inline formatting and to-do Blocks
   that create real Tasks.
6. Shows an Inbox rail driven by the "no date and no folder" rule.
7. Parses the minimal Quick Add syntax (§7.6) and shows parsed chips.
8. Persists everything in SQLite across restarts, with no network access.
9. Exports a Woday-shaped CSV.
10. Ships as AppImage + `.deb`, with README, CONTRIBUTING and the disclaimer.

---

## 13. Build order

No time estimates (D7, D9). Order is by dependency:

1. `qmake6` project + empty `MainWindow.qml` running with Qt 6 Quick.
2. `Database` + schema + repositories (§8.2).
3. Three-panel shell: Sidebar, TopBar, RightRail, panel tokens (§5).
4. Today page: DayStrip, summary, Item rows, Day note (§6.3, §7.2).
5. Task CRUD + detail pane + badges + subtasks.
6. Block editor (5 types) + inline formatting.
7. Task's own Note pane + to-do Block → Task link.
8. Inbox rail + Folder/Tag sections + drag-to-file.
9. Quick Add parser + chips.
10. CSV export.
11. Visual tuning pass against the three screenshots; per-OS font/chrome
    checks.
12. Packaging (AppImage, `.deb`), README, CONTRIBUTING, tagged release.

---

## 14. Risks & known trade-offs

| Risk | Mitigation | Source |
|---|---|---|
| Pixel fidelity is costlier without a webview | Hand-tuned QML tokens + a dedicated visual pass; accept the cost | C13, ADR-0002 |
| Font rendering / native chrome differ per OS | Platform-correct modifiers, per-OS tuning; inner UI identical | D15, C10 |
| Copyright / trademark exposure | Original strings/assets; docs reference-only; disclaimer | ADR-0001, ADR-0004 |
| The 134 guides contradict themselves | Written spec-authority rule; contradictions logged | A.9, ADR-0004 |
| No cloud means single-device data | One-way CSV export as an exit path | D3, D21 |
| "Completely similar" vs. a bounded slice | Only populated pages built; deviations recorded | C1, C2, §3.4 |

---

## 15. Roadmap (deferred, not committed)

From D18 / ADR-0003, in no committed order: Calendar page, Upcoming, Habits
and streaks, saved Views, Settings, Analytics, AI assistant, local MCP server,
third-party integrations, full CSV import, Trash UI, search/command palette,
standalone list pages, the remaining 12 block types, symbol menus, Magic Lines
calculator, multi-language Quick Add, and Windows/macOS binaries.

Each of these, when picked up, gets its own grilling round and, if hard to
reverse, its own ADR.
