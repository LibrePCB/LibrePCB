TEMPLATE = subdirs

SUBDIRS = \
    clipper \
    delaunay-triangulation \
    fontobene-qt5 \
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
    fontobene-qt5 \
    muparser \
    optional \
    parseagle \
    hoedown \
    sexpresso \

!contains(UNBUNDLE, quazip) {
    SUBDIRS += quazip
    librepcb.depends += quazip
}
