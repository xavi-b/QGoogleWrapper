TEMPLATE        = lib
CONFIG         += c++17
DEFINES        += QT_DEPRECATED_WARNINGS
QT             += core network
TARGET          = QGoogleWrapper
DESTDIR         = $$PWD

unix {
target.path = /usr/lib
INSTALLS += target
}

SUBDIRS += \
    $$PWD/../include \
    $$PWD/../src

include($$PWD/../include/include.pri)
include($$PWD/../src/src.pri)
