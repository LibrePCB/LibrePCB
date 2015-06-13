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

#ifndef LIBRARY_FOOTPRINT_H
#define LIBRARY_FOOTPRINT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "../libraryelement.h"
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
class Footprint final : public LibraryElement
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit Footprint(const QUuid& uuid = QUuid::createUuid(),
                           const Version& version = Version(),
                           const QString& author = QString(),
                           const QString& name_en_US = QString(),
                           const QString& description_en_US = QString(),
                           const QString& keywords_en_US = QString()) throw (Exception);
        explicit Footprint(const FilePath& xmlFilePath) throw (Exception);
        ~Footprint() noexcept;

        // Getters
        const FootprintPad* getPadByUuid(const QUuid& uuid) const noexcept {return mPads.value(uuid);}
        const QHash<QUuid, const FootprintPad*>& getPads() const noexcept {return mPads;}
        const QList<const FootprintPolygon*>& getPolygons() const noexcept {return mPolygons;}
        const QList<const FootprintText*>& getTexts() const noexcept {return mTexts;}
        const QList<const FootprintEllipse*>& getEllipses() const noexcept {return mEllipses;}

        // General Methods
        void addPad(const FootprintPad* pad) noexcept {mPads.insert(pad->getUuid(), pad);}
        void addPolygon(const FootprintPolygon* polygon) noexcept {mPolygons.append(polygon);}
        void removePolygon(const FootprintPolygon* polygon) noexcept {mPolygons.removeAll(polygon); delete polygon;}
        void addText(const FootprintText* text) noexcept {mTexts.append(text);}
        void addEllipse(const FootprintEllipse* ellipse) noexcept {mEllipses.append(ellipse);}


    private:

        // make some methods inaccessible...
        Footprint();
        Footprint(const Footprint& other);
        Footprint& operator=(const Footprint& rhs);


        // Private Methods
        void parseDomTree(const XmlDomElement& root) throw (Exception);
        XmlDomElement* serializeToXmlDomElement() const throw (Exception);
        bool checkAttributesValidity() const noexcept;


        // Symbol Attributes
        QHash<QUuid, const FootprintPad*> mPads;
        QList<const FootprintPolygon*> mPolygons;
        QList<const FootprintText*> mTexts;
        QList<const FootprintEllipse*> mEllipses;
};

} // namespace library

#endif // LIBRARY_FOOTPRINT_H
