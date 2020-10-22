TEMPLATE = subdirs

# Use common project definitions
include(../common.pri)

SUBDIRS = \
    delaunay-triangulation \
    googletest \
    librepcb \
    muparser \
    optional \
    parseagle \
    sexpresso \

librepcb.depends = \
    delaunay-triangulation \
    muparser \
    optional \
    parseagle \
    sexpresso \

# Hoedown is only needed for Qt <5.14
equals(QT_MAJOR_VERSION, 5):lessThan(QT_MINOR_VERSION, 14) {
    SUBDIRS += hoedown
    librepcb.depends += hoedown
}

!contains(UNBUNDLE, quazip) {
    SUBDIRS += quazip
    librepcb.depends += quazip
}

!contains(UNBUNDLE, polyclipping) {
    SUBDIRS += polyclipping
    librepcb.depends += polyclipping
}

!contains(UNBUNDLE, fontobene-qt5) {
    SUBDIRS += fontobene-qt5
    librepcb.depends += fontobene-qt5
}
