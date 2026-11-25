# open-woday — Tech Stack

> **Constitution, file 2 of 3.** Companion to `mission.md` (why/what) and
> `roadmap.md` (order of work). This is the "how" layer: the single,
> self-contained authority for technology, architecture, data, visuals, build,
> and engineering rules.
>
> **Consolidation note (2026-11-24):** the technical content of the former
> `docs/PLAN.md` (§§4–10, 14) and the full `ADR-0002` (Qt Quick / no webview)
> were merged into this file and the originals removed. Nothing was dropped.
>
> **Language:** English. **Status:** agreed; no implementation has started.

---

## 1. What this document is

`/bnvt-spec` reads this file together with `mission.md` for guidance. It fixes the
stack and the architectural boundaries so specs do not re-decide them. Settled
decisions are in `mission.md` §11; the ordered work is in `roadmap.md`.

---

## 2. Platform & technology

| Choice | Value | Source |
|---|---|---|
| Language / UI | C++ + Qt Quick / QML | D13, ADR-0002 |
| Qt version | 6.12 (installed locally) | A.11 |
| Build system | `qmake6` (`.pro` file); `cmake` is **not** installed | A.11 |
| OS targets | Linux (primary, v0.1) → Windows → macOS | D6, D11 |
| Codebase | one; Windows/macOS are later build targets, not ports | D11 |
| Webview | **none** — no Electron, Tauri, HTML/CSS rendering | D11, ADR-0002 |
| Storage | SQLite via Qt SQL (`QSqlDatabase`) | D21 |
| Network | none at runtime | D3 |

Qt Quick uses its own GPU scene graph, which is not a web engine, so it satisfies
"no webview". The block editor, rich text and all panels are built in QML/C++ —
no web-based editor library (ADR-0002 consequences).

---

## 3. UI toolkit: Qt Quick / QML, no webview (was ADR-0002, accepted)

**Decision.** The reference app's UI is a three-panel desktop window with rounded
white panels, badge chips and a day strip that would be cheapest to reproduce in a
browser engine. We nevertheless chose Qt Quick / QML with C++ (built with
`qmake6`), with **no webview of any kind**, from one codebase targeting Linux
first, then Windows and macOS. The user explicitly rejected a webview even though
a single web-based shell would have made both pixel fidelity and three-platform
support easier. Qt 6.12, `qmake6` and `g++` are already installed on this machine,
and QML is declarative enough to hand-build the reference visual language.

**Considered options (rejected — do not re-litigate).**
- **Electron / Tauri** (HTML + CSS + JS in a webview). Rejected: the user ruled
  out webviews. This was the obvious path for design fidelity, so the rejection
  is worth remembering — do not "fix" it later by introducing a webview.
- **Flutter, Rust + Slint/egui/iced, Avalonia, GTK4.** Rejected: Flutter/Dart and
  CMake are not installed here; GTK4 has weak Windows/macOS support relative to
  the one-codebase decision; Qt is present and buildable today.

**Consequences.**
- Pixel fidelity costs more than CSS and needs per-OS font and window-chrome
  tuning (see `mission.md` §7.2 / D15).
- The block editor, rich text and any embedded views must be built in QML/C++,
  not with a web-based editor library.
- Local storage uses Qt's own facilities rather than IndexedDB/localStorage.

---

## 4. Local toolchain inventory (checked 2026-11-23, A.11)

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

Implication (factual): a Qt Quick / QML + C++ project can be configured and built
here immediately with `qmake6`; Flutter and CMake-based paths would require
installing toolchains first.

---

## 5. Architecture & source layout

### 5.1 Layers

- **QML views** (`src/qml/`): declarative UI, no business logic.
- **C++ models/controllers** (`src/app/`): `ItemModel`, `TodayModel`,
  `InboxModel`, `QuickAddParser`, `CsvExporter`, exposed to QML via
  `QAbstractListModel` / `Q_PROPERTY` / `Q_INVOKABLE`.
- **Store** (`src/store/`): `Database` (schema, migrations, queries),
  `ItemRepository`, `BlockRepository`.
- **No network layer, no plugin system, no scripting engine.**

### 5.2 Proposed file tree

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

### 5.3 Build

`qmake6 open-woday.pro && make`. Qt 6 Quick, Widgets (for native dialogs) and SQL
(SQLite driver) modules. `linuxdeployqt` bundles Qt libs for the AppImage;
`dpkg-deb` builds the `.deb`. **No CMake.**

