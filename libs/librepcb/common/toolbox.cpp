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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "toolbox.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

QPainterPath Toolbox::shapeFromPath(const QPainterPath &path, const QPen &pen) noexcept
{
    // http://code.qt.io/cgit/qt/qtbase.git/tree/src/widgets/graphicsview/qgraphicsitem.cpp
    // Function: qt_graphicsItem_shapeFromPath()

    if (path == QPainterPath() || pen == Qt::NoPen) {
        return path;
    } else {
        QPainterPathStroker ps;
        ps.setCapStyle(pen.capStyle());
        ps.setWidth(qMax(pen.widthF(), qreal(0.00000001)));
        ps.setJoinStyle(pen.joinStyle());
        ps.setMiterLimit(pen.miterLimit());
        QPainterPath p = ps.createStroke(path);
        p.addPath(path);
        return p;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
