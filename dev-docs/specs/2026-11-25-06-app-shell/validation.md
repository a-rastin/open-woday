# Phase 3 — app-shell — Validation

> **Spec:** `dev-docs/specs/2026-11-25-06-app-shell/` · **Branch:** `phase-3-app-shell`
> How to know the Phase 3 implementation succeeded and can be merged. Holds the
> `/to-spec` "Testing Decisions" and the roadmap's "Verifiable when" gate. Uses
> `GLOSSARY.md` vocabulary; no schedule or effort estimates (D7 / D9).

## Verifiable when (the merge gate)

From `roadmap.md` Phase 3: **"the window shows a dark sidebar, white rounded
centre panel and white rounded right rail in the reference visual language, with
no macOS traffic lights on Linux."** A phase ships only if it is a runnable /
verifiable increment (`roadmap.md` §1). Phase 3 does **not** satisfy any full
v0.1 Definition-of-Done item (`mission.md` §10 / `roadmap.md` §3) on its own;
those are completed in Phases 4–12. Its gate is the condition above plus the
checks below. Because the phase renders a frame and no live data, the gate is
proven through the build-and-launch appearance of the window, not through Item
or Task behaviour.

## Testing Decisions

### Seam decision — please confirm

Per the `/to-spec` skill, seams are sketched at the **highest possible level**,
existing seams preferred, and the ideal number is **one**. Phase 1 established
the build-and-launch seam; Phase 2 added the persistence seam. This phase is
greenfield for product UI (`tech-stack.md` §12: no test framework), so there are
no lower unit seams to reuse.

- **The single seam:** the application's **externally observable build-and-launch
  appearance** — `qmake6 && make` produces a binary, and launching that binary
  opens a window showing the dark sidebar, white rounded centre panel and white
  rounded right rail in the original visual language. This is the highest seam
  available and needs no new seam around QML component internals.
- **No new lower seams.** Phase 3 adds no unit-test framework, no test doubles
  and no seams around individual QML properties, token values or paint order.
  Testing whether a specific token constant was declared would be testing
  implementation details; the observable contract is "the window renders the
  three-panel frame with original strings and assets".
- **Reviewer confirmation:** if the maintainer expects a different seam (for
  example a committed screenshot-diff harness, a UI automation job or a CI gate
  in this phase), say so before `/bnvt-ticket`; otherwise this single-seam
  decision stands.
- **Approval status (spec approval process).** This spec was synthesized without
  an interview and is a **proposal**. No maintainer confirmation of the seam
  decision, nor of the next-phase reading in `requirements.md` §"Where this
  phase comes from", has been recorded. Both are left **open before
  `/bnvt-ticket`**, following the project's spec approval process: the spec is
  published as a proposal awaiting confirmation rather than claiming a sign-off
  that was never given. This note is an explicit record of that open question —
  it is a judgement call about how the phase is verified, **not** as maintainer
  sign-off.

### What makes a good test here

- Test **external behaviour**, not implementation details: "the build produces a
  runnable binary and the launched window shows the three panels", not "this QML
  symbol exists" or "this exact token value is set".
- Assert on observable outcomes (a clean configure/compile, one visible native
  window, the dark sidebar / white rounded centre / white rounded right rail,
  absent traffic lights on Linux, platform-correct modifier labels, the
  Sidebar-then-RightRail fold order, original strings and no bundled upstream
  assets), never on internal structure.
- Keep the verification framework-free and at one seam; do not add a test
  framework or low-level seams (`tech-stack.md` §12).

## Modules in scope for validation

- The declarative QML frame (`Sidebar`, `TopBar`, `RightRail`, centre panel) and
  its shared token set.
- The root `Window`, its native chrome and the Phase 1 loud-failure guard.
- `resources.qrc` embedding, and the set of original strings/icons shipped.

## Validation checks

1. `qmake6 open-woday.pro && make` configures and compiles cleanly with no new
   compiler warnings; no CMake and no new build tooling are added.
2. Launching the binary opens exactly one window (the Phase 1 root-QML load guard
   still fails loudly with a non-zero exit if the root component cannot be
   created).
3. The window shows a dark sidebar, a white rounded centre panel and a white
   rounded right rail on a dark canvas, with a top bar spanning the frame.
4. Panels use the shared `radiusPanel` / `radiusChip` / `radiusControl` radii and
   `gap` spacing; typography uses Inter with a system-UI fallback.
5. Native OS decorations are used; no macOS traffic-light controls are drawn on
   Linux (D15).
6. Modifier labels are `Ctrl` on Linux/Windows and `⌘` on macOS; the inner UI is
   identical across platforms.
7. Resizing folds the Sidebar first, then the RightRail; the centre panel stays
   legible.
8. Every frame string, placeholder, empty state and icon is original; no
   upstream logo, wordmark, mascot, screenshot or guide text is bundled or
   shipped; `logo/`, `screenshots/`, `app-document.md` and `DESIGN.md` remain
   reference material only.
9. The frame renders no live Item, Task, Note, Event, Tag, Folder or Inbox data;
   no Day strip, Day note, Task/Event lines, check-off, block editor, Inbox
   contents, Quick Add parsing or CSV export appears.
10. No persistence, schema, migration, repository or `Database` change is
    present; no network, webview, plugin, scripting or web-editor code is added.
11. The C++ entry point stays free of business logic; QML stays declarative.
12. No new test framework, CI configuration, unit seam or packaging script is
    added (`tech-stack.md` §12).
13. `GLOSSARY.md` vocabulary is used; no settled decision or absorbed ADR is
    re-litigated; the spec carries no schedule or effort estimates (D7 / D9).
14. The spec is recorded as a proposal with maintainer confirmation outstanding
    (spec approval process); no maintainer sign-off is fabricated.
15. The phase is one runnable / verifiable increment (`roadmap.md` §1).

## Merge criteria

The merge gate holds when checks 1–15 pass, the window is visually the
three-panel frame in the original visual language, and the spec remains an
approved-by-nobody proposal whose open questions are carried forward for the
maintainer to confirm **before `/bnvt-ticket`**. This phase ships a shell
only: any live page, Task/Event line, Day note, editor, Inbox content or
persistence change belongs to a later phase. A change to the constitution
requires an explicit amendment (`mission.md` §13), not a re-litigation here.
