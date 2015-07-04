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
        explicit FootprintPad(const QUuid& uuid = QUuid::createUuid(),
                              const QString& name_en_US = QString(),
                              const QString& description_en_US = QString()) noexcept;
        explicit FootprintPad(const XmlDomElement& domElement) throw (Exception);
        ~FootprintPad() noexcept;

        // Getters
        const QUuid& getUuid() const noexcept {return mUuid;}
        Type_t getType() const noexcept {return mType;}
        const Point& getPosition() const noexcept {return mPosition;}
        const Angle& getRotation() const noexcept {return mRotation;}
        const Length& getWidth() const noexcept {return mWidth;}
        const Length& getHeight() const noexcept {return mHeight;}
        const Length& getDrillDiameter() const noexcept {return mDrillDiameter;}
        uint getLayerId() const noexcept {return mLayerId;}
        QString getName(const QStringList& localeOrder) const noexcept;
        QString getDescription(const QStringList& localeOrder) const noexcept;
        const QMap<QString, QString>& getNames() const noexcept {return mNames;}
        const QMap<QString, QString>& getDescriptions() const noexcept {return mDescriptions;}

        // Setters
        void setType(Type_t type) noexcept {mType = type;}
        void setPosition(const Point& pos) noexcept {mPosition = pos;}
        void setRotation(const Angle& rotation) noexcept {mRotation = rotation;}
        void setWidth(const Length& width) noexcept {mWidth = width;}
        void setHeight(const Length& height) noexcept {mHeight = height;}
        void setDrillDiameter(const Length& diameter) noexcept {mDrillDiameter = diameter;}
        void setLayerId(uint id) noexcept {mLayerId = id;}
        void setName(const QString& locale, const QString& name) noexcept;
        void setDescription(const QString& locale, const QString& description) noexcept;

        // General Methods

        /// @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement(uint version) const throw (Exception) override;

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
        QUuid mUuid;
        Type_t mType;
        Point mPosition;
        Angle mRotation;
        Length mWidth;
        Length mHeight;
        Length mDrillDiameter;
        uint mLayerId;
        QMap<QString, QString> mNames;
        QMap<QString, QString> mDescriptions;
};

} // namespace library

#endif // LIBRARY_FOOTPRINTPAD_H
