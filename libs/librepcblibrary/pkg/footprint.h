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

#ifndef LIBRARY_FOOTPRINT_H
#define LIBRARY_FOOTPRINT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <librepcbcommon/fileio/if_xmlserializableobject.h>
#include "footprintpad.h"
#include "footprintpolygon.h"
#include "footprintellipse.h"
#include "footprinttext.h"

/*****************************************************************************************
 *  Class Footprint
 ****************************************************************************************/

namespace library {

/**
 * @brief The Footprint class
 */
class Footprint final : public IF_XmlSerializableObject
{
        Q_DECLARE_TR_FUNCTIONS(Footprint)

    public:

        // Types
        struct FootprintHole_t {
            Point pos;
            Length diameter;
        };


        // Constructors / Destructor
        explicit Footprint(const Uuid& uuid = Uuid::createRandom(),
                           const QString& name_en_US = QString(),
                           const QString& description_en_US = QString(),
                           bool isDefault = false) throw (Exception);
        explicit Footprint(const XmlDomElement& domElement) throw (Exception);
        ~Footprint() noexcept;

        // Getters: Attributes
        const Uuid& getUuid() const noexcept {return mUuid;}
        bool isDefault() const noexcept {return mIsDefault;}
        QString getName(const QStringList& localeOrder) const noexcept;
        QString getDescription(const QStringList& localeOrder) const noexcept;
        const QMap<QString, QString>& getNames() const noexcept {return mNames;}
        const QMap<QString, QString>& getDescriptions() const noexcept {return mDescriptions;}

        // Getters: Items
        const FootprintPad* getPadByUuid(const Uuid& uuid) const noexcept {return mPads.value(uuid);}
        const QHash<Uuid, const FootprintPad*>& getPads() const noexcept {return mPads;}
        const QList<const FootprintPolygon*>& getPolygons() const noexcept {return mPolygons;}
        const QList<const FootprintText*>& getTexts() const noexcept {return mTexts;}
        const QList<const FootprintEllipse*>& getEllipses() const noexcept {return mEllipses;}

        // General Methods
        void addPad(const FootprintPad* pad) noexcept {mPads.insert(pad->getPadUuid(), pad);}
        void addPolygon(const FootprintPolygon* polygon) noexcept {mPolygons.append(polygon);}
        void removePolygon(const FootprintPolygon* polygon) noexcept {mPolygons.removeAll(polygon); delete polygon;}
        void addText(const FootprintText* text) noexcept {mTexts.append(text);}
        void addEllipse(const FootprintEllipse* ellipse) noexcept {mEllipses.append(ellipse);}
        void addHole(const FootprintHole_t* hole) noexcept {mHoles.append(hole);}

        // General Methods

        /// @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;


    private:

        // make some methods inaccessible...
        Footprint();
        Footprint(const Footprint& other);
        Footprint& operator=(const Footprint& rhs);


        // Private Methods

        /// @copydoc IF_XmlSerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;


        // Footprint Attributes
        Uuid mUuid;
        QMap<QString, QString> mNames;
        QMap<QString, QString> mDescriptions;
        bool mIsDefault;
        QHash<Uuid, const FootprintPad*> mPads;
        QList<const FootprintPolygon*> mPolygons;
        QList<const FootprintText*> mTexts;
        QList<const FootprintEllipse*> mEllipses;
        QList<const FootprintHole_t*> mHoles;
};

} // namespace library

#endif // LIBRARY_FOOTPRINT_H
