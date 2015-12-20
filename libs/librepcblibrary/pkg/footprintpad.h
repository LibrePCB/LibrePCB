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

#ifndef LIBRARY_FOOTPRINTPAD_H
#define LIBRARY_FOOTPRINTPAD_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <librepcbcommon/units/all_length_units.h>
#include <librepcbcommon/fileio/if_xmlserializableobject.h>

/*****************************************************************************************
 *  Class FootprintPad
 ****************************************************************************************/

namespace library {

/**
 * @brief The FootprintPad class
 *
 * @todo add subclasses for each footprint type
 */
class FootprintPad final : public IF_XmlSerializableObject
{
        Q_DECLARE_TR_FUNCTIONS(FootprintPad)

    public:

        // Types

        enum class Type_t {
            ThtRect,
            ThtOctagon,
            ThtRound,
            SmdRect
        };

        // Constructors / Destructor
        explicit FootprintPad(const QUuid& padUuid) noexcept;
        explicit FootprintPad(const XmlDomElement& domElement) throw (Exception);
        ~FootprintPad() noexcept;

        // Getters
        const QUuid& getPadUuid() const noexcept {return mPadUuid;}
        Type_t getType() const noexcept {return mType;}
        const Point& getPosition() const noexcept {return mPosition;}
        const Angle& getRotation() const noexcept {return mRotation;}
        const Length& getWidth() const noexcept {return mWidth;}
        const Length& getHeight() const noexcept {return mHeight;}
        const Length& getDrillDiameter() const noexcept {return mDrillDiameter;}
        int getLayerId() const noexcept {return mLayerId;}

        // Setters
        void setType(Type_t type) noexcept {mType = type;}
        void setPosition(const Point& pos) noexcept {mPosition = pos;}
        void setRotation(const Angle& rotation) noexcept {mRotation = rotation;}
        void setWidth(const Length& width) noexcept {mWidth = width;}
        void setHeight(const Length& height) noexcept {mHeight = height;}
        void setDrillDiameter(const Length& diameter) noexcept {mDrillDiameter = diameter;}
        void setLayerId(int id) noexcept {mLayerId = id;}

        // General Methods

        /// @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;

        // Static Methods
        static Type_t stringToType(const QString& type) throw (Exception);
        static QString typeToString(Type_t type) noexcept;


    private:

        // make some methods inaccessible...
        FootprintPad(const FootprintPad& other);
        FootprintPad& operator=(const FootprintPad& rhs);

        // Private Methods

        /// @copydoc IF_XmlSerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;


        // Pin Attributes
        QUuid mPadUuid;
        Type_t mType;
        Point mPosition;
        Angle mRotation;
        Length mWidth;
        Length mHeight;
        Length mDrillDiameter;
        int mLayerId;
};

} // namespace library

#endif // LIBRARY_FOOTPRINTPAD_H
