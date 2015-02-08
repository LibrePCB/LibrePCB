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
        explicit Symbol(const FilePath& xmlFilePath) throw (Exception);
        ~Symbol() noexcept;

        // Getters
        const SymbolPin* getPinByUuid(const QUuid& uuid) const noexcept {return mPins.value(uuid);}
        const QHash<QUuid, const SymbolPin*>& getPins() const noexcept {return mPins;}
        const QList<const SymbolPolygon*>& getPolygons() const noexcept {return mPolygons;}
        const QList<const SymbolText*>& getTexts() const noexcept {return mTexts;}


    private:

        // make some methods inaccessible...
        Symbol();
        Symbol(const Symbol& other);
        Symbol& operator=(const Symbol& rhs);


        // Private Methods
        void parseDomTree(const XmlDomElement& root) throw (Exception);


        // Symbol Attributes
        QHash<QUuid, const SymbolPin*> mPins;
        QList<const SymbolPolygon*> mPolygons;
        QList<const SymbolText*> mTexts;
};

} // namespace library

#endif // LIBRARY_SYMBOL_H
