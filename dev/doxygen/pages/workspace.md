Workspace Specification {#doc_workspace}
========================================

[TOC]

This is the documentation of a LibrePCB workspace. Such a workspace is represented by an instance of
the class librepcb::workspace::Workspace.


# File Structure {#doc_workspace_file_structure}

Basically, a workspace contains following entries:

    workspace-root/
        ├── .librepcb-workspace
        ├── projects/
        └── v<VERSION>/
            ├── metadata/
            └── libraries/
                ├── local/
                └── remote/

## .librepcb-workspace

This file indicates that the directory is a LibrePCB workspace and also contains the file format
version. See @ref doc_versioning for more information about LibrePCB's file format versioning system.
Because the workspace is intended to be forward compatible (i.e. should be usable with all future
LibrePCB releases), the workspace file format should always stay at `v0.1`.

## projects/ {#doc_workspace_projects_dir}

This is the default location where all newly created projects are stored. The user can organize his
projects in an arbitrary, hierarchical system inside that directory. But of course it's also
possible to store projects outside this directory.

Each project is represented by a directory, see its
[file structure documentation](@ref doc_project_file_structure) for details.

## v<VERSION>/ {#doc_workspace_version_dir}

For each major application version, a separate directory is created to store workspace metadata and
libraries. These directories are called for example `v0.1`, `v1` or `v2`. A specific application
version will only access the metadata/libraries of its corresponding directory, all other `v*`
directories will be ignored.

## v<VERSION>/metadata/ {#doc_workspace_metadata_dir}

All workspace settings and other workspace related stuff is stored in this directory.

## v<VERSION>/libraries/ {#doc_workspace_libraries_dir}

Here are all libraries located, splitted up into two subdirectories. By the way, all of these
libraries together are sometimes referred as *the workspace library*.

## v<VERSION>/libraries/local/ {#doc_workspace_local_dir}

In this directory are all regular, writable libraries located. For example if the user creates a new
library, it will be stored here. Each library is represented by one subdirectory. The contained file
structure is documented [here](@ref doc_library_file_structure).

The basename of the directories is freely choosable by the user, and the suffix must be `.lplib`.

## v<VERSION>/libraries/remote/ {#doc_workspace_remote_dir}

This directory contains also libraries, but only those which are managed by LibrePCB itself. So all
libraries which are downloaded through the [LibrePCB Repository API](@ref doc_repository) using the
library manager. Because they are updated by LibrePCB, the user cannot modify them (i.e. they are
read-only). To modify them anyway, the user can copy them as local libraries and make changes there.

The libraries [UUID] is used as directory basenames (with `.lplib` suffix) to avoid name conflicts.

# Example {#doc_workspace_example}

This is an example how a workspace could look like:

    MyAwesomeWorkspace/
        ├── .librepcb-workspace
        ├── projects/
        │   ├── Project_A/
        │   ├── Project_B/
        │   └── Project_C/
        ├── v0.1/
        │   ├── metadata/
        │   └── libraries/
        │       ├── local/
        │       │   ├── Library_A.lplib/
        │       │   ├── Library_B.lplib/
        │       │   └── Library_C.lplib/
        │       └── remote/
        │           ├── 5d9abd1b-cf0b-4cf7-8666-20a1add9971e.lplib/
        │           ├── 193ef70d-8dab-4a6c-a672-274c5bf09b68.lplib/
        │           └── c2c427a9-17c6-4400-981c-6ece1c9735c3.lplib/
        ├── v1/
        │   ├── metadata/
        │   └── libraries/
        │       ├── local/
        │       │   ├── Library_A.lplib/
        │       │   ├── Library_B.lplib/
        │       │   └── Library_C.lplib/
        │       └── remote/
        │           ├── 5d9abd1b-cf0b-4cf7-8666-20a1add9971e.lplib/
        │           ├── 193ef70d-8dab-4a6c-a672-274c5bf09b68.lplib/
        │           └── c2c427a9-17c6-4400-981c-6ece1c9735c3.lplib/
        └── v2/
            ├── metadata/
            └── libraries/
                ├── local/
                │   ├── Library_A.lplib/
                │   ├── Library_B.lplib/
                │   └── Library_C.lplib/
                └── remote/
                    ├── 5d9abd1b-cf0b-4cf7-8666-20a1add9971e.lplib/
                    ├── 193ef70d-8dab-4a6c-a672-274c5bf09b68.lplib/
                    └── c2c427a9-17c6-4400-981c-6ece1c9735c3.lplib/


# Upgrade workspace to newer application version {#doc_workspace_upgrade}

When the user opens the workspace with a specific application's major version the first time (e.g.
after updating LibrePCB from `v1.x` to `v2.x`), LibrePCB realizes that the new [version directory]
(e.g. `v2`) does not yet exist. Then probably a wizard will be shown to upgrade the workspace to the
newer version. Basically, an older [version directory] (e.g. `v1`) will just be cloned (e.g. to
`v2`) and the contained files will be upgraded then, if required.


# Downgrade workspace to older application version {#doc_workspace_downgrade}

As the old [version directory] is not removed automatically after an upgrade, a downgrade is not
required at all. The workspace can thus smoothly be used with different application versions at the
same time. If an older [version directory] is no longer required for sure, the user can just remove
it by himself (to reduce disk usage).


# Library Index (SQLite Database) {#doc_workspace_library_database}

For better performance of the workspace library (searching in the filesystem can be quite slow), an
[SQLite] database is used for caching/indexing (see librepcb::workspace::WorkspaceLibraryDb). From
time to time this database is filled/updated with metadata of all libraries and their elements.
Searching for elements is then done with `SELECT` statements in the database, which is very fast.

The database is stored in the [`metadata`](#doc_workspace_metadata_dir) directory of the workspace.


[UUID]: https://en.wikipedia.org/wiki/Universally_unique_identifier "Universally Unique Identifier"
[SQLite]: https://sqlite.org/ "SQLite"
[version directory]: #doc_workspace_version_dir
