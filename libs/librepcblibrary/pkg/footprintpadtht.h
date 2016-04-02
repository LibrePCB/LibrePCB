/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
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

#ifndef LIBREPCB_LIBRARY_FOOTPRINTPADTHT_H
#define LIBREPCB_LIBRARY_FOOTPRINTPADTHT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include "footprintpad.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Class FootprintPadTht
 ****************************************************************************************/

/**
 * @brief The FootprintPadTht class
 */
class FootprintPadTht final : public FootprintPad
{
        Q_DECLARE_TR_FUNCTIONS(FootprintPadTht)

    public:

        // Types
        enum class Shape_t { ROUND, RECT, OCTAGON };

        // Constructors / Destructor
        explicit FootprintPadTht(const Uuid& padUuid, const Point& pos, const Angle& rot,
                                 const Length& width, const Length& height,
                                 Shape_t shape, const Length& drillDiameter) noexcept;
        explicit FootprintPadTht(const XmlDomElement& domElement) throw (Exception);
        ~FootprintPadTht() noexcept;

        // Getters
        Shape_t getShape() const noexcept {return mShape;}
        const Length& getDrillDiameter() const noexcept {return mDrillDiameter;}
        int getLayerId() const noexcept override;
        bool isOnLayer(int id) const noexcept override;
        const QPainterPath& toQPainterPathPx() const noexcept override;
        QPainterPath toMaskQPainterPathPx(const Length& clearance) const noexcept override;

        // Setters
        void setShape(Shape_t shape) noexcept;
        void setDrillDiameter(const Length& diameter) noexcept;

        // General Methods

        /// @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;

        // Static Methods
        static Shape_t stringToShape(const QString& shape) throw (Exception);
        static QString shapeToString(Shape_t shape) noexcept;


    private:

        // make some methods inaccessible...
        FootprintPadTht() = delete;
        FootprintPadTht(const FootprintPadTht& other) = delete;
        FootprintPadTht& operator=(const FootprintPadTht& rhs) = delete;

        // Private Methods

        /// @copydoc IF_XmlSerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;


        // Pin Attributes
        Shape_t mShape;
        Length mDrillDiameter;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb

#endif // LIBREPCB_LIBRARY_FOOTPRINTPADTHT_H
