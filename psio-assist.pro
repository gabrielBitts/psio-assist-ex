QT += core gui sql network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = psio-assist-ex
TEMPLATE = app

SOURCES += src/main.cpp \
           src/mainwindow.cpp \
           src/database.cpp \
           src/game.cpp \
           src/binfile.cpp \
           src/config.cpp \
           src/preferencesdialog.cpp \
           src/cuesheet.cpp

HEADERS += src/mainwindow.h \
           src/database.h \
           src/game.h \
           src/binfile.h \
           src/config.h \
           src/cu2generator.h \
           src/preferencesdialog.h \
           src/uiconfig.h \
           src/utils.h \
           src/cuesheet.h