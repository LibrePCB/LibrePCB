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

#ifndef LIBREPCB_PROJECT_BI_FOOTPRINT_H
#define LIBREPCB_PROJECT_BI_FOOTPRINT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "bi_base.h"
#include <librepcb/common/fileio/serializableobject.h>
#include <librepcb/common/attributes/attributeprovider.h>
#include "bi_stroketext.h"
#include "../graphicsitems/bgi_footprint.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

namespace library {
class Footprint;
}

namespace project {

class Board;
class BI_Device;
class BI_FootprintPad;

/*****************************************************************************************
 *  Class BI_Footprint
 ****************************************************************************************/

/**
 * @brief The BI_Footprint class
 *
 * @author ubruhin
 * @date 2015-05-24
 */
class BI_Footprint final : public BI_Base, public SerializableObject,
                           public AttributeProvider
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        BI_Footprint() = delete;
        BI_Footprint(const BI_Footprint& other) = delete;
        BI_Footprint(BI_Device& device, const BI_Footprint& other);
        BI_Footprint(BI_Device& device, const SExpression& node);
        explicit BI_Footprint(BI_Device& device);
        ~BI_Footprint() noexcept;

        // Getters
        const Uuid& getComponentInstanceUuid() const noexcept;
        BI_Device& getDeviceInstance() const noexcept {return mDevice;}
        BI_FootprintPad* getPad(const Uuid& padUuid) const noexcept {return mPads.value(padUuid);}
        const QMap<Uuid, BI_FootprintPad*>& getPads() const noexcept {return mPads;}
        const library::Footprint& getLibFootprint() const noexcept;
        const Angle& getRotation() const noexcept;
        bool isSelectable() const noexcept override;
        bool isUsed() const noexcept;
        BGI_Footprint& getGraphicsItem() noexcept {return *mGraphicsItem;}

        // StrokeText Methods
        const QList<BI_StrokeText*>& getStrokeTexts() const noexcept {return mStrokeTexts;}
        void addStrokeText(BI_StrokeText& text);
        void removeStrokeText(BI_StrokeText& text);

        // General Methods
        void resetStrokeTextsToLibraryFootprint();
        void addToBoard() override;
        void removeFromBoard() override;

        /// @copydoc librepcb::SerializableObject::serialize()
        void serialize(SExpression& root) const override;

        // Helper Methods
        Point mapToScene(const Point& relativePos) const noexcept;

        // Inherited from AttributeProvider
        /// @copydoc librepcb::AttributeProvider::getAttributeProviderParents()
        QVector<const AttributeProvider*> getAttributeProviderParents() const noexcept override;

        // Inherited from BI_Base
        Type_t getType() const noexcept override {return BI_Base::Type_t::Footprint;}
        const Point& getPosition() const noexcept override;
        bool getIsMirrored() const noexcept override;
        QPainterPath getGrabAreaScenePx() const noexcept override;
        void setSelected(bool selected) noexcept override;

        // Operator Overloadings
        BI_Footprint& operator=(const BI_Footprint& rhs) = delete;


    private slots:
        void deviceInstanceAttributesChanged();
        void deviceInstanceMoved(const Point& pos);
        void deviceInstanceRotated(const Angle& rot);
        void deviceInstanceMirrored(bool mirrored);


    signals:
        /// @copydoc AttributeProvider::attributesChanged()
        void attributesChanged() override;


    private:
        void init();
        void updateGraphicsItemTransform() noexcept;

        // General
        BI_Device& mDevice;
        QScopedPointer<BGI_Footprint> mGraphicsItem;
        QMap<Uuid, BI_FootprintPad*> mPads; ///< key: footprint pad UUID
        QList<BI_StrokeText*> mStrokeTexts;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_BI_FOOTPRINT_H
