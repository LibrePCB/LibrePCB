TEMPLATE = subdirs

SUBDIRS = \
    clipper \
    fontobene \
    hoedown \
    googletest \
    librepcb \
    parseagle \
    quazip \
    sexpresso

librepcb.depends = clipper fontobene parseagle hoedown quazip sexpresso
