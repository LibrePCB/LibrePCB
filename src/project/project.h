/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PROJECT_PROJECT_H
#define PROJECT_PROJECT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>
#include "../common/exceptions.h"
#include "../common/filelock.h"
#include "../common/filepath.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class XmlFile;
class Workspace;

namespace project {
class ProjectLibrary;
class Circuit;
class SchematicEditor;
}

/*****************************************************************************************
 *  Class Project
 ****************************************************************************************/

namespace project {

/**
 * @brief The Project class represents a whole (opened!) project with all its content
 *
 * This class represents a whole project with all the content of its directory:
 *  - circuit, schematics and boards
 *  - the project's library
 *  - project settings
 *  - GUI elements: schematic editor, board editor and other windows
 *  - and much more...
 *
 * Project objects will be created in a Workspace object. The constructor of the Project
 * class needs the filepath to a project file. Then the project will be opened. The
 * destructor will close the project (without saving). Use the slot #save() to write the
 * whole project to the harddisc.
 *
 * @note !! A detailed description about projects is available here: @ref doc_project !!
 *
 * @author ubruhin
 * @date 2014-06-24
 */
class Project final : public QObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor

        /**
         * @brief The constructor to open an existing project with all its content
         *
         * @note    The constructor will always try to open the specified project. You
         *          cannot create an instance of Project without opening the project.
         *          The destructor then will close the project. It's not possible to
         *          close a project without destroying the Project instance.
         *
         * @param workspace     A pointer to the Workspace object
         * @param filepath      The filepath to the *.e4u project file
         *
         * @throw Exception     If the project could not be opened successfully, the
         *                      constructor will throw an exception of type Exception
         *                      (or a subclass of it). Before throwing an exception, the
         *                      user will get a messagebox with the error message.
         */
        explicit Project(Workspace& workspace, const FilePath& filepath) throw (Exception);

        /**
         * @brief The destructor will close the whole project (without saving!)
         */
        ~Project() noexcept;


        // Getters

        /**
         * @brief Get the filepath of the project file (*.e4u)
         *
         * @return The absolute filepath
         */
        const FilePath& getFilepath() const noexcept {return mFilepath;}

        /**
         * @brief Get the path to the project directory
         *
         * @return The filepath to the project directory
         */
        const FilePath& getPath() const noexcept {return mPath;}

        /**
         * @brief Get a pointer to the undo stack of the project
         *
         * @return A pointer to the QUndoStack object (read only)
         */
        const QUndoStack* getUndoStack() const noexcept {return mUndoStack;}

        /**
         * @brief Get the Circuit object
         *
         * @return A reference to the Circuit object
         */
        Circuit& getCircuit() const noexcept {return *mCircuit;}


        // General Methods

        /**
         * @brief Inform the project that a project related window is about to close
         *
         * The project will be closed and destroyed automatically after the last opened
         * window of the project is closed, because without a window the user is no longer
         * able to close the project himself. So, every project related window have to
         * "ask" the Project object whether it is allowed to close or not. If the last
         * opened window wants to close, the project will first ask the user if unsaved
         * changes should be written to the harddisc. Only if the user accepts this
         * question and the project is saved successfully, the method will return true
         * to allow the last window to close. Then it will also close the whole project.
         *
         * @param window    A pointer to the window which is about to close
         *
         * @return true if the window can be closed, false if closing the window is denied
         */
        bool windowIsAboutToClose(QMainWindow* window);

        /**
         * @brief Execute a command and push it to the undo stack
         *
         * @param cmd   The command to execute
         *
         * @throw Exception If the command cannot be executed, an exception will be thrown
         */
        void executeCommand(QUndoCommand* cmd) throw (Exception);


    public slots:

        /**
         * @brief Open the schematic editor window and bring it to the front
         */
        void showSchematicEditor();

        /**
         * @brief Save the whole project to the harddisc
         *
         * @note The whole save procedere is described in @ref doc_project_save.
         *
         * @return true on success, false on failure
         */
        bool save() noexcept;

