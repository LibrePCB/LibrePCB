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

#ifndef LIBREPCB_CORE_PROJECT_H
#define LIBREPCB_CORE_PROJECT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../attribute/attribute.h"
#include "../attribute/attributeprovider.h"
#include "../fileio/directorylock.h"
#include "../fileio/transactionaldirectory.h"
#include "../types/elementname.h"
#include "../types/uuid.h"
#include "../types/version.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Board;
class Circuit;
class ProjectLibrary;
class ProjectSettings;
class Schematic;
class SchematicLayerProvider;
class StrokeFontPool;

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
 * The constructor of the ::librepcb::Project class needs the filepath
 * to a project file. Then the project will be opened. A new project can be
 * created with the static method #create(). The destructor will close the
 * project (without saving). Use the method #save() to write the whole project
 * to the harddisc.
 *
 * @note !! A detailed description about projects is available here: @ref
 * doc_project !!
 */
class Project final : public QObject, public AttributeProvider {
  Q_OBJECT

public:
  // Constructors / Destructor
  Project() = delete;
  Project(const Project& other) = delete;

  /**
   * @brief Create a new, default initialized project
   *
   * @param directory     The project directory to use.
   * @param filename      The filename of the *.lpp project file.
   *
   * @throw Exception     If the project could not be opened successfully
   */
  Project(std::unique_ptr<TransactionalDirectory> directory,
          const QString& filename);

  /**
   * @brief The destructor will close the whole project (without saving!)
   */
  ~Project() noexcept;

  // Getters

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
   * @brief Get the UUID of the project
   *
   * @return The project UUID
   */
  const Uuid& getUuid() const noexcept { return mUuid; }

  /**
   * @brief Get the name of the project
   *
   * @return The name of the project
   */
  const ElementName& getName() const noexcept { return mName; }

  /**
   * @brief Get the author of the project
   *
   * @return The author of the project
   */
  const QString& getAuthor() const noexcept { return mAuthor; }

  /**
   * @brief Get the version of the project
   *
   * @return The version of the project (arbitrary string)
   */
  const QString& getVersion() const noexcept { return mVersion; }

  /**
   * @brief Get the date and time when the project was created
   *
   * @return The local date and time of creation
   */
  const QDateTime& getCreated() const noexcept { return mCreated; }

  /**
   * @brief Get the date and time when the project was last modified
   *
   * @return The local date and time of last modification
   *
   * @todo    Dynamically determine the datetime of the last modification from
   *          version control system, file attributes or something like that.
   */
  const QDateTime& getLastModified() const noexcept { return mLastModified; }

  /**
   * @brief Get the list of attributes
   *
   * @return All attributes in a specific order
   */
  const AttributeList& getAttributes() const noexcept { return mAttributes; }

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
   * @brief Get the Circuit object
   *
   * @return A reference to the Circuit object
   */
  Circuit& getCircuit() const noexcept { return *mCircuit; }

  /**
   * @brief Get all ERC message approvals
   *
   * @return Approval nodes
   */
  const QSet<SExpression>& getErcMessageApprovals() const noexcept {
    return mErcMessageApprovals;
  }

  // Setters

  /**
   * @brief Set the project's UUID
   *
   * @warning Only call this right after instantiating a new Project object,
   *          not some time later! Not intended to be accessible by the UI.
   *
   * @param newUuid           The new UUID.
   */
  void setUuid(const Uuid& newUuid) noexcept;

  /**
   * @brief Set the name of the project
   *
   * @param newName           The new name
   */
  void setName(const ElementName& newName) noexcept;

  /**
   * @brief Set the author of the project
   *
   * @param newAuthor         The new author
   */
  void setAuthor(const QString& newAuthor) noexcept;

  /**
   * @brief Set the version of the project
   *
   * @param newVersion        The new version (can be an arbitrary string)
   */
  void setVersion(const QString& newVersion) noexcept;

  /**
   * @brief Set the creation date/time
   *
   * @param newCreated        The new date/time of creation.
   */
  void setCreated(const QDateTime& newCreated) noexcept;

