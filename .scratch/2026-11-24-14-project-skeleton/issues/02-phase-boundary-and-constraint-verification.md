# 02: Phase-boundary and constraint verification

**What to build:** A reviewer can confirm the Phase 1 increment is mergeable.
Inspecting the completed change shows that the smallest runnable increment
stayed inside its constitutional and phase boundaries: no webview, no network,
no data-layer code, no three-panel shell, no upstream assets or strings, no
test/CI/packaging scaffolding, canonical domain vocabulary, and nothing from
later phases or the deferred backlog leaked in. The change is demoable on its
own and satisfies the roadmap's "Verifiable when" gate.

**Blocked by:** 01: Runnable empty-window skeleton

**Status:** ready-for-agent

- [ ] Diff inspection finds no webview of any kind (no web engine view, no
      HTML/CSS/JS shell) — ADR-0002.
- [ ] Diff inspection finds no network classes and confirms zero runtime network
      access — D3.
- [ ] Diff inspection finds no schema, migrations, schema version, repositories,
      SQLite file, or Item/Block model code — the Phase 2 boundary.
- [ ] Diff inspection finds no three-panel shell, visual tokens or panel
      components — the Phase 3 boundary.
- [ ] Diff inspection finds no upstream logo, mascot, UI string, guide text or
      screenshot bundled or published; all shipped strings are written fresh and
      the reference asset folders remain reference material only — ADR-0001 /
      ADR-0004.
- [ ] Diff inspection finds no test framework, CI configuration, packaging
      scripts or CMake files, and no Linux-only build hacks (one codebase,
      Linux primary, D6 / D11).
- [ ] Identifiers and comments use canonical `GLOSSARY.md` vocabulary wherever
      domain concepts appear, with none of the `_Avoid_` terms used for domain
      concepts.
- [ ] No out-of-scope item leaked in: none of Phases 2–12, none of the deferred
      backlog, none of the permanent non-goals.
- [ ] The spec contains no schedule or effort estimates, and no settled decision
      or absorbed ADR is re-litigated — D7 / D9.
- [ ] The "Verifiable when" gate is met (the app configures, compiles and opens a
      window on Linux), and the increment is runnable / demoable on its own
      rather than a non-demoable horizontal layer.
