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

#ifndef LIBREPCB_LIBRARY_FOOTPRINTPADSMT_H
#define LIBREPCB_LIBRARY_FOOTPRINTPADSMT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include "footprintpad.h"
#include <librepcb/common/geometry/polygon.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Class FootprintPadSmt
 ****************************************************************************************/

/**
 * @brief The FootprintPadSmt class
 */
class FootprintPadSmt final : public FootprintPad
{
        Q_DECLARE_TR_FUNCTIONS(FootprintPadSmt)

    public:

        // Types
        enum class BoardSide_t { TOP, BOTTOM };

        // Constructors / Destructor
        explicit FootprintPadSmt(const Uuid& padUuid, const Point& pos, const Angle& rot,
                                 const Length& width, const Length& height, BoardSide_t side) noexcept;
        explicit FootprintPadSmt(const DomElement& domElement) throw (Exception);
        ~FootprintPadSmt() noexcept;

        // Getters
        BoardSide_t getBoardSide() const noexcept {return mBoardSide;}
        int getLayerId() const noexcept override;
        bool isOnLayer(int id) const noexcept override;
        const QPainterPath& toQPainterPathPx() const noexcept override;
        QPainterPath toMaskQPainterPathPx(const Length& clearance) const noexcept override;

        // Setters
        void setBoardSide(BoardSide_t side) noexcept;

        // General Methods

        /// @copydoc librepcb::SerializableObject::serialize()
        void serialize(DomElement& root) const throw (Exception) override;

        // Static Methods
        static BoardSide_t stringToBoardSide(const QString& side) throw (Exception);
        static QString boardSideToString(BoardSide_t side) noexcept;


    private:

        // make some methods inaccessible...
        FootprintPadSmt() = delete;
        FootprintPadSmt(const FootprintPadSmt& other) = delete;
        FootprintPadSmt& operator=(const FootprintPadSmt& rhs) = delete;


        // Pin Attributes
        BoardSide_t mBoardSide;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb

#endif // LIBREPCB_LIBRARY_FOOTPRINTPADSMT_H
