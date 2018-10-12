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

#ifndef LIBREPCB_LIBRARY_EDITOR_SYMBOLEDITORSTATE_ADDVALUES_H
#define LIBREPCB_LIBRARY_EDITOR_SYMBOLEDITORSTATE_ADDVALUES_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "symboleditorstate_drawtextbase.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*******************************************************************************
 *  Class SymbolEditorState_AddValues
 ******************************************************************************/

/**
 * @brief The SymbolEditorState_AddValues class
 *
 * @author  ubruhin
 * @date    2017-01-03
 */
class SymbolEditorState_AddValues final
  : public SymbolEditorState_DrawTextBase {
  Q_OBJECT

public:
  // Constructors / Destructor
  SymbolEditorState_AddValues() = delete;
  SymbolEditorState_AddValues(const SymbolEditorState_AddValues& other) =
      delete;
  explicit SymbolEditorState_AddValues(const Context& context) noexcept;
  ~SymbolEditorState_AddValues() noexcept;

  // Operator Overloadings
  SymbolEditorState_AddValues& operator       =(
      const SymbolEditorState_AddValues& rhs) = delete;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_EDITOR_SYMBOLEDITORSTATE_ADDVALUES_H
