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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>
#include "cadscene.h"

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CADScene::CADScene() :
    QGraphicsScene(), mEventHandlerObject(0)
{
}

CADScene::~CADScene()
{
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void CADScene::setEventHandlerObject(IF_CADSceneEventHandler* object)
{
    mEventHandlerObject = object;
}

/*****************************************************************************************
 *  Inherited from QGraphicsScene
 ****************************************************************************************/

void CADScene::mousePressEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
    if (mEventHandlerObject)
    {
        if (mEventHandlerObject->cadSceneEventHandler(mouseEvent))
            return;
    }
    QGraphicsScene::mousePressEvent(mouseEvent);
}

void CADScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
    if (mEventHandlerObject)
    {
        if (mEventHandlerObject->cadSceneEventHandler(mouseEvent))
            return;
    }
    QGraphicsScene::mouseReleaseEvent(mouseEvent);
}

void CADScene::mouseMoveEvent(QGraphicsSceneMouseEvent* mouseEvent)
{
    if (mEventHandlerObject)
    {
        if (mEventHandlerObject->cadSceneEventHandler(mouseEvent))
            return;
    }
    QGraphicsScene::mouseMoveEvent(mouseEvent);
}

void CADScene::wheelEvent(QGraphicsSceneWheelEvent* event)
{
    if (mEventHandlerObject)
    {
        if (mEventHandlerObject->cadSceneEventHandler(event))
            return;
    }
    QGraphicsScene::wheelEvent(event);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
