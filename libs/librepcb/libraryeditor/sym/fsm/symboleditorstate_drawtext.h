/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

#ifndef LIBREPCB_LIBRARY_EDITOR_SYMBOLEDITORSTATE_DRAWTEXT_H
#define LIBREPCB_LIBRARY_EDITOR_SYMBOLEDITORSTATE_DRAWTEXT_H

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
 *  Class SymbolEditorState_DrawText
 ******************************************************************************/

/**
 * @brief The SymbolEditorState_DrawText class
 *
 * @author  ubruhin
 * @date    2017-01-03
 */
class SymbolEditorState_DrawText final : public SymbolEditorState_DrawTextBase {
  Q_OBJECT

public:
  // Constructors / Destructor
  SymbolEditorState_DrawText()                                        = delete;
  SymbolEditorState_DrawText(const SymbolEditorState_DrawText& other) = delete;
  explicit SymbolEditorState_DrawText(const Context& context) noexcept;
  ~SymbolEditorState_DrawText() noexcept;

  // Operator Overloadings
  SymbolEditorState_DrawText& operator=(const SymbolEditorState_DrawText& rhs) =
      delete;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_EDITOR_SYMBOLEDITORSTATE_DRAWTEXT_H
