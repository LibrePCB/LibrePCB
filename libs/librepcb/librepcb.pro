TEMPLATE = subdirs

SUBDIRS = \
    common \
    library \
    project \
    workspace \
    eagleimport \
    libraryeditor \
    projecteditor \
    librarymanager

library.depends = common
project.depends = library
workspace.depends = project
eagleimport.depends = workspace
libraryeditor.depends = eagleimport
projecteditor.depends = eagleimport
librarymanager.depends = libraryeditor
