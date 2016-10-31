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
#include <librepcb/common/uuid.h>
#include <librepcb/common/units/all_length_units.h>
#include <librepcb/common/fileio/if_xmlserializableobject.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Class FootprintPad
 ****************************************************************************************/

/**
 * @brief The FootprintPad class
 *
 * @todo add subclasses for each footprint type
 */
class FootprintPad : public IF_XmlSerializableObject
{
        Q_DECLARE_TR_FUNCTIONS(FootprintPad)

    public:

        // Types
        enum class Technology_t { THT, SMT };

        // Constructors / Destructor
        explicit FootprintPad(Technology_t technology, const Uuid& padUuid,
                              const Point& pos, const Angle& rot, const Length& width,
                              const Length& height) noexcept;
        explicit FootprintPad(const XmlDomElement& domElement) throw (Exception);
        virtual ~FootprintPad() noexcept;

        // Getters
        const Uuid& getUuid() const noexcept {return mUuid;}
        Technology_t getTechnology() const noexcept {return mTechnology;}
        const Point& getPosition() const noexcept {return mPosition;}
        const Angle& getRotation() const noexcept {return mRotation;}
        const Length& getWidth() const noexcept {return mWidth;}
        const Length& getHeight() const noexcept {return mHeight;}
        virtual int getLayerId() const noexcept = 0;
        virtual bool isOnLayer(int id) const noexcept = 0;
        QRectF getBoundingRectPx() const noexcept;
        virtual const QPainterPath& toQPainterPathPx() const noexcept = 0;
        virtual QPainterPath toMaskQPainterPathPx(const Length& clearance) const noexcept = 0;

        // Setters
        void setPosition(const Point& pos) noexcept;
        void setRotation(const Angle& rot) noexcept;
        void setWidth(const Length& width) noexcept;
        void setHeight(const Length& height) noexcept;

        // General Methods

        /// @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
        virtual XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;

        /// @copydoc IF_XmlSerializableObject#checkAttributesValidity()
        virtual bool checkAttributesValidity() const noexcept override;

        // Static Methods
        static Technology_t stringToTechnology(const QString& technology) throw (Exception);
        static QString technologyToString(Technology_t technology) noexcept;
        static FootprintPad* fromDomElement(const XmlDomElement& domElement) throw (Exception);


    private:

        // make some methods inaccessible...
        FootprintPad() = delete;
        FootprintPad(const FootprintPad& other) = delete;
        FootprintPad& operator=(const FootprintPad& rhs) = delete;


    protected:

        // Pin Attributes
        Technology_t mTechnology;
        Uuid mUuid;
        Point mPosition;
        Angle mRotation;
        Length mWidth;
        Length mHeight;

        // Cached Attributes
        mutable QPainterPath mPainterPathPx;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb

#endif // LIBREPCB_LIBRARY_FOOTPRINTPAD_H
