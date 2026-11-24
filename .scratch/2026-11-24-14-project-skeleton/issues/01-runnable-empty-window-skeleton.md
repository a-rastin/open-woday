# 01: Runnable empty-window skeleton

**What to build:** A contributor who clones the repo can run the single documented
build command and get an `open-woday` binary. Launching that binary opens exactly
one empty window on Linux, using the OS's native decorations and titled
`open-woday` only; closing the window quits cleanly and launching again behaves
the same way. If the root QML component cannot be created, startup fails loudly
with a clear message and a non-zero exit code instead of a silent blank app.

This establishes the three seams every later phase extends — the build, the
C++/QML layer split and the launch seam — while deliberately shipping no product
surface. The build declares Qt 6 Quick, Widgets and SQL up front and bundles the
root QML component through the Qt resource system so the binary is
self-contained. The C++ entry point holds no business logic, and QML stays
declarative. Nothing is persisted, nothing is networked, and no downstream
phase's work is pulled forward.

**Blocked by:** None (can start immediately)

**Status:** ready-for-agent

- [ ] `qmake6` configuration exits `0` with no errors and produces a Makefile;
      the already-documented build command works as written; no CMake build files
      are introduced.
- [ ] `make` exits `0` with no errors and no new compiler warnings and produces
      the `open-woday` binary.
- [ ] The build definition targets `open-woday`, declares the Qt 6 Quick,
      Widgets and SQL modules up front, uses a C++ standard suitable for Qt 6.12,
      and bundles the root QML component through the Qt resource system so the
      built binary is self-contained.
- [ ] Launching the binary opens exactly one visible window on the Linux desktop
      using native OS decorations (no drawn macOS traffic lights), titled
      `open-woday` and nothing else.
- [ ] The window is empty by design: no placeholder pages, disabled destinations,
      badge chips, day strip, panels or fake content.
- [ ] Closing the window exits the process with code `0`; relaunching opens the
      window again.
- [ ] If the root QML component cannot be created, startup fails loudly with a
      clear error message and a non-zero exit code rather than a silent blank
      app.
- [ ] The README's already-documented `qmake6 && make` command is now accurate,
      and the README is not edited in this phase.
