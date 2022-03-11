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

#ifndef LIBREPCB_EDITOR_SYMBOLEDITORSTATE_DRAWARC_H
#define LIBREPCB_EDITOR_SYMBOLEDITORSTATE_DRAWARC_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "symboleditorstate_drawpolygonbase.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class SymbolEditorState_DrawArc
 ******************************************************************************/

/**
 * @brief The SymbolEditorState_DrawArc class
 */
class SymbolEditorState_DrawArc final
  : public SymbolEditorState_DrawPolygonBase {
  Q_OBJECT

public:
  // Constructors / Destructor
  SymbolEditorState_DrawArc() = delete;
  SymbolEditorState_DrawArc(const SymbolEditorState_DrawArc& other) = delete;
  explicit SymbolEditorState_DrawArc(const Context& context) noexcept;
  ~SymbolEditorState_DrawArc() noexcept;

  // Operator Overloadings
  SymbolEditorState_DrawArc& operator=(const SymbolEditorState_DrawArc& rhs) =
      delete;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
