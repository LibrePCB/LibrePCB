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

#ifndef LIBRARY_SYMBOLPOLYGON_H
#define LIBRARY_SYMBOLPOLYGON_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "../common/exceptions.h"
#include "../common/units/all_length_units.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class XmlDomElement;

namespace library {
class Symbol;
}

/*****************************************************************************************
 *  Class SymbolPolygon
 ****************************************************************************************/

namespace library {

/**
 * @brief The SymbolPolygon class
 */
class SymbolPolygon final : public QObject
{
        Q_OBJECT

    public:

        // Types
        struct PolygonSegment_t {
            enum {Line, Arc} type;
            Point endPos;
        };


        // Constructors / Destructor
        explicit SymbolPolygon(Symbol& symbol, const XmlDomElement& domElement) throw (Exception);
        ~SymbolPolygon() noexcept;

        // Getters
        uint getLineLayerId() const noexcept {return mLineLayerId;}
        uint getFillLayerId() const noexcept {return mFillLayerId;}
        const Length& getLineWidth() const noexcept {return mLineWidth;}
        bool isGrabArea() const noexcept {return mIsGrabArea;}
        const Point& getStartPos() const noexcept {return mStartPos;}
        const QList<const PolygonSegment_t*>& getSegments() const noexcept {return mSegments;}


    private:

        // make some methods inaccessible...
        SymbolPolygon();
        SymbolPolygon(const SymbolPolygon& other);
        SymbolPolygon& operator=(const SymbolPolygon& rhs);


        // General Attributes
        Symbol& mSymbol;

        // Polygon Attributes
        uint mLineLayerId;
        uint mFillLayerId;
        Length mLineWidth;
        bool mIsGrabArea;
        Point mStartPos;
        QList<const PolygonSegment_t*> mSegments;
};

} // namespace library

#endif // LIBRARY_SYMBOLPOLYGON_H
