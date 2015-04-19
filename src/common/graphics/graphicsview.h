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

#ifndef GRAPHICSVIEW_H
#define GRAPHICSVIEW_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class IF_GraphicsViewEventHandler;
class GraphicsScene;
class GridProperties;

/*****************************************************************************************
 *  Class GraphicsView
 ****************************************************************************************/

/**
 * @brief The GraphicsView class
 */
class GraphicsView final : public QGraphicsView
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit GraphicsView(QWidget* parent = nullptr,
                              IF_GraphicsViewEventHandler* eventHandler = nullptr) noexcept;
        ~GraphicsView() noexcept;

        // Getters
        GraphicsScene* getScene() const noexcept {return mScene;}
        QRectF getVisibleSceneRect() const noexcept;

        // Setters
        void setGridProperties(const GridProperties& properties) noexcept;
        void setScene(GraphicsScene* scene) noexcept;
        void setVisibleSceneRect(const QRectF& rect) noexcept;

        // General Methods
        void zoomIn() noexcept;
        void zoomOut() noexcept;
        void zoomAll() noexcept;
        void handleMouseWheelEvent(QGraphicsSceneWheelEvent* event) noexcept;


    private slots:

        // Private Slots
        void zoomAnimationValueChanged(const QVariant& value) noexcept;


    private:

        // Inherited Methods
        bool eventFilter(QObject* obj, QEvent* event);
        void drawBackground(QPainter* painter, const QRectF& rect);
        void drawForeground(QPainter* painter, const QRectF& rect);


        // General Attributes
        IF_GraphicsViewEventHandler* mEventHandlerObject;
        GraphicsScene* mScene;
        QVariantAnimation* mZoomAnimation;
        GridProperties* mGridProperties;

        // Static Variables
        static constexpr qreal sZoomStepFactor = 1.5;
};

#endif // GRAPHICSVIEW_H
