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

#ifndef PROJECT_SI_NETLABEL_H
#define PROJECT_SI_NETLABEL_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "si_base.h"
#include <librepcbcommon/fileio/if_xmlserializableobject.h>
#include "../graphicsitems/sgi_netlabel.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

namespace project {
class Circuit;
class Schematic;
class NetSignal;
}

/*****************************************************************************************
 *  Class SI_NetLabel
 ****************************************************************************************/

namespace project {

/**
 * @brief The SI_NetLabel class
 */
class SI_NetLabel final : public SI_Base, public IF_XmlSerializableObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit SI_NetLabel(Schematic& schematic, const XmlDomElement& domElement) throw (Exception);
        explicit SI_NetLabel(Schematic& schematic, NetSignal& netsignal, const Point& position) throw (Exception);
        ~SI_NetLabel() noexcept;

        // Getters
        Project& getProject() const noexcept;
        Schematic& getSchematic() const noexcept {return mSchematic;}
        const QUuid& getUuid() const noexcept {return mUuid;}
        const Angle& getRotation() const noexcept {return mRotation;}
        NetSignal& getNetSignal() const noexcept {return *mNetSignal;}

        // Setters
        void setNetSignal(NetSignal& netsignal) noexcept;
        void setPosition(const Point& position) noexcept;
        void setRotation(const Angle& rotation) noexcept;

        // General Methods
        void addToSchematic(GraphicsScene& scene) throw (Exception);
        void removeFromSchematic(GraphicsScene& scene) throw (Exception);

        /// @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;


        // Inherited from SI_Base
        Type_t getType() const noexcept override {return SI_Base::Type_t::NetLabel;}
        const Point& getPosition() const noexcept override {return mPosition;}
        QPainterPath getGrabAreaScenePx() const noexcept override;
        void setSelected(bool selected) noexcept override;


    private slots:

        void netSignalNameChanged(const QString& newName) noexcept;


    private:

        // make some methods inaccessible...
        SI_NetLabel() = delete;
        SI_NetLabel(const SI_NetLabel& other) = delete;
        SI_NetLabel& operator=(const SI_NetLabel& rhs) = delete;

        // Private Methods
        void init() throw (Exception);

        /// @copydoc IF_XmlSerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;


        // General
        Schematic& mSchematic;
        SGI_NetLabel* mGraphicsItem;

        // Attributes
        QUuid mUuid;
        Point mPosition;
        Angle mRotation;
        NetSignal* mNetSignal;
};

} // namespace project

#endif // PROJECT_SI_NETLABEL_H
