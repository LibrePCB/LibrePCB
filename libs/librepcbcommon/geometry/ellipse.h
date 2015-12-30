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

#ifndef ELLIPSE_H
#define ELLIPSE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "../units/all_length_units.h"
#include "../fileio/if_xmlserializableobject.h"

/*****************************************************************************************
 *  Class Ellipse
 ****************************************************************************************/

/**
 * @brief The Ellipse class
 */
class Ellipse final : public IF_XmlSerializableObject
{
        Q_DECLARE_TR_FUNCTIONS(Ellipse)

    public:

        // Constructors / Destructor
        explicit Ellipse(int layerId, const Length& lineWidth, bool fill, bool isGrabArea,
                         const Point& center, const Length& radiusX, const Length& radiusY,
                         const Angle& rotation) noexcept;
        explicit Ellipse(const XmlDomElement& domElement) throw (Exception);
        ~Ellipse() noexcept;

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
        void setLayerId(int id) noexcept;
        void setLineWidth(const Length& width) noexcept;
        void setIsFilled(bool isFilled) noexcept;
        void setIsGrabArea(bool isGrabArea) noexcept;
        void setCenter(const Point& center) noexcept;
        void setRadiusX(const Length& radius) noexcept;
        void setRadiusY(const Length& radius) noexcept;
        void setRotation(const Angle& rotation) noexcept;

        // General Methods

        /// @copydoc #IF_XmlSerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;


    private:

        // make some methods inaccessible...
        Ellipse() = delete;
        Ellipse(const Ellipse& other) = delete;
        Ellipse& operator=(const Ellipse& rhs) = delete;

        // Private Methods

        /// @copydoc #IF_XmlSerializableObject#checkAttributesValidity()
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

#endif // ELLIPSE_H
