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

#ifndef LIBREPCB_CMDTEXTEDIT_H
#define LIBREPCB_CMDTEXTEDIT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "../../undocommand.h"
#include "../../alignment.h"
#include "../../units/all_length_units.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class Text;

/*****************************************************************************************
 *  Class CmdTextEdit
 ****************************************************************************************/

/**
 * @brief The CmdTextEdit class
 */
class CmdTextEdit final : public UndoCommand
{
    public:

        // Constructors / Destructor
        CmdTextEdit() = delete;
        CmdTextEdit(const CmdTextEdit& other) = delete;
        explicit CmdTextEdit(Text& text) noexcept;
        ~CmdTextEdit() noexcept;

        // Setters
        void setLayerName(const QString& name, bool immediate) noexcept;
        void setText(const QString& text, bool immediate) noexcept;
        void setHeight(const Length& height, bool immediate) noexcept;
        void setAlignment(const Alignment& align, bool immediate) noexcept;
        void setPosition(const Point& pos, bool immediate) noexcept;
        void setDeltaToStartPos(const Point& deltaPos, bool immediate) noexcept;
        void setRotation(const Angle& angle, bool immediate) noexcept;
        void rotate(const Angle& angle, const Point& center, bool immediate) noexcept;

        // Operator Overloadings
        CmdTextEdit& operator=(const CmdTextEdit& rhs) = delete;


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
        Text& mText;

        // General Attributes
        QString mOldLayerName;
        QString mNewLayerName;
        QString mOldText;
        QString mNewText;
        Point mOldPosition;
        Point mNewPosition;
        Angle mOldRotation;
        Angle mNewRotation;
        Length mOldHeight;
        Length mNewHeight;
        Alignment mOldAlign;
        Alignment mNewAlign;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_CMDTEXTEDIT_H