---

## 6. Domain model (D17, `GLOSSARY.md`)

### 6.1 Canonical terms

**Item** — the single thing a user creates; everything is one Item of a
particular kind. **Task**, **Note**, **Event** — the three kinds. **Block** — one
line of a Note; a to-do Block is also a real Task. **Tag** — a label attachable to
any Item, crossing Folders. **Folder** — the single optional place an Item is
filed. **Inbox** — Items with no due date and no Folder. **Day note** — the
editable Note belonging to a single day. Full definitions in `GLOSSARY.md`.

### 6.2 Entities

One `Item` table with a `kind` discriminator (`task`, `note`, `event`), rather
than separate Task/Note tables. This is what makes @-mentions, type conversions
and "every Task/Event owns a Note" cheap later.

- **Item:** identity, kind, title, timestamps, completion, soft-delete, optional
  scheduling (`due_at`, `due_precision`, `start_at`, `end_at`, `all_day`),
  `priority` (0–3), `duration_min`, `parent_id` (subtasks), `folder_id`, `day`
  (for a Day note), `sort_order`.
- **Block:** belongs to an Item (a Note body), ordered, one of 5 types, with
  inline formatting ranges and an optional `task_id` for to-do Blocks.
- **Tag / Folder:** as above; `item_tags` is many-to-many.
- **Link:** @-mentions and linked notes/events between Items.

**Not entities in v0.1:** View, Habit, a separate DayNote type (§10.2).

### 6.3 The day note

`items` row with `kind='note'` and `day='YYYY-MM-DD'`, unique per date among
active Day notes. The `one_day_note_per_day` partial unique index covers only
rows with `deleted_at IS NULL`, matching normal reads: a soft-deleted Day note
keeps its date but no longer reserves it, so a replacement for that date can be
created. The Today page opens it; its Blocks include the day's Task and Event
lines (to-do Blocks point at real Task Items; Event lines point at real Event
Items) plus free writing. Writing a to-do line in it creates a Task due that day.

### 6.4 Rules

