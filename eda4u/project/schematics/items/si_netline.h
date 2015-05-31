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

#ifndef PROJECT_SI_NETLINE_H
#define PROJECT_SI_NETLINE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "si_base.h"
#include <eda4ucommon/fileio/if_xmlserializableobject.h>
#include "../graphicsitems/sgi_netline.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

namespace project {
class NetSignal;
class Schematic;
class SI_NetPoint;
}

/*****************************************************************************************
 *  Class SI_NetLine
 ****************************************************************************************/

namespace project {

/**
 * @brief The SI_NetLine class
 */
class SI_NetLine final : public SI_Base, public IF_XmlSerializableObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit SI_NetLine(Schematic& schematic, const XmlDomElement& domElement) throw (Exception);
        explicit SI_NetLine(Schematic& schematic, SI_NetPoint& startPoint,
                            SI_NetPoint& endPoint, const Length& width) throw (Exception);
        ~SI_NetLine() noexcept;

        // Getters
        Schematic& getSchematic() const noexcept {return mSchematic;}
        const QUuid& getUuid() const noexcept {return mUuid;}
        const Length& getWidth() const noexcept {return mWidth;}
        SI_NetPoint& getStartPoint() const noexcept {return *mStartPoint;}
        SI_NetPoint& getEndPoint() const noexcept {return *mEndPoint;}
        NetSignal* getNetSignal() const noexcept;
        bool isAttachedToSymbol() const noexcept;

        // Setters
        void setWidth(const Length& width) noexcept;

        // General Methods
        void updateLine() noexcept;
        void addToSchematic(GraphicsScene& scene) throw (Exception);
        void removeFromSchematic(GraphicsScene& scene) throw (Exception);
        XmlDomElement* serializeToXmlDomElement() const throw (Exception);

        // Inherited from SI_Base
        Type_t getType() const noexcept override {return SI_Base::Type_t::NetLine;}
        const Point& getPosition() const noexcept override {return mPosition;}
        QPainterPath getGrabAreaScenePx() const noexcept override;
        void setSelected(bool selected) noexcept override;


    private:

        // make some methods inaccessible...
        SI_NetLine();
        SI_NetLine(const SI_NetLine& other);
        SI_NetLine& operator=(const SI_NetLine& rhs);

        // Private Methods
        void init() throw (Exception);
        bool checkAttributesValidity() const noexcept;


        // General
        Schematic& mSchematic;
        SGI_NetLine* mGraphicsItem;
        Point mPosition; ///< the center of startpoint and endpoint

        // Attributes
        QUuid mUuid;
        SI_NetPoint* mStartPoint;
        SI_NetPoint* mEndPoint;
        Length mWidth;
};

} // namespace project

#endif // PROJECT_SI_NETLINE_H
