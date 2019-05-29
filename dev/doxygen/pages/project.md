Project Specification {#doc_project}
====================================

[TOC]

This is the documentation of a LibrePCB project. Such a project is represented by an instance of the
class librepcb::project::Project.


# The File Structure of a Project {#doc_project_filestructure}

A project directory contains following files and subdirectories:

    project-root/
        ├── .librepcb-project
        ├── <PROJECTNAME>.lpp
        ├── README.md
        ├── boards/
        │   ├── boards.lp
        │   └── <BOARDNAME>/
        │       ├── board.lp
        │       └── settings.user.lp
        ├── circuit/
        │   ├── circuit.lp
        │   └── erc.lp
        ├── library/
        ├── output/
        ├── project/
        │   ├── metadata.lp
        │   └── settings.lp
        ├── resources/
        └── schematics/
            ├── schematics.lp
            └── <SCHEMATICNAME>/
                └── schematic.lp

## .librepcb-project

This file indicates that the directory is a LibrePCB project and also contains the file format
version. See @ref doc_versioning for more information about LibrePCB's file format versioning system.

## <PROJECTNAME\>.lpp

This is the main project file which can be opened by double-clicking in the file manager.
Because this is the only purpose of that file, it doesn't contain any information.

## README.md

Optional readme file in Markdown. Content will be rendered in the Control Panel.

## boards/boards.lp

Contains the list of all boards (in a specific order) of the project.

## boards/<BOARDNAME\>/board.lp

A board file containing all the package positions and traces. See librepcb::project::Board.

## boards/<BOARDNAME\>/settings.user.lp

User-specific settings of a board, for example which layers are visible or hidden.
See librepcb::project::BoardUserSettings.

## circuit/circuit.lp

Contains the whole netlist with all net classes, net signals and connections between
components. See librepcb::project::Circuit.

## circuit/erc.lp

ERC settings (e.g. which ERC messages are ignored). See librepcb::project::ErcMsgList.

## library/

Directory containing all the library elements used in the project. Same directory
structure as of workspace libraries. See librepcb::project::ProjectLibrary.

## output/

Directory intended to store all generated output files of the project, e.g. Gerber or PDFs.

## project/metadata.lp

Contains project meta data. See librepcb::project::ProjectMetadata.

## project/settings.lp

Contains project settings. See librepcb::project::ProjectSettings.

## resources/

Contains various resource files needed in the project, e.g. fonts.

## schematics/schematics.lp

Contains the list of all schematics (in a specific order) of the project.

## schematics/<SCHEMATICNAME\>/schematic.lp

A schematic file containing all the symbol positions and wires. See librepcb::project::Schematic.

## Example

This is an example how a project could look like:

    MyProject/
        ├── .librepcb-project
        ├── MyProject.lpp
        ├── README.md
        ├── boards/
        │   ├── boards.lp
        │   ├── MyBoard1/
        │   │   ├── board.lp
        │   │   └── settings.user.lp
        │   └── MyBoard2/
        │       ├── board.lp
        │       └── settings.user.lp
        ├── circuit/
        │   ├── circuit.lp
        │   └── erc.lp
        ├── library/
        │   ├── cmp/
        │   │   ├── e995a552-12c4-413a-8258-1d6ec9a8471d/
        │   │   │   └── component.lp
        │   │   └── ef80cd5e-2689-47ee-8888-31d04fc99174/
        │   │       └── component.lp
        │   ├── dev/
        │   │   ├── f7fb22e8-0bbc-4f0f-aa89-596823b5bc3e/
        │   │   │   └── device.lp
        │   │   └── f83a5ae8-7f42-42be-9dd6-e762f4da2ec2/
        │   │       └── device.lp
        │   ├── pkg/
        │   │   ├── bf77778d-eee6-4b56-ad82-d24f9c668a63/
        │   │   │   └── package.lp
        │   │   └── d6527201-0b43-4ec6-96e1-8dde11a38645/
        │   │       └── package.lp
        │   └── sym/
        │       ├── eaa5837a-a451-40ae-8620-d21e9af42151/
        │       │   └── symbol.lp
        │       └── f00ab942-6980-442b-86a8-51b92de5704d/
        │           └── symbol.lp
        ├── output/
        │   └── v1/
        │       ├── MyProject_Schematics.pdf
        │       └── gerber/
        │           ├── MyProject_COPPER-BOTTOM.gbr
        │           ├── MyProject_COPPER-TOP.gbr
        │           ├── MyProject_DRILLS-PTH.drl
        │           ├── MyProject_OUTLINES.gbr
        │           ├── MyProject_SILKSCREEN-BOTTOM.gbr
        │           ├── MyProject_SILKSCREEN-TOP.gbr
        │           ├── MyProject_SOLDERMASK-BOTTOM.gbr
        │           └── MyProject_SOLDERMASK-TOP.gbr
        ├── project/
        │   ├── metadata.lp
        │   └── settings.lp
        ├── resources/
        │   └── fontobene/
        │       └── newstroke.bene
        └── schematics/
            ├── schematics.lp
            ├── MySchematicPage1/
            │   └── schematic.lp
            └── MySchematicPage2/
                └── schematic.lp


# Compatibility between different application versions {#doc_project_compatibility}

See @ref doc_versioning.


# File format upgrade procedure {#doc_project_upgrade}

When opening a project with an older file format, LibrePCB first upgrades it
step by step to the latest file format version. Afterwards it is opened as
usual. Thanks to the transactional file system
librepcb::TransactionalFileSystem, the whole upgrade procedure is done in RAM.
So unless the user saves the upgraded project, no files get overwritten at all.
This allows to open projects with an older file format without actually
upgrading the files.


# Avoid opening a project multiple times (Directory Lock) {#doc_project_lock}

Opening a project multiple times simultaneously can be dangerous. Even if the application's design
does not allow to open a project multiple times, it's still possible to save a project in a shared
folder and open it from different computers at the same time (for example). This could demage the
project files.

To avoid such problems, the constructor of librepcb::project::Project tries to lock the project's
directory with a librepcb::DirectoryLock object (librepcb::project::Project::mLock). If the project
is already locked by another application instance, the project cannot be opened again (the
constructor of librepcb::project::Project throws an exception). If the lock file exists because the
application was crashed while the project was open, the user gets a message box which asks whether
the last automatic backup should be restored or not (see also @ref doc_project_save). The lock will
be released automatically when the librepcb::project::Project instance is destroyed (RAII, see
librepcb::DirectoryLock).

@see librepcb::DirectoryLock, librepcb::project::Project::Project(), librepcb::project::Project::mLock


# Automatic Periodic Saving (autosave) {#doc_project_autosave}

An autosave system is an absolutely must-have to avoid the loose of all
modifications after saving a project the last time. If the application crashes
with unsaved chamodificationsnges, it must be possible to restore at least
some of these modifications when opening that project the next time.

LibrePCB uses following concept to automatically create a backup of projects:

* Unsaved modifications (only the diff, not a whole snapshot) are periodically
  saved into the `.autosave` directory inside the project. Basically it
  contains all modified files and an SExpression file with a list of files and
  directories which were removed.
* When gracefully closing a project (or the whole application), the `.autosave`
  directory will be removed.
* If the application crashes while a project is opened, the cleanup code is
  never executed and thus the `.autosave` directory won't be removed. When
  opening this project the next time, the existence of the `.autosave` directory
  (resp. its contained file `autosave.lp`) is detected and the user is asked
  whether to restore the autosave backup or not.

Most of these things are implemented in the librepcb::TransactionalFileSystem
class which is used by librepcb::project::Project.


# Atomic Saving Procedure {#doc_project_save}

It's important to keep all files of a project valid and consistent. If the
application crashes, the project files must not be demaged. Even a crash while
saving the project should not demage the project, i.e. the save procedure must
be 100% atomic.

To guarantee consistent project files in every case, following steps are done
when the user saves a project:

1. All modifications since the last save (i.e. only the diff) are written to
   the `.backup` directory inside the project.
2. The `.autosave` directory (if existing) is removed because it might contain
   a backup which is now outdated (it should not be possible to restore
   outdated autosave backups).
3. All modifications since the last save (i.e. only the diff) are now also
   saved to the actual project files.
4. The `.backup` directory is removed.

If one of these steps fails (e.g. by throwing an exception), the save procedure
is aborted and subsequent steps won't be executed.

If the application crashes during step 1, there is no valid backup so the
next time the project will be loaded as usual. Even if the unsaved modifications
are lost, the project is not broken. Thanks to the autosave, the user might
even be able to restore most of the apparently lost modifications.

If the application crashes during step 2 or 3, there is a valid backup available
which will automatically be loaded when opening the project the next time. So
no changes are lost.

If the application crashes during step 4, either the actual project files or the
backup will be loaded the next time opening the project, depending on whether
the `backup.lp` file was removed or not. As both locations contain exactly the
same state, it doesn't matter from which location the files are loaded. The
project can be opened successfully anyway, without any lost modifications.

Most of these things are implemented in the librepcb::TransactionalFileSystem
class which is used by librepcb::project::Project.


# The undo/redo system (Command Design Pattern) {#doc_project_undostack}

It's very important to have undo and a redo commands for the whole project (and maybe also for
independent parts of the project). For this purpose we use the "Command Design Pattern". Our common
classes librepcb::UndoCommand and librepcb::UndoStack implement this design pattern. Look at their
documentation for more details.