- An Item is in the **Inbox** when `due_at IS NULL AND folder_id IS NULL AND
  `completed_at IS NULL AND deleted_at IS NULL`. Computed by query, not stored.
- Completing a Task clears In Progress state; v0.1 has no Waiting state.
- Deleting a Task soft-deletes its subtasks.
- Priority colours the checkbox; tag chips appear on rows.

---

## 7. Data layer (D21)

### 7.1 Engine & location

SQLite via `QSqlDatabase`, a single file in the platform app-data directory
(e.g. `$XDG_DATA_HOME/open-woday/open-woday.db` on Linux). No network access.
Schema is created on first launch; migrations are simple versioned SQL.

### 7.2 Schema

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
CREATE UNIQUE INDEX one_day_note_per_day ON items(day) WHERE day IS NOT NULL AND deleted_at IS NULL;

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

### 7.3 Block serialization

Blocks live in their own table rather than a JSON blob on the Item. Reason: a
to-do Block needs a foreign key to a real Task, and per-block editing/reorder is
far simpler with rows. `inline_json` holds formatting ranges
(`[{start,end,type,href}]`). `content` is plain UTF-8 text.

### 7.4 CSV export

One file, UTF-8, one row per Item, ordered Notes → Tasks → Events, excluding
soft-deleted rows. Column shape matches the upstream export for compatibility:
`Type, Title, Status, Start/Due, End, All Day, Duration (min), Priority,
Repeat, Location, Tags, Folder, Parent, Linked Note, Details, Created, Modified,
Completed`. Columns with no v0.1 equivalent (`Repeat`, `Location`,
`Linked Note`) are emitted empty. **No import mapper in v0.1.**

### 7.5 No sync

There is no sync, no remote, no conflict resolution. Data lives on one device.
This is a constraint, not a deferred feature (D3).

---

## 8. Visual specification

### 8.1 Source of truth (ADR-0004, `mission.md` §6)

- **Visuals:** the three in-app screenshots in `screenshots/` (Today, Upcoming,
  Today + AI Assistant). Dark sidebar, white rounded content panels, a top bar
  and a rounded right rail.
- **Behaviour:** `dev-docs/app-document.md` (the 134 guides), for anything not
  visible in a screenshot — notably the Notes editor and the task detail pane.
- **Never:** copy guide text, upstream UI strings, logos, the mascot, or use
  `DESIGN.md` as the UI spec. Guides are internal reference only.

### 8.2 Window structure

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

Rounded white panels sit on a dark canvas; the sidebar is dark; the right rail is
a white rounded card. Resize behaviour: the sidebar folds first, then the right
rail.

### 8.3 Tokens (hand-tuned from the screenshots, NOT from `DESIGN.md`)

Starting values to be verified during the visual-tuning pass; they are **not** the
marketing site's 80 px display type or 60 px radii.

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

Tag chips use pastel backgrounds (e.g. green for Health, blue for Work, purple for
Ideas) as seen in the screenshots; exact hues are tuned in the same pass.

### 8.4 Strings & branding

- Product name `open-woday` appears in the window title and About panel only.
- No upstream string is reused. All placeholders, button labels, menu items,
  summary text and empty states are written fresh.
- `logo/` and `screenshots/` are reference material, never bundled as assets.
  Original icons are drawn or sourced with a compatible license.

### 8.5 Window chrome & shortcuts (D15)

- Use each OS's native window decorations. Do not draw traffic-light controls on
  Linux or Windows. Record this as a deliberate deviation.
- Labels: `Ctrl` on Linux/Windows, `⌘` on macOS. Bind `Ctrl+K`/`⌘K` to focus the
  Quick Add field and `Ctrl+N`/`⌘N` to create a Task.
- The inner app UI is identical on all three platforms.

---

## 9. Behavioural specification

Behaviour for screens without screenshots comes from the 134 guides (ADR-0004),
reduced to what the main slice needs.

### 9.1 Shell & navigation
Sidebar: Today, Folders section, Tags section. Back/forward history. Right-rail
toggle. Clicking a Task row opens its detail pane in the right rail; the Inbox is
the rail's default scope. Window resizes by folding sidebar first, then rail.

### 9.2 Today page
Day strip (≈10 days), a summary line, Task/Event lines with badge chips (due
time, tag, priority, countdown), check-off with the completion sound optional, a
"done today" progress ring. The page body is the editable Day note (§6.3). A
to-do written here is due that day; an Event line renders from a lightweight Event
Item.

### 9.3 Tasks
Create from the Quick Add field, the Today page, the Inbox rail or the New
button; rename inline; set due date (day, optional time), priority, tags, folder,
duration, subtasks; complete/reopen. Detail pane shows the title, checkbox, badge
row (only set details), a "+" badge for unset details, and the Task's own Note
editor. Subtasks nest via `parent_id` and via to-do Blocks in the Task's note.

### 9.4 Notes & the block editor (D19)
In-house QML editor. Exactly 5 block types: **text**, **heading**, **to-do**
(creates and links a real Task), **bullet list**, **code** (plain, no syntax
highlighting in v0.1). Inline bold / italic / link. Return splits, Backspace
merges, Tab indents one level, six-dot handle for drag / turn-into / duplicate /
delete, Markdown shortcuts on typing. No images, tables, views, videos, files,
quotes or dividers in v0.1.

### 9.5 Inbox rail
Lists Inbox Items grouped as Overdue / Inbox / Today, with a quick-add line at
top. Dropping an Item on a Folder or Tag files it; an Item with no date and no
folder lands here.

### 9.6 Quick Add (D20)
Minimal parser, English only, applied to the top-bar field:
- kind: `T:`/`task:`, `N:`/`note:`, `E:`/`event:` prefixes, plus kind words;
  default Task.
- dates: `today`, `tomorrow`, weekday names, `HH:mm` (24-hour).
- `#tag` (create if missing), `!`/`!!`/`!!!` for low/medium/high priority.
- parsed tokens are removed from the title and shown as chips.

No periods (week/month/Someday), no `$folder` creation, no rhythm phrases, no
symbol menus, no live preview, no multi-language, no Magic Lines.

### 9.7 Events
Lightweight only: title, `start_at`, `end_at`, `all_day`. Rendered as a line on
Today. No calendar grid, no reminders, no meeting links, no RSVP, no
synced-calendar behaviour.

### 9.8 Data lifecycle
All writes go straight to SQLite; there is no undo stack requirement in v0.1. No
Trash UI (§10.3). CSV export is one-way (§7.4).

---

## 10. Settled interpretations (implementation level)

