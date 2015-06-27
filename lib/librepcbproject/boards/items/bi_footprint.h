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

#ifndef PROJECT_BI_FOOTPRINT_H
#define PROJECT_BI_FOOTPRINT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "bi_base.h"
#include <librepcbcommon/fileio/if_xmlserializableobject.h>
#include <librepcbcommon/if_attributeprovider.h>
#include "../graphicsitems/bgi_footprint.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

namespace project {
class Board;
class ComponentInstance;
class BI_FootprintPad;
}

namespace library {
class Footprint;
}

/*****************************************************************************************
 *  Class BI_Footprint
 ****************************************************************************************/

namespace project {

/**
 * @brief The BI_Footprint class
 *
 * @author ubruhin
 * @date 2015-05-24
 */
class BI_Footprint final : public BI_Base, public IF_XmlSerializableObject,
                           public IF_AttributeProvider
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit BI_Footprint(ComponentInstance& component, const XmlDomElement& domElement) throw (Exception);
        explicit BI_Footprint(ComponentInstance& component) throw (Exception);
        ~BI_Footprint() noexcept;

        // Getters
        ComponentInstance& getComponentInstance() const noexcept {return mComponentInstance;}
        BI_FootprintPad* getPad(const QUuid& padUuid) const noexcept {return mPads.value(padUuid);}
        const QHash<QUuid, BI_FootprintPad*>& getPads() const noexcept {return mPads;}
        const library::Footprint& getLibFootprint() const noexcept {return *mFootprint;}
        const Angle& getRotation() const noexcept;

        // General Methods
        void addToBoard(GraphicsScene& scene) throw (Exception);
        void removeFromBoard(GraphicsScene& scene) throw (Exception);
        XmlDomElement* serializeToXmlDomElement() const throw (Exception);

        // Helper Methods
        Point mapToScene(const Point& relativePos) const noexcept;
        bool getAttributeValue(const QString& attrNS, const QString& attrKey,
                               bool passToParents, QString& value) const noexcept;

        // Inherited from BI_Base
        Type_t getType() const noexcept override {return BI_Base::Type_t::Footprint;}
        const Point& getPosition() const noexcept override;
        QPainterPath getGrabAreaScenePx() const noexcept override;
        void setSelected(bool selected) noexcept override;


    private slots:

        void componentInstanceAttributesChanged();
        void componentInstanceMoved(const Point& pos);
        void componentInstanceRotated(const Angle& rot);


    signals:

        /// @copydoc IF_AttributeProvider#attributesChanged()
        void attributesChanged();


    private:

        // make some methods inaccessible...
        BI_Footprint();
        BI_Footprint(const BI_Footprint& other);
        BI_Footprint& operator=(const BI_Footprint& rhs);

        // Private Methods
        void init() throw (Exception);
        bool checkAttributesValidity() const noexcept;


        // General
        ComponentInstance& mComponentInstance;
        const library::Footprint* mFootprint;
        QHash<QUuid, BI_FootprintPad*> mPads; ///< key: footprint pad UUID
        BGI_Footprint* mGraphicsItem;
};

} // namespace project

#endif // PROJECT_BI_FOOTPRINT_H
