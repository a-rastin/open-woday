# The single high-level persistence seam for Phase 2 (issues 02, 03, 04 and 05).
# Framework-free by design: no test framework, no CI configuration, no CMake.
# Run locally from tests/: qmake6 verify-persistence.pro && make && ./verify-persistence
TEMPLATE = app
TARGET = verify-persistence
QT = core sql
LIBS += -lsqlite3
CONFIG += c++17 console
CONFIG -= app_bundle

INCLUDEPATH += $$PWD/../src

SOURCES += verify-persistence.cpp \
           ../src/app/TodayModel.cpp \
           ../src/store/Database.cpp \
           ../src/store/ItemRepository.cpp \
           ../src/store/BlockRepository.cpp

HEADERS += ../src/store/Database.h \
           ../src/app/TodayModel.h \
           ../src/store/StoreHelpers.h \
           ../src/store/ItemRepository.h \
           ../src/store/BlockRepository.h

RESOURCES += ../resources.qrc
