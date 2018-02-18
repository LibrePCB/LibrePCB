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

#ifndef LIBREPCB_LIBRARY_FOOTPRINTPAD_H
#define LIBREPCB_LIBRARY_FOOTPRINTPAD_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include <librepcb/common/fileio/serializableobjectlist.h>
#include <librepcb/common/fileio/cmd/cmdlistelementinsert.h>
#include <librepcb/common/fileio/cmd/cmdlistelementremove.h>
#include <librepcb/common/fileio/cmd/cmdlistelementsswap.h>
#include <librepcb/common/units/all_length_units.h>
#include <librepcb/common/geometry/path.h>
#include <librepcb/common/uuid.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace library {

class FootprintPadGraphicsItem;

/*****************************************************************************************
 *  Class FootprintPad
 ****************************************************************************************/

/**
 * @brief The FootprintPad class represents a pad of a footprint
 */
class FootprintPad final : public SerializableObject
{
        Q_DECLARE_TR_FUNCTIONS(FootprintPad)

    public:

        // Types
        enum class Shape { ROUND, RECT, OCTAGON };
        enum class BoardSide { TOP, BOTTOM, THT };

        // Constructors / Destructor
        FootprintPad() = delete;
        FootprintPad(const FootprintPad& other) noexcept;
        FootprintPad(const Uuid& padUuid, const Point& pos, const Angle& rot,
                     Shape shape, const Length& width, const Length& height,
                     const Length& drillDiameter, BoardSide side) noexcept;
        explicit FootprintPad(const SExpression& node);
        ~FootprintPad() noexcept;

        // Getters
        const Uuid& getUuid() const noexcept {return getPackagePadUuid();} // for SerializableObjectList
        const Uuid& getPackagePadUuid() const noexcept {return mPackagePadUuid;}
        const Point& getPosition() const noexcept {return mPosition;}
        const Angle& getRotation() const noexcept {return mRotation;}
        Shape getShape() const noexcept {return mShape;}
        const Length& getWidth() const noexcept {return mWidth;}
        const Length& getHeight() const noexcept {return mHeight;}
        const Length& getDrillDiameter() const noexcept {return mDrillDiameter;}
        BoardSide getBoardSide() const noexcept {return mBoardSide;}
        QString getLayerName() const noexcept;
        bool isOnLayer(const QString& name) const noexcept;
        Path getOutline(const Length& expansion = Length(0)) const noexcept;
        QPainterPath toQPainterPathPx(const Length& expansion = Length(0)) const noexcept;

        // Setters
        void setPackagePadUuid(const Uuid& pad) noexcept;
        void setPosition(const Point& pos) noexcept;
        void setRotation(const Angle& rot) noexcept;
        void setShape(Shape shape) noexcept;
        void setWidth(const Length& width) noexcept;
        void setHeight(const Length& height) noexcept;
        void setDrillDiameter(const Length& diameter) noexcept;
        void setBoardSide(BoardSide side) noexcept;

        // General Methods
        void registerGraphicsItem(FootprintPadGraphicsItem& item) noexcept;
        void unregisterGraphicsItem(FootprintPadGraphicsItem& item) noexcept;

        /// @copydoc librepcb::SerializableObject::serialize()
        virtual void serialize(SExpression& root) const override;
        virtual bool checkAttributesValidity() const noexcept;

        // Operator Overloadings
        bool operator==(const FootprintPad& rhs) const noexcept;
        bool operator!=(const FootprintPad& rhs) const noexcept {return !(*this == rhs);}
        FootprintPad& operator=(const FootprintPad& rhs) noexcept;

        // Static Methods
        static Shape stringToShape(const QString& shape);
        static QString shapeToString(Shape shape) noexcept;
        static BoardSide stringToBoardSide(const QString& side);
        static QString boardSideToString(BoardSide side) noexcept;


    protected: // Data
        Uuid mPackagePadUuid;
        Point mPosition;
        Angle mRotation;
        Shape mShape;
        Length mWidth;
        Length mHeight;
        Length mDrillDiameter; // no effect if BoardSide != THT!
        BoardSide mBoardSide;
        FootprintPadGraphicsItem* mRegisteredGraphicsItem;
};

/*****************************************************************************************
 *  Class FootprintPadList
 ****************************************************************************************/

struct FootprintPadListNameProvider {static constexpr const char* tagname = "pad";};
using FootprintPadList = SerializableObjectList<FootprintPad, FootprintPadListNameProvider>;
using CmdFootprintPadInsert = CmdListElementInsert<FootprintPad, FootprintPadListNameProvider>;
using CmdFootprintPadRemove = CmdListElementRemove<FootprintPad, FootprintPadListNameProvider>;
using CmdFootprintPadsSwap = CmdListElementsSwap<FootprintPad, FootprintPadListNameProvider>;

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb

#endif // LIBREPCB_LIBRARY_FOOTPRINTPAD_H
