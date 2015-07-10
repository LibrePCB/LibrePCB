TEMPLATE = subdirs

SUBDIRS = \
    3rdparty \
    libs \
    librepcb \
    tools \
    tests

librepcb.depends = libs
tools.depends = libs
tests.depends = 3rdparty libs
