# 02: Create and read an Item across an app restart

**What to build:** A contributor can persist a single Item and trust it across a
restart. Through the store's repository API, writing one Task Item and then
closing and reopening the database (or relaunching the app) returns the very same
Item unchanged. This is the first, thinnest complete path that stands the
persistence seam up — it proves the app is capable of remembering one thing —
while deliberately shipping no product UI and adding no test framework or
lower-level seams. It is the tracer bullet that the wider Item and Block slices
extend.

**Blocked by:** 01: Stand up the durable local database

**Status:** ready-for-agent

- [ ] The Item repository can create one Item (Task kind) and read it back by id
      with its fields unchanged.
- [ ] An Item written before a restart reads back with the same values after
      closing and reopening the connection against the same file — the single
      highest seam (create → restart → read back).
- [ ] The restart round trip exercises the database, schema/migrations and
      repository together as one observable behaviour; no new lower seams, test
      doubles, or assertions about internal SQL strings or private helpers are
      introduced.
- [ ] All writes use bound values; no SQL is assembled by concatenating user text,
      and foreign-key enforcement stays on.
- [ ] No state lives only in memory: the Item's survival comes from writing
      straight to SQLite, with no in-memory-only cache.
- [ ] The existing empty window still opens, and the build stays green
      (`qmake6 && make`, no new warnings, no CMake).
- [ ] Canonical `GLOSSARY.md` vocabulary is used: a "to-do Block" is a Block type
      and is not used as a synonym for a Task; no "record/object/card" for an
      Item, no "document/page/doc" for a Note, no "daily note" for a Day note.
- [ ] No test framework, CI configuration or low-level unit seam is introduced;
      the persistence behaviour remains the single seam in the codebase.
- [ ] No product UI, no separate Task/Note/DayNote/Habit entity, no webview, no
      network, and none of Phases 3–12 pulled forward.