        /**
         * @brief Make a automatic backup of the project (save to temporary files)
         *
         * @note The whole save procedere is described in @ref doc_project_save.
         *
         * @return true on success, false on failure
         */
        bool autosave() noexcept;

        /**
         * @brief Close the project (this will destroy this object!)
         *
         * If there are unsaved changes to the project, this method will ask the user
         * whether the changes should be saved or not. If the user clicks on "cancel"
         * or the project could not be saved successfully, this method will return false.
         * If there was no such error, this method will call QObject#deleteLater() which
         * means that this object will be deleted in the Qt's event loop.
         *
         * @warning This method can be called both from within this class and from outside
         *          this class (for example from the Workspace). But if you call this
         *          method from outside this class, you have to delete the object yourself
         *          afterwards! In special cases, the deleteLater() mechanism could lead
         *          in fatal errors otherwise!
         *
         * @param msgBoxParent  Here you can specify a parent window for the message box
         *
         * @return true on success, false on failure (= project stays open)
         */
        bool close(QWidget* msgBoxParent = 0);


    private:

        // make some methods inaccessible...
        Project();
        Project(const Project& other);
        Project& operator=(const Project& rhs);

        // Private Methods
        bool save(bool toOriginal, QStringList& errors) noexcept;

        // General
        Workspace& mWorkspace; ///< a reference to the workspace

        // Project File (*.e4u)
        FilePath mPath; ///< the path to the project directory
        FilePath mFilepath; ///< the filepath of the *.e4u project file
        XmlFile* mXmlFile;
        FileLock mFileLock; ///< See @ref doc_project_lock

        // General
        QTimer mAutoSaveTimer; ///< the timer for the periodically automatic saving functionality (see also @ref doc_project_save)
        QUndoStack* mUndoStack; ///< See @ref doc_project_undostack
        ProjectLibrary* mProjectLibrary;
        Circuit* mCircuit;
        SchematicEditor* mSchematicEditor;
};

} // namespace project

/*****************************************************************************************
 *  Doxygen: Project Documentation
 ****************************************************************************************/

