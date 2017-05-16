/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
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

#ifndef LIBREPCB_PROJECT_PROJECT_H
#define LIBREPCB_PROJECT_PROJECT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcb/common/fileio/serializableobject.h>
#include <librepcb/common/if_attributeprovider.h>
#include <librepcb/common/if_schematiclayerprovider.h>
#include <librepcb/common/if_boardlayerprovider.h>
#include <librepcb/common/exceptions.h>
#include <librepcb/common/uuid.h>
#include <librepcb/common/version.h>
#include <librepcb/common/fileio/directorylock.h>
#include <librepcb/common/attributes/attribute.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
class QPrinter;

namespace librepcb {

class SmartTextFile;
class SmartXmlFile;
class SmartVersionFile;

namespace project {

class ProjectSettings;
class ProjectLibrary;
class Circuit;
class Schematic;
class SchematicLayerProvider;
class ErcMsgList;
class Board;

/*****************************************************************************************
 *  Class Project
 ****************************************************************************************/

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
                      public IF_SchematicLayerProvider, public SerializableObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        Project() = delete;
        Project(const Project& other) = delete;

        /**
         * @brief The constructor to open an existing project with all its content
         *
         * @param filepath      The filepath to the an existing *.lpp project file
         * @param readOnly      It true, the project will be opened in read-only mode
         *
         * @throw Exception     If the project could not be opened successfully
         */
        Project(const FilePath& filepath, bool readOnly) throw (Exception) :
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
         * @brief Get the author of the project
         *
         * @return The author of the project
         */
        const QString& getAuthor() const noexcept {return mAuthor;}

        /**
         * @brief Get the version of the project
         *
         * @return The version of the project (arbitrary string)
         */
        const QString& getVersion() const noexcept {return mVersion;}

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
         *
         * @todo    Dynamically determine the datetime of the last modification from
         *          version control system, file attributes or something like that.
         */
        const QDateTime& getLastModified() const noexcept {return mLastModified;}

