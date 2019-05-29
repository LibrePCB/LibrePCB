/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/attributes/attribute.h>
#include <librepcb/common/attributes/attributeprovider.h>
#include <librepcb/common/elementname.h>
#include <librepcb/common/exceptions.h>
#include <librepcb/common/fileio/directorylock.h>
#include <librepcb/common/fileio/transactionaldirectory.h>
#include <librepcb/common/uuid.h>
#include <librepcb/common/version.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
class QPrinter;

namespace librepcb {

class StrokeFontPool;

namespace project {

class ProjectMetadata;
class ProjectSettings;
class ProjectLibrary;
class Circuit;
class Schematic;
class SchematicLayerProvider;
class ErcMsgList;
class Board;

/*******************************************************************************
 *  Class Project
 ******************************************************************************/

/**
 * @brief The Project class represents a whole (opened) project with all its
 * content
 *
 * This class represents a whole project with all the content of its directory:
 *  - circuit, schematics and boards
 *  - the project's library
 *  - project settings
 *  - and much more...
 *
 * The constructor of the #Project class needs the filepath to a project file.
 * Then the project will be opened. A new project can be created with the static
 * method #create(). The destructor will close the project (without saving). Use
 * the method #save() to write the whole project to the harddisc.
 *
 * @note !! A detailed description about projects is available here: @ref
 * doc_project !!
 */
class Project final : public QObject, public AttributeProvider {
  Q_OBJECT

public:
  // Constructors / Destructor
  Project()                     = delete;
  Project(const Project& other) = delete;

  /**
   * @brief The constructor to open an existing project with all its content
   *
   * @param filepath      The filepath to the an existing *.lpp project file
   * @param readOnly      It true, the project will be opened in read-only mode
   * @param interactive   If true, message boxes may be shown.
   *
   * @throw Exception     If the project could not be opened successfully
   */
  Project(std::unique_ptr<TransactionalDirectory> directory,
          const QString&                          filename)
    : Project(std::move(directory), filename, false) {}

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
  FilePath getFilepath() const noexcept {
    return mDirectory->getAbsPath(mFilename);
  }

  /**
   * @brief Get the path to the project directory
   *
   * @return The filepath to the project directory
   */
  FilePath getPath() const noexcept { return mDirectory->getAbsPath(); }

  const TransactionalDirectory& getDirectory() const noexcept {
    return *mDirectory;
  }

  TransactionalDirectory& getDirectory() noexcept { return *mDirectory; }

  /**
   * @brief Get the StrokeFontPool which contains all stroke fonts of the
   * project
   *
   * @return A reference to the librepcb::StrokeFontPool object
   */
  StrokeFontPool& getStrokeFonts() const noexcept { return *mStrokeFontPool; }

  /**
   * @brief Get the ProjectMetadata object which contains all project metadata
   *
   * @return A reference to the ProjectMetadata object
   */
  ProjectMetadata& getMetadata() const noexcept { return *mProjectMetadata; }

  /**
   * @brief Get the ProjectSettings object which contains all project settings
   *
   * @return A reference to the ProjectSettings object
   */
  ProjectSettings& getSettings() const noexcept { return *mProjectSettings; }

  /**
   * @brief Get the ProjectLibrary object which contains all library elements
   * used in this project
   *
   * @return A reference to the ProjectLibrary object
   */
  ProjectLibrary& getLibrary() const noexcept { return *mProjectLibrary; }

  /**
   * @brief Get the ERC messages list
   *
   * @return A reference to the ErcMsgList object
   */
  ErcMsgList& getErcMsgList() const noexcept { return *mErcMsgList; }

  /**
   * @brief Get the Circuit object
   *
   * @return A reference to the Circuit object
   */
  Circuit& getCircuit() const noexcept { return *mCircuit; }

  // Schematic Methods

  SchematicLayerProvider& getLayers() noexcept {
    return *mSchematicLayerProvider;
  }
  const SchematicLayerProvider& getLayers() const noexcept {
    return *mSchematicLayerProvider;
  }

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
  const QList<Schematic*>& getSchematics() const noexcept {
    return mSchematics;
  }

  /**
   * @brief Get the schematic page at a specific index
   *
   * @param index     The page index (zero is the first)
   *
   * @return A pointer to the specified schematic, or nullptr if index is
   * invalid
   */
  Schematic* getSchematicByIndex(int index) const noexcept {
    return mSchematics.value(index, nullptr);
  }

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
  Schematic* createSchematic(const ElementName& name);

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
  void addSchematic(Schematic& schematic, int newIndex = -1);

  /**
   * @brief Remove a schematic from this project
   *
   * @param schematic         The schematic to remove
   * @param deleteSchematic   If true, the schematic object will be deleted
   *                          (Set this to true only when called from ctor or
   * dtor!!)
   *
   * @throw Exception     On error
   *
   * @undocmd{project#CmdSchematicRemove}
   */
  void removeSchematic(Schematic& schematic, bool deleteSchematic = false);

