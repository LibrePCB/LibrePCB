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

#ifndef ATTRIBUTEUNIT_H
#define ATTRIBUTEUNIT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "../exceptions.h"

/*****************************************************************************************
 *  Class AttributeUnit
 ****************************************************************************************/

/**
 * @brief The AttributeUnit class
 */
class AttributeUnit final
{
    public:

        // Constructors / Destructor
        explicit AttributeUnit(const QString& name, const QString& symbolTr) noexcept;
        ~AttributeUnit() noexcept;

        // Getters
        const QString& getName() const noexcept {return mName;}
        const QString& getSymbolTr() const noexcept {return mSymbolTr;}


    private:

        // make some methods inaccessible...
        AttributeUnit() = delete;
        AttributeUnit(const AttributeUnit& other) = delete;
        AttributeUnit& operator=(const AttributeUnit& rhs) = delete;


        // General Attributes
        QString mName;          ///< to convert from/to string, e.g. "millivolt"
        QString mSymbolTr;      ///< e.g. "mV"
};

#endif // ATTRIBUTEUNIT_H
