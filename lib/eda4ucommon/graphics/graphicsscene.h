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

#ifndef GRAPHICSSCENE_H
#define GRAPHICSSCENE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>
#include "../units/point.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class GraphicsItem;

/*****************************************************************************************
 *  Class GraphicsScene
 ****************************************************************************************/

/**
 * @brief The GraphicsScene class
 */
class GraphicsScene final : public QGraphicsScene
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit GraphicsScene() noexcept;
        ~GraphicsScene() noexcept;

        // General Methods
        void addItem(GraphicsItem& item) noexcept;
        void removeItem(GraphicsItem& item) noexcept;
        void setSelectionRect(const Point& p1, const Point& p2) noexcept;


    private:

        QGraphicsRectItem* mSelectionRectItem;
};

#endif // GRAPHICSSCENE_H
