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

#ifndef LIBREPCB_LIBRARY_EDITOR_PACKAGEEDITORSTATE_DRAWTEXTBASE_H
#define LIBREPCB_LIBRARY_EDITOR_PACKAGEEDITORSTATE_DRAWTEXTBASE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include "packageeditorstate.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class Text;
class TextGraphicsItem;
class CmdTextEdit;

namespace library {

namespace editor {

/*****************************************************************************************
 *  Class PackageEditorState_DrawTextBase
 ****************************************************************************************/

/**
 * @brief The PackageEditorState_DrawTextBase class
 *
 * @author  ubruhin
 * @date    2017-05-29
 */
class PackageEditorState_DrawTextBase : public PackageEditorState
{
        Q_OBJECT

    public:

        // Types
        enum class Mode {NAME, VALUE, TEXT};

        // Constructors / Destructor
        PackageEditorState_DrawTextBase() = delete;
        PackageEditorState_DrawTextBase(const PackageEditorState_DrawTextBase& other) = delete;
        explicit PackageEditorState_DrawTextBase(Context& context, Mode mode) noexcept;
        virtual ~PackageEditorState_DrawTextBase() noexcept;

        // General Methods
        bool entry() noexcept override;
        bool exit() noexcept override;

        // Event Handlers
        bool processGraphicsSceneMouseMoved(QGraphicsSceneMouseEvent& e) noexcept override;
        bool processGraphicsSceneLeftMouseButtonPressed(QGraphicsSceneMouseEvent& e) noexcept override;
        bool processRotateCw() noexcept override;
        bool processRotateCcw() noexcept override;

        // Operator Overloadings
        PackageEditorState_DrawTextBase& operator=(const PackageEditorState_DrawTextBase& rhs) = delete;


    private: // Methods
        bool startAddText(const Point& pos) noexcept;
        bool finishAddText(const Point& pos) noexcept;
        bool abortAddText() noexcept;
        void resetToDefaultParameters() noexcept;

        void layerComboBoxValueChanged(const QString& layerName) noexcept;
        void heightSpinBoxValueChanged(double value) noexcept;
        void textComboBoxValueChanged(const QString& value) noexcept;


    private: // Types / Data
        Mode mMode;
        Point mStartPos;
        QScopedPointer<CmdTextEdit> mEditCmd;
        Text* mCurrentText;
        TextGraphicsItem* mCurrentGraphicsItem;

        // parameter memory
        QString mLastLayerName;
        Angle mLastRotation;
        Length mLastHeight;
        QString mLastText;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace library
} // namespace librepcb

#endif // LIBREPCB_LIBRARY_EDITOR_PACKAGEEDITORSTATE_DRAWTEXTBASE_H
