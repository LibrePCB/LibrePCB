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

#ifndef LIBRARY_GENCOMPSYMBVAR_H
#define LIBRARY_GENCOMPSYMBVAR_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "../common/exceptions.h"
#include "gencompsymbvaritem.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class XmlDomElement;

namespace library {
class GenericComponent;
}

/*****************************************************************************************
 *  Class GenCompSymbVar
 ****************************************************************************************/

namespace library {

/**
 * @brief The GenCompSymbVar class
 */
class GenCompSymbVar final : public QObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit GenCompSymbVar(GenericComponent& genComp,
                                const XmlDomElement& domElement) throw (Exception);
        ~GenCompSymbVar() noexcept;

        // Getters: Attributes
        const QUuid& getUuid() const noexcept {return mUuid;}
        const QString& getNorm() const noexcept {return mNorm;}
        bool isDefault() const noexcept {return mIsDefault;}
        QString getName(const QString& locale = QString()) const noexcept;
        QString getDescription(const QString& locale = QString()) const noexcept;
        const QHash<QString, QString>& getNames() const noexcept {return mNames;}
        const QHash<QString, QString>& getDescriptions() const noexcept {return mDescriptions;}

        // Getters: Symbol Items
        const QHash<QUuid, const GenCompSymbVarItem*>& getItems() const noexcept {return mSymbolItems;}
        const GenCompSymbVarItem* getItemByUuid(const QUuid& uuid) const noexcept {return mSymbolItems.value(uuid, 0);}
        const GenCompSymbVarItem* getItemByAddOrderIndex(unsigned int index) const noexcept;

    private:

        // make some methods inaccessible...
        GenCompSymbVar();
        GenCompSymbVar(const GenCompSymbVar& other);
        GenCompSymbVar& operator=(const GenCompSymbVar& rhs);


        // General Attributes
        GenericComponent& mGenericComponent;

        // Symbol Variant Attributes
        QUuid mUuid;
        QString mNorm;
        bool mIsDefault;
        QHash<QString, QString> mNames;
        QHash<QString, QString> mDescriptions;
        QHash<QUuid, const GenCompSymbVarItem*> mSymbolItems; ///< minimum one item
};

} // namespace library

#endif // LIBRARY_GENCOMPSYMBVAR_H
