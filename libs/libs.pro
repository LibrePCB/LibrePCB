TEMPLATE = subdirs

SUBDIRS = \
    clipper \
    hoedown \
    googletest \
    librepcb \
    parseagle \
    quazip \
    sexpresso

librepcb.depends = clipper parseagle hoedown quazip sexpresso
