TEMPLATE = subdirs

SUBDIRS = \
    common \
    library \
    project \
    workspace \
    libraryeditor \
    projecteditor

library.depends = common
project.depends = library
workspace.depends = project
libraryeditor.depends = workspace
projecteditor.depends = workspace
