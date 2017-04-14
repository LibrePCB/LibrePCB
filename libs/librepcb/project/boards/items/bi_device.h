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

#ifndef LIBREPCB_PROJECT_BI_DEVICE_H
#define LIBREPCB_PROJECT_BI_DEVICE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "bi_base.h"
#include <librepcb/common/uuid.h>
#include <librepcb/common/if_attributeprovider.h>
#include "../../erc/if_ercmsgprovider.h"
#include <librepcb/common/fileio/serializableobject.h>
#include "../graphicsitems/bgi_footprint.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

namespace library {
class Device;
class Package;
class Footprint;
}

namespace project {

class Project;
class Board;
class ComponentInstance;
class BI_Footprint;

/*****************************************************************************************
 *  Class BI_Device
 ****************************************************************************************/

/**
 * @brief The BI_Device class
 */
class BI_Device final : public BI_Base, public IF_AttributeProvider,
                        public IF_ErcMsgProvider, public SerializableObject
{
        Q_OBJECT
        DECLARE_ERC_MSG_CLASS_NAME(BI_Device)

    public:

        // Constructors / Destructor
        BI_Device() = delete;
        BI_Device(const BI_Device& other) = delete;
        BI_Device(Board& board, const BI_Device& other) throw (Exception);
        BI_Device(Board& board, const XmlDomElement& domElement) throw (Exception);
        BI_Device(Board& board, ComponentInstance& compInstance, const Uuid& deviceUuid,
                  const Uuid& footprintUuid, const Point& position, const Angle& rotation,
                  bool mirror) throw (Exception);
        ~BI_Device() noexcept;

        // Getters
        const Uuid& getComponentInstanceUuid() const noexcept;
        ComponentInstance& getComponentInstance() const noexcept {return *mCompInstance;}
        const library::Device& getLibDevice() const noexcept {return *mLibDevice;}
        const library::Package& getLibPackage() const noexcept {return *mLibPackage;}
        const library::Footprint& getLibFootprint() const noexcept {return *mLibFootprint;}
        BI_Footprint& getFootprint() const noexcept {return *mFootprint;}
        const Angle& getRotation() const noexcept {return mRotation;}
        bool isSelectable() const noexcept override;
        bool isUsed() const noexcept;

        // Setters
        void setPosition(const Point& pos) noexcept;
        void setRotation(const Angle& rot) noexcept;
        void setIsMirrored(bool mirror) throw (Exception);

        // General Methods
        void addToBoard(GraphicsScene& scene) throw (Exception) override;
        void removeFromBoard(GraphicsScene& scene) throw (Exception) override;

        /// @copydoc SerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;

        // Helper Methods
        bool getAttributeValue(const QString& attrNS, const QString& attrKey,
                               bool passToParents, QString& value) const noexcept override;

        // Inherited from BI_Base
        Type_t getType() const noexcept override {return BI_Base::Type_t::Device;}
        const Point& getPosition() const noexcept override {return mPosition;}
        bool getIsMirrored() const noexcept override {return mIsMirrored;}
        QPainterPath getGrabAreaScenePx() const noexcept override;
        void setSelected(bool selected) noexcept override;

        // Operator Overloadings
        BI_Device& operator=(const BI_Device& rhs);


    signals:

        /// @copydoc IF_AttributeProvider#attributesChanged()
        void attributesChanged() override;

        void moved(const Point& newPos);
        void rotated(const Angle& newRotation);
        void mirrored(bool newIsMirrored);


    private:

        void initDeviceAndPackageAndFootprint(const Uuid& deviceUuid,
                                              const Uuid& footprintUuid) throw (Exception);
        void init() throw (Exception);

        /// @copydoc SerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;

        void updateErcMessages() noexcept;


        // General
        ComponentInstance* mCompInstance;
        const library::Device* mLibDevice;
        const library::Package* mLibPackage;
        const library::Footprint* mLibFootprint;
        QScopedPointer<BI_Footprint> mFootprint;

        // Attributes
        Point mPosition;
        Angle mRotation;
        bool mIsMirrored;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_BI_DEVICE_H
