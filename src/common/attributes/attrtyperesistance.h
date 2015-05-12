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

#ifndef ATTRTYPERESISTANCE_H
#define ATTRTYPERESISTANCE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "attributetype.h"

/*****************************************************************************************
 *  Class AttrTypeResistance
 ****************************************************************************************/

/**
 * @brief The AttrTypeResistance class
 */
class AttrTypeResistance final : public AttributeType
{

    public:

        bool isValueValid(const QString& value) const noexcept;
        QString valueFromTr(const QString& value) const noexcept;
        QString printableValueTr(const QString& value, const AttributeUnit* unit = nullptr) const noexcept;
        static const AttrTypeResistance& instance() noexcept {static AttrTypeResistance x; return x;}


    private:

        // make some methods inaccessible...
        AttrTypeResistance(const AttrTypeResistance& other) = delete;
        AttrTypeResistance& operator=(const AttrTypeResistance& rhs) = delete;


        // Constructors / Destructor
        AttrTypeResistance() noexcept;
        ~AttrTypeResistance() noexcept;
};

#endif // ATTRTYPERESISTANCE_H
