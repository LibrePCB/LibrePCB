TEMPLATE = subdirs

# Use common project definitions
include(common.pri)

SUBDIRS = \
    apps \
    libs \
    tests \

apps.depends = libs
tests.depends = libs

TRANSLATIONS = ./i18n/librepcb.ts
