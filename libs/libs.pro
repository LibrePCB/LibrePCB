TEMPLATE = subdirs

SUBDIRS = \
    clipper \
    delaunay-triangulation \
    hoedown \
    googletest \
    librepcb \
    muparser \
    optional \
    parseagle \
    sexpresso \

librepcb.depends = \
    clipper \
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

!contains(UNBUNDLE, quazip) {
    SUBDIRS += fontobene-qt5
    librepcb.depends += fontobene-qt5
}
