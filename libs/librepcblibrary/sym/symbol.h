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

#ifndef LIBREPCB_LIBRARY_SYMBOL_H
#define LIBREPCB_LIBRARY_SYMBOL_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcbcommon/geometry/polygon.h>
#include <librepcbcommon/geometry/ellipse.h>
#include <librepcbcommon/geometry/text.h>
#include "../libraryelement.h"
#include "symbolpin.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace library {

/*****************************************************************************************
 *  Class Symbol
 ****************************************************************************************/

/**
 * @brief The Symbol class
 */
class Symbol final : public LibraryElement
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        Symbol() = delete;
        Symbol(const Symbol& other) = delete;
        explicit Symbol(const Uuid& uuid, const Version& version, const QString& author,
                        const QString& name_en_US, const QString& description_en_US,
                        const QString& keywords_en_US) throw (Exception);
        explicit Symbol(const FilePath& elementDirectory, bool readOnly) throw (Exception);
        ~Symbol() noexcept;

        // SymbolPin Methods
        const QMap<Uuid, SymbolPin*>& getPins() noexcept {return mPins;}
        QList<Uuid> getPinUuids() const noexcept {return mPins.keys();}
        SymbolPin* getPinByUuid(const Uuid& uuid) noexcept {return mPins.value(uuid);}
        const SymbolPin* getPinByUuid(const Uuid& uuid) const noexcept {return mPins.value(uuid);}
        void addPin(SymbolPin& pin) noexcept;
        void removePin(SymbolPin& pin) noexcept;

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

        // Operator Overloadings
        Symbol& operator=(const Symbol& rhs) = delete;


    private:

        // Private Methods

        /// @copydoc #IF_XmlSerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;


        // Symbol Attributes
        QMap<Uuid, SymbolPin*> mPins;
        QList<Polygon*> mPolygons;
        QList<Ellipse*> mEllipses;
        QList<Text*> mTexts;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb

#endif // LIBREPCB_LIBRARY_SYMBOL_H
