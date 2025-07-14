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

#ifndef LIBREPCB_EDITOR_PACKAGEEDITORSTATE_DRAWLINE_H
#define LIBREPCB_EDITOR_PACKAGEEDITORSTATE_DRAWLINE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "packageeditorstate_drawpolygonbase.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class PackageEditorState_DrawLine
 ******************************************************************************/

/**
 * @brief The PackageEditorState_DrawLine class
 */
class PackageEditorState_DrawLine final
  : public PackageEditorState_DrawPolygonBase {
  Q_OBJECT

public:
  // Constructors / Destructor
  PackageEditorState_DrawLine() = delete;
  PackageEditorState_DrawLine(const PackageEditorState_DrawLine& other) =
      delete;
  explicit PackageEditorState_DrawLine(Context& context) noexcept;
  ~PackageEditorState_DrawLine() noexcept;

  // Operator Overloadings
  PackageEditorState_DrawLine& operator=(
      const PackageEditorState_DrawLine& rhs) = delete;

protected:
  void notifyToolEnter() noexcept override;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
