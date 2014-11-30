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

class QPrinter;
class XmlFile;
class IniFile;
class UndoStack;
class SchematicLayer;

namespace project {
class ProjectLibrary;
class Circuit;
class SchematicEditor;
class Schematic;
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
         * @brief The constructor to create or open a project with all its content
         *
         * @note    The constructor will always try to open the specified project. You
         *          cannot create an instance of Project without opening the project.
         *          The destructor then will close the project. It's not possible to
         *          close a project without destroying the Project instance.
         *
         * @param filepath      The filepath to the new or existing *.e4u project file
         * @param create        True if the specified project does not exist already and
         *                      must be created.
         *
         * @throw Exception     If the project could not be created/opened successfully,
         *                      the constructor will throw an exception of type #Exception
         *                      (or a subclass of it). The catcher of the exception should
         *                      then show a message box with the error message. Only
         *                      exceptions of type #UserCanceled should be ignored (no
         *                      msg box) because the user has aborted opening the project.
         */
        explicit Project(const FilePath& filepath, bool create) throw (Exception);

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
         * @brief Get a reference to the undo stack of the project
         *
         * @return A reference to the UndoStack object
         */
        UndoStack& getUndoStack() const noexcept {return *mUndoStack;}

        /**
         * @brief Get the Circuit object
         *
         * @return A reference to the Circuit object
         */
        Circuit& getCircuit() const noexcept {return *mCircuit;}

        /**
         * @brief Get the ProjectLibrary object which contains all library elements used
         *        in this project
         *
         * @return A reference to the ProjectLibrary object
         */
        ProjectLibrary& getLibrary() const noexcept {return *mProjectLibrary;}

        /**
         * @brief Get all Schematic Layers
         *
         * @return A reference to the QHash with all schematic layers
         */
        const QHash<unsigned int, SchematicLayer*>& getSchematicLayers() const noexcept {return mSchematicLayers;}

        /**
         * @brief Get a Schematic Layer with a specific ID
         *
         * @param id    The ID of the layer
         *
         * @return  A pointer to the SchematicLayer object, or NULL if there is no layer
         *          with the specified ID
         */
        SchematicLayer* getSchematicLayer(unsigned int id) const noexcept {return mSchematicLayers.value(id, 0);}

        /**
         * @brief Get the page index of a specific schematic
         *
         * @return the schematic index (-1 if the schematic does not exist)
         */
        int getSchematicIndex(Schematic* schematic) const noexcept;

        /**
         * @brief Get the count of schematic pages
         *
         * @return Count of schematics
         */
        int getSchematicCount() const noexcept {return mSchematics.count();}

        /**
         * @brief Get the schematic page at a specific index
         *
         * @param index     The page index (zero is the first)
         *
         * @return A pointer to the specified schematic, or NULL if index is invalid
         */
        Schematic* getSchematicByIndex(int index) const noexcept {return mSchematics.value(index);}

        /**
         * @brief Get the schematic page with a specific UUID
         *
         * @param uuid      The schematic UUID
         *
         * @return A pointer to the specified schematic, or NULL if uuid is invalid
         */
        Schematic* getSchematicByUuid(const QUuid& uuid) const noexcept;

        /**
         * @brief Get the schematic page with a specific name
         *
         * @param name      The schematic name
         *
         * @return A pointer to the specified schematic, or NULL if name is invalid
         */
        Schematic* getSchematicByName(const QString& name) const noexcept;


        // General Methods

        /**
         * @brief Create a new schematic (page)
         *
         * @param name  The schematic page name
         *
         * @return A pointer to the new schematic
         *
         * @throw Exception This method throws an exception on error.
         */
        Schematic* createSchematic(const QString& name) throw (Exception);

        /**
         * @brief Add an existing schematic to this project
         *
         * @param schematic     The schematic to add
         * @param newIndex      The desired index in the list (after inserting it)
         * @param toList        If true, the schematic will be added to the schematics
         *                      list in "schematics/schematics.ini"
         *
         * @throw Exception     On error
         */
        void addSchematic(Schematic* schematic, int newIndex = -1, bool toList = true) throw (Exception);

        /**
         * @brief Remove a schematic from this project
         *
         * @param schematic         The schematic to remove
         * @param fromList          If true, the schematic will be removed from the
         *                          schematics list in "schematics/schematics.ini"
         * @param deleteSchematic   If true, the schematic object will be deleted
         *
         * @throw Exception     On error
         */
        void removeSchematic(Schematic* schematic, bool fromList = true,
                             bool deleteSchematic = false) throw (Exception);

        /**
         * @brief Export the schematic pages as a PDF
         *
         * @param filepath  The filepath where the PDF should be saved. If the file exists
         *                  already, it will be overwritten.
         *
         * @throw Exception     On error
         *
         * @todo add more parameters (paper size, orientation, pages to print, ...)
         */
        void exportSchematicsAsPdf(const FilePath& filepath) throw (Exception);

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


    signals:

        void schematicAdded(int newIndex);
        void schematicRemoved(int oldIndex);

    private:

        // make some methods inaccessible...
        Project();
        Project(const Project& other);
        Project& operator=(const Project& rhs);

        // Private Methods
        void updateSchematicsList() throw (Exception);
        bool save(bool toOriginal, QStringList& errors) noexcept;
        void printSchematicPages(QPrinter& printer, QList<unsigned int>& pages) throw (Exception);


        // Project File (*.e4u)
        FilePath mPath; ///< the path to the project directory
        FilePath mFilepath; ///< the filepath of the *.e4u project file
        XmlFile* mXmlFile; ///< the *.e4u project file
        FileLock mFileLock; ///< See @ref doc_project_lock
        bool mIsRestored; ///< the constructor will set this to true if the project was restored
        bool mIsReadOnly; ///< the constructor will set this to true if the project was opened in read only mode

        // Other Files
        IniFile* mSchematicsIniFile; ///< schematics/schematics.ini

        // General
        QTimer mAutoSaveTimer; ///< the timer for the periodically automatic saving functionality (see also @ref doc_project_save)
        UndoStack* mUndoStack; ///< See @ref doc_project_undostack
        ProjectLibrary* mProjectLibrary;
        Circuit* mCircuit;
        QList<Schematic*> mSchematics;
        SchematicEditor* mSchematicEditor;
        QHash<unsigned int, SchematicLayer*> mSchematicLayers;
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
        the "Command Design Pattern". Our common classes #UndoCommand and #UndoStack
        implement this design pattern. Look at their documentation for more details.

        @note The documentation "Overview of Qt's Undo Framework" (http://qt-project.org/doc/qt-5/qundo.html)
        and/or other documentations about the Command Design Pattern may be useful to
        understand the whole undo/redo system. Our own undo classes are quite similar to
        Qt's classes QUndoCommand and QUndoStack.

        There is a #UndoStack object for the whole project (project#Project#mUndoStack).
        All important changes to the project which should appear in the undo stack
        needs to be implemented as a subclass of #UndoCommand and must be appended to
        project#Project#mUndoStack. Changes which do not need an undo action (like
        changing the color of a layer) can be done without the use of the undo/redo
        system. These changes then cannot be undone. Basically, all changes to the
        circuit/schematics/boards and so on must have an undo action! If you are unsure
        if you should use the project's undo/redo system, try to answer the question
        "do the user wants to undo/redo this action with the undo/redo buttons in the
        schematic- or board-editor?".

        It's possible to use seperate #UndoStack objects for independent parts of the
        project. For example, changing the project settings could be done by using an
        undo/redo system to provide an undo command. But these undo stacks need their
        own undo/redo buttons. The undo/redo buttons in the schematic editor and in the
        board editor are only connected to the stack project#Project#mUndoStack.

        #UndoCommand objects can also have an unlimited amount of child objects.
        This is very useful to group actions together. For example, if multiple symbols
        are selected in the schematic and the user wants to remove them all, multiple
        #UndoCommand objects will be created (one for each symbol). But if the user
        now wants to undo the deletion, the undo command (Ctrl+Z) will only bring back
        the last symbol, so the user needs to press the undo command multiple times. This
        is not good. So we use the parent/child mechanism of #UndoCommand. This means,
        we will still create a #UndoCommand for each removed symbol. But we will also
        create a parent #UndoCommand for all these child commands. Only the parent command
        will be added to the undo stack (for example project#Project#mUndoStack).
        Now, if the user wants to undo the deletion, he only needs to press undo (Ctrl+Z)
        once. And with pressing redo (Ctrl+Y) once, all symbols will be restored.

        @see #UndoCommand, #UndoStack, project#Project#mUndoStack
        @see http://qt-project.org/doc/qt-5/qundo.html (Overview of Qt's Undo Framework)


    @section doc_project_compatibility Compatibility between different application versions

        @todo

        We need a system to ensure that a project with all its files can be loaded and
        saved with different application versions without corrupting the project.

        For this purpose, the *.e4u XML file of each project has two attributes in the
        root XML element: "project_version" and "compatible_downto". Both are unsigned
        integer values, with a minimum value of "1". Zero is invalid.

        "project_version" describes the version of all project files. For example, the
        version "1" is a very old version. Version "2" supports some more features.
        Version "3" has a new file format, which is not compatible to the older versions.
        If a project is saved, the value from Project#sProjectFileVersion is written
        to the "project_version" attribute of the project file.

        But if you want to open a very new project with an old application version, the
        "project_version" attribute of the project can be higher than the value in
        Project#sProjectFileVersion. This means that the application cannot know if
        it can open the project or not (if the version is backward-compatible). To avoid
        this problem, there is an additional attribute in the project's XML files:
        "compatible_downto". With this value, the XML file can "tell" the application,
        if it can open the project or not. If "compatible_downto" is less or equal to
        the application's Project#sProjectFileVersion, the application can open the
        project, even if the application is older than the new project file format.
        If a project is saved, the value from Project#sProjectFileCompatibleDowntoVersion
        is written to the "compatible_downto" attribute of the project file.

        @see project#Project#mProjectFileVersion,
             project#Project#mProjectFileCompatibleDowntoVersion,
             project#Project#sProjectFileVersion,
             project#Project#sProjectFileCompatibleDowntoVersion


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
