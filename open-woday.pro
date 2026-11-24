TEMPLATE = app
TARGET = open-woday

QT += quick widgets sql
CONFIG += c++17

SOURCES += src/main.cpp \
           src/store/Database.cpp

HEADERS += src/store/Database.h

RESOURCES += resources.qrc
