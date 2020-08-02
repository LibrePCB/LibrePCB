TEMPLATE = subdirs

# Use common project definitions
include(../common.pri)

SUBDIRS = \
    delaunay-triangulation \
    hoedown \
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
    hoedown \
    sexpresso \

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
