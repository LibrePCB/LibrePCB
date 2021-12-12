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

#ifndef LIBREPCB_EDITOR_PACKAGEEDITORSTATE_DRAWRECT_H
#define LIBREPCB_EDITOR_PACKAGEEDITORSTATE_DRAWRECT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "packageeditorstate_drawpolygonbase.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class PackageEditorState_DrawRect
 ******************************************************************************/

/**
 * @brief The PackageEditorState_DrawRect class
 */
class PackageEditorState_DrawRect final
  : public PackageEditorState_DrawPolygonBase {
  Q_OBJECT

public:
  // Constructors / Destructor
  PackageEditorState_DrawRect() = delete;
  PackageEditorState_DrawRect(const PackageEditorState_DrawRect& other) =
      delete;
  explicit PackageEditorState_DrawRect(Context& context) noexcept;
  ~PackageEditorState_DrawRect() noexcept;

  // Operator Overloadings
  PackageEditorState_DrawRect& operator=(
      const PackageEditorState_DrawRect& rhs) = delete;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
