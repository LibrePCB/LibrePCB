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

#ifndef LIBREPCB_POLYGON_H
#define LIBREPCB_POLYGON_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include "../fileio/serializableobjectlist.h"
#include "../fileio/cmd/cmdlistelementinsert.h"
#include "../fileio/cmd/cmdlistelementremove.h"
#include "../fileio/cmd/cmdlistelementsswap.h"
#include "../units/all_length_units.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class PolygonSegment;

/*****************************************************************************************
 *  Interface IF_PolygonSegmentObserver
 ****************************************************************************************/

/**
 * @brief The IF_PolygonSegmentObserver class
 *
 * @author ubruhin
 * @date 2017-01-05
 */
class IF_PolygonSegmentObserver
{
    public:
        virtual void polygonSegmentEndPosChanged(const PolygonSegment& segment, const Point& newEndPos) noexcept = 0;
        virtual void polygonSegmentAngleChanged(const PolygonSegment& segment, const Angle& newAngle) noexcept = 0;

    protected:
        IF_PolygonSegmentObserver() noexcept {}
        virtual ~IF_PolygonSegmentObserver() noexcept {}
        IF_PolygonSegmentObserver& operator=(const IF_PolygonSegmentObserver& rhs) = delete;
};

/*****************************************************************************************
 *  Class PolygonSegment
 ****************************************************************************************/

/**
 * @brief The PolygonSegment class
 */
class PolygonSegment final : public SerializableObject
{
        Q_DECLARE_TR_FUNCTIONS(PolygonSegment)

    public:

        // Constructors / Destructor
        PolygonSegment() = delete;
        PolygonSegment(const PolygonSegment& other) noexcept;
        PolygonSegment(const Point& endPos, const Angle& angle) noexcept :
            mEndPos(endPos), mAngle(angle) {}
        explicit PolygonSegment(const SExpression& node);
        ~PolygonSegment() noexcept {}

        // Getters
        const Point& getEndPos() const noexcept {return mEndPos;}
        const Angle& getAngle() const noexcept {return mAngle;}
        Point calcArcCenter(const Point& startPos) const noexcept;

        // Setters
        void setEndPos(const Point& pos) noexcept;
        void setAngle(const Angle& angle) noexcept;

        // General Methods
        void registerObserver(IF_PolygonSegmentObserver& object) const noexcept;
        void unregisterObserver(IF_PolygonSegmentObserver& object) const noexcept;

        /// @copydoc librepcb::SerializableObject::serialize()
        void serialize(SExpression& root) const override;

        // Operator Overloadings
        bool operator==(const PolygonSegment& rhs) const noexcept;
        bool operator!=(const PolygonSegment& rhs) const noexcept {return !(*this == rhs);}
        PolygonSegment& operator=(const PolygonSegment& rhs) noexcept;


    private: // Data
        Point mEndPos;
        Angle mAngle;

        // Misc
        mutable QSet<IF_PolygonSegmentObserver*> mObservers; ///< A list of all observer objects
};

/*****************************************************************************************
 *  Class PolygonSegmentList
 ****************************************************************************************/

struct PolygonSegmentListNameProvider {static constexpr const char* tagname = "segment";};
using PolygonSegmentList = SerializableObjectList<PolygonSegment, PolygonSegmentListNameProvider>;
using CmdPolygonSegmentInsert = CmdListElementInsert<PolygonSegment, PolygonSegmentListNameProvider>;
using CmdPolygonSegmentRemove = CmdListElementRemove<PolygonSegment, PolygonSegmentListNameProvider>;
using CmdPolygonSegmentsSwap = CmdListElementsSwap<PolygonSegment, PolygonSegmentListNameProvider>;

/*****************************************************************************************
 *  Interface IF_PolygonObserver
 ****************************************************************************************/

/**
 * @brief The IF_PolygonObserver class
 *
 * @author ubruhin
 * @date 2017-01-05
 */
class IF_PolygonObserver
{
    public:
        virtual void polygonLayerNameChanged(const QString& newLayerName) noexcept = 0;
        virtual void polygonLineWidthChanged(const Length& newLineWidth) noexcept = 0;
        virtual void polygonIsFilledChanged(bool newIsFilled) noexcept = 0;
        virtual void polygonIsGrabAreaChanged(bool newIsGrabArea) noexcept = 0;
        virtual void polygonStartPosChanged(const Point& newStartPos) noexcept = 0;
        virtual void polygonSegmentAdded(int newSegmentIndex) noexcept = 0;
        virtual void polygonSegmentRemoved(int oldSegmentIndex) noexcept = 0;
        virtual void polygonSegmentEndPosChanged(int segmentIndex, const Point& newEndPos) noexcept = 0;
        virtual void polygonSegmentAngleChanged(int segmentIndex, const Angle& newAngle) noexcept = 0;

    protected:
        IF_PolygonObserver() noexcept {}
        explicit IF_PolygonObserver(const IF_PolygonObserver& other) = delete;
        virtual ~IF_PolygonObserver() noexcept {}
        IF_PolygonObserver& operator=(const IF_PolygonObserver& rhs) = delete;
};

/*****************************************************************************************
 *  Class Polygon
 ****************************************************************************************/

