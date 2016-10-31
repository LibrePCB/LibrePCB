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

#ifndef LIBREPCB_EXCELLONGENERATOR_H
#define LIBREPCB_EXCELLONGENERATOR_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "../exceptions.h"
#include "../fileio/filepath.h"
#include "../units/all_length_units.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Class GerberGenerator
 ****************************************************************************************/

/**
 * @brief The ExcellonGenerator class
 *
 * @author ubruhin
 * @date 2016-03-31
 */
class ExcellonGenerator final
{
        Q_DECLARE_TR_FUNCTIONS(ExcellonGenerator)

    public:

        // Constructors / Destructor
        //ExcellonGenerator() = delete;
        ExcellonGenerator(const ExcellonGenerator& other) = delete;
        ExcellonGenerator() noexcept;
        ~ExcellonGenerator() noexcept;

        // Getters
        const QString& toStr() const noexcept {return mOutput;}

        // General Methods
        void drill(const Point& pos, const Length& dia) noexcept;
        void generate() throw (Exception);
        void saveToFile(const FilePath& filepath) const throw (Exception);
        void reset() noexcept;

        // Operator Overloadings
        ExcellonGenerator& operator=(const ExcellonGenerator& rhs) = delete;


    private:

        void printHeader() noexcept;
        void printToolList() noexcept;
        void printDrills() noexcept;
        void printFooter() noexcept;


        // Excellon Data
        QString mOutput;
        QMultiMap<Length, Point> mDrillList;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_EXCELLONGENERATOR_H
