TEMPLATE = subdirs

SUBDIRS = \
    polyclipping \
    delaunay-triangulation \
    hoedown \
    googletest \
    librepcb \
    muparser \
    optional \
    parseagle \
    sexpresso \

librepcb.depends = \
    polyclipping \
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

!contains(UNBUNDLE, fontobene-qt5) {
    SUBDIRS += fontobene-qt5
    librepcb.depends += fontobene-qt5
}
