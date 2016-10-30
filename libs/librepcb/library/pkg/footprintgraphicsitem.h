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

#ifndef LIBREPCB_LIBRARY_FOOTPRINTGRAPHICSITEM_H
#define LIBREPCB_LIBRARY_FOOTPRINTGRAPHICSITEM_H


/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include <librepcb/common/uuid.h>
#include <librepcb/common/units/all_length_units.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class Ellipse;
class Polygon;
class Text;
class Hole;
class IF_GraphicsLayerProvider;
class PolygonGraphicsItem;
class EllipseGraphicsItem;
class TextGraphicsItem;
class HoleGraphicsItem;

namespace library {

class Footprint;
class FootprintPad;
class FootprintPadGraphicsItem;

/*****************************************************************************************
 *  Class FootprintGraphicsItem
 ****************************************************************************************/

/**
 * @brief The FootprintGraphicsItem class
 *
 * @author ubruhin
 * @date 2017-05-28
 */
class FootprintGraphicsItem final : public QGraphicsItem
{
    public:

        // Constructors / Destructor
        FootprintGraphicsItem() = delete;
        FootprintGraphicsItem(const FootprintGraphicsItem& other) = delete;
        FootprintGraphicsItem(Footprint& fpt, const IF_GraphicsLayerProvider& lp) noexcept;
        ~FootprintGraphicsItem() noexcept;

        // Getters
        Footprint& getFootprint() noexcept {return mFootprint;}
        FootprintPadGraphicsItem* getPadGraphicsItem(const FootprintPad& pin) noexcept;
        EllipseGraphicsItem* getEllipseGraphicsItem(const Ellipse& ellipse) noexcept;
        PolygonGraphicsItem* getPolygonGraphicsItem(const Polygon& polygon) noexcept;
        TextGraphicsItem* getTextGraphicsItem(const Text& text) noexcept;
        HoleGraphicsItem* getHoleGraphicsItem(const Hole& hole) noexcept;
        int getItemsAtPosition(const Point& pos,
                               QList<QSharedPointer<FootprintPadGraphicsItem>>* pads,
                               QList<QSharedPointer<EllipseGraphicsItem>>* ellipses,
                               QList<QSharedPointer<PolygonGraphicsItem>>* polygons,
                               QList<QSharedPointer<TextGraphicsItem>>* texts,
                               QList<QSharedPointer<HoleGraphicsItem>>* holes) noexcept;
        QList<QSharedPointer<FootprintPadGraphicsItem>> getSelectedPads() noexcept;
        QList<QSharedPointer<EllipseGraphicsItem>> getSelectedEllipses() noexcept;
        QList<QSharedPointer<PolygonGraphicsItem>> getSelectedPolygons() noexcept;
        QList<QSharedPointer<TextGraphicsItem>> getSelectedTexts() noexcept;
        QList<QSharedPointer<HoleGraphicsItem>> getSelectedHoles() noexcept;

        // Setters
        void setPosition(const Point& pos) noexcept;
        void setRotation(const Angle& rot) noexcept;

        // General Methods
        void addPad(FootprintPad& pad) noexcept;
        void removePad(FootprintPad& pad) noexcept;
        void addEllipse(Ellipse& ellipse) noexcept;
        void removeEllipse(Ellipse& ellipse) noexcept;
        void addPolygon(Polygon& polygon) noexcept;
        void removePolygon(Polygon& polygon) noexcept;
        void addText(Text& text) noexcept;
        void removeText(Text& text) noexcept;
        void addHole(Hole& hole) noexcept;
        void removeHole(Hole& hole) noexcept;
        void setSelectionRect(const QRectF rect) noexcept;

        // Inherited from QGraphicsItem
        QRectF boundingRect() const noexcept override {return QRectF();}
        QPainterPath shape() const noexcept override {return QPainterPath();}
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0) noexcept override;

        // Operator Overloadings
        FootprintGraphicsItem& operator=(const FootprintGraphicsItem& rhs) = delete;


    private: // Data
        Footprint& mFootprint;
        const IF_GraphicsLayerProvider& mLayerProvider;
        QHash<const FootprintPad*, QSharedPointer<FootprintPadGraphicsItem>> mPadGraphicsItems;
        QHash<const Ellipse*, QSharedPointer<EllipseGraphicsItem>> mEllipseGraphicsItems;
        QHash<const Polygon*, QSharedPointer<PolygonGraphicsItem>> mPolygonGraphicsItems;
        QHash<const Text*, QSharedPointer<TextGraphicsItem>> mTextGraphicsItems;
        QHash<const Hole*, QSharedPointer<HoleGraphicsItem>> mHoleGraphicsItems;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb

#endif // LIBREPCB_LIBRARY_FOOTPRINTGRAPHICSITEM_H
