# The single high-level persistence seam (Phases 2-5).
# Framework-free by design: no test framework, no CI configuration, no CMake.
# Run locally from tests/: qmake6 verify-persistence.pro && make && ./verify-persistence
TEMPLATE = app
TARGET = verify-persistence
QT = core gui sql
LIBS += -lsqlite3
CONFIG += c++17 console
CONFIG -= app_bundle

INCLUDEPATH += $$PWD/../src

SOURCES += verify-persistence.cpp \
           ../src/app/ItemModel.cpp \
           ../src/app/TodayModel.cpp \
           ../src/store/Database.cpp \
           ../src/store/FolderRepository.cpp \
           ../src/store/ItemRepository.cpp \
           ../src/store/BlockRepository.cpp \
           ../src/store/TagRepository.cpp

HEADERS += ../src/store/Database.h \
           ../src/app/ItemModel.h \
           ../src/app/TodayModel.h \
           ../src/store/StoreHelpers.h \
           ../src/store/FolderRepository.h \
           ../src/store/ItemRepository.h \
           ../src/store/BlockRepository.h \
           ../src/store/TagRepository.h

RESOURCES += ../resources.qrc
