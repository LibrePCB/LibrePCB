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

#ifndef LIBREPCB_TOOLBOX_H
#define LIBREPCB_TOOLBOX_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include "units/point.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Class Toolbox
 ****************************************************************************************/

/**
 * @brief The Toolbox class provides some useful general purpose methods
 *
 * @author  ubruhin
 * @date    2016-10-30
 */
class Toolbox final
{
    public:

        // Constructors / Destructor
        Toolbox() = delete;
        Toolbox(const Toolbox& other) = delete;
        ~Toolbox() = delete;

        // Operator Overloadings
        Toolbox& operator=(const Toolbox& rhs) = delete;

        // Static Methods
        template <typename T>
        static QList<T> sortedQSet(const QSet<T>& set) noexcept {
            QList<T> list = set.toList();
            qSort(list);
            return list;
        }

        static QRectF boundingRectFromRadius(qreal radius) noexcept {
            return QRectF(-radius, -radius, 2*radius, 2*radius);
        }

        static QRectF boundingRectFromRadius(qreal rx, qreal ry) noexcept {
            return QRectF(-rx, -ry, 2*rx, 2*ry);
        }

        static QRectF adjustedBoundingRect(const QRectF& rect, qreal offset) noexcept {
            return rect.adjusted(-offset, -offset, offset, offset);
        }

        static QPainterPath shapeFromPath(const QPainterPath &path, const QPen &pen,
                                          const QBrush& brush, const Length& minWidth = Length(0)) noexcept;

        /**
         * @brief Calculate the point on a given line which is nearest to a given point
         *
         * @param p         An arbitrary point
         * @param l1        Start point of the line
         * @param l2        End point of the line
         *
         * @return Nearest point on the given line (either l1, l2, or a point between them)
         *
         * @warning This method works with floating point numbers and thus the result may
         *          not be perfectly precise.
         */
        static Point nearestPointOnLine(const Point& p, const Point& l1, const Point& l2) noexcept;

        /**
         * @brief Calculate the shortest distance between a given point and a given line
         *
         * @param p         An arbitrary point
         * @param l1        Start point of the line
         * @param l2        End point of the line
         * @param nearest   If not `nullptr`, the nearest point is returned here
         *
         * @return Shortest distance between the given point and the given line (>=0)
         */
        static Length shortestDistanceBetweenPointAndLine(const Point& p, const Point& l1,
                                                          const Point& l2, Point* nearest = nullptr) noexcept;

        /**
         * @brief Convert a numeric or non-numeric string to the corresponding QVariant
         *
         * @param string  The string to be converted
         * @return A QVariant with either a QVariant::Int or a QVariant::String
         */
        static QVariant stringOrNumberToQVariant(const QString& string) noexcept;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_TOOLBOX_H
