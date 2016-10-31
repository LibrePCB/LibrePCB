TEMPLATE = subdirs

SUBDIRS = \
    hoedown \
    googletest \
    librepcb \
    quazip

librepcb.depends = hoedown quazip
