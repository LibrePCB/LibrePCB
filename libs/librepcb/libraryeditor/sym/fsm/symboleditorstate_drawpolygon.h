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

#ifndef LIBREPCB_LIBRARY_EDITOR_SYMBOLEDITORSTATE_DRAWPOLYGON_H
#define LIBREPCB_LIBRARY_EDITOR_SYMBOLEDITORSTATE_DRAWPOLYGON_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include "symboleditorstate_drawpolygonbase.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*****************************************************************************************
 *  Class SymbolEditorState_DrawPolygon
 ****************************************************************************************/

/**
 * @brief The SymbolEditorState_DrawPolygon class
 *
 * @author  ubruhin
 * @date    2017-02-23
 */
class SymbolEditorState_DrawPolygon final : public SymbolEditorState_DrawPolygonBase
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        SymbolEditorState_DrawPolygon() = delete;
        SymbolEditorState_DrawPolygon(const SymbolEditorState_DrawPolygon& other) = delete;
        explicit SymbolEditorState_DrawPolygon(const Context& context) noexcept;
        ~SymbolEditorState_DrawPolygon() noexcept;

        // Operator Overloadings
        SymbolEditorState_DrawPolygon& operator=(const SymbolEditorState_DrawPolygon& rhs) = delete;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace library
} // namespace librepcb

#endif // LIBREPCB_LIBRARY_EDITOR_SYMBOLEDITORSTATE_DRAWPOLYGON_H
