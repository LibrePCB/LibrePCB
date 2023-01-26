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

#ifndef LIBREPCB_CORE_PROJECTLOADER_H
#define LIBREPCB_CORE_PROJECTLOADER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../serialization/fileformatmigration.h"

#include <optional/tl/optional.hpp>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Board;
class Project;
class ProjectLibrary;
class SExpression;
class Schematic;
class TransactionalDirectory;

/*******************************************************************************
 *  Class ProjectLoader
 ******************************************************************************/

/**
 * @brief Helper to load a ::librepcb::Project from the file system
 */
class ProjectLoader final : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit ProjectLoader(QObject* parent = nullptr) noexcept;
  ProjectLoader(const ProjectLoader& other) = delete;
  ~ProjectLoader() noexcept;

  // General Methods
  std::unique_ptr<Project> open(
      std::unique_ptr<TransactionalDirectory> directory,
      const QString& filename);
  const tl::optional<QList<FileFormatMigration::Message>>& getUpgradeMessages()
      const noexcept {
    return mUpgradeMessages;
  }

  // Operator Overloadings
  ProjectLoader& operator=(const ProjectLoader& rhs) = delete;

private:  // Methods
  void loadMetadata(Project& p);
  void loadSettings(Project& p);
  void loadLibrary(Project& p);
  template <typename ElementType>
  void loadLibraryElements(Project& p, const QString& dirname,
                           const QString& type,
                           void (ProjectLibrary::*addFunction)(ElementType&));
  void loadCircuit(Project& p);
  void loadSchematics(Project& p);
  void loadSchematic(Project& p, const QString& relativeFilePath);
  void loadSchematicSymbol(Schematic& s, const SExpression& node);
  void loadSchematicNetSegment(Schematic& s, const SExpression& node);
  void loadBoards(Project& p);
  void loadBoard(Project& p, const QString& relativeFilePath);
  void loadBoardDeviceInstance(Board& b, const SExpression& node);
  void loadBoardNetSegment(Board& b, const SExpression& node);
  void loadBoardPlane(Board& b, const SExpression& node);
  void loadBoardUserSettings(Board& b);
  void restoreApprovedErcMessages(Project& p);

private:  // Data
  tl::optional<QList<FileFormatMigration::Message>> mUpgradeMessages;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
