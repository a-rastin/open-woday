-- open-woday local schema
-- Migration 1 (v0.1): the complete initial schema.
--
-- This is the single versioned SQL source for all v0.1 DDL. Database applies
-- the migrations in ascending version order and writes the applied version to
-- the meta table, so no DDL is scattered through C++. Identifiers use the
-- GLOSSARY.md vocabulary: Item, Task, Note, Event, Block, Tag, Folder, Day note.

CREATE TABLE items (
  id             INTEGER PRIMARY KEY,
  kind           TEXT NOT NULL CHECK (kind IN ('task','note','event')),
  title          TEXT NOT NULL DEFAULT '',
  created_at     TEXT NOT NULL,
  modified_at    TEXT NOT NULL,
  completed_at   TEXT,
  deleted_at     TEXT,
  due_at         TEXT,
  due_precision  TEXT CHECK (due_precision IN ('day','time','week','month','someday')),
  start_at       TEXT,
  end_at         TEXT,
  all_day        INTEGER NOT NULL DEFAULT 0,
  priority       INTEGER NOT NULL DEFAULT 0,
  duration_min   INTEGER,
  parent_id      INTEGER REFERENCES items(id),
  folder_id      INTEGER REFERENCES folders(id),
  day            TEXT,
  sort_order     INTEGER NOT NULL DEFAULT 0
);
-- Only active Day notes take part in the one-note-per-date rule. A soft-deleted
-- Day note keeps its day but no longer blocks reusing that date, because normal
-- reads and this index both ignore rows whose deleted_at is set.
CREATE UNIQUE INDEX one_day_note_per_day ON items(day) WHERE day IS NOT NULL AND deleted_at IS NULL;

CREATE TABLE blocks (
  id           INTEGER PRIMARY KEY,
  item_id      INTEGER NOT NULL REFERENCES items(id) ON DELETE CASCADE,
  position     INTEGER NOT NULL,
  type         TEXT NOT NULL CHECK (type IN ('text','heading','todo','bullet','code')),
  content      TEXT NOT NULL DEFAULT '',
  inline_json  TEXT,
  task_id      INTEGER REFERENCES items(id),
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

CREATE TABLE meta (key TEXT PRIMARY KEY, value TEXT NOT NULL);
