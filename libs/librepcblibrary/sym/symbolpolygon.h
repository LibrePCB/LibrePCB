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

#ifndef LIBRARY_SYMBOLPOLYGON_H
#define LIBRARY_SYMBOLPOLYGON_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>
#include <librepcbcommon/units/all_length_units.h>
#include <librepcbcommon/fileio/if_xmlserializableobject.h>

/*****************************************************************************************
 *  Class SymbolPolygonSegment
 ****************************************************************************************/

namespace library {

/**
 * @brief The SymbolPolygonSegment class
 *
 * @note If you make changes in this class, please check if you also need to modify
 *       the class #library#FootprintPolygonSegment as these classes are very similar.
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

        /// @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;


    private:

        // make some methods inaccessible...
        SymbolPolygonSegment();
        SymbolPolygonSegment(const SymbolPolygonSegment& other);
        SymbolPolygonSegment& operator=(const SymbolPolygonSegment& rhs);

        // Private Methods

        /// @copydoc IF_XmlSerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;


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
 *
 * @note If you make changes in this class, please check if you also need to modify
 *       the class #library#FootprintPolygon as these classes are very similar.
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
        int getLayerId() const noexcept {return mLayerId;}
        const Length& getWidth() const noexcept {return mWidth;}
        bool isFilled() const noexcept {return mIsFilled;}
        bool isGrabArea() const noexcept {return mIsGrabArea;}
        const Point& getStartPos() const noexcept {return mStartPos;}
        const QList<const SymbolPolygonSegment*>& getSegments() const noexcept {return mSegments;}
        const QPainterPath& toQPainterPathPx() const noexcept;

        // Setters
        void setLayerId(int id) noexcept {mLayerId = id;}
        void setWidth(const Length& width) noexcept {mWidth = width;}
        void setIsFilled(bool isFilled) noexcept {mIsFilled = isFilled;}
        void setIsGrabArea(bool isGrabArea) noexcept {mIsGrabArea = isGrabArea;}
        void setStartPos(const Point& pos) noexcept {mStartPos = pos; mPainterPathPx = QPainterPath();}

        // General Methods
        void clearSegments() noexcept;
        void appendSegment(const SymbolPolygonSegment* segment) noexcept;

        /// @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;


    private:

        // make some methods inaccessible...
        SymbolPolygon(const SymbolPolygon& other);
        SymbolPolygon& operator=(const SymbolPolygon& rhs);

        // Private Methods

        /// @copydoc IF_XmlSerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;


        // Polygon Attributes
        int mLayerId;
        Length mWidth;
        bool mIsFilled;
        bool mIsGrabArea;
        Point mStartPos;
        QList<const SymbolPolygonSegment*> mSegments;

        // Cached Attributes
        mutable QPainterPath mPainterPathPx;
};

} // namespace library

#endif // LIBRARY_SYMBOLPOLYGON_H
