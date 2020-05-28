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
    quazip \
    sexpresso \

librepcb.depends = \
    clipper \
    delaunay-triangulation \
    fontobene-qt5 \
    muparser \
    optional \
    parseagle \
    hoedown \
    quazip \
    sexpresso \
