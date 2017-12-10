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

Point Toolbox::nearestPointOnLine(const Point& p, const Point& l1, const Point& l2) noexcept
{
    Point a = l2 - l1;
    Point b = p - l1;
    Point c = p - l2;
    qreal d = ((b.getX().toMm() * a.getX().toMm()) + (b.getY().toMm() * a.getY().toMm()));
    qreal e = ((a.getX().toMm() * a.getX().toMm()) + (a.getY().toMm() * a.getY().toMm()));
    if (a.isOrigin() || b.isOrigin() || (d <= 0.0)) {
        return l1;
    } else if (c.isOrigin() || (e <= d)) {
        return l2;
    } else {
        Q_ASSERT(e > 0.0);
        return l1 + Point::fromMm(a.getX().toMm() * d / e, a.getY().toMm() * d / e);
    }
}

Length Toolbox::shortestDistanceBetweenPointAndLine(const Point& p, const Point& l1,
                                                    const Point& l2, Point* nearest) noexcept
{
    Point np = nearestPointOnLine(p, l1, l2);
    if (nearest) { *nearest = np; }
    return (p - np).getLength();
}

QVariant Toolbox::stringOrNumberToQVariant(const QString& string) noexcept
{
    bool isInt;
    int intval = string.toInt(&isInt, 10);
    if (isInt) {
        return QVariant(intval);
    }
    return QVariant(string);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
