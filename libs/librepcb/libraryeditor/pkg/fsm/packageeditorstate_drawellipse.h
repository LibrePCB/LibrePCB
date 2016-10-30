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

#ifndef LIBREPCB_LIBRARY_EDITOR_PACKAGEEDITORSTATE_DRAWELLIPSE_H
#define LIBREPCB_LIBRARY_EDITOR_PACKAGEEDITORSTATE_DRAWELLIPSE_H

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

class Ellipse;
class EllipseGraphicsItem;
class CmdEllipseEdit;

namespace library {
namespace editor {

/*****************************************************************************************
 *  Class PackageEditorState_DrawEllipse
 ****************************************************************************************/

/**
 * @brief The PackageEditorState_DrawEllipse class
 *
 * @author  ubruhin
 * @date    2017-05-29
 */
class PackageEditorState_DrawEllipse final : public PackageEditorState
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        PackageEditorState_DrawEllipse() = delete;
        PackageEditorState_DrawEllipse(const PackageEditorState_DrawEllipse& other) = delete;
        explicit PackageEditorState_DrawEllipse(Context& context) noexcept;
        ~PackageEditorState_DrawEllipse() noexcept;

        // General Methods
        bool entry() noexcept override;
        bool exit() noexcept override;

        // Event Handlers
        bool processGraphicsSceneMouseMoved(QGraphicsSceneMouseEvent& e) noexcept override;
        bool processGraphicsSceneLeftMouseButtonPressed(QGraphicsSceneMouseEvent& e) noexcept override;
        bool processAbortCommand() noexcept override;

        // Operator Overloadings
        PackageEditorState_DrawEllipse& operator=(const PackageEditorState_DrawEllipse& rhs) = delete;


    private: // Methods
        bool startAddEllipse(const Point& pos) noexcept;
        bool updateEllipseSize(const Point& pos) noexcept;
        bool finishAddEllipse(const Point& pos) noexcept;
        bool abortAddEllipse() noexcept;

        void layerComboBoxValueChanged(const QString& layerName) noexcept;
        void lineWidthSpinBoxValueChanged(double value) noexcept;
        void fillCheckBoxCheckedChanged(bool checked) noexcept;
        void grabAreaCheckBoxCheckedChanged(bool checked) noexcept;


    private: // Types / Data
        Point mStartPos;
        QScopedPointer<CmdEllipseEdit> mEditCmd;
        Ellipse* mCurrentEllipse;
        EllipseGraphicsItem* mCurrentGraphicsItem;

        // parameter memory
        QString mLastLayerName;
        Length mLastLineWidth;
        bool mLastFill;
        bool mLastGrabArea;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace library
} // namespace librepcb

#endif // LIBREPCB_LIBRARY_EDITOR_PACKAGEEDITORSTATE_DRAWELLIPSE_H
