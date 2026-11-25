# The high-level build-and-launch appearance seam for the Phase 3 app shell.
# Framework-free by design: no test framework, no CI configuration, no CMake.
# Build and run with: qmake6 verify-app-shell.pro && make && ./verify-app-shell
TEMPLATE = app
TARGET = verify-app-shell
QT = core quick
CONFIG += c++17 console
CONFIG -= app_bundle

SOURCES += verify-app-shell.cpp

RESOURCES += ../resources.qrc
