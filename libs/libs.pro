TEMPLATE = subdirs

SUBDIRS = \
    clipper \
    delaunay-triangulation \
    fontobene \
    hoedown \
    googletest \
    librepcb \
    parseagle \
    quazip \
    sexpresso

librepcb.depends = \
    clipper \
    delaunay-triangulation \
    fontobene \
    parseagle \
    hoedown \
    quazip \
    sexpresso \