/**
 * @brief The Polygon class
 */
class Polygon final : public SerializableObject, private PolygonSegmentList::IF_Observer,
                      private IF_PolygonSegmentObserver
{
        Q_DECLARE_TR_FUNCTIONS(Polygon)

    public:

        // Constructors / Destructor
        Polygon() = delete;
        Polygon(const Polygon& other) noexcept;
        Polygon(const Uuid& uuid, const Polygon& other) noexcept;
        Polygon(const Uuid& uuid, const QString& layerName, const Length& lineWidth,
                bool fill, bool isGrabArea, const Point& startPos) noexcept;
        explicit Polygon(const SExpression& node);
        ~Polygon() noexcept;

        // Getters
        const Uuid& getUuid() const noexcept {return mUuid;}
        const QString& getLayerName() const noexcept {return mLayerName;}
        const Length& getLineWidth() const noexcept {return mLineWidth;}
        bool isFilled() const noexcept {return mIsFilled;}
        bool isGrabArea() const noexcept {return mIsGrabArea;}
        bool isClosed() const noexcept;
        const Point& getStartPos() const noexcept {return mStartPos;}
        PolygonSegmentList& getSegments() noexcept {return mSegments;}
        const PolygonSegmentList& getSegments() const noexcept {return mSegments;}
        Point getStartPointOfSegment(int index) const noexcept;
        Point calcCenterOfArcSegment(int index) const noexcept;
        Point calcCenter() const noexcept;
        const QPainterPath& toQPainterPathPx() const noexcept;

        // Setters
        void setLayerName(const QString& name) noexcept;
        void setLineWidth(const Length& width) noexcept;
        void setIsFilled(bool isFilled) noexcept;
        void setIsGrabArea(bool isGrabArea) noexcept;
        void setStartPos(const Point& pos) noexcept;

        // Transformations
        Polygon& translate(const Point& offset) noexcept;
        Polygon& rotate(const Angle& angle, const Point& center = Point(0, 0)) noexcept;
        Polygon& mirror(Qt::Orientation orientation, const Point& center = Point(0, 0)) noexcept;

        // General Methods
        bool close() noexcept;
        void registerObserver(IF_PolygonObserver& object) const noexcept;
        void unregisterObserver(IF_PolygonObserver& object) const noexcept;

        /// @copydoc librepcb::SerializableObject::serialize()
        void serialize(SExpression& root) const override;

        // Operator Overloadings
        bool operator==(const Polygon& rhs) const noexcept;
        bool operator!=(const Polygon& rhs) const noexcept {return !(*this == rhs);}
        Polygon& operator=(const Polygon& rhs) noexcept;

        // Static Methods
        static Polygon* createLine(const Uuid& uuid, const QString& layerName,
                                   const Length& lineWidth, bool fill, bool isGrabArea,
                                   const Point& p1, const Point& p2) noexcept;
        static Polygon* createCurve(const Uuid& uuid, const QString& layerName,
                                    const Length& lineWidth, bool fill, bool isGrabArea,
                                    const Point& p1, const Point& p2, const Angle& angle) noexcept;
        static Polygon* createRect(const Uuid& uuid, const QString& layerName,
                                   const Length& lineWidth, bool fill, bool isGrabArea,
                                   const Point& pos, const Length& width, const Length& height) noexcept;
        static Polygon* createCenteredRect(const Uuid& uuid, const QString& layerName,
                                           const Length& lineWidth, bool fill,
                                           bool isGrabArea, const Point& center,
                                           const Length& width, const Length& height) noexcept;


    private: // Methods
        void listObjectAdded(const PolygonSegmentList& list, int newIndex,
                             const std::shared_ptr<PolygonSegment>& ptr) noexcept override;
        void listObjectRemoved(const PolygonSegmentList& list, int oldIndex,
                               const std::shared_ptr<PolygonSegment>& ptr) noexcept override;
        void polygonSegmentEndPosChanged(const PolygonSegment& segment, const Point& newEndPos) noexcept override;
        void polygonSegmentAngleChanged(const PolygonSegment& segment, const Angle& newAngle) noexcept override;
        bool checkAttributesValidity() const noexcept;


    private: // Data
        Uuid mUuid;
        QString mLayerName;
        Length mLineWidth;
        bool mIsFilled;
        bool mIsGrabArea;
        Point mStartPos;
        PolygonSegmentList mSegments;

        // Misc
        mutable QSet<IF_PolygonObserver*> mObservers; ///< A list of all observer objects

        // Cached Attributes
        mutable QPainterPath mPainterPathPx;
};

/*****************************************************************************************
 *  Class PolygonList
 ****************************************************************************************/

struct PolygonListNameProvider {static constexpr const char* tagname = "polygon";};
using PolygonList = SerializableObjectList<Polygon, PolygonListNameProvider>;
using CmdPolygonInsert = CmdListElementInsert<Polygon, PolygonListNameProvider>;
using CmdPolygonRemove = CmdListElementRemove<Polygon, PolygonListNameProvider>;
using CmdPolygonsSwap = CmdListElementsSwap<Polygon, PolygonListNameProvider>;

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_POLYGON_H
