TEMPLATE = subdirs

SUBDIRS = \
    clipper \
    delaunay-triangulation \
    fontobene \
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
    fontobene \
    muparser \
    optional \
    parseagle \
    hoedown \
    quazip \
    sexpresso \
