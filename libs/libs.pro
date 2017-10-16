TEMPLATE = subdirs

SUBDIRS = \
    hoedown \
    googletest \
    librepcb \
    parseagle \
    quazip

librepcb.depends = parseagle hoedown quazip
