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

#ifndef LIBREPCB_TEXT_H
#define LIBREPCB_TEXT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "../fileio/if_xmlserializableobject.h"
#include "../units/all_length_units.h"
#include "../alignment.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Class Text
 ****************************************************************************************/

/**
 * @brief The Text class
 */
class Text final : public IF_XmlSerializableObject
{
        Q_DECLARE_TR_FUNCTIONS(Text)

    public:

        // Constructors / Destructor
        explicit Text(int layerId, const QString& text, const Point& pos,
                      const Angle& rotation, const Length& height,
                      const Alignment& align) noexcept;
        explicit Text(const XmlDomElement& domElement) throw (Exception);
        ~Text() noexcept;

        // Getters
        int getLayerId() const noexcept {return mLayerId;}
        const Point& getPosition() const noexcept {return mPosition;}
        const Angle& getRotation() const noexcept {return mRotation;}
        const Length& getHeight() const noexcept {return mHeight;}
        const Alignment& getAlign() const noexcept {return mAlign;}
        const QString& getText() const noexcept {return mText;}

        // Setters
        void setLayerId(int id) noexcept;
        void setText(const QString& text) noexcept;
        void setPosition(const Point& pos) noexcept;
        void setRotation(const Angle& rotation) noexcept;
        void setHeight(const Length& height) noexcept;
        void setAlign(const Alignment& align) noexcept;

        // General Methods

        /// @copydoc #IF_XmlSerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;


    private:

        // make some methods inaccessible...
        Text() = delete;
        Text(const Text& other) = delete;
        Text& operator=(const Text& rhs) = delete;

        // Private Methods

        /// @copydoc #IF_XmlSerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;


        // Text Attributes
        int mLayerId;
        QString mText;
        Point mPosition;
        Angle mRotation;
        Length mHeight;
        Alignment mAlign;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_TEXT_H