These were deliberately left below the design tree's granularity and settled in
the former `PLAN.md` §2.3. They are recorded here so nothing is silently assumed.

1. **Sidebar rows.** v0.1 renders only real, populated destinations: `Today`, a
   `Folders` section and a `Tags` section, plus an Inbox entry in the right rail.
   Deferred pages (Calendar, Upcoming, Habits, Views, Events list) are **absent**,
   not shown as disabled placeholders — consistent with D5/D12 ("no empty
   shells"). Full-width page lists are roadmap items.
2. **Day note is not a new entity.** D17 deferred "day-note-as-entity"; D19
   requires an editable day note. It is therefore a `Note` Item with a `day`
   column (`items.day`), one per date — no separate table, no new kind.
3. **Deletion.** Soft-delete via `items.deleted_at`. No Trash UI in v0.1;
   permanent deletion is not exposed.
4. **Search.** The top-bar field is the **Quick Add** entry point (D12). A full
   search / command palette is not in v0.1; it is a roadmap item. `Ctrl+K`
   (`⌘K` on macOS) focuses that field.
5. **Standalone list pages.** No separate Tasks / Notes / Events pages in v0.1.
   Items are reached through Today, the Inbox rail, the Folders/Tags sections and
   the detail pane.
6. **Visual tokens** (§8.3) are hand-tuned from the screenshots, **not** copied
   from `DESIGN.md`, which is the marketing site's design system (D4/D14).

---

## 11. Build & distribution (D22)

- **Build:** `qmake6 open-woday.pro && make`. Qt 6 Quick, Widgets, SQL modules.
  No CMake.
- **Distribution:** `v0.1` ships **Linux only** — an AppImage (single binary,
  distro-agnostic, bundled via `linuxdeployqt`) and a `.deb` (via `dpkg-deb`).
  Windows and macOS are build targets for later, produced from the same codebase
  only after they are tested.
- **Releases:** tagged on GitHub (D16). The README documents the build and the
  AppImage run command.

---

## 12. Engineering rules for spec & implementation work

- **Workflow:** spec → tickets (tracer-bullet vertical slices) → TDD implement →
  review → revise → documentation → commit → merge. See `mission.md` §13 and
  `roadmap.md`.
- **Glossary:** use `GLOSSARY.md` vocabulary in code, docs, tickets and specs;
  respect the absorbed ADR decisions (`mission.md` §§5, 6, 8; this file §3).
- **Testing:** test external behaviour, not implementation details; prefer the
  highest seam and existing seams; the fewer seams the better (ideal = one).
  The one executable seam is the framework-free
  `tests/verify-persistence.cpp`. Keep verification framework-free; add no test
  framework, no unit seams and no CI test job (unchanged from the original
  rule).
- **Test execution — local rule:** every run of the verification suite happens
  on the developer's machine. From `tests/`, run
  `qmake6 verify-persistence.pro && make -j"$(nproc)" && ./verify-persistence`.
  Verification passes only when the build and executable both exit zero. No
  remote runtime, Google account or GPU is required. `AGENTS.md` carries the
  agent-facing rule; `README.md` § Testing documents the command.
- **No runtime network, no plugin system, no scripting engine, no webview.**
- **No schedule or effort estimates** (D7, D9); order is by dependency.
- **Legal hygiene:** never bundle or publish the guide scrape, upstream
  logos/mascot, upstream UI strings, or the screenshots. All shipped strings and
  assets are original.
- **Authority:** this file, `mission.md` and `roadmap.md` are the constitution.
  `GLOSSARY.md` (vocabulary) and `dev-docs/grill-session.md` (historical record)
  are separate, untouched authorities.

---

## 13. Technical risks & trade-offs

| Risk | Mitigation | Source |
|---|---|---|
| Pixel fidelity is costlier without a webview | Hand-tuned QML tokens + a dedicated visual pass; accept the cost | C13, ADR-0002 |
| Font rendering / native chrome differ per OS | Platform-correct modifiers, per-OS tuning; inner UI identical | D15, C10 |
| The 134 guides contradict themselves | Written spec-authority rule; contradictions logged | A.9, ADR-0004 |
| No cloud means single-device data | One-way CSV export as an exit path | D3, D21 |
| `QTextDocument`/web editors would fight the block model | Build the in-house QML editor instead | D19, ADR-0002 |

The full cross-cutting risk table is in `mission.md` §12.
