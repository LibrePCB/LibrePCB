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

#ifndef CADVIEW_H
#define CADVIEW_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>
#include "units/all_length_units.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class CADScene;
class GridProperties;

/*****************************************************************************************
 *  Class CADView
 ****************************************************************************************/

/**
 * @brief The CADView class
 *
 * @author ubruhin
 *
 * @date 2014-06-22
 */
class CADView : public QGraphicsView
{
        Q_OBJECT

    public:;

        // Constructors / Destructor
        explicit CADView(QWidget* parent = 0);
        virtual ~CADView();

        // Getters
        CADScene*           getCadScene()           const;
        QRectF              getVisibleSceneRect()   const;

        // Setters
        void setCadScene(CADScene* scene); ///< Use always this method instead of QGraphicsView::setScene()!
        void setVisibleSceneRect(const QRectF& rect);
        void setGridProperties(const GridProperties& properties) noexcept;
        void setOriginCrossVisible(bool visible) noexcept;
        void setPaperSize(const Point& size) noexcept;
        void setPositionLabelVisible(bool visible) noexcept;

        // Static Functions
        static qreal getZoomFactor() {return sZoomFactor;}
        static void setZoomFactor(qreal factor) {sZoomFactor = factor;}


    public slots:

        // Zoom Functions
        void zoomIn();
        void zoomOut();
        void zoomAll();


    private slots:

        // Private Slots
        void zoomAnimationValueChanged(const QVariant& value) noexcept;


    protected:

        // Inherited Methods
        virtual void drawBackground(QPainter* painter, const QRectF& rect);
        virtual void drawForeground(QPainter* painter, const QRectF& rect);
        virtual void wheelEvent(QWheelEvent* event);
        virtual void mouseMoveEvent(QMouseEvent* event);


    private:

        // make some methods inaccessible...
        CADView();
        CADView(const CADView& other);
        CADView& operator=(const CADView& rhs);

        // Private Methods
        void updatePositionLabelText(const QPointF pos = QPointF(0, 0));


        // Attributes
        GridProperties* mGridProperties;
        QColor mGridColor;
        bool mGridBoundedToPageBorders;
        bool mOriginCrossVisible;
        QColor mOriginCrossColor;
        QSizeF mPageSizePx;

        Point mLastMouseMoveEventPos;
        QLabel* mPositionLabel;
        QVariantAnimation* mZoomAnimation;


        // Static Variables
        static qreal sZoomFactor;
};

#endif // CADVIEW_H