        /**
         * @brief Get the list of attributes
         *
         * @return All attributes in a specific order
         */
        const AttributeList& getAttributes() const noexcept {return *mAttributes;}


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
         * @brief Set the author of the project
         *
         * @param newAuthor         The new author
         *
         * @undocmd{project#CmdProjectSetMetadata}
         */
        void setAuthor(const QString& newAuthor) noexcept;

        /**
         * @brief Set the version of the project
         *
         * @param newVersion        The new version (can be an arbitrary string)
         *
         * @undocmd{project#CmdProjectSetMetadata}
         */
        void setVersion(const QString& newVersion) noexcept;

        /**
         * @brief Set all project attributes
         *
         * @param newAttributes     The new list of attributes
         *
         * @undocmd{librepcb::project::CmdProjectSetMetadata}
         */
        void setAttributes(const AttributeList& newAttributes) noexcept;


        // Schematic Methods

        /**
         * @copydoc IF_SchematicLayerProvider#getSchematicLayer()
         */
        SchematicLayer* getSchematicLayer(int id) const noexcept override;

        /**
         * @brief Get the page index of a specific schematic
         *
         * @return the schematic index (-1 if the schematic does not exist)
         */
        int getSchematicIndex(const Schematic& schematic) const noexcept;

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
        Schematic* getSchematicByUuid(const Uuid& uuid) const noexcept;

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
        void addSchematic(Schematic& schematic, int newIndex = -1) throw (Exception);

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
        void removeSchematic(Schematic& schematic, bool deleteSchematic = false) throw (Exception);

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
         * @brief Get the index of a specific board
         *
         * @return the board index (-1 if the board does not exist)
         */
        int getBoardIndex(const Board& board) const noexcept;

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
        Board* getBoardByUuid(const Uuid& uuid) const noexcept;

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
         * @brief Create a new board as a copy of an existing board
         *
         * @param other The board to copy
         * @param name  The board name
         *
         * @return A pointer to the new board
         *
         * @throw Exception This method throws an exception on error.
         */
        Board* createBoard(const Board& other, const QString& name) throw (Exception);

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
        void addBoard(Board& board, int newIndex = -1) throw (Exception);

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
        void removeBoard(Board& board, bool deleteBoard = false) throw (Exception);


        // General Methods

        /**
         * @brief Save the whole project to the harddisc
         *
         * @param toOriginal    If false, the project is saved only to temporary files
         *
         * @note The whole save procedere is described in @ref doc_project_save.
         *
         * @throw Exception on error
         */
        void save(bool toOriginal) throw (Exception);


        // Helper Methods

        /**
         * @copydoc IF_AttributeProvider#getAttributeValue()
         */
        bool getAttributeValue(const QString& attrNS, const QString& attrKey,
                               bool passToParents, QString& value) const noexcept override;


        // Operator Overloadings
        Project& operator=(const Project& rhs) = delete;


        // Static Methods

        static Project* create(const FilePath& filepath) throw (Exception)
        {return new Project(filepath, true, false);}

        static bool isValidProjectDirectory(const FilePath& dir) noexcept;
        static Version getProjectFileFormatVersion(const FilePath& dir) throw (Exception);


    signals:

        /// @copydoc IF_AttributeProvider#attributesChanged()
        void attributesChanged() override;

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

        bool checkAttributesValidity() const noexcept;

        /**
         * @copydoc librepcb::SerializableObject::serialize()
         */
        void serialize(XmlDomElement& root) const throw (Exception) override;

        /**
         * @brief Save the project to the harddisc (to temporary or original files)
         *
         * @param toOriginal    True: save to original files; False: save to temporary files
         * @param errors        All errors will be added to this string list (translated)
         *
         * @return True on success (then the error list should be empty), false otherwise
         */
        bool save(bool toOriginal, QStringList& errors) noexcept;

        /**
         * @brief Print some schematics to a QPrinter (printer or file)
         *
         * @param printer   The QPrinter where to print the schematic pages
         * @param pages     A list with all schematic page indexes which should be printed
         *
         * @throw Exception     On error
         */
        void printSchematicPages(QPrinter& printer, QList<int>& pages) throw (Exception);


        // Project File (*.lpp)
        FilePath mPath; ///< the path to the project directory
        FilePath mFilepath; ///< the filepath of the *.lpp project file
        QScopedPointer<SmartVersionFile> mVersionFile; ///< the ".librepcb-project" file
        QScopedPointer<SmartXmlFile> mXmlFile; ///< the *.lpp project file
        DirectoryLock mLock; ///< Lock for the whole project directory (see @ref doc_project_lock)
        bool mIsRestored; ///< the constructor will set this to true if the project was restored
        bool mIsReadOnly; ///< the constructor will set this to true if the project was opened in read only mode

        // Attributes
        QString mName;              ///< the name of the project
        QString mAuthor;            ///< the author of the project
        QString mVersion;           ///< the version of the project (arbitrary string)
        QDateTime mCreated;         ///< the datetime of the project creation
        QDateTime mLastModified;    ///< the datetime of the last project modification

        // General
        QScopedPointer<ProjectSettings> mProjectSettings; ///< all project specific settings
        QScopedPointer<ProjectLibrary> mProjectLibrary; ///< the library which contains all elements needed in this project
        QScopedPointer<ErcMsgList> mErcMsgList; ///< A list which contains all electrical rule check (ERC) messages
        QScopedPointer<Circuit> mCircuit; ///< The whole circuit of this project (contains all netclasses, netsignals, component instances, ...)
        QList<Schematic*> mSchematics; ///< All schematics of this project
        QList<Schematic*> mRemovedSchematics; ///< All removed schematics of this project
        QScopedPointer<SchematicLayerProvider> mSchematicLayerProvider; ///< All schematic layers of this project
        QList<Board*> mBoards; ///< All boards of this project
        QList<Board*> mRemovedBoards; ///< All removed boards of this project
        QScopedPointer<AttributeList> mAttributes; ///< all attributes in a specific order
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_PROJECT_H
