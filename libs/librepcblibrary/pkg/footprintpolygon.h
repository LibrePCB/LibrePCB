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

#ifndef LIBRARY_FOOTPRINTPOLYGON_H
#define LIBRARY_FOOTPRINTPOLYGON_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>
#include <librepcbcommon/units/all_length_units.h>
#include <librepcbcommon/fileio/if_xmlserializableobject.h>

/*****************************************************************************************
 *  Class FootprintPolygonSegment
 ****************************************************************************************/

namespace library {

/**
 * @brief The FootprintPolygonSegment class
 *
 * @note If you make changes in this class, please check if you also need to modify
 *       the class library#SymbolPolygonSegment as these classes are very similar.
 */
class FootprintPolygonSegment final : public IF_XmlSerializableObject
{
        Q_DECLARE_TR_FUNCTIONS(FootprintPolygonSegment)

    public:

        // Constructors / Destructor
        explicit FootprintPolygonSegment(const Point& endPos, const Angle& angle = Angle(0)) noexcept :
            mEndPos(endPos), mAngle(angle) {}
        explicit FootprintPolygonSegment(const XmlDomElement& domElement) throw (Exception);
        ~FootprintPolygonSegment() noexcept {}

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
        FootprintPolygonSegment();
        FootprintPolygonSegment(const FootprintPolygonSegment& other);
        FootprintPolygonSegment& operator=(const FootprintPolygonSegment& rhs);

        // Private Methods

        /// @copydoc IF_XmlSerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;


        // Attributes
        Point mEndPos;
        Angle mAngle;
};

} // namespace library

/*****************************************************************************************
 *  Class FootprintPolygon
 ****************************************************************************************/

namespace library {

/**
 * @brief The FootprintPolygon class
 *
 * @note If you make changes in this class, please check if you also need to modify
 *       the class library#SymbolPolygon as these classes are very similar.
 */
class FootprintPolygon final : public IF_XmlSerializableObject
{
        Q_DECLARE_TR_FUNCTIONS(FootprintPolygon)

    public:

        // Constructors / Destructor
        explicit FootprintPolygon() noexcept;
        explicit FootprintPolygon(const XmlDomElement& domElement) throw (Exception);
        ~FootprintPolygon() noexcept;

        // Getters
        int getLayerId() const noexcept {return mLayerId;}
        const Length& getWidth() const noexcept {return mWidth;}
        bool isFilled() const noexcept {return mIsFilled;}
        bool isGrabArea() const noexcept {return mIsGrabArea;}
        const Point& getStartPos() const noexcept {return mStartPos;}
        const QList<const FootprintPolygonSegment*>& getSegments() const noexcept {return mSegments;}
        const QPainterPath& toQPainterPathPx() const noexcept;

        // Setters
        void setLayerId(int id) noexcept {mLayerId = id;}
        void setWidth(const Length& width) noexcept {mWidth = width;}
        void setIsFilled(bool isFilled) noexcept {mIsFilled = isFilled;}
        void setIsGrabArea(bool isGrabArea) noexcept {mIsGrabArea = isGrabArea;}
        void setStartPos(const Point& pos) noexcept {mStartPos = pos; mPainterPathPx = QPainterPath();}

        // General Methods
        void clearSegments() noexcept;
        void appendSegment(const FootprintPolygonSegment* segment) noexcept;

        /// @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;


    private:

        // make some methods inaccessible...
        FootprintPolygon(const FootprintPolygon& other);
        FootprintPolygon& operator=(const FootprintPolygon& rhs);

        // Private Methods

        /// @copydoc IF_XmlSerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;


        // Polygon Attributes
        int mLayerId;
        Length mWidth;
        bool mIsFilled;
        bool mIsGrabArea;
        Point mStartPos;
        QList<const FootprintPolygonSegment*> mSegments;

        // Cached Attributes
        mutable QPainterPath mPainterPathPx;
};

} // namespace library

#endif // LIBRARY_FOOTPRINTPOLYGON_H
