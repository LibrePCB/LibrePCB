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

#ifndef LIBREPCB_CIRCLE_H
#define LIBREPCB_CIRCLE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "../fileio/serializableobjectlist.h"
#include "../fileio/cmd/cmdlistelementinsert.h"
#include "../fileio/cmd/cmdlistelementremove.h"
#include "../fileio/cmd/cmdlistelementsswap.h"
#include "../units/all_length_units.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Interface IF_CircleObserver
 ****************************************************************************************/

/**
 * @brief The IF_CircleObserver class
 *
 * @author ubruhin
 * @date 2017-01-01
 */
class IF_CircleObserver
{
    public:
        virtual void circleLayerNameChanged(const QString& newLayerName) noexcept = 0;
        virtual void circleLineWidthChanged(const Length& newLineWidth) noexcept = 0;
        virtual void circleIsFilledChanged(bool newIsFilled) noexcept = 0;
        virtual void circleIsGrabAreaChanged(bool newIsGrabArea) noexcept = 0;
        virtual void circleCenterChanged(const Point& newCenter) noexcept = 0;
        virtual void circleRadiusXChanged(const Length& newRadiusX) noexcept = 0;
        virtual void circleRadiusYChanged(const Length& newRadiusY) noexcept = 0;
        virtual void circleRotationChanged(const Angle& newRotation) noexcept = 0;

    protected:
        IF_CircleObserver() noexcept {}
        explicit IF_CircleObserver(const IF_CircleObserver& other) = delete;
        virtual ~IF_CircleObserver() noexcept {}
        IF_CircleObserver& operator=(const IF_CircleObserver& rhs) = delete;
};

/*****************************************************************************************
 *  Class Circle
 ****************************************************************************************/

/**
 * @brief The Circle class
 */
class Circle : public SerializableObject
{
        Q_DECLARE_TR_FUNCTIONS(Circle)

    public:

        // Constructors / Destructor
        Circle() = delete;
        Circle(const Circle& other) noexcept;
        Circle(const Uuid& uuid, const Circle& other) noexcept;
        Circle(const Uuid& uuid, const QString& layerName, const Length& lineWidth,
                bool fill, bool isGrabArea, const Point& center, const Length& radiusX,
                const Length& radiusY, const Angle& rotation) noexcept;
        explicit Circle(const SExpression& node);
        virtual ~Circle() noexcept;

        // Getters
        const Uuid& getUuid() const noexcept {return mUuid;}
        const QString& getLayerName() const noexcept {return mLayerName;}
        const Length& getLineWidth() const noexcept {return mLineWidth;}
        bool isFilled() const noexcept {return mIsFilled;}
        bool isGrabArea() const noexcept {return mIsGrabArea;}
        const Point& getCenter() const noexcept {return mCenter;}
        const Length& getRadiusX() const noexcept {return mRadiusX;}
        const Length& getRadiusY() const noexcept {return mRadiusY;}
        const Angle& getRotation() const noexcept {return mRotation;}
        bool isRound() const noexcept {return mRadiusX == mRadiusY;}

        // Setters
        void setLayerName(const QString& name) noexcept;
        void setLineWidth(const Length& width) noexcept;
        void setIsFilled(bool isFilled) noexcept;
        void setIsGrabArea(bool isGrabArea) noexcept;
        void setCenter(const Point& center) noexcept;
        void setRadiusX(const Length& radius) noexcept;
        void setRadiusY(const Length& radius) noexcept;
        void setRotation(const Angle& rotation) noexcept;

        // Transformations
        Circle& translate(const Point& offset) noexcept;
        Circle& rotate(const Angle& angle, const Point& center = Point(0, 0)) noexcept;
        Circle& mirror(Qt::Orientation orientation, const Point& center = Point(0, 0)) noexcept;

        // General Methods
        void registerObserver(IF_CircleObserver& object) const noexcept;
        void unregisterObserver(IF_CircleObserver& object) const noexcept;

        /// @copydoc librepcb::SerializableObject::serialize()
        void serialize(SExpression& root) const override;

        // Operator Overloadings
        bool operator==(const Circle& rhs) const noexcept;
        bool operator!=(const Circle& rhs) const noexcept {return !(*this == rhs);}
        Circle& operator=(const Circle& rhs) noexcept;


    private: // Methods
        bool checkAttributesValidity() const noexcept;


    private: // Data
        Uuid mUuid;
        QString mLayerName;
        Length mLineWidth;
        bool mIsFilled;
        bool mIsGrabArea;
        Point mCenter;
        Length mRadiusX; // TODO: change radius (x/y) to size (width/height)
        Length mRadiusY; // TODO: change radius (x/y) to size (width/height)
        Angle mRotation;

        // Misc
        mutable QSet<IF_CircleObserver*> mObservers; ///< A list of all observer objects
};

/*****************************************************************************************
 *  Class CircleList
 ****************************************************************************************/

struct CircleListNameProvider {static constexpr const char* tagname = "circle";};
using CircleList = SerializableObjectList<Circle, CircleListNameProvider>;
using CmdCircleInsert = CmdListElementInsert<Circle, CircleListNameProvider>;
using CmdCircleRemove = CmdListElementRemove<Circle, CircleListNameProvider>;
using CmdCirclesSwap = CmdListElementsSwap<Circle, CircleListNameProvider>;

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_CIRCLE_H
