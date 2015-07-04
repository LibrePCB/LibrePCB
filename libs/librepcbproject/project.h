/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://librepcb.org/
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
#include <librepcbcommon/fileio/if_xmlserializableobject.h>
#include <librepcbcommon/if_attributeprovider.h>
#include <librepcbcommon/if_schematiclayerprovider.h>
#include <librepcbcommon/if_boardlayerprovider.h>
#include <librepcbcommon/exceptions.h>
#include <librepcbcommon/fileio/filelock.h>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class QPrinter;
class SmartTextFile;
class SmartXmlFile;
class SmartIniFile;

namespace project {
class ProjectSettings;
class ProjectLibrary;
class Circuit;
class Schematic;
class SchematicLayerProvider;
class ErcMsgList;
class Board;
class BoardLayerProvider;
}

/*****************************************************************************************
 *  Class Project
 ****************************************************************************************/

namespace project {

/**
 * @brief The Project class represents a whole (opened) project with all its content
 *
 * This class represents a whole project with all the content of its directory:
 *  - circuit, schematics and boards
 *  - the project's library
 *  - project settings
 *  - and much more...
 *
 * The constructor of the #Project class needs the filepath to a project file. Then the
 * project will be opened. A new project can be created with the static method #create().
 * The destructor will close the project (without saving). Use the method #save() to write
 * the whole project to the harddisc.
 *
 * @note !! A detailed description about projects is available here: @ref doc_project !!
 *
 * @author ubruhin
 * @date 2014-06-24
 */
class Project final : public QObject, public IF_AttributeProvider,
                      public IF_SchematicLayerProvider, public IF_BoardLayerProvider,
                      public IF_XmlSerializableObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor

        /**
         * @brief The constructor to open an existing project with all its content
         *
         * @param filepath      The filepath to the an existing *.lpp project file
         * @param readOnly      It true, the project will be opened in read-only mode
         *
         * @throw Exception     If the project could not be opened successfully
         */
        explicit Project(const FilePath& filepath, bool readOnly) throw (Exception) :
            Project(filepath, false, readOnly) {}

        /**
         * @brief The destructor will close the whole project (without saving!)
         */
        ~Project() noexcept;


        // Getters: General

        /**
         * @brief Get the filepath of the project file (*.lpp)
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
         * @brief Check whether this project was opened in read-only mode or not
         *
         * @return See #mIsReadOnly
         */
        bool isReadOnly() const noexcept {return mIsReadOnly;}

        /**
         * @brief Check whether this project restored from temporary files or not
         *
         * @return See #mIsRestored
         */
        bool isRestored() const noexcept {return mIsRestored;}

        /**
         * @brief Get the ProjectSettings object which contains all project settings
         *
         * @return A reference to the ProjectSettings object
         */
        ProjectSettings& getSettings() const noexcept {return *mProjectSettings;}

        /**
         * @brief Get the ProjectLibrary object which contains all library elements used
         *        in this project
         *
         * @return A reference to the ProjectLibrary object
         */
        ProjectLibrary& getLibrary() const noexcept {return *mProjectLibrary;}

        /**
         * @brief Get the ERC messages list
         *
         * @return A reference to the ErcMsgList object
         */
        ErcMsgList& getErcMsgList() const noexcept {return *mErcMsgList;}

        /**
         * @brief Get the Circuit object
         *
         * @return A reference to the Circuit object
         */
        Circuit& getCircuit() const noexcept {return *mCircuit;}


        // Getters: Attributes

        /**
         * @brief Get the name of the project
         *
         * @return The name of the project
         */
        const QString& getName() const noexcept {return mName;}

        /**
         * @brief Get the description (in HTML) of the project
         *
         * @return The description of the project (HTML)
         */
        QString getDescription() const noexcept;

        /**
         * @brief Get the author of the project
         *
         * @return The author of the project
         */
        const QString& getAuthor() const noexcept {return mAuthor;}

