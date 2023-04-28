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

#ifndef LIBREPCB_EDITOR_CMDPACKAGEMODELADD_H
#define LIBREPCB_EDITOR_CMDPACKAGEMODELADD_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../undocommand.h"

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Footprint;
class Package;
class PackageModel;

namespace editor {

/*******************************************************************************
 *  Class CmdPackageModelAdd
 ******************************************************************************/

/**
 * @brief The CmdPackageModelAdd class
 */
class CmdPackageModelAdd final : public UndoCommand {
public:
  // Constructors / Destructor
  CmdPackageModelAdd() = delete;
  CmdPackageModelAdd(const CmdPackageModelAdd& other) = delete;
  CmdPackageModelAdd(Package& pkg, std::shared_ptr<PackageModel> model,
                     const QByteArray& fileContent,
                     bool addToFootprints) noexcept;
  ~CmdPackageModelAdd() noexcept;

  // Operator Overloadings
  CmdPackageModelAdd& operator=(const CmdPackageModelAdd& rhs) = delete;

private:  // Methods
  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  bool performExecute() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performUndo()
  void performUndo() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performRedo()
  void performRedo() override;

private:  // Data
  Package& mPackage;
  std::shared_ptr<PackageModel> mModel;
  QByteArray mFileContent;
  bool mAddToFootprints;
  QVector<std::shared_ptr<Footprint>> mAddedToFootprints;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