/**
    @page doc_project Project Documentation
    @tableofcontents

    This is the documentation of an EDA4U project. Such a project is represented by an
    instance of the class project#Project.


    @section doc_project_filestructure The File Structure of a Project

        See @subpage doc_project_file_structure "Project File Structure Documentation".


    @section doc_project_lock Avoid opening a project multiple times (File Lock)

        It's very dangerous if a project is opened multiple times simultaneously. Even if
        the application's design does not allow to open a project multiple times, it's
        still possible to save a project in a shared folder and open it from different
        computers at the same time (for example). This can demage the project files!

        To avoid such problems, the Project's constructor will try to lock the project
        file (*.e4u) with a FileLock object (project#Project#mFileLock). If the project
        file is already locked by another application instance, the project cannot be
        opened (the Project's constructor throws an exception). If the lock file exists
        because the application was crashed while the project was open, the user gets a
        message box to ask whether the last automatic backup should be restored or not
        (see also @ref doc_project_save). The lock will be released automatically when
        the project#Project object is destroyed (RAII, see FileLock documentation).

        @see FileLock, project#Project#Project(), project#Project#mFileLock


    @section doc_project_undostack The undo/redo system (Command Design Pattern)

        It's very important to have an undo and a redo command for the whole project
        (and maybe also for independent parts of the project). For this purpose we use
        the "Command Design Pattern" respectively the Qt classes which implements this
        design pattern: QUndoCommand, QUndoStack, QUndoGroup, QUndoView.

        @note Please read the documentation "Overview of Qt's Undo Framework"
        (http://qt-project.org/doc/qt-5/qundo.html) for more information. Maybe other
        documentations about the Command Design Pattern are also useful to understand
        the undo/redo system.

        There is a QUndoStack object for the whole project (project#Project#mUndoStack).
        All important changes to the project which should appear in the undo stack
        needs to be implemented as a subclass of QUndoCommand and must be appended to
        project#Project#mUndoStack. Changes which do not need an undo action (like
        changing the color of a layer) can be done without the use of the undo/redo
        system. These changes cannot be undone. Basically, all changes to the
        circuit/schematics/boards and so on must have an undo action! If you are unsure
        if you should use the project's undo/redo system, try to answer the question
        "do the user wants to undo/redo this action with the undo/redo buttons in the
        schematic- or board-editor?".

        It's possible to use seperate QUndoStack objects for independent parts of the
        project. For example, changing the project settings could be done by using an
        undo/redo system to provide an undo command. But these undo stacks need their
        own undo/redo buttons. The undo/redo buttons in the schematic editor and in the
        board editor are only connected to the stack project#Project#mUndoStack.

        QUndoCommands can also have an unlimited amount of child QUndoCommand objects.
        This is very useful to group actions together. For example, if multiple symbols
        are selected in the schematic and the user wants to remove them all, multiple
        QUndoCommand objects will be created (for each symbol one). But if the user
        now wants to undo the deletion, the undo command (Ctrl+Z) will only bring back
        one symbol, so the user needs to press the undo command multiple times. This
        is not good. So we use the parent/child mechanism of QUndoCommand. This means,
        we will still create a QUndoCommand for each removed symbol. But we will also
        create a parent QUndoCommand for all these child commands. Only the parent command
        will be added to the undo stack (for example project#Project#mUndoStack).
        Now, if the user wants to undo the deletion, he only needs to press "Ctrl+Z"
        (undo) once. And with pressing "Ctrl+Y" (redo) once, all symbols will be restored.

        @see project#Project#mUndoStack
        @see http://qt-project.org/doc/qt-5/qundo.html (Overview of Qt's Undo Framework)


    @section doc_project_save Saving Procedere / Automatic Periodically Saving

        It's very important to keep all files of a project valid and consistent. If the
        application crashes, the project files must not be demaged! Also a crash while
        saving the project should not demage the project (yes, this is difficult...).

        Additionally, an automatic backup/restore system is useful to avoid the loose of
        all the changes made in a project after the last saving. If the application
        crashes with unsaved changes, it should be possible to restore (some of) these
        changes while opening that project the next time. To indicate a crash, the
        FileLock class is used (see @ref doc_project_lock)

        <b>To try to reach these goals, we use temporary copies of the project files while a
        project is open. This means:</b>
            -# Each project module which open some files (project#Project, project#Circuit,
               project#Schematic and so on) must copy their needed files to a temporary
               file (same filepath but with a '~' at the end) before opening them.
            -# Now, NOT the needed files will be opened, but the copied temporary files!
            -# The automatic backup will save all changes to these temporary files.
            -# If the user wants to save the project, all changes will be saved to the
               temporary files first! Only after ALL changes to the whole project were
               saved SUCCESSFULLY to these files, the changes can be written to the
               original files! The temporary files will still exist, do not remove them!
               If saving to the temporary files was not successful, you mustn't begin
               saving the changes to the original files! This way, the original files
               will stay valid even if there is an error in the save algorithm.
            -# On closing a project, all temporary files will be removed from the harddisc.

        If the application crashes while unsaved changes are made in a project, the
        temporary files won't be removed (and also the file lock still exist). On opening
        this project the next time, the user can choose whether to restore the last backup
        or not. If not, the normal, original files will be loaded (as if there was no
        crash). But if the backup should be restored, on opening the files the original
        files must not copied to a backup file, but the backup files must be loaded
        directly (skip #1 at the list above)! For this purpose, the constructor of classes
        like project#Project, project#Circuit and so on needs a parameter "bool restore"
        (or similar).


        <b>Details of #4 of the list above (very important):</b><br>
        To save a project, all related classes need a method with this signature:
        @code bool save(bool toOriginal, QStringList& errors) noexcept; @endcode

        This method will be called to save all the changes of the corresponding object
        <b>and all its child objects</b>. So, the class project#Project provides such a
        method to save <b>the whole project with all its content</b> (circuit, schematics
        and so on). If the parameter "toOriginal" is false, all changes will be saved to
        the temporary files. Otherwise they are saved to the original files. Error
        messages must be appended the QStringList "errors" (after translation). All
        entries in the list will be printet out in a message box. The return value is true
        if the saving proccess was successful, and false if there was an error. These
        methods should never throw an exception (keyword "noexcept")!

        If an error occurs while saving the project, you should not abort the saving
        proccess. "Save all what you can" is the name of the game :-) Simply add the
        error message to the error list and return "false" after the job is finished.

        The class project#Project also provides the public slot project#Project#save().
        This is the only method you are allowed to call from outside the Project class
        to save the whole project! This method will try to save the whole project to
        the temporary files (this->save(false)). <b>Only if this call has returned "true"
        (project successful saved to temporary files), it will also save the project to
        the original files (this->save(true))!</b>

        @todo add the ability to show the saving progress in a progress bar

        Each project also contains a QTimer for automatic backups. This timer will call
        Project::save(false) periodically. So, the project will be saved only to the
        temporary files, NOT to the original files (because the user did NOT click on the
        "Save" button).

*/