        /**
         * @brief Get the date and time when the project was created
         *
         * @return The local date and time of creation
         */
        const QDateTime& getCreated() const noexcept {return mCreated;}

        /**
         * @brief Get the date and time when the project was last modified
         *
         * @return The local date and time of last modification
         */
        const QDateTime& getLastModified() const noexcept {return mLastModified;}


        // Setters: Attributes

        /**
         * @brief Set the name of the project
         *
         * @param newName           The new name (should not be empty!)
         *
         * @undocmd{project#CmdProjectSetMetadata}
         */
        void setName(const QString& newName) noexcept;

        /**
         * @brief Set the description (in HTML) of the project
         *
         * @param newDescription    The new description (HTML)
         *
         * @undocmd{project#CmdProjectSetMetadata}
         */
        void setDescription(const QString& newDescription) noexcept;

        /**
         * @brief Set the author of the project
         *
         * @param newAuthor         The new author
         *
         * @undocmd{project#CmdProjectSetMetadata}
         */
        void setAuthor(const QString& newAuthor) noexcept;

        /**
         * @brief Set the date and time when the project was created
         *
         * @param newCreated        The new created datetime
         *
         * @undocmd{project#CmdProjectSetMetadata}
         */
        void setCreated(const QDateTime& newCreated) noexcept;

        /**
         * @brief Set the date and time when the project was last modified
         *
         * @param newLastModified   The new last modified datetime
         *
         * @note This method is automatically called before saving the project.
         */
        void setLastModified(const QDateTime& newLastModified) noexcept;


        // Schematic Methods

        /**
         * @copydoc IF_SchematicLayerProvider#getSchematicLayer()
         */
        SchematicLayer* getSchematicLayer(uint id) const noexcept;

        /**
         * @brief Get the page index of a specific schematic
         *
         * @return the schematic index (-1 if the schematic does not exist)
         */
        int getSchematicIndex(const Schematic* schematic) const noexcept;

        /**
         * @brief Get all schematics
         *
         * @return A QList with all schematics
         */
        const QList<Schematic*>& getSchematics() const noexcept {return mSchematics;}

        /**
         * @brief Get the schematic page at a specific index
         *
         * @param index     The page index (zero is the first)
         *
         * @return A pointer to the specified schematic, or nullptr if index is invalid
         */
        Schematic* getSchematicByIndex(int index) const noexcept {return mSchematics.value(index, nullptr);}

        /**
         * @brief Get the schematic page with a specific UUID
         *
         * @param uuid      The schematic UUID
         *
         * @return A pointer to the specified schematic, or nullptr if uuid is invalid
         */
        Schematic* getSchematicByUuid(const QUuid& uuid) const noexcept;

