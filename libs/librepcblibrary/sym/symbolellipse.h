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

#ifndef LIBRARY_SYMBOLELLIPSE_H
#define LIBRARY_SYMBOLELLIPSE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <librepcbcommon/units/all_length_units.h>
#include <librepcbcommon/fileio/if_xmlserializableobject.h>

/*****************************************************************************************
 *  Class SymbolEllipse
 ****************************************************************************************/

namespace library {

/**
 * @brief The SymbolEllipse class
 *
 * @note If you make changes in this class, please check if you also need to modify
 *       the class library#FootprintEllipse as these classes are very similar.
 */
class SymbolEllipse final : public IF_XmlSerializableObject
{
        Q_DECLARE_TR_FUNCTIONS(SymbolEllipse)

    public:

        // Constructors / Destructor
        explicit SymbolEllipse() noexcept;
        explicit SymbolEllipse(const XmlDomElement& domElement) throw (Exception);
        ~SymbolEllipse() noexcept;

        // Getters
        int getLayerId() const noexcept {return mLayerId;}
        const Length& getLineWidth() const noexcept {return mLineWidth;}
        bool isFilled() const noexcept {return mIsFilled;}
        bool isGrabArea() const noexcept {return mIsGrabArea;}
        const Point& getCenter() const noexcept {return mCenter;}
        const Length& getRadiusX() const noexcept {return mRadiusX;}
        const Length& getRadiusY() const noexcept {return mRadiusY;}
        const Angle& getRotation() const noexcept {return mRotation;}

        // Setters
        void setLayerId(int id) noexcept {mLayerId = id;}
        void setLineWidth(const Length& width) noexcept {mLineWidth = width;}
        void setIsFilled(bool isFilled) noexcept {mIsFilled = isFilled;}
        void setIsGrabArea(bool isGrabArea) noexcept {mIsGrabArea = isGrabArea;}
        void setCenter(const Point& center) noexcept {mCenter = center;}
        void setRadiusX(const Length& radius) noexcept {mRadiusX = radius;}
        void setRadiusY(const Length& radius) noexcept {mRadiusY = radius;}
        void setRotation(const Angle& rotation) noexcept {mRotation = rotation;}

        // General Methods

        /// @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;


    private:

        // make some methods inaccessible...
        SymbolEllipse(const SymbolEllipse& other) = delete;
        SymbolEllipse& operator=(const SymbolEllipse& rhs) = delete;

        // Private Methods

        /// @copydoc IF_XmlSerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;


        // Polygon Attributes
        int mLayerId;
        Length mLineWidth;
        bool mIsFilled;
        bool mIsGrabArea;
        Point mCenter;
        Length mRadiusX;
        Length mRadiusY;
        Angle mRotation;
};

} // namespace library

#endif // LIBRARY_SYMBOLELLIPSE_H
