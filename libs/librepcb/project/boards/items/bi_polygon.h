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

#ifndef LIBREPCB_PROJECT_BI_POLYGON_H
#define LIBREPCB_PROJECT_BI_POLYGON_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "bi_base.h"
#include <librepcb/common/fileio/serializableobject.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class Polygon;
class PolygonGraphicsItem;

namespace project {

class Project;
class Board;
class BGI_Polygon;

/*****************************************************************************************
 *  Class BI_Polygon
 ****************************************************************************************/

/**
 * @brief The BI_Polygon class
 *
 * @author ubruhin
 * @date 2016-01-12
 */
class BI_Polygon final : public BI_Base, public SerializableObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        BI_Polygon() = delete;
        BI_Polygon(const BI_Polygon& other) = delete;
        BI_Polygon(Board& board, const BI_Polygon& other);
        BI_Polygon(Board& board, const SExpression& node);
        BI_Polygon(Board& board, const QString& layerName, const Length& lineWidth, bool fill,
                   bool isGrabArea, const Point& startPos);
        ~BI_Polygon() noexcept;

        // Getters
        Polygon& getPolygon() noexcept {return *mPolygon;}
        const Polygon& getPolygon() const noexcept {return *mPolygon;}
        bool isSelectable() const noexcept override;

        // General Methods
        void addToBoard() override;
        void removeFromBoard() override;

        /// @copydoc librepcb::SerializableObject::serialize()
        void serialize(SExpression& root) const override;

        // Inherited from BI_Base
        Type_t getType() const noexcept override {return BI_Base::Type_t::Polygon;}
        const Point& getPosition() const noexcept override {static Point p(0, 0); return p;}
        bool getIsMirrored() const noexcept override {return false;}
        QPainterPath getGrabAreaScenePx() const noexcept override;
        void setSelected(bool selected) noexcept override;

        // Operator Overloadings
        BI_Polygon& operator=(const BI_Polygon& rhs) = delete;


    private slots:

        void boardAttributesChanged();


    private:
        void init();


        // General
        QScopedPointer<Polygon> mPolygon;
        QScopedPointer<PolygonGraphicsItem> mGraphicsItem;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_BI_POLYGON_H
