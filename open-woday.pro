TEMPLATE = app
TARGET = open-woday

QT += quick widgets sql
CONFIG += c++17

SOURCES += src/main.cpp \
           src/app/ItemModel.cpp \
           src/app/TodayModel.cpp \
           src/store/Database.cpp \
           src/store/FolderRepository.cpp \
           src/store/ItemRepository.cpp \
           src/store/BlockRepository.cpp \
           src/store/TagRepository.cpp

HEADERS += src/app/ItemModel.h \
           src/app/TodayModel.h \
           src/store/Database.h \
           src/store/StoreHelpers.h \
           src/store/FolderRepository.h \
           src/store/ItemRepository.h \
           src/store/BlockRepository.h \
           src/store/TagRepository.h

RESOURCES += resources.qrc

INCLUDEPATH += src
