TEMPLATE = subdirs

SUBDIRS = \
    librepcbcommon \
    librepcblibrary \
    librepcbproject \
    librepcbworkspace \
    librepcblibraryeditor \
    librepcbprojecteditor

librepcblibrary.depends = librepcbcommon
librepcbproject.depends = librepcblibrary
librepcbworkspace.depends = librepcbproject
librepcblibraryeditor.depends = librepcbworkspace
librepcbprojecteditor.depends = librepcbworkspace
