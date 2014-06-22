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

#ifndef CADSCENE_H
#define CADSCENE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>
#include "units.h"

/*****************************************************************************************
 *  Class CADScene
 ****************************************************************************************/

/**
 * @brief The CADScene class
 *
 * @author ubruhin
 *
 * @date 2014-06-22
 */
class CADScene : public QGraphicsScene
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit CADScene(const Length& gridInterval = Length(2540000));
        virtual ~CADScene();

        // Getters
        const Length& getGridInterval() const {return mGridInterval;}

        // Setters
        void setGridInterval(const Length& newInterval);

    private:

        Length mGridInterval;

};

#endif // CADSCENE_H
