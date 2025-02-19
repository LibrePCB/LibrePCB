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

#ifndef LIBREPCB_EDITOR_CMDLIBRARYCATEGORYEDIT_H
#define LIBREPCB_EDITOR_CMDLIBRARYCATEGORYEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "cmdlibrarybaseelementedit.h"

#include <librepcb/core/library/cat/librarycategory.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class CmdLibraryCategoryEdit
 ******************************************************************************/

/**
 * @brief The CmdLibraryCategoryEdit class
 */
class CmdLibraryCategoryEdit : public CmdLibraryBaseElementEdit {
public:
  // Constructors / Destructor
  CmdLibraryCategoryEdit() = delete;
  CmdLibraryCategoryEdit(const CmdLibraryCategoryEdit& other) = delete;
  explicit CmdLibraryCategoryEdit(LibraryCategory& category) noexcept;
  virtual ~CmdLibraryCategoryEdit() noexcept;

  // Setters
  void setParentUuid(const std::optional<Uuid>& parentUuid) noexcept;

  // Operator Overloadings
  CmdLibraryCategoryEdit& operator=(const CmdLibraryCategoryEdit& rhs) = delete;

private:  // Methods
  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  virtual bool performExecute() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performUndo()
  virtual void performUndo() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performRedo()
  virtual void performRedo() override;

private:  // Data
  LibraryCategory& mCategory;

  std::optional<Uuid> mOldParentUuid;
  std::optional<Uuid> mNewParentUuid;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