  /**
   * @brief Update the last modified date/time
   */
  void updateLastModified() noexcept;

  /**
   * @brief Set all project attributes
   *
   * @param newAttributes     The new list of attributes
   */
  void setAttributes(const AttributeList& newAttributes) noexcept;

  /**
   * @brief Set all ERC message approvals
   *
   * @param approvals   Approval nodes
   *
   * @retval false      If approvals have not been modified (no change)
   * @retval true       If approvals have been moified
   */
  bool setErcMessageApprovals(const QSet<SExpression>& approvals) noexcept;

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
   * @brief Add an existing schematic to this project
   *
   * @param schematic     The schematic to add
   * @param newIndex      The desired index in the list (after inserting it)
   *
   * @throw Exception     On error
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
   */
  void removeSchematic(Schematic& schematic, bool deleteSchematic = false);

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
   * @brief Add an existing board to this project
   *
   * @param board         The board to add
   * @param newIndex      The desired index in the list (after inserting it)
   *
   * @throw Exception     On error
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
   */
  void removeBoard(Board& board, bool deleteBoard = false);

  // General Methods

  /**
   * @brief Save the project to the transactional file system
   *
   * @throw Exception     If an error occurred.
   */
  void save();

  // Inherited from AttributeProvider
  /// @copydoc ::librepcb::AttributeProvider::getUserDefinedAttributeValue()
  QString getUserDefinedAttributeValue(const QString& key) const
      noexcept override;
  /// @copydoc ::librepcb::AttributeProvider::getBuiltInAttributeValue()
  QString getBuiltInAttributeValue(const QString& key) const noexcept override;

  // Operator Overloadings
  bool operator==(const Project& rhs) noexcept { return (this == &rhs); }
  bool operator!=(const Project& rhs) noexcept { return (this != &rhs); }

  // Static Methods

  static std::unique_ptr<Project> create(
      std::unique_ptr<TransactionalDirectory> directory,
      const QString& filename);
  static bool isFilePathInsideProjectDirectory(const FilePath& fp) noexcept;
  static bool isProjectFile(const FilePath& file) noexcept;
  static bool isProjectDirectory(const FilePath& dir) noexcept;
  static Version getProjectFileFormatVersion(const FilePath& dir);

signals:

  /// @copydoc AttributeProvider::attributesChanged()
  void attributesChanged() override;

  /**
   * @brief Called by #setErcMessageApprovals()
   *
   * @param approvals   The new approvals
   */
  void ercMessageApprovalsChanged(const QSet<SExpression>& approvals);

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
  /// Project root directory.
  std::unique_ptr<TransactionalDirectory> mDirectory;

  /// Name of the *.lpp project file.
  QString mFilename;

  /// All fonts from ./resources/fontobene/.
  QScopedPointer<StrokeFontPool> mStrokeFontPool;

  /// The project's UUID.
  Uuid mUuid;

  /// The project name.
  ElementName mName;

  /// Author (optional).
  QString mAuthor;

  /// Version (arbitrary string, optional).
  QString mVersion;

  /// Date/time of project creation.
  QDateTime mCreated;

  /// Date/time of last modification.
  QDateTime mLastModified;

  /// User-defined attributes in the specified order.
  AttributeList mAttributes;

  /// All project specific settings.
  QScopedPointer<ProjectSettings> mProjectSettings;

  /// Ehe library which contains all elements needed in this project.
  QScopedPointer<ProjectLibrary> mProjectLibrary;

  /// The whole circuit of this project (contains all netclasses, netsignals,
  /// component instances, ...)
  QScopedPointer<Circuit> mCircuit;

  /// All schematics of this project
  QList<Schematic*> mSchematics;

  /// All removed schematics of this project
  QList<Schematic*> mRemovedSchematics;

  /// All schematic layers of this project
  QScopedPointer<SchematicLayerProvider> mSchematicLayerProvider;

  /// All boards of this project
  QList<Board*> mBoards;

  /// All removed boards of this project
  QList<Board*> mRemovedBoards;

  /// All approved ERC messages
  QSet<SExpression> mErcMessageApprovals;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
