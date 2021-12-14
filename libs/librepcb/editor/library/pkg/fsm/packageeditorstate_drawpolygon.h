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

#ifndef LIBREPCB_LIBRARYEDITOR_PACKAGEEDITORSTATE_DRAWPOLYGON_H
#define LIBREPCB_LIBRARYEDITOR_PACKAGEEDITORSTATE_DRAWPOLYGON_H

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
namespace library {
namespace editor {

/*******************************************************************************
 *  Class PackageEditorState_DrawPolygon
 ******************************************************************************/

/**
 * @brief The PackageEditorState_DrawPolygon class
 */
class PackageEditorState_DrawPolygon final
  : public PackageEditorState_DrawPolygonBase {
  Q_OBJECT

public:
  // Constructors / Destructor
  PackageEditorState_DrawPolygon() = delete;
  PackageEditorState_DrawPolygon(const PackageEditorState_DrawPolygon& other) =
      delete;
  explicit PackageEditorState_DrawPolygon(Context& context) noexcept;
  ~PackageEditorState_DrawPolygon() noexcept;

  // Operator Overloadings
  PackageEditorState_DrawPolygon& operator=(
      const PackageEditorState_DrawPolygon& rhs) = delete;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif
