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

#ifndef PROJECT_DEVICEINSTANCE_H
#define PROJECT_DEVICEINSTANCE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <librepcbcommon/uuid.h>
#include <librepcbcommon/units/all_length_units.h>
#include <librepcbcommon/if_attributeprovider.h>
#include "../erc/if_ercmsgprovider.h"
#include <librepcbcommon/fileio/if_xmlserializableobject.h>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class GraphicsScene;

namespace project {
class Project;
class Board;
class ComponentInstance;
class BI_Footprint;
}

namespace library {
class Device;
class Package;
class Footprint;
}

/*****************************************************************************************
 *  Class DeviceInstance
 ****************************************************************************************/

namespace project {

/**
 * @brief The DeviceInstance class
 */
class DeviceInstance final : public QObject, public IF_AttributeProvider,
                             public IF_ErcMsgProvider, public IF_XmlSerializableObject
{
        Q_OBJECT
        DECLARE_ERC_MSG_CLASS_NAME(DeviceInstance)

    public:

        // Constructors / Destructor
        explicit DeviceInstance(Board& board, const XmlDomElement& domElement) throw (Exception);
        explicit DeviceInstance(Board& board, ComponentInstance& compInstance,
                                const Uuid& deviceUuid, const Uuid& footprintUuid,
                                const Point& position = Point(),
                                const Angle& rotation = Angle()) throw (Exception);
        ~DeviceInstance() noexcept;

        // Getters
        Project& getProject() const noexcept;
        Board& getBoard() const noexcept {return mBoard;}
        ComponentInstance& getComponentInstance() const noexcept {return *mCompInstance;}
        const library::Device& getLibDevice() const noexcept {return *mLibDevice;}
        const library::Package& getLibPackage() const noexcept {return *mLibPackage;}
        const library::Footprint& getLibFootprint() const noexcept {return *mLibFootprint;}
        BI_Footprint& getFootprint() const noexcept {return *mFootprint;}
        const Angle& getRotation() const noexcept {return mRotation;}
        const Point& getPosition() const noexcept {return mPosition;}
        bool getIsMirrored() const noexcept {return mIsMirrored;}

        // Setters
        void setPosition(const Point& pos) noexcept;
        void setRotation(const Angle& rot) noexcept;
        void setIsMirrored(bool mirror) noexcept;

        // General Methods
        void addToBoard(GraphicsScene& scene) throw (Exception);
        void removeFromBoard(GraphicsScene& scene) throw (Exception);

        /// @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;

        // Helper Methods
        bool getAttributeValue(const QString& attrNS, const QString& attrKey,
                               bool passToParents, QString& value) const noexcept;


    signals:

        /// @copydoc IF_AttributeProvider#attributesChanged()
        void attributesChanged();

        void moved(const Point& newPos);
        void rotated(const Angle& newRotation);
        void mirrored(bool newIsMirrored);


    private:

        // make some methods inaccessible...
        DeviceInstance();
        DeviceInstance(const DeviceInstance& other);
        DeviceInstance& operator=(const DeviceInstance& rhs);

        // Private Methods
        void initDeviceAndPackageAndFootprint(const Uuid& deviceUuid,
                                              const Uuid& footprintUuid) throw (Exception);
        void init() throw (Exception);

        /// @copydoc IF_XmlSerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;

        void updateErcMessages() noexcept;


        // General
        Board& mBoard;
        bool mAddedToBoard;
        ComponentInstance* mCompInstance;
        const library::Device* mLibDevice;
        const library::Package* mLibPackage;
        const library::Footprint* mLibFootprint;
        BI_Footprint* mFootprint;

        // Attributes
        Point mPosition;
        Angle mRotation;
        bool mIsMirrored;
};

} // namespace project

#endif // PROJECT_DEVICEINSTANCE_H
