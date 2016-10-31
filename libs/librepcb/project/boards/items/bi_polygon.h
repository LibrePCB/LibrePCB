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

#ifndef LIBREPCB_PROJECT_BI_POLYGON_H
#define LIBREPCB_PROJECT_BI_POLYGON_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "bi_base.h"
#include <librepcbcommon/fileio/if_xmlserializableobject.h>
#include <librepcbcommon/if_attributeprovider.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class Polygon;

namespace project {

class Project;
class Board;
class BGI_Polygon;

/*****************************************************************************************
 *  Class BI_Polygon
 ****************************************************************************************/

/**
 * @brief The BI_Polygon class
 *
 * @author ubruhin
 * @date 2016-01-12
 */
class BI_Polygon final : public BI_Base, public IF_XmlSerializableObject,
                         public IF_AttributeProvider
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        BI_Polygon() = delete;
        BI_Polygon(const BI_Polygon& other) = delete;
        BI_Polygon(Board& board, const BI_Polygon& other) throw (Exception);
        BI_Polygon(Board& board, const XmlDomElement& domElement) throw (Exception);
        BI_Polygon(Board& board, int layerId, const Length& lineWidth, bool fill,
                   bool isGrabArea, const Point& startPos) throw (Exception);
        ~BI_Polygon() noexcept;

        // Getters
        const Polygon& getPolygon() const noexcept {return *mPolygon;}
        bool isSelectable() const noexcept override;

        // General Methods
        void addToBoard(GraphicsScene& scene) throw (Exception) override;
        void removeFromBoard(GraphicsScene& scene) throw (Exception) override;

        /// @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;

        bool getAttributeValue(const QString& attrNS, const QString& attrKey,
                               bool passToParents, QString& value) const noexcept override;

        // Inherited from BI_Base
        Type_t getType() const noexcept override {return BI_Base::Type_t::Polygon;}
        const Point& getPosition() const noexcept override {static Point p(0, 0); return p;}
        bool getIsMirrored() const noexcept override {return false;}
        QPainterPath getGrabAreaScenePx() const noexcept override;
        void setSelected(bool selected) noexcept override;

        // Operator Overloadings
        BI_Polygon& operator=(const BI_Polygon& rhs) = delete;


    private slots:

        void boardAttributesChanged();


    signals:

        /// @copydoc IF_AttributeProvider#attributesChanged()
        void attributesChanged() override;


    private:

        void init() throw (Exception);

        /// @copydoc IF_XmlSerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;


        // General
        QScopedPointer<Polygon> mPolygon;
        QScopedPointer<BGI_Polygon> mGraphicsItem;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_BI_POLYGON_H
