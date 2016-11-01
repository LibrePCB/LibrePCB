TEMPLATE = subdirs

SUBDIRS = \
    common \
    library \
    project \
    workspace \
    libraryeditor \
    projecteditor \
    librarymanager

library.depends = common
project.depends = library
workspace.depends = project
libraryeditor.depends = workspace
projecteditor.depends = workspace
librarymanager.depends = libraryeditor
