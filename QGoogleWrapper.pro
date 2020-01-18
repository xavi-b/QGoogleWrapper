TEMPLATE        = lib
CONFIG         += c++17
DEFINES        += QT_DEPRECATED_WARNINGS
QT             += network

SUBDIRS += \
    src/

include(src/src.pri)
