/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
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

#ifndef POLYGON_H
#define POLYGON_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>
#include "../units/all_length_units.h"
#include "../fileio/if_xmlserializableobject.h"

/*****************************************************************************************
 *  Class PolygonSegment
 ****************************************************************************************/

/**
 * @brief The PolygonSegment class
 */
class PolygonSegment final : public IF_XmlSerializableObject
{
        Q_DECLARE_TR_FUNCTIONS(PolygonSegment)

    public:

        // Constructors / Destructor
        explicit PolygonSegment(const Point& endPos, const Angle& angle) noexcept :
            mEndPos(endPos), mAngle(angle) {}
        explicit PolygonSegment(const XmlDomElement& domElement) throw (Exception);
        ~PolygonSegment() noexcept {}

        // Getters
        const Point& getEndPos() const noexcept {return mEndPos;}
        const Angle& getAngle() const noexcept {return mAngle;}

        // Setters
        void setEndPos(const Point& pos) noexcept {mEndPos = pos;}
        void setAngle(const Angle& angle) noexcept {mAngle = angle;}

        // General Methods

        /// @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;


    private:

        // make some methods inaccessible...
        PolygonSegment();
        PolygonSegment(const PolygonSegment& other);
        PolygonSegment& operator=(const PolygonSegment& rhs);

        // Private Methods

        /// @copydoc #IF_XmlSerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;


        // Attributes
        Point mEndPos;
        Angle mAngle;
};

/*****************************************************************************************
 *  Class Polygon
 ****************************************************************************************/

/**
 * @brief The Polygon class
 */
class Polygon final : public IF_XmlSerializableObject
{
        Q_DECLARE_TR_FUNCTIONS(Polygon)

    public:

        // Constructors / Destructor
        explicit Polygon(int layerId, const Length& lineWidth, bool fill, bool isGrabArea,
                         const Point& startPos) noexcept;
        explicit Polygon(const XmlDomElement& domElement) throw (Exception);
        ~Polygon() noexcept;

        // Getters
        int getLayerId() const noexcept {return mLayerId;}
        const Length& getWidth() const noexcept {return mLineWidth;}
        bool isFilled() const noexcept {return mIsFilled;}
        bool isGrabArea() const noexcept {return mIsGrabArea;}
        const Point& getStartPos() const noexcept {return mStartPos;}
        const QList<PolygonSegment*>& getSegments() noexcept {return mSegments;}
        int getSegmentCount() const noexcept {return mSegments.count();}
        PolygonSegment* getSegment(int index) noexcept {return mSegments.value(index);}
        const PolygonSegment* getSegment(int index) const noexcept {return mSegments.value(index);}
        const QPainterPath& toQPainterPathPx() const noexcept;

        // Setters
        void setLayerId(int id) noexcept;
        void setLineWidth(const Length& width) noexcept;
        void setIsFilled(bool isFilled) noexcept;
        void setIsGrabArea(bool isGrabArea) noexcept;
        void setStartPos(const Point& pos) noexcept;

        // General Methods
        PolygonSegment* close() noexcept;
        void appendSegment(PolygonSegment& segment) noexcept;
        void removeSegment(PolygonSegment& segment) throw (Exception);

        /// @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;

        // Static Methods
        static Polygon* createLine(int layerId, const Length& lineWidth, bool fill, bool isGrabArea, const Point& p1, const Point& p2) noexcept;
        static Polygon* createCurve(int layerId, const Length& lineWidth, bool fill, bool isGrabArea, const Point& p1, const Point& p2, const Angle& angle) noexcept;
        static Polygon* createRect(int layerId, const Length& lineWidth, bool fill, bool isGrabArea, const Point& pos, const Length& width, const Length& height) noexcept;
        static Polygon* createCenteredRect(int layerId, const Length& lineWidth, bool fill, bool isGrabArea, const Point& center, const Length& width, const Length& height) noexcept;


    private:

        // make some methods inaccessible...
        Polygon() = delete;
        Polygon(const Polygon& other) = delete;
        Polygon& operator=(const Polygon& rhs) = delete;

        // Private Methods

        /// @copydoc #IF_XmlSerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;


        // Polygon Attributes
        int mLayerId;
        Length mLineWidth;
        bool mIsFilled;
        bool mIsGrabArea;
        Point mStartPos;
        QList<PolygonSegment*> mSegments;

        // Cached Attributes
        mutable QPainterPath mPainterPathPx;
};

#endif // POLYGON_H
