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

#ifndef LIBREPCB_LIBRARY_FOOTPRINT_H
#define LIBREPCB_LIBRARY_FOOTPRINT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcb/common/fileio/serializableobject.h>
#include <librepcb/common/geometry/polygon.h>
#include <librepcb/common/geometry/ellipse.h>
#include <librepcb/common/geometry/text.h>
#include <librepcb/common/geometry/hole.h>
#include "footprintpadsmt.h"
#include "footprintpadtht.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Class Footprint
 ****************************************************************************************/

/**
 * @brief The Footprint class
 */
class Footprint final : public SerializableObject
{
        Q_DECLARE_TR_FUNCTIONS(Footprint)

    public:

        // Constructors / Destructor
        explicit Footprint(const Uuid& uuid, const QString& name_en_US,
                           const QString& description_en_US) throw (Exception);
        explicit Footprint(const DomElement& domElement) throw (Exception);
        ~Footprint() noexcept;

        // Getters: Attributes
        const Uuid& getUuid() const noexcept {return mUuid;}
        QString getName(const QStringList& localeOrder) const noexcept;
        QString getDescription(const QStringList& localeOrder) const noexcept;
        const QMap<QString, QString>& getNames() const noexcept {return mNames;}
        const QMap<QString, QString>& getDescriptions() const noexcept {return mDescriptions;}

        // FootprintPad Methods
        const QMap<Uuid, FootprintPad*>& getPads() noexcept {return mPads;}
        QList<Uuid> getPadUuids() const noexcept {return mPads.keys();}
        FootprintPad* getPadByUuid(const Uuid& uuid) noexcept {return mPads.value(uuid);}
        const FootprintPad* getPadByUuid(const Uuid& uuid) const noexcept {return mPads.value(uuid);}
        void addPad(FootprintPad& pad) noexcept;
        void removePad(FootprintPad& pad) noexcept;

        // Polygon Methods
        const QList<Polygon*>& getPolygons() noexcept {return mPolygons;}
        int getPolygonCount() const noexcept {return mPolygons.count();}
        Polygon* getPolygon(int index) noexcept {return mPolygons.value(index);}
        const Polygon* getPolygon(int index) const noexcept {return mPolygons.value(index);}
        void addPolygon(Polygon& polygon) noexcept;
        void removePolygon(Polygon& polygon) noexcept;

        // Ellipse Methods
        const QList<Ellipse*>& getEllipses() noexcept {return mEllipses;}
        int getEllipseCount() const noexcept {return mEllipses.count();}
        Ellipse* getEllipse(int index) noexcept {return mEllipses.value(index);}
        const Ellipse* getEllipse(int index) const noexcept {return mEllipses.value(index);}
        void addEllipse(Ellipse& ellipse) noexcept;
        void removeEllipse(Ellipse& ellipse) noexcept;

        // Text Methods
        const QList<Text*>& getTexts() noexcept {return mTexts;}
        int getTextCount() const noexcept {return mTexts.count();}
        Text* getText(int index) noexcept {return mTexts.value(index);}
        const Text* getText(int index) const noexcept {return mTexts.value(index);}
        void addText(Text& text) noexcept;
        void removeText(Text& text) noexcept;

        // Hole Methods
        const QList<Hole*>& getHoles() noexcept {return mHoles;}
        int getHoleCount() const noexcept {return mHoles.count();}
        Hole* getHole(int index) noexcept {return mHoles.value(index);}
        const Hole* getHole(int index) const noexcept {return mHoles.value(index);}
        void addHole(Hole& hole) noexcept;
        void removeHole(Hole& hole) noexcept;


        // General Methods

        /// @copydoc librepcb::SerializableObject::serialize()
        void serialize(DomElement& root) const throw (Exception) override;


    private:

        // make some methods inaccessible...
        Footprint() = delete;
        Footprint(const Footprint& other) = delete;
        Footprint& operator=(const Footprint& rhs) = delete;


        // Private Methods
        bool checkAttributesValidity() const noexcept;


        // Footprint Attributes
        Uuid mUuid;
        QMap<QString, QString> mNames;
        QMap<QString, QString> mDescriptions;
        QMap<Uuid, FootprintPad*> mPads;
        QList<Polygon*> mPolygons;
        QList<Ellipse*> mEllipses;
        QList<Text*> mTexts;
        QList<Hole*> mHoles;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb

#endif // LIBREPCB_LIBRARY_FOOTPRINT_H
