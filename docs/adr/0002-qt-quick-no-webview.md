---
status: accepted
---

# Qt Quick / QML + C++ with no webview

The reference app's UI is a three-panel desktop window with rounded white
panels, badge chips and a day strip that would be cheapest to reproduce in a
browser engine. We nevertheless chose Qt Quick / QML with C++ (built with
`qmake6`), with **no webview of any kind**, from one codebase targeting Linux
first, then Windows and macOS. The user explicitly rejected a webview even
though a single web-based shell would have made both pixel fidelity and
three-platform support easier. Qt 6.12, `qmake6` and `g++` are already
installed on this machine, and QML is declarative enough to hand-build the
reference visual language.

## Considered Options

- **Electron / Tauri** (HTML + CSS + JS in a webview). Rejected: the user ruled
  out webviews. This was the obvious path for design fidelity, so the rejection
  is worth remembering — do not "fix" it later by introducing a webview.
- **Flutter, Rust + Slint/egui/iced, Avalonia, GTK4.** Rejected: Flutter/Dart
  and CMake are not installed here; GTK4 has weak Windows/macOS support
  relative to the one-codebase decision; Qt is present and buildable today.

## Consequences

- Pixel fidelity costs more than CSS and needs per-OS font and window-chrome
  tuning (see Q15 / D-d).
- The block editor, rich text and any embedded views must be built in QML/C++,
  not with a web-based editor library.
- Local storage uses Qt's own facilities rather than IndexedDB/localStorage.
