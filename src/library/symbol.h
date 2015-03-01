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

#ifndef LIBRARY_SYMBOL_H
#define LIBRARY_SYMBOL_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "libraryelement.h"
#include "symbolpin.h"
#include "symbolpolygon.h"
#include "symboltext.h"

/*****************************************************************************************
 *  Class Symbol
 ****************************************************************************************/

namespace library {

/**
 * @brief The Symbol class
 */
class Symbol final : public LibraryElement
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit Symbol(const QUuid& uuid = QUuid::createUuid(),
                        const Version& version = Version(),
                        const QString& author = QString(),
                        const QString& name_en_US = QString(),
                        const QString& description_en_US = QString(),
                        const QString& keywords_en_US = QString()) throw (Exception);
        explicit Symbol(const FilePath& xmlFilePath) throw (Exception);
        ~Symbol() noexcept;

        // Getters
        const SymbolPin* getPinByUuid(const QUuid& uuid) const noexcept {return mPins.value(uuid);}
        const QHash<QUuid, const SymbolPin*>& getPins() const noexcept {return mPins;}
        const QList<const SymbolPolygon*>& getPolygons() const noexcept {return mPolygons;}
        const QList<const SymbolText*>& getTexts() const noexcept {return mTexts;}

        // General Methods
        void addPin(const SymbolPin* pin) noexcept {mPins.insert(pin->getUuid(), pin);}
        void addPolygon(const SymbolPolygon* polygon) noexcept {mPolygons.append(polygon);}
        void addText(const SymbolText* text) noexcept {mTexts.append(text);}
        void convertLineRectsToPolygonRects(bool fill, bool makeGrabArea) noexcept;


    private:

        // make some methods inaccessible...
        Symbol(const Symbol& other);
        Symbol& operator=(const Symbol& rhs);


        // Private Methods
        void parseDomTree(const XmlDomElement& root) throw (Exception);
        XmlDomElement* serializeToXmlDomElement() const throw (Exception);
        bool checkAttributesValidity() const noexcept;
        bool findLineRectangle(QList<const SymbolPolygon*>& lines) noexcept;
        bool findHLine(const QList<const SymbolPolygon*>& lines, Point& p, Length* width,
                       const SymbolPolygon** line) noexcept;
        bool findVLine(const QList<const SymbolPolygon*>& lines, Point& p, Length* width,
                       const SymbolPolygon** line) noexcept;


        // Symbol Attributes
        QHash<QUuid, const SymbolPin*> mPins;
        QList<const SymbolPolygon*> mPolygons;
        QList<const SymbolText*> mTexts;
};

} // namespace library

#endif // LIBRARY_SYMBOL_H
