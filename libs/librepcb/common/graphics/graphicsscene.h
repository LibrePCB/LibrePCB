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

#ifndef LIBREPCB_GRAPHICSSCENE_H
#define LIBREPCB_GRAPHICSSCENE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class Point;

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
        void addItem(QGraphicsItem& item) noexcept;
        void removeItem(QGraphicsItem& item) noexcept;
        void setSelectionRect(const Point& p1, const Point& p2) noexcept;


    private:

        QGraphicsRectItem* mSelectionRectItem;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_GRAPHICSSCENE_H
