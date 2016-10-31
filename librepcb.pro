TEMPLATE = subdirs

SUBDIRS = \
    libs \
    librepcb \
    tools \
    tests

librepcb.depends = libs
tools.depends = libs
tests.depends = libs
