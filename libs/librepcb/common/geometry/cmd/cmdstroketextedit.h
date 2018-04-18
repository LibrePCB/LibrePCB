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

#ifndef LIBREPCB_CMDSTROKETEXTEDIT_H
#define LIBREPCB_CMDSTROKETEXTEDIT_H


/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "../../undocommand.h"
#include "../stroketext.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class StrokeText;

/*****************************************************************************************
 *  Class CmdStrokeTextEdit
 ****************************************************************************************/

/**
 * @brief The CmdStrokeTextEdit class
 */
class CmdStrokeTextEdit final : public UndoCommand
{
    public:

        // Constructors / Destructor
        CmdStrokeTextEdit() = delete;
        CmdStrokeTextEdit(const CmdStrokeTextEdit& other) = delete;
        explicit CmdStrokeTextEdit(StrokeText& text) noexcept;
        ~CmdStrokeTextEdit() noexcept;

        // Setters
        void setLayerName(const QString& name, bool immediate) noexcept;
        void setText(const QString& text, bool immediate) noexcept;
        void setHeight(const Length& height, bool immediate) noexcept;
        void setStrokeWidth(const Length& strokeWidth, bool immediate) noexcept;
        void setLetterSpacing(const StrokeTextSpacing& spacing, bool immediate) noexcept;
        void setLineSpacing(const StrokeTextSpacing& spacing, bool immediate) noexcept;
        void setAlignment(const Alignment& align, bool immediate) noexcept;
        void setPosition(const Point& pos, bool immediate) noexcept;
        void setDeltaToStartPos(const Point& deltaPos, bool immediate) noexcept;
        void translate(const Point& delta, bool immediate) noexcept;
        void setRotation(const Angle& angle, bool immediate) noexcept;
        void rotate(const Angle& angle, const Point& center, bool immediate) noexcept;
        void setMirrored(bool mirrored, bool immediate) noexcept;
        void mirror(const Point& center, Qt::Orientation orientation, bool immediate) noexcept;
        void setAutoRotate(bool autoRotate, bool immediate) noexcept;

        // Operator Overloadings
        CmdStrokeTextEdit& operator=(const CmdStrokeTextEdit& rhs) = delete;


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
        StrokeText& mText;

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
        Length mOldStrokeWidth;
        Length mNewStrokeWidth;
        StrokeTextSpacing mOldLetterSpacing;
        StrokeTextSpacing mNewLetterSpacing;
        StrokeTextSpacing mOldLineSpacing;
        StrokeTextSpacing mNewLineSpacing;
        Alignment mOldAlign;
        Alignment mNewAlign;
        bool mOldMirrored;
        bool mNewMirrored;
        bool mOldAutoRotate;
        bool mNewAutoRotate;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_CMDSTROKETEXTEDIT_H
