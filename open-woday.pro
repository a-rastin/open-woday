TEMPLATE = app
TARGET = open-woday

QT += quick widgets sql
CONFIG += c++17

SOURCES += src/main.cpp \
           src/store/Database.cpp \
           src/store/ItemRepository.cpp

HEADERS += src/store/Database.h \
           src/store/ItemRepository.h

RESOURCES += resources.qrc
