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

#ifndef LIBRARY_SYMBOLTEXT_H
#define LIBRARY_SYMBOLTEXT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "../common/exceptions.h"
#include "../common/units/all_length_units.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class XmlDomElement;

namespace library {
class Symbol;
}

/*****************************************************************************************
 *  Class SymbolText
 ****************************************************************************************/

namespace library {

/**
 * @brief The SymbolText class
 */
class SymbolText final : public QObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit SymbolText(Symbol& symbol, const XmlDomElement& domElement) throw (Exception);
        ~SymbolText() noexcept;

        // Getters
        unsigned int getLayerId() const noexcept {return mLayerId;}
        const Point& getPosition() const noexcept {return mPosition;}
        const Angle& getAngle() const noexcept {return mAngle;}
        const Length& getHeight() const noexcept {return mHeight;}
        const Qt::Alignment& getAlign() const noexcept {return mAlign;}
        const QString& getText() const noexcept {return mText;}


    private:

        // make some methods inaccessible...
        SymbolText();
        SymbolText(const SymbolText& other);
        SymbolText& operator=(const SymbolText& rhs);


        // General Attributes
        Symbol& mSymbol;

        // Text Attributes
        unsigned int mLayerId;
        Point mPosition;
        Angle mAngle;
        Length mHeight;
        Qt::Alignment mAlign;
        QString mText;
};

} // namespace library

#endif // LIBRARY_SYMBOLTEXT_H
