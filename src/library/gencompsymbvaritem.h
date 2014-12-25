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

#ifndef LIBRARY_GENCOMPSYMBVARITEM_H
#define LIBRARY_GENCOMPSYMBVARITEM_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QDomElement>
#include "../common/exceptions.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

namespace library {
class GenericComponent;
class GenCompSymbVar;
}

/*****************************************************************************************
 *  Class GenCompSymbVarItem
 ****************************************************************************************/

namespace library {

/**
 * @brief The GenCompSymbVarItem class
 */
class GenCompSymbVarItem final : public QObject
{
        Q_OBJECT

    public:

        /// Pin Display Type Enum
        enum class PinDisplayType_t {
            None,
            PinName,
            GenCompSignal,
            NetSignal
        };

        /// Pin-Signal-Map item struct
        struct PinSignalMapItem_t {
            QUuid pin;                      ///< must be valid
            QUuid signal;                   ///< NULL if not connected to a signal
            PinDisplayType_t displayType;
        };

        // Constructors / Destructor
        explicit GenCompSymbVarItem(GenericComponent& genComp, GenCompSymbVar& symbVar,
                                    const QDomElement& domElement) throw (Exception);
        ~GenCompSymbVarItem() noexcept;

        // Getters: Attributes
        const QUuid& getUuid() const noexcept {return mUuid;}
        const QUuid& getSymbolUuid() const noexcept {return mSymbolUuid;}
        int getAddOrderIndex() const noexcept {return mAddOrderIndex;}
        bool isRequired() const noexcept {return mIsRequired;}
        const QString& getSuffix() const noexcept {return mSuffix;}

        // Getters: Pin-Signal-Map
        const QHash<QUuid, PinSignalMapItem_t>& getPinSignalMap() const noexcept {return mPinSignalMap;}
        QUuid getSignalOfPin(const QUuid& pinUuid) const noexcept;
        PinDisplayType_t getDisplayTypeOfPin(const QUuid& pinUuid) const noexcept;


    private:

        // make some methods inaccessible...
        GenCompSymbVarItem();
        GenCompSymbVarItem(const GenCompSymbVarItem& other);
        GenCompSymbVarItem& operator=(const GenCompSymbVarItem& rhs);


        // General Attributes
        GenericComponent& mGenericComponent;
        GenCompSymbVar& mSymbolVariant;
        QDomElement mDomElement;

        // Symbol Variant Item Attributes
        QUuid mUuid;
        QUuid mSymbolUuid;
        int mAddOrderIndex;
        bool mIsRequired;
        QString mSuffix;
        QHash<QUuid, PinSignalMapItem_t> mPinSignalMap; ///< All pins required!
};

} // namespace library

#endif // LIBRARY_GENCOMPSYMBVARITEM_H
