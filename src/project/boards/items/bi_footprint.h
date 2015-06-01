/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
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
#include "../../../common/file_io/if_xmlserializableobject.h"
#include "../../../common/if_attributeprovider.h"
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
class BI_Footprint final : public BI_Base, public IF_XmlSerializableObject/*,
                           public IF_AttributeProvider*/
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit BI_Footprint(ComponentInstance& component, const XmlDomElement& domElement) throw (Exception);
        ~BI_Footprint() noexcept;

        // Getters
        ComponentInstance& getComponentInstance() const noexcept {return mComponentInstance;}
        const library::Footprint& getLibFootprint() const noexcept {return *mFootprint;}

        // General Methods
        void addToBoard(GraphicsScene& scene) throw (Exception);
        void removeFromBoard(GraphicsScene& scene) throw (Exception);
        XmlDomElement* serializeToXmlDomElement() const throw (Exception);

        // Helper Methods
        //Point mapToScene(const Point& relativePos) const noexcept;
        //bool getAttributeValue(const QString& attrNS, const QString& attrKey,
        //                       bool passToParents, QString& value) const noexcept;

        // Inherited from BI_Base
        Type_t getType() const noexcept override {return BI_Base::Type_t::Footprint;}
        const Point& getPosition() const noexcept override {return mPosition;}
        QPainterPath getGrabAreaScenePx() const noexcept override;
        void setSelected(bool selected) noexcept override;


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

        Point mPosition;
};

} // namespace project

#endif // PROJECT_BI_FOOTPRINT_H
