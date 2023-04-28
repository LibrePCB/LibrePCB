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

#ifndef LIBREPCB_EDITOR_CMDPACKAGEMODELREMOVE_H
#define LIBREPCB_EDITOR_CMDPACKAGEMODELREMOVE_H

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
 *  Class CmdPackageModelRemove
 ******************************************************************************/

/**
 * @brief The CmdPackageModelRemove class
 */
class CmdPackageModelRemove final : public UndoCommand {
public:
  // Constructors / Destructor
  CmdPackageModelRemove() = delete;
  CmdPackageModelRemove(const CmdPackageModelRemove& other) = delete;
  CmdPackageModelRemove(Package& pkg,
                        std::shared_ptr<PackageModel> model) noexcept;
  ~CmdPackageModelRemove() noexcept;

  // Operator Overloadings
  CmdPackageModelRemove& operator=(const CmdPackageModelRemove& rhs) = delete;

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
  QVector<std::shared_ptr<Footprint>> mRemovedFromFootprints;
  int mIndex;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
