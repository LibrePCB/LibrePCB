TEMPLATE = subdirs

SUBDIRS = \
    clipper \
    delaunay-triangulation \
    fontobene \
    hoedown \
    googletest \
    librepcb \
    optional \
    parseagle \
    quazip \
    sexpresso

librepcb.depends = \
    clipper \
    delaunay-triangulation \
    fontobene \
    optional \
    parseagle \
    hoedown \
    quazip \
    sexpresso \

