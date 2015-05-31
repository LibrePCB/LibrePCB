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

#ifndef ATTRTYPEFREQUENCY_H
#define ATTRTYPEFREQUENCY_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "attributetype.h"

/*****************************************************************************************
 *  Class AttrTypeFrequency
 ****************************************************************************************/

/**
 * @brief The AttrTypeFrequency class
 */
class AttrTypeFrequency final : public AttributeType
{

    public:

        bool isValueValid(const QString& value) const noexcept;
        QString valueFromTr(const QString& value) const noexcept;
        QString printableValueTr(const QString& value, const AttributeUnit* unit = nullptr) const noexcept;
        static const AttrTypeFrequency& instance() noexcept {static AttrTypeFrequency x; return x;}


    private:

        // make some methods inaccessible...
        AttrTypeFrequency(const AttrTypeFrequency& other) = delete;
        AttrTypeFrequency& operator=(const AttrTypeFrequency& rhs) = delete;


        // Constructors / Destructor
        AttrTypeFrequency() noexcept;
        ~AttrTypeFrequency() noexcept;
};

#endif // ATTRTYPEFREQUENCY_H
