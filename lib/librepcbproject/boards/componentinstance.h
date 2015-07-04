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

#ifndef PROJECT_COMPONENTINSTANCE_H
#define PROJECT_COMPONENTINSTANCE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <librepcbcommon/units/all_length_units.h>
#include <librepcbcommon/if_attributeprovider.h>
#include "../erc/if_ercmsgprovider.h"
#include <librepcbcommon/fileio/if_xmlserializableobject.h>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class Workspace;
class GraphicsScene;

namespace project {
class Project;
class Board;
class GenCompInstance;
class BI_Footprint;
}

namespace library {
class Component;
class Package;
}

/*****************************************************************************************
 *  Class ComponentInstance
 ****************************************************************************************/

namespace project {

/**
 * @brief The ComponentInstance class
 */
class ComponentInstance final : public QObject, public IF_AttributeProvider,
                                public IF_ErcMsgProvider, public IF_XmlSerializableObject
{
        Q_OBJECT
        DECLARE_ERC_MSG_CLASS_NAME(ComponentInstance)

    public:

        // Constructors / Destructor
        explicit ComponentInstance(Board& board, const XmlDomElement& domElement) throw (Exception);
        explicit ComponentInstance(Board& board, GenCompInstance& genCompInstance,
                                   const QUuid& componentUuid,
                                   const Point& position = Point(),
                                   const Angle& rotation = Angle()) throw (Exception);
        ~ComponentInstance() noexcept;

        // Getters
        Workspace& getWorkspace() const noexcept;
        Project& getProject() const noexcept;
        Board& getBoard() const noexcept {return mBoard;}
        GenCompInstance& getGenCompInstance() const noexcept {return *mGenCompInstance;}
        const library::Component& getLibComponent() const noexcept {return *mComponent;}
        const library::Package& getLibPackage() const noexcept {return *mPackage;}
        BI_Footprint& getFootprint() const noexcept {return *mFootprint;}
        const Angle& getRotation() const noexcept {return mRotation;}
        const Point& getPosition() const noexcept {return mPosition;}

        // Setters
        void setPosition(const Point& pos) noexcept;
        void setRotation(const Angle& rot) noexcept;

        // General Methods
        void addToBoard(GraphicsScene& scene) throw (Exception);
        void removeFromBoard(GraphicsScene& scene) throw (Exception);

        /// @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement(uint version) const throw (Exception) override;

        // Helper Methods
        bool getAttributeValue(const QString& attrNS, const QString& attrKey,
                               bool passToParents, QString& value) const noexcept;


    signals:

        /// @copydoc IF_AttributeProvider#attributesChanged()
        void attributesChanged();

        void moved(const Point& newPos);
        void rotated(const Angle& newRotation);


    private:

        // make some methods inaccessible...
        ComponentInstance();
        ComponentInstance(const ComponentInstance& other);
        ComponentInstance& operator=(const ComponentInstance& rhs);

        // Private Methods
        void initComponentAndPackage(const QUuid& componentUuid) throw (Exception);
        void init() throw (Exception);

        /// @copydoc IF_XmlSerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;

        void updateErcMessages() noexcept;


        // General
        Board& mBoard;
        bool mAddedToBoard;
        GenCompInstance* mGenCompInstance;
        const library::Component* mComponent;
        const library::Package* mPackage;
        BI_Footprint* mFootprint;

        // Attributes
        Point mPosition;
        Angle mRotation;
};

} // namespace project

#endif // PROJECT_COMPONENTINSTANCE_H