@note The documentation [Overview of Qt's Undo Framework](http://doc.qt.io/qt-5/qundo.html) and/or
other documentations about the Command Design Pattern may be useful to understand the whole
undo/redo system. Our own undo classes are quite similar to Qt's classes `QUndoCommand` and
`QUndoStack`, but provide some additional functionality.

All librepcb::UndoCommand subclasses which are used to modify a librepcb::project::Project are located
in the namespace librepcb::project. But the librepcb::UndoStack is only used in the namespace
librepcb::project::editor, so it is located there: librepcb::project::editor::ProjectEditor::mUndoStack.

All important changes to the project which should appear in the undo stack need to be implemented as
subclasses of librepcb::UndoCommand and must be appended to the project's undo stack
librepcb::project::editor::ProjectEditor::mUndoStack. Changes which do not need an undo action (like
changing the color of a layer) can be done without the use of the undo/redo system. These changes
then cannot be undone. Basically, all changes to the circuit/schematics/boards and so on must have
an undo action. If you are unsure if you should use the project's undo/redo system, try to answer
the question *do the user wants to undo/redo this action with the undo/redo buttons in the editors?*.

It's possible to use seperate librepcb::UndoStack objects for independent parts of the project. For
example, changing the project settings could be done by using an undo/redo system to provide an undo
command. But these undo stacks need their own undo/redo buttons. The undo/redo buttons in the
schematic editor and in the board editor are only connected to the project's undo stack
librepcb::project::editor::ProjectEditor::mUndoStack.

librepcb::UndoCommand objects can also have an unlimited amount of child objects. This is very
useful to group actions together. For example if multiple symbols are selected in the schematic and
the user wants to remove them all, multiple librepcb::UndoCommand objects will be created (one for
each symbol). But if the user now wants to undo the deletion, the undo command (Ctrl+Z) will only
bring back the last symbol, so the user needs to press the undo command multiple times. This is not
good, so we use the parent/child mechanism of librepcb::UndoCommand. This means that we will still
create an librepcb::UndoCommand for each removed symbol. But we will also create a parent
librepcb::UndoCommand for all these child commands. Only the parent command will be added to the
undo stack. If the user now wants to undo the deletion, he only needs to press undo (Ctrl+Z) once.
And with pressing redo (Ctrl+Y) once, all symbols will be restored.

@see librepcb::UndoCommand, librepcb::UndoStack, librepcb::project::editor::ProjectEditor::mUndoStack
@see http://doc.qt.io/qt-5/qundo.html (Overview of Qt's Undo Framework)
