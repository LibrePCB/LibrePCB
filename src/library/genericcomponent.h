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

#ifndef LIBRARY_GENERICCOMPONENT_H
#define LIBRARY_GENERICCOMPONENT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "libraryelement.h"
#include "gencompsignal.h"
#include "gencompsymbvar.h"

/*****************************************************************************************
 *  Class GenericComponent
 ****************************************************************************************/

namespace library {

/**
 * @brief The GenericComponent class
 */
class GenericComponent final : public LibraryElement
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit GenericComponent(const FilePath& xmlFilePath) throw (Exception);
        ~GenericComponent() noexcept;

        // Getters: Prefixes
        const QHash<QString, QString>& getPrefixes() const noexcept {return mPrefixes;}
        QString getPrefix(const QString& norm = QString()) const noexcept;
        const QString& getDefaultPrefixNorm() const noexcept {return mDefaultPrefixNorm;}
        QString getDefaultPrefix() const noexcept {return mPrefixes.value(mDefaultPrefixNorm);}

        // Getters: Signals
        const QHash<QUuid, const GenCompSignal*>& getSignals() const noexcept {return mSignals;}
        const GenCompSignal* getSignalByUuid(const QUuid& uuid) const noexcept {return mSignals.value(uuid, 0);}

        // Getters: Symbol Variants
        const QHash<QUuid, const GenCompSymbVar*>& getSymbolVariants() const noexcept {return mSymbolVariants;}
        const GenCompSymbVar* getSymbolVariantByUuid(const QUuid& uuid) const noexcept {return mSymbolVariants.value(uuid, 0);}
        const QUuid& getDefaultSymbolVariantUuid() const noexcept {return mDefaultSymbolVariantUuid;}
        const GenCompSymbVar* getDefaultSymbolVariant() const noexcept {return mSymbolVariants.value(mDefaultSymbolVariantUuid);}


    private:

        // make some methods inaccessible...
        GenericComponent();
        GenericComponent(const GenericComponent& other);
        GenericComponent& operator=(const GenericComponent& rhs);


        // Generic Conponent Attributes
        QHash<QString, QString> mPrefixes; ///< key: norm, value: prefix
        QString mDefaultPrefixNorm; ///< must be an existing key of #mPrefixes
        QHash<QUuid, const GenCompSignal*> mSignals; ///< empty if the component has no signals
        QHash<QUuid, const GenCompSymbVar*> mSymbolVariants; ///< minimum one entry
        QUuid mDefaultSymbolVariantUuid; ///< must be an existing key of #mSymbolVariants
};

} // namespace library

#endif // LIBRARY_GENERICCOMPONENT_H
