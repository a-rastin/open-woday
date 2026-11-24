---
status: accepted
---

# Spec authority: screenshots for visuals, upstream guides for behaviour

`dev-docs/app-document.md` is a verbatim scrape of the proprietary Woday
documentation, and `dev-docs/DESIGN.md` describes Woday's marketing website,
not its app. We decided that the three in-app screenshots are the **visual**
source of truth and the 134 guides are the **behavioural** source of truth for
any screen without a screenshot; anything contradictory or unspecified is
decided in the grilling record and logged here only when hard to reverse. The
guides are never copied verbatim into our product or our documentation.

## Considered Options

- Treat the 134 guides as a literal specification. Rejected: they contain three
  internal contradictions (Mac-only vs. a shipping iOS section; consent-based
  vs. silent auto-update; macOS 26 alongside GPT-6.1-class model names) and are
  copyrighted verbatim text.
- Screenshots only. Rejected: they cover three of 12+ pages, so the Notes
  editor and task detail pane would have no behavioural basis at all.
- Treat `DESIGN.md` as the app UI spec. Rejected: it is the marketing site's
  design system (black canvas, 80 px display type, 60 px radii), a different
  surface from the app UI (dark sidebar, white rounded panels, compact type).

## Consequences

- `dev-docs/app-document.md` stays internal reference material and is never
  published as project documentation; `logo/` and `screenshots/` are reference
  only, never shipped assets.
- Screens with no screenshot are designed by extrapolating the screenshots'
  visual language, never by copying upstream strings or assets.
- Future contradictions are resolved in `dev-docs/grill-session.md` and
  promoted to an ADR only if the decision is hard to reverse.
