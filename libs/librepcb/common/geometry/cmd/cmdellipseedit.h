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

#ifndef LIBREPCB_CMDELLIPSEEDIT_H
#define LIBREPCB_CMDELLIPSEEDIT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "../../undocommand.h"
#include "../../units/all_length_units.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class Ellipse;

/*****************************************************************************************
 *  Class CmdEllipseEdit
 ****************************************************************************************/

/**
 * @brief The CmdEllipseEdit class
 */
class CmdEllipseEdit final : public UndoCommand
{
    public:

        // Constructors / Destructor
        CmdEllipseEdit() = delete;
        CmdEllipseEdit(const CmdEllipseEdit& other) = delete;
        explicit CmdEllipseEdit(Ellipse& ellipse) noexcept;
        ~CmdEllipseEdit() noexcept;

        // Setters
        void setLayerName(const QString& name, bool immediate) noexcept;
        void setLineWidth(const Length& width, bool immediate) noexcept;
        void setIsFilled(bool filled, bool immediate) noexcept;
        void setIsGrabArea(bool grabArea, bool immediate) noexcept;
        void setRadiusX(const Length& rx, bool immediate) noexcept;
        void setRadiusY(const Length& ry, bool immediate) noexcept;
        void setCenter(const Point& pos, bool immediate) noexcept;
        void setDeltaToStartCenter(const Point& deltaPos, bool immediate) noexcept;
        void setRotation(const Angle& angle, bool immediate) noexcept;
        void rotate(const Angle& angle, const Point& center, bool immediate) noexcept;

        // Operator Overloadings
        CmdEllipseEdit& operator=(const CmdEllipseEdit& rhs) = delete;


    private:

        // Private Methods

        /// @copydoc UndoCommand::performExecute()
        bool performExecute() override;

        /// @copydoc UndoCommand::performUndo()
        void performUndo() override;

        /// @copydoc UndoCommand::performRedo()
        void performRedo() override;


        // Private Member Variables

        // Attributes from the constructor
        Ellipse& mEllipse;

        // General Attributes
        QString mOldLayerName;
        QString mNewLayerName;
        Length mOldLineWidth;
        Length mNewLineWidth;
        bool mOldIsFilled;
        bool mNewIsFilled;
        bool mOldIsGrabArea;
        bool mNewIsGrabArea;
        Length mOldRadiusX;
        Length mNewRadiusX;
        Length mOldRadiusY;
        Length mNewRadiusY;
        Point mOldCenter;
        Point mNewCenter;
        Angle mOldRotation;
        Angle mNewRotation;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_CMDELLIPSEEDIT_H
