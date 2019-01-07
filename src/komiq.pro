#-------------------------------------------------
#
# Project created by QtCreator 2019-01-04T01:28:32
#
#-------------------------------------------------

QT += core gui widgets

TARGET = komiq
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000

CONFIG += c++14

SOURCES += \
    main.cpp \
    zip/zip.c \
    centralwidget.cpp \
    entryiterator.cpp \
    image.cpp

HEADERS += \
    zip/miniz.h \
    zip/zip.h \
    centralwidget.h \
    entryiterator.h \
    image.h

FORMS +=

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
