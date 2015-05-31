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
#include <QtWidgets>
#include <eda4ucommon/units/all_length_units.h>
#include <eda4ucommon/fileio/if_xmlserializableobject.h>

/*****************************************************************************************
 *  Class SymbolPolygonSegment
 ****************************************************************************************/

namespace library {

/**
 * @brief The SymbolPolygonSegment class
 */
class SymbolPolygonSegment final : public IF_XmlSerializableObject
{
        Q_DECLARE_TR_FUNCTIONS(SymbolPolygonSegment)

    public:

        // Constructors / Destructor
        explicit SymbolPolygonSegment(const Point& endPos, const Angle& angle = Angle(0)) noexcept :
            mEndPos(endPos), mAngle(angle) {}
        explicit SymbolPolygonSegment(const XmlDomElement& domElement) throw (Exception);
        ~SymbolPolygonSegment() noexcept {}

        // Getters
        const Point& getEndPos() const noexcept {return mEndPos;}
        const Angle& getAngle() const noexcept {return mAngle;}

        // Setters
        void setEndPos(const Point& pos) noexcept {mEndPos = pos;}
        void setAngle(const Angle& angle) noexcept {mAngle = angle;}

        // General Methods
        XmlDomElement* serializeToXmlDomElement() const throw (Exception);


    private:

        // make some methods inaccessible...
        SymbolPolygonSegment();
        SymbolPolygonSegment(const SymbolPolygonSegment& other);
        SymbolPolygonSegment& operator=(const SymbolPolygonSegment& rhs);

        // Private Methods
        bool checkAttributesValidity() const noexcept;


        // Attributes
        Point mEndPos;
        Angle mAngle;
};

} // namespace library

/*****************************************************************************************
 *  Class SymbolPolygon
 ****************************************************************************************/

namespace library {

/**
 * @brief The SymbolPolygon class
 */
class SymbolPolygon final : public IF_XmlSerializableObject
{
        Q_DECLARE_TR_FUNCTIONS(SymbolPolygon)

    public:

        // Constructors / Destructor
        explicit SymbolPolygon() noexcept;
        explicit SymbolPolygon(const XmlDomElement& domElement) throw (Exception);
        ~SymbolPolygon() noexcept;

        // Getters
        uint getLineLayerId() const noexcept {return mLineLayerId;}
        uint getFillLayerId() const noexcept {return mFillLayerId;}
        const Length& getLineWidth() const noexcept {return mLineWidth;}
        bool isGrabArea() const noexcept {return mIsGrabArea;}
        const Point& getStartPos() const noexcept {return mStartPos;}
        const QList<const SymbolPolygonSegment*>& getSegments() const noexcept {return mSegments;}
        const QPainterPath& toQPainterPathPx() const noexcept;

        // Setters
        void setLineLayerId(uint id) noexcept {mLineLayerId = id;}
        void setFillLayerId(uint id) noexcept {mFillLayerId = id;}
        void setLineWidth(const Length& width) noexcept {mLineWidth = width;}
        void setIsGrabArea(bool isGrabArea) noexcept {mIsGrabArea = isGrabArea;}
        void setStartPos(const Point& pos) noexcept {mStartPos = pos; mPainterPathPx = QPainterPath();}

        // General Methods
        void clearSegments() noexcept;
        void appendSegment(const SymbolPolygonSegment* segment) noexcept;
        XmlDomElement* serializeToXmlDomElement() const throw (Exception);


    private:

        // make some methods inaccessible...
        SymbolPolygon(const SymbolPolygon& other);
        SymbolPolygon& operator=(const SymbolPolygon& rhs);

        // Private Methods
        bool checkAttributesValidity() const noexcept;


        // Polygon Attributes
        uint mLineLayerId;
        uint mFillLayerId;
        Length mLineWidth;
        bool mIsGrabArea;
        Point mStartPos;
        QList<const SymbolPolygonSegment*> mSegments;

        // Cached Attributes
        mutable QPainterPath mPainterPathPx;
};

} // namespace library

#endif // LIBRARY_SYMBOLPOLYGON_H
