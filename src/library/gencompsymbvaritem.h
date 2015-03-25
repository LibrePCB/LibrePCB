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
#include "../common/file_io/if_xmlserializableobject.h"

/*****************************************************************************************
 *  Class GenCompSymbVarItem
 ****************************************************************************************/

namespace library {

/**
 * @brief The GenCompSymbVarItem class
 */
class GenCompSymbVarItem final : public IF_XmlSerializableObject
{
        Q_DECLARE_TR_FUNCTIONS(GenCompSymbVarItem)

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
        explicit GenCompSymbVarItem(const QUuid& uuid = QUuid::createUuid(),
                                    const QUuid& symbolUuid = QUuid(),
                                    bool isRequired = false,
                                    const QString& suffix = QString()) noexcept;
        explicit GenCompSymbVarItem(const XmlDomElement& domElement) throw (Exception);
        ~GenCompSymbVarItem() noexcept;

        // Getters: Attributes
        const QUuid& getUuid() const noexcept {return mUuid;}
        const QUuid& getSymbolUuid() const noexcept {return mSymbolUuid;}
        bool isRequired() const noexcept {return mIsRequired;}
        const QString& getSuffix() const noexcept {return mSuffix;}

        // Getters: Pin-Signal-Map
        const QHash<QUuid, PinSignalMapItem_t>& getPinSignalMap() const noexcept {return mPinSignalMap;}
        QUuid getSignalOfPin(const QUuid& pinUuid) const noexcept;
        PinDisplayType_t getDisplayTypeOfPin(const QUuid& pinUuid) const noexcept;

        // General Methods
        void addPinSignalMapping(const QUuid& pin, const QUuid& signal, PinDisplayType_t display) noexcept;
        XmlDomElement* serializeToXmlDomElement() const throw (Exception);


    private:

        // make some methods inaccessible...
        GenCompSymbVarItem(const GenCompSymbVarItem& other);
        GenCompSymbVarItem& operator=(const GenCompSymbVarItem& rhs);

        // Private Methods
        bool checkAttributesValidity() const noexcept;


        // Symbol Variant Item Attributes
        QUuid mUuid;
        QUuid mSymbolUuid;
        bool mIsRequired;
        QString mSuffix;
        QHash<QUuid, PinSignalMapItem_t> mPinSignalMap; ///< All pins required!
};

} // namespace library

#endif // LIBRARY_GENCOMPSYMBVARITEM_H
