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

#ifndef LIBREPCB_ELLIPSE_H
#define LIBREPCB_ELLIPSE_H

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
 *  Interface IF_EllipseObserver
 ****************************************************************************************/

/**
 * @brief The IF_EllipseObserver class
 *
 * @author ubruhin
 * @date 2017-01-01
 */
class IF_EllipseObserver
{
    public:
        virtual void ellipseLayerNameChanged(const QString& newLayerName) noexcept = 0;
        virtual void ellipseLineWidthChanged(const Length& newLineWidth) noexcept = 0;
        virtual void ellipseIsFilledChanged(bool newIsFilled) noexcept = 0;
        virtual void ellipseIsGrabAreaChanged(bool newIsGrabArea) noexcept = 0;
        virtual void ellipseCenterChanged(const Point& newCenter) noexcept = 0;
        virtual void ellipseRadiusXChanged(const Length& newRadiusX) noexcept = 0;
        virtual void ellipseRadiusYChanged(const Length& newRadiusY) noexcept = 0;
        virtual void ellipseRotationChanged(const Angle& newRotation) noexcept = 0;

    protected:
        IF_EllipseObserver() noexcept {}
        explicit IF_EllipseObserver(const IF_EllipseObserver& other) = delete;
        virtual ~IF_EllipseObserver() noexcept {}
        IF_EllipseObserver& operator=(const IF_EllipseObserver& rhs) = delete;
};

/*****************************************************************************************
 *  Class Ellipse
 ****************************************************************************************/

/**
 * @brief The Ellipse class
 */
class Ellipse : public SerializableObject
{
        Q_DECLARE_TR_FUNCTIONS(Ellipse)

    public:

        // Constructors / Destructor
        Ellipse() = delete;
        Ellipse(const Ellipse& other) noexcept;
        Ellipse(const Uuid& uuid, const Ellipse& other) noexcept;
        Ellipse(const Uuid& uuid, const QString& layerName, const Length& lineWidth,
                bool fill, bool isGrabArea, const Point& center, const Length& radiusX,
                const Length& radiusY, const Angle& rotation) noexcept;
        explicit Ellipse(const SExpression& node);
        virtual ~Ellipse() noexcept;

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
        Ellipse& translate(const Point& offset) noexcept;
        Ellipse& rotate(const Angle& angle, const Point& center = Point(0, 0)) noexcept;

        // General Methods
        void registerObserver(IF_EllipseObserver& object) const noexcept;
        void unregisterObserver(IF_EllipseObserver& object) const noexcept;

        /// @copydoc librepcb::SerializableObject::serialize()
        void serialize(SExpression& root) const override;

        // Operator Overloadings
        bool operator==(const Ellipse& rhs) const noexcept;
        bool operator!=(const Ellipse& rhs) const noexcept {return !(*this == rhs);}
        Ellipse& operator=(const Ellipse& rhs) noexcept;


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
        mutable QSet<IF_EllipseObserver*> mObservers; ///< A list of all observer objects
};

/*****************************************************************************************
 *  Class EllipseList
 ****************************************************************************************/

struct EllipseListNameProvider {static constexpr const char* tagname = "ellipse";};
using EllipseList = SerializableObjectList<Ellipse, EllipseListNameProvider>;
using CmdEllipseInsert = CmdListElementInsert<Ellipse, EllipseListNameProvider>;
using CmdEllipseRemove = CmdListElementRemove<Ellipse, EllipseListNameProvider>;
using CmdEllipsesSwap = CmdListElementsSwap<Ellipse, EllipseListNameProvider>;

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_ELLIPSE_H
