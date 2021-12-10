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

#ifndef LIBREPCB_LIBRARYEDITOR_SYMBOLEDITORSTATE_ADDNAMES_H
#define LIBREPCB_LIBRARYEDITOR_SYMBOLEDITORSTATE_ADDNAMES_H

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
 *  Class SymbolEditorState_AddNames
 ******************************************************************************/

/**
 * @brief The SymbolEditorState_AddNames class
 */
class SymbolEditorState_AddNames final : public SymbolEditorState_DrawTextBase {
  Q_OBJECT

public:
  // Constructors / Destructor
  SymbolEditorState_AddNames() = delete;
  SymbolEditorState_AddNames(const SymbolEditorState_AddNames& other) = delete;
  explicit SymbolEditorState_AddNames(const Context& context) noexcept;
  ~SymbolEditorState_AddNames() noexcept;

  // Operator Overloadings
  SymbolEditorState_AddNames& operator=(const SymbolEditorState_AddNames& rhs) =
      delete;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif
