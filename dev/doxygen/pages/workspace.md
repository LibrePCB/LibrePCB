Workspace Specification {#doc_workspace}
========================================

[TOC]

This is the documentation of a LibrePCB workspace. Such a workspace is
represented by an instance of the class ::librepcb::Workspace.


# File Structure {#doc_workspace_file_structure}

Basically, a workspace contains following entries:

    workspace-root/
        ├── .librepcb-workspace
        ├── projects/
        └── data/
            ├── .librepcb-data
            ├── settings.lp
            └── libraries/
                ├── local/
                └── remote/

## .librepcb-workspace

This file indicates that the directory is a LibrePCB workspace and also
contains the file format version. See @ref doc_versioning for more
information about LibrePCB's file format versioning system. Because the
workspace is intended to be forward compatible (i.e. should be usable with
all future LibrePCB releases), the workspace file format should always
stay at version `0.1`.

## projects/ {#doc_workspace_projects_dir}

This is the default location where all newly created projects are stored.
The user can organize his projects in an arbitrary, hierarchical system
inside that directory. But of course it's also possible to store projects
outside this directory.

Each project is represented by a directory, see its
[file structure documentation](@ref doc_project_filestructure) for details.

## data/ {#doc_workspace_data_dir}

All the settings and libraries of the workspace are stored within this
directory.

**Important:** There can exist several data directories with exactly the
same purpose and the same organization, just with different names. If a
directory with the name `v` followed by the application's file format
version number (e.g. `v0.1`) exists, this directory will be used instead
of the `data` directory. See @ref doc_workspace_upgrade and
@ref doc_workspace_downgrade for details.

## data/.librepcb-data

Contains the file format of the [data directory], to determine whether the
contained files are compatible with the application version. If this version
is older, the contained files need to be upgraded to the latest file format.
If it is newer, the [data directory] will not be accessed because it's not
compatible.

*Note: This file didn't exist in file format 0.1, so the absence of that
file is considered as file format 0.1.*

## data/settings.lp {#doc_workspace_settings_file}

This file contains workspace settings (selected language/locale, measurement
unit, ...). See ::librepcb::WorkspaceSettings.

## data/libraries/ {#doc_workspace_libraries_dir}

Here are all libraries located, split up into two subdirectories. By the way,
all of these libraries together are sometimes referred as *the workspace
library*.

## data/libraries/local/ {#doc_workspace_local_dir}

In this directory are all regular, writable libraries located. For example
if the user creates a new library, it will be stored here. Each library is
represented by one subdirectory. The contained file structure is documented
[here](@ref doc_library_file_structure).

The basename of the directories is freely choosable by the user, and the
suffix must be `.lplib`.

## data/libraries/remote/ {#doc_workspace_remote_dir}

This directory contains also libraries, but only those which are managed by
LibrePCB itself. So all libraries which are downloaded through the
[LibrePCB Server API](@ref doc_server_api) using the library manager.
Because they are updated by LibrePCB, the user cannot modify them (i.e. they
are read-only). To modify them anyway, the user can copy them as local
libraries and make changes there.

The libraries [UUID] is used as directory basenames (with `.lplib` suffix)
to avoid name conflicts.


# Example {#doc_workspace_example}

This is an example how a workspace could look like:

    MyAwesomeWorkspace/
        ├── .librepcb-workspace
        ├── projects/
        │   ├── Project_A/
        │   ├── Project_B/
        │   └── Project_C/
        ├── data/
        │   ├── .librepcb-data
        │   ├── settings.lp
        │   └── libraries/
        │       ├── local/
        │       │   ├── Library_A.lplib/
        │       │   ├── Library_B.lplib/
        │       │   └── Library_C.lplib/
        │       └── remote/
        │           ├── 5d9abd1b-cf0b-4cf7-8666-20a1add9971e.lplib/
        │           ├── 193ef70d-8dab-4a6c-a672-274c5bf09b68.lplib/
        │           └── c2c427a9-17c6-4400-981c-6ece1c9735c3.lplib/
        ├── v0.1/
        │   ├── settings.lp
        │   └── libraries/
        │       ├── local/
        │       │   ├── Library_A.lplib/
        │       │   ├── Library_B.lplib/
        │       │   └── Library_C.lplib/
        │       └── remote/
        │           ├── 5d9abd1b-cf0b-4cf7-8666-20a1add9971e.lplib/
        │           ├── 193ef70d-8dab-4a6c-a672-274c5bf09b68.lplib/
        │           └── c2c427a9-17c6-4400-981c-6ece1c9735c3.lplib/
        └── v0.2/
            ├── .librepcb-data
            └── settings.lp


# Upgrade workspace to newer application version {#doc_workspace_upgrade}

When the user opens the workspace with a specific application's major version
the first time -- let's say after updating LibrePCB from `v1.x` to `v2.x` --
LibrePCB realizes that the [data directory] contains an outdated file format.
Then a dialog is shown to create a backup first, by recursively copying
the `data` directory to `v1`. Afterwards, the `data` directory is upgraded
to file format `v2`.

There's one special case: File format `v0.1` didn't use the `data` directory
yet, it used the directory `v0.1` instead. When opening that workspace with
a newer LibrePCB version, a wizard is shown to suggest copying the `v0.1`
directory to `data`, i.e. the "backup" mechanism works the other way around.


# Downgrade workspace to older application version {#doc_workspace_downgrade}

As the [data directory] backups are not removed automatically, a downgrade is
not required at all. The application simply uses the backup directory (e.g.
`v0.1`) instead of the `data` directory, so all settings and libraries of
the older application version are still there and compatible. The workspace
can thus smoothly be used with different application versions at the same
time.

If a [data directory] backup is no longer required for sure, the user can
just remove it by himself (to reduce disk usage).


# Library Index (SQLite Database) {#doc_workspace_library_database}

For better performance of the workspace library (searching in the filesystem
can be quite slow), an [SQLite] database is used for caching/indexing (see
::librepcb::WorkspaceLibraryDb). From time to time this database is
filled/updated with metadata of all libraries and their elements. Searching
for elements is then done with `SELECT` statements in the database, which
is very fast.

The database is stored within the [data directory] directory of the workspace.

Here is the current structure of the database:

![Database ERM](database_diagram.png)


[UUID]: https://en.wikipedia.org/wiki/Universally_unique_identifier "Universally Unique Identifier"
[SQLite]: https://sqlite.org/ "SQLite"
[data directory]: #doc_workspace_data_dir
