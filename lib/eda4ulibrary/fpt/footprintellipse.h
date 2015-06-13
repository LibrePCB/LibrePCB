/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
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

#ifndef LIBRARY_FOOTPRINTELLIPSE_H
#define LIBRARY_FOOTPRINTELLIPSE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <eda4ucommon/units/all_length_units.h>
#include <eda4ucommon/fileio/if_xmlserializableobject.h>

/*****************************************************************************************
 *  Class FootprintEllipse
 ****************************************************************************************/

namespace library {

/**
 * @brief The FootprintEllipse class
 *
 * @note If you make changes in this class, please check if you also need to modify
 *       the class library#SymbolEllipse as these classes are very similar.
 *
 * @author ubruhin
 * @date 2015-06-07
 */
class FootprintEllipse final : public IF_XmlSerializableObject
{
        Q_DECLARE_TR_FUNCTIONS(FootprintEllipse)

    public:

        // Constructors / Destructor
        explicit FootprintEllipse() noexcept;
        explicit FootprintEllipse(const XmlDomElement& domElement) throw (Exception);
        ~FootprintEllipse() noexcept;

        // Getters
        uint getLayerId() const noexcept {return mLayerId;}
        const Length& getLineWidth() const noexcept {return mLineWidth;}
        bool isFilled() const noexcept {return mIsFilled;}
        bool isGrabArea() const noexcept {return mIsGrabArea;}
        const Point& getCenter() const noexcept {return mCenter;}
        const Length& getRadiusX() const noexcept {return mRadiusX;}
        const Length& getRadiusY() const noexcept {return mRadiusY;}
        const Angle& getRotation() const noexcept {return mRotation;}

        // Setters
        void setLayerId(uint id) noexcept {mLayerId = id;}
        void setLineWidth(const Length& width) noexcept {mLineWidth = width;}
        void setIsFilled(bool isFilled) noexcept {mIsFilled = isFilled;}
        void setIsGrabArea(bool isGrabArea) noexcept {mIsGrabArea = isGrabArea;}
        void setCenter(const Point& center) noexcept {mCenter = center;}
        void setRadiusX(const Length& radius) noexcept {mRadiusX = radius;}
        void setRadiusY(const Length& radius) noexcept {mRadiusY = radius;}
        void setRotation(const Angle& rotation) noexcept {mRotation = rotation;}

        // General Methods
        XmlDomElement* serializeToXmlDomElement() const throw (Exception);


    private:

        // make some methods inaccessible...
        FootprintEllipse(const FootprintEllipse& other) = delete;
        FootprintEllipse& operator=(const FootprintEllipse& rhs) = delete;

        // Private Methods
        bool checkAttributesValidity() const noexcept;


        // Polygon Attributes
        uint mLayerId;
        Length mLineWidth;
        bool mIsFilled;
        bool mIsGrabArea;
        Point mCenter;
        Length mRadiusX;
        Length mRadiusY;
        Angle mRotation;
};

} // namespace library

#endif // LIBRARY_FOOTPRINTELLIPSE_H
