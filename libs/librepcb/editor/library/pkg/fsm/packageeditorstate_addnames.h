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

#ifndef LIBREPCB_EDITOR_PACKAGEEDITORSTATE_ADDNAMES_H
#define LIBREPCB_EDITOR_PACKAGEEDITORSTATE_ADDNAMES_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "packageeditorstate_drawtextbase.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class PackageEditorState_AddNames
 ******************************************************************************/

/**
 * @brief The PackageEditorState_AddNames class
 */
class PackageEditorState_AddNames final
  : public PackageEditorState_DrawTextBase {
  Q_OBJECT

public:
  // Constructors / Destructor
  PackageEditorState_AddNames() = delete;
  PackageEditorState_AddNames(const PackageEditorState_AddNames& other) =
      delete;
  explicit PackageEditorState_AddNames(Context& context) noexcept;
  ~PackageEditorState_AddNames() noexcept;

  // Operator Overloadings
  PackageEditorState_AddNames& operator=(
      const PackageEditorState_AddNames& rhs) = delete;

protected:
  void notifyToolEnter() noexcept override;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
