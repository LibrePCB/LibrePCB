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
    pns_router \
    quazip \
    sexpresso

librepcb.depends = \
    clipper \
    delaunay-triangulation \
    fontobene \
    optional \
    parseagle \
    pns_router \
    hoedown \
    quazip \
    sexpresso \