        /**
         * @brief Get the schematic page with a specific name
         *
         * @param name      The schematic name
         *
         * @return A pointer to the specified schematic, or nullptr if name is invalid
         */
        Schematic* getSchematicByName(const QString& name) const noexcept;

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
         *
         * @throw Exception     On error
         *
         * @undocmd{project#CmdSchematicAdd}
         */
        void addSchematic(Schematic* schematic, int newIndex = -1) throw (Exception);

        /**
         * @brief Remove a schematic from this project
         *
         * @param schematic         The schematic to remove
         * @param deleteSchematic   If true, the schematic object will be deleted
         *                          (Set this to true only when called from ctor or dtor!!)
         *
         * @throw Exception     On error
         *
         * @undocmd{project#CmdSchematicRemove}
         */
        void removeSchematic(Schematic* schematic, bool deleteSchematic = false) throw (Exception);

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


        // Board Methods

        /**
         * @copydoc IF_BoardLayerProvider#getBoardLayer()
         */
        BoardLayer* getBoardLayer(uint id) const noexcept;

        /**
         * @brief Get the index of a specific board
         *
         * @return the board index (-1 if the board does not exist)
         */
        int getBoardIndex(const Board* board) const noexcept;

        /**
         * @brief Get all boards
         *
         * @return A QList with all boards
         */
        const QList<Board*>& getBoards() const noexcept {return mBoards;}

        /**
         * @brief Get the board at a specific index
         *
         * @param index     The board index (zero is the first)
         *
         * @return A pointer to the specified board, or nullptr if index is invalid
         */
        Board* getBoardByIndex(int index) const noexcept {return mBoards.value(index, nullptr);}

        /**
         * @brief Get the board with a specific UUID
         *
         * @param uuid      The board UUID
         *
         * @return A pointer to the specified board, or nullptr if uuid is invalid
         */
        Board* getBoardByUuid(const QUuid& uuid) const noexcept;

        /**
         * @brief Get the board with a specific name
         *
         * @param name      The board name
         *
         * @return A pointer to the specified board, or nullptr if name is invalid
         */
        Board* getBoardByName(const QString& name) const noexcept;

        /**
         * @brief Create a new board
         *
         * @param name  The board name
         *
         * @return A pointer to the new board
         *
         * @throw Exception This method throws an exception on error.
         */
        Board* createBoard(const QString& name) throw (Exception);

        /**
         * @brief Add an existing board to this project
         *
         * @param board         The board to add
         * @param newIndex      The desired index in the list (after inserting it)
         *
         * @throw Exception     On error
         *
         * @undocmd{project#CmdBoardAdd}
         */
        void addBoard(Board* board, int newIndex = -1) throw (Exception);

        /**
         * @brief Remove a board from this project
         *
         * @param board             The board to remove
         * @param deleteBoard       If true, the board object will be deleted
         *                          (Set this to true only when called from ctor or dtor!!)
         *
         * @throw Exception     On error
         *
         * @undocmd{project#CmdBoardRemove}
         */
        void removeBoard(Board* board, bool deleteBoard = false) throw (Exception);


        // General Methods

        /**
         * @brief Save the whole project to the harddisc
         *
         * @param version       The version of the files which will be created/overwritten
         * @param toOriginal    If false, the project is saved only to temporary files
         *
         * @note The whole save procedere is described in @ref doc_project_save.
         *
         * @throw Exception on error
         */
        void save(uint version, bool toOriginal) throw (Exception);


        // Helper Methods

        /**
         * @copydoc IF_AttributeProvider#getAttributeValue()
         */
        bool getAttributeValue(const QString& attrNS, const QString& attrKey,
                               bool passToParents, QString& value) const noexcept;


        // Static Methods

        static Project* create(const FilePath& filepath) throw (Exception)
        {return new Project(filepath, true, false);}


    signals:

        /// @copydoc IF_AttributeProvider#attributesChanged()
        void attributesChanged();

        /**
         * @brief This signal is emitted after a schematic was added to the project
         *
         * @param newIndex  The index of the added schematic
         */
        void schematicAdded(int newIndex);

        /**
         * @brief This signal is emitted after a schematic was removed from the project
         *
         * @param oldIndex  The index of the removed schematic
         */
        void schematicRemoved(int oldIndex);

        /**
         * @brief This signal is emitted after a board was added to the project
         *
         * @param newIndex  The index of the added board
         */
        void boardAdded(int newIndex);

        /**
         * @brief This signal is emitted after a board was removed from the project
         *
         * @param oldIndex  The index of the removed board
         */
        void boardRemoved(int oldIndex);


    private:

        // make some methods inaccessible...
        Project();
        Project(const Project& other);
        Project& operator=(const Project& rhs);


        // Private Methods

        /**
         * @brief The constructor to create or open a project with all its content
         *
         * @param filepath      The filepath to the new or existing *.lpp project file
         * @param create        True if the specified project does not exist already and
         *                      must be created.
         * @param readOnly      If true, the project will be opened in read-only mode
         *
         * @throw Exception     If the project could not be created/opened successfully
         */
        explicit Project(const FilePath& filepath, bool create, bool readOnly) throw (Exception);

        /// @copydoc IF_XmlSerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;

        /**
         * @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
         */
        XmlDomElement* serializeToXmlDomElement(uint version) const throw (Exception);

        /**
         * @brief Save the project to the harddisc (to temporary or original files)
         *
         * @param version       The version of the files which will be created/overwritten
         * @param toOriginal    True: save to original files; False: save to temporary files
         * @param errors        All errors will be added to this string list (translated)
         *
         * @return True on success (then the error list should be empty), false otherwise
         */
        bool save(uint version, bool toOriginal, QStringList& errors) noexcept;

        /**
         * @brief Print some schematics to a QPrinter (printer or file)
         *
         * @param printer   The QPrinter where to print the schematic pages
         * @param pages     A list with all schematic page indexes which should be printed
         *
         * @throw Exception     On error
         */
        void printSchematicPages(QPrinter& printer, QList<uint>& pages) throw (Exception);


        // Project File (*.lpp)
        FilePath mPath; ///< the path to the project directory
        FilePath mFilepath; ///< the filepath of the *.lpp project file
        SmartXmlFile* mXmlFile; ///< the *.lpp project file
        FileLock mFileLock; ///< See @ref doc_project_lock
        bool mIsRestored; ///< the constructor will set this to true if the project was restored
        bool mIsReadOnly; ///< the constructor will set this to true if the project was opened in read only mode

        // Other Files
        SmartTextFile* mDescriptionHtmlFile; ///< description/index.html

        // Attributes
        QString mName;              ///< the name of the project
        QString mAuthor;            ///< the author of the project
        QDateTime mCreated;         ///< the datetime of the project creation
        QDateTime mLastModified;    ///< the datetime of the last project modification

        // General
        ProjectSettings* mProjectSettings; ///< all project specific settings
        ProjectLibrary* mProjectLibrary; ///< the library which contains all elements needed in this project
        ErcMsgList* mErcMsgList; ///< A list which contains all electrical rule check (ERC) messages
        Circuit* mCircuit; ///< The whole circuit of this project (contains all netclasses, netsignals, generic component instances, ...)
        QList<Schematic*> mSchematics; ///< All schematics of this project
        QList<Schematic*> mRemovedSchematics; ///< All removed schematics of this project
        SchematicLayerProvider* mSchematicLayerProvider; ///< All schematic layers of this project
        BoardLayerProvider* mBoardLayerProvider; ///< All board layers of this project
        QList<Board*> mBoards; ///< All boards of this project
        QList<Board*> mRemovedBoards; ///< All removed boards of this project
};

} // namespace project

/*****************************************************************************************
 *  Doxygen: Project Documentation
 ****************************************************************************************/

/**
    @page doc_project Project Documentation
    @tableofcontents

    This is the documentation of an LibrePCB project. Such a project is represented by an
    instance of the class project#Project.


    @section doc_project_filestructure The File Structure of a Project

        See @subpage doc_project_file_structure "Project File Structure Documentation".


    @section doc_project_lock Avoid opening a project multiple times (File Lock)

        It's very dangerous if a project is opened multiple times simultaneously. Even if
        the application's design does not allow to open a project multiple times, it's
        still possible to save a project in a shared folder and open it from different
        computers at the same time (for example). This can demage the project files!

        To avoid such problems, the Project's constructor will try to lock the project
        file (*.lpp) with a FileLock object (project#Project#mFileLock). If the project
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

        For this purpose, the *.lpp XML file of each project has two attributes in the
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

        In the project directory, there are only files from LibrePCB, the user mustn't save other files there!
        There are the following directories and files in a LibrePCB project directory:

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
            These directories are 100 percent identical with the elements in the LibrePCB library, these are 1:1 copies of them.

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
