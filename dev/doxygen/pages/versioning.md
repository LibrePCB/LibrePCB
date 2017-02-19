File Format Versioning {#doc_versioning}
========================================

[TOC]

# Version Files {#doc_versioning_file}

All LibrePCB entities which are saved as directories (workspaces, projects, libraries and library
elements) contain a hidden file in their root directory which is called `.librepcb-<ELEMENTNAME>`
(e.g. `.librepcb-project`). This file contains only a version number (e.g. `0.1`) which specifies
the file format version of the corresponding entity.

When opening an element (e.g. a project), LibrePCB first reads the version number from that file to
check whether the element is older, newer or equal to the current application version. Depending on
the result, LibrePCB decides if and how that element can be opened.


# Backward Compatibility

LibrePCB provides backward compatibility, at least to the previous major application version
(ideally even down to `v0.1`). This means that a specific application version can always open
elements which were created with older application versions.

If the [version file] of an element to open contains a version number lower than the currently used
application version, an update mechanism first migrates all files to the new file format before
the element is opened ordinary.


# Forward Compatibility

On the other hand, LibrePCB does *not* provide forward compatibility for all kinds of elements! So
it's not possible to open elements which were saved with a newer than the currently used application
version.

If the [version file] of an element to open contains a version number higher than the currently used
application version, LibrePCB refuses to open that element. Because the minimum required application
version which is required to open an element is the same as the version number in the [version file],
LibrePCB is able to display an error message like *"You need at least LibrePCB version X.Y.Z to open
that element"*.

To make this always working properly, [version file]s actually *do* need to be forward compatible!
So [version file]s are the only files of LibrePCB which are designed to be forward compatible, while
all other files are not. This gives us maximum flexibility to change the file format in future (we
could even switch from XML to another file format some day).


[version file]: #doc_versioning_file