/*****************************************************************************************
 *  Doxygen: Project File Structure Documentation
 ****************************************************************************************/

/**

    @page doc_project_file_structure Project File Structure Documentation

    @tableofcontents

    @warning THIS PAGE IS NOT UP TO DATE!!!

    @section project_file_hierarchy Project File Hierarchy

        In the project directory, there are only files from EDA4U, the user mustn't save other files there!
        There are the following directories and files in a EDA4U project directory:

            - @ref project_dir_boards "boards/"
                - board_name/
                    - ...
                - ...
            - @ref project_dir_lib "lib/"
                - {0af210c0-7625-4f8c-bc6d-c9f5b49b7024}/
                    - ...
                - ...
            - @ref project_dir_panels "panels/"
                - panel_name/
                    - ...
                - ...
            - @ref project_dir_schematics "schematics/"
                - schematic_name
                    - ...
                - ...
            - @ref project_dir_simulations "simulations/"
                - simulation_name
                    - ...
                - ...
            - @ref project_dir_user "user/"
                - ...


        @subsection project_dir_boards Directory "boards/"

            In this directory there are all boards (PCB layouts) saved.
            For each board, there is a seperate directory which is named by the board's name.
            But the directory name mustn't contain special characters:
                - only lower case letters, numbers and underlines are allowed - nothing more!
                - all other characters (including spaces) will be replaced with underlines


        @subsection project_dir_lib Directory "lib/"

            In this directory there are all library elements (footprints, components, symbols, ...) saved which are needed in the project.
            Each element is saved in his own directory which are named with the element's uuid.
            These directories are 100 percent identical with the elements in the EDA4U library, these are 1:1 copies of them.

            If the user adds a new component to the project, this component and all its possible symbols, footprints and so on will be copied to this directory.
            If the user removes a component, that component and all its possible symbols, footprints and so on, which aren't needed by other components in the project,
            will be deleted immediately.


        @subsection project_dir_panels Directory "panels/"

            In this directory there are all panels saved.
            For each panel, there is a seperate directory which is named by the panel's name.
            But the directory name mustn't contain special characters:
                - only lower case letters, numbers and underlines are allowed - nothing more!
                - all other characters (including spaces) will be replaced with underlines


        @subsection project_dir_schematics Directory "schematics/"

            In this directory there are all schematics (pages) saved.
            For each schematic, there is a seperate directory which is named by the schematic's name.
            But the directory name mustn't contain special characters:
                - only lower case letters, numbers and underlines are allowed - nothing more!
                - all other characters (including spaces) will be replaced with underlines


        @subsection project_dir_simulations Directory "simulations/"

            In this directory there are all simulations saved.
            For each simulation, there is a seperate directory which is named by the simulation's name.
            But the directory name mustn't contain special characters:
                - only lower case letters, numbers and underlines are allowed - nothing more!
                - all other characters (including spaces) will be replaced with underlines


        @subsection project_dir_user Directory "user/"

            In this directory are all user related settings for that project saved.
            It's important to have all this data in one directory, so the user only needs to ignore one directory if he uses a version contol system.
            ALl data in this direcotory is not really important.

            @warning The user's version control system (if he uses one) needs to ignore this directory!

*/

#endif // PROJECT_PROJECT_H
