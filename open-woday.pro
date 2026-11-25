TEMPLATE = app
TARGET = open-woday

QT += quick widgets sql
CONFIG += c++17

SOURCES += src/main.cpp \
           src/app/TodayModel.cpp \
           src/store/Database.cpp \
           src/store/ItemRepository.cpp \
           src/store/BlockRepository.cpp

HEADERS += src/app/TodayModel.h \
           src/store/Database.h \
           src/store/StoreHelpers.h \
           src/store/ItemRepository.h \
           src/store/BlockRepository.h

RESOURCES += resources.qrc

INCLUDEPATH += src
