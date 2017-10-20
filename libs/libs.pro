TEMPLATE = subdirs

SUBDIRS = \
    hoedown \
    googletest \
    librepcb \
    parseagle \
    quazip \
    sexpresso

librepcb.depends = parseagle hoedown quazip sexpresso
