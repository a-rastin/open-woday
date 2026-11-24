---
status: accepted
amended: 2026-11-23
---

# Keep the `open-woday` name; upstream docs and assets are reference-only

Woday is a paid, proprietary macOS product, and this repository began with a
verbatim scrape of its 134-guide documentation, its logo and wordmark files, and
a design system derived from its marketing website. We decided to keep
`open-woday` as the product and repository name, but to ship with original
visual assets: no upstream logo, wordmark or mascot files, no verbatim UI
strings ("Welcome to Woday", "Ask Wodie"), and no redistribution of the scraped
documentation. `dev-docs/app-document.md` stays an internal behavioural
reference; `dev-docs/DESIGN.md` is a reference to the marketing site, not our
app's UI spec. The README must state plainly that this is an unaffiliated,
non-commercial community project.

## Considered Options

- Adopt a fully original name as well (the original Q4(b) answer). **Rejected in
  Q8/Q10**: `open-woday` is a descriptive, nominative handle for an unaffiliated
  FOSS alternative, which is low-risk and aids discoverability. The legal
  exposure that motivated Q4(b) came from **assets and verbatim copy**, not from
  the slug. Recorded so a future contributor does not re-litigate the rename.
- Keep the upstream logos, mascot and exact UI strings for familiarity.
  Rejected: trademark/copyright exposure, and it undercuts the independence
  that makes the project useful to non-Mac users.

## Consequences

- `dev-docs/app-document.md` is never published as project documentation.
- `open-woday` may appear as the product name in the window title / About, but
  standalone "Woday" / "Wodie" and the mascot never appear in the app.
- `logo/` and the three `screenshots/` are reference material only, never
  shipped assets.
- Visual similarity (Q1 / Q5) must be reached through layout and visual
  language, not copied brand elements or strings.
