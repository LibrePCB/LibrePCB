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
#include <librepcb/common/if_attributeprovider.h>
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
                           public IF_AttributeProvider
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        BI_Footprint() = delete;
        BI_Footprint(const BI_Footprint& other) = delete;
        BI_Footprint(BI_Device& device, const BI_Footprint& other) throw (Exception);
        BI_Footprint(BI_Device& device, const XmlDomElement& domElement) throw (Exception);
        explicit BI_Footprint(BI_Device& device) throw (Exception);
        ~BI_Footprint() noexcept;

        // Getters
        const Uuid& getComponentInstanceUuid() const noexcept;
        BI_Device& getDeviceInstance() const noexcept {return mDevice;}
        BI_FootprintPad* getPad(const Uuid& padUuid) const noexcept {return mPads.value(padUuid);}
        const QHash<Uuid, BI_FootprintPad*>& getPads() const noexcept {return mPads;}
        const library::Footprint& getLibFootprint() const noexcept;
        const Angle& getRotation() const noexcept;
        bool isSelectable() const noexcept override;
        bool isUsed() const noexcept;

        // General Methods
        void addToBoard(GraphicsScene& scene) throw (Exception) override;
        void removeFromBoard(GraphicsScene& scene) throw (Exception) override;

        /// @copydoc librepcb::SerializableObject::serialize()
        void serialize(XmlDomElement& root) const throw (Exception) override;

        // Helper Methods
        Point mapToScene(const Point& relativePos) const noexcept;
        bool getAttributeValue(const QString& attrNS, const QString& attrKey,
                               bool passToParents, QString& value) const noexcept override;

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

        /// @copydoc IF_AttributeProvider#attributesChanged()
        void attributesChanged() override;


    private:

        void init() throw (Exception);
        void updateGraphicsItemTransform() noexcept;
        bool checkAttributesValidity() const noexcept;


        // General
        BI_Device& mDevice;
        QScopedPointer<BGI_Footprint> mGraphicsItem;
        QHash<Uuid, BI_FootprintPad*> mPads; ///< key: footprint pad UUID
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_BI_FOOTPRINT_H
