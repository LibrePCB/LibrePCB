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

/*****************************************************************************************
 *  Interface IF_CADSceneEventHandler
 ****************************************************************************************/

/**
 * @brief The CADViewEventReceiver class
 */
class IF_CADSceneEventHandler
{
    public:

        /// The event handler method
        virtual bool cadSceneEventHandler(QEvent* event) = 0;
};

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

        // Types

        /**
         * @brief All custom QGraphicsItem types which are used in CADScene
         *
         * See QGraphicsItem::Type for more information.
         *
         * @note We do not use "enum class" because it must be easily compareable with int.
         */
        enum ItemType_t {
            Type_UserType = QGraphicsItem::UserType, ///< the base number for user types
            // Types which are used in library elements
            Type_Symbol,                    ///< library#Symbol
            Type_SymbolPin,                 ///< library#SymbolPin
            // Types which are used in schematic scenes
            Type_SchematicNetPoint,         ///< project#SchematicNetPoint
            Type_SchematicNetLine,          ///< project#SchematicNetLine
        };


        // Constructors / Destructor
        explicit CADScene();
        virtual ~CADScene();

        // Setters
        void setEventHandlerObject(IF_CADSceneEventHandler* object);


    protected:

        // Inherited from QGraphicsScene
        virtual void keyPressEvent(QKeyEvent* keyEvent);
        virtual void keyReleaseEvent(QKeyEvent* keyEvent);
        virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* mouseEvent);
        virtual void mousePressEvent(QGraphicsSceneMouseEvent* mouseEvent);
        virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* mouseEvent);
        virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* mouseEvent);
        virtual void wheelEvent(QGraphicsSceneWheelEvent* event);

    private:

        // make some methods inaccessible...
        CADScene(const CADScene& other);
        CADScene& operator=(const CADScene& rhs);

        IF_CADSceneEventHandler* mEventHandlerObject;
};

#endif // CADSCENE_H
