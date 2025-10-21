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

#ifndef LIBREPCB_EDITOR_CMDPACKAGERELOAD_H
#define LIBREPCB_EDITOR_CMDPACKAGERELOAD_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "cmdpackageedit.h"

#include <librepcb/core/fileio/transactionalfilesystem.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Package;

namespace editor {

/*******************************************************************************
 *  Class CmdPackageReload
 ******************************************************************************/

/**
 * @brief The CmdPackageReload class
 */
class CmdPackageReload final : public CmdPackageEdit {
public:
  // Constructors / Destructor
  CmdPackageReload() = delete;
  CmdPackageReload(const CmdPackageReload& other) = delete;
  explicit CmdPackageReload(Package& element) noexcept;
  virtual ~CmdPackageReload() noexcept;

  // Operator Overloadings
  CmdPackageReload& operator=(const CmdPackageReload& rhs) = delete;

protected:  // Methods
  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  virtual bool performExecute() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performUndo()
  virtual void performUndo() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performRedo()
  virtual void performRedo() override;

private:  // Data
  Package& mElement;

  TransactionalFileSystem::State mOldFiles;
  TransactionalFileSystem::State mNewFiles;

  PackagePadList mOldPads;
  PackagePadList mNewPads;
  PackageModelList mOldModels;
  PackageModelList mNewModels;
  FootprintList mOldFootprints;
  FootprintList mNewFootprints;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
