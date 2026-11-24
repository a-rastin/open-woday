---
status: accepted
---

# Permanent non-goals vs deferred features

open-woday is a Linux-first, Qt Quick/C++ desktop app with no webview, built as
an unaffiliated alternative to a proprietary macOS product. We split the
upstream feature set into **permanent non-goals** and **deferred roadmap items**
rather than calling everything "later". Permanent: iCloud/CloudKit, Apple
Calendar, Reminders, Notes, Weather and Apple Intelligence; the macOS-only
shell (menu bar, widgets, Save for Later, ⌥Space launcher); and the iOS app.
Deferred: the Calendar page, Upcoming, Habits, saved Views, Settings,
Analytics, the AI assistant, the local MCP server, and third-party integrations
(Notion, Obsidian, Todoist, Linear, Google Calendar, ICS feeds).

## Considered Options

- Mark the entire upstream surface as "deferred". Rejected: the Apple-ecosystem
  items are impossible without Apple frameworks on a Linux-first Qt app, so
  labelling them deferred would mislead contributors into waiting for them.
- Keep no explicit non-goals list. Rejected: without it, the reference
  screenshots and the 134-guide documentation keep generating "why isn't X
  here?" work that has already been answered.

## Consequences

- Requests for Apple-only features are answered by pointing here, not by
  re-opening the platform decision.
- The README must carry this list so new contributors see the boundary before
  proposing work.
- Deferred items stay on a roadmap; nothing here forbids a later ADR that
  promotes one of them.