  /**
   * @brief Export the schematic pages as a PDF
   *
   * @param filepath  The filepath where the PDF should be saved. If the file
   * exists already, it will be overwritten.
   *
   * @throw Exception     On error
   */
  void exportSchematicsAsPdf(const FilePath& filepath);

  /**
   * @brief Print some schematics to a QPrinter (printer or file)
   *
   * @param printer   The QPrinter where to print the schematic pages
   * @param pages     A list with all schematic page indexes which should be
   * printed
   *
   * @throw Exception     On error
   */
  void printSchematicPages(QPrinter& printer, QList<int>& pages);

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
  const QList<Board*>& getBoards() const noexcept { return mBoards; }

  /**
   * @brief Get the board at a specific index
   *
   * @param index     The board index (zero is the first)
   *
   * @return A pointer to the specified board, or nullptr if index is invalid
   */
  Board* getBoardByIndex(int index) const noexcept {
    return mBoards.value(index, nullptr);
  }

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
  Board* createBoard(const ElementName& name);

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
  Board* createBoard(const Board& other, const ElementName& name);

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
  void addBoard(Board& board, int newIndex = -1);

  /**
   * @brief Remove a board from this project
   *
   * @param board             The board to remove
   * @param deleteBoard       If true, the board object will be deleted
   *                          (Set this to true only when called from ctor or
   * dtor!!)
   *
   * @throw Exception     On error
   *
   * @undocmd{project#CmdBoardRemove}
   */
  void removeBoard(Board& board, bool deleteBoard = false);

  // General Methods

  /**
   * @brief Save the project to the transactional file system
   *
   * @throw Exception     If an error occured.
   */
  void save();

  // Inherited from AttributeProvider
  /// @copydoc librepcb::AttributeProvider::getUserDefinedAttributeValue()
  QString getUserDefinedAttributeValue(const QString& key) const
      noexcept override;
  /// @copydoc librepcb::AttributeProvider::getBuiltInAttributeValue()
  QString getBuiltInAttributeValue(const QString& key) const noexcept override;

  // Operator Overloadings
  bool operator==(const Project& rhs) noexcept { return (this == &rhs); }
  bool operator!=(const Project& rhs) noexcept { return (this != &rhs); }

  // Static Methods

  static Project* create(std::unique_ptr<TransactionalDirectory> directory,
                         const QString&                          filename) {
    return new Project(std::move(directory), filename, true);
  }

  static bool    isFilePathInsideProjectDirectory(const FilePath& fp) noexcept;
  static bool    isProjectFile(const FilePath& file) noexcept;
  static bool    isProjectDirectory(const FilePath& dir) noexcept;
  static Version getProjectFileFormatVersion(const FilePath& dir);

signals:

  /// @copydoc AttributeProvider::attributesChanged()
  void attributesChanged() override;

  /**
   * @brief This signal is emitted after a schematic was added to the project
   *
   * @param newIndex  The index of the added schematic
   */
  void schematicAdded(int newIndex);

  /**
   * @brief This signal is emitted after a schematic was removed from the
   * project
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
   * @param create        True if the specified project does not exist already
   * and must be created.
   * @param readOnly      If true, the project will be opened in read-only mode
   * @param interactive   If true, message boxes may be shown.
   *
   * @throw Exception     If the project could not be created/opened
   * successfully
   *
   * @todo Remove interactive message boxes, should be done at a higher layer!
   */
  explicit Project(std::unique_ptr<TransactionalDirectory> directory,
                   const QString& filename, bool create);

  std::unique_ptr<TransactionalDirectory> mDirectory;
  QString mFilename;  ///< the name of the *.lpp project file

  // General
  QScopedPointer<StrokeFontPool>
      mStrokeFontPool;  ///< all fonts from ./resources/fontobene/
  QScopedPointer<ProjectMetadata>
      mProjectMetadata;  ///< e.g. project name, author, ...
  QScopedPointer<ProjectSettings>
      mProjectSettings;  ///< all project specific settings
  QScopedPointer<ProjectLibrary>
      mProjectLibrary;  ///< the library which contains all elements needed in
                        ///< this project
  QScopedPointer<ErcMsgList>
      mErcMsgList;  ///< A list which contains all electrical rule check (ERC)
                    ///< messages
  QScopedPointer<Circuit>
      mCircuit;  ///< The whole circuit of this project (contains all
                 ///< netclasses, netsignals, component instances, ...)
  QList<Schematic*> mSchematics;  ///< All schematics of this project
  QList<Schematic*>
      mRemovedSchematics;  ///< All removed schematics of this project
  QScopedPointer<SchematicLayerProvider>
                mSchematicLayerProvider;  ///< All schematic layers of this project
  QList<Board*> mBoards;                  ///< All boards of this project
  QList<Board*> mRemovedBoards;  ///< All removed boards of this project
  QScopedPointer<AttributeList>
      mAttributes;  ///< all attributes in a specific order
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_PROJECT_H
