TEMPLATE = subdirs

SUBDIRS = \
    apps \
    libs \
    tests

apps.depends = libs
tests.depends = libs

TRANSLATIONS = ./i18n/librepcb.ts
