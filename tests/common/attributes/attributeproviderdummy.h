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

#ifndef ATTRIBUTEPROVIDERDUMMY_H
#define ATTRIBUTEPROVIDERDUMMY_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <librepcb/common/attributes/attributeprovider.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace tests {

/*****************************************************************************************
 *  Class AttributeProviderDummy
 ****************************************************************************************/

class AttributeProviderDummy final : public AttributeProvider
{
    public:
        AttributeProviderDummy() noexcept {}
        AttributeProviderDummy(const AttributeProviderDummy& other) = delete;
        AttributeProviderDummy& operator=(const AttributeProviderDummy& rhs) = delete;
        ~AttributeProviderDummy() noexcept {}

        QString getUserDefinedAttributeValue(const QString& key) const noexcept override {
            if (key == "KEY")   return "";
            if (key == "KEY_1") return "Normal value";
            if (key == "KEY_2") return "Value with {}}}{{ noise";
            if (key == "KEY_3") return "Recursive {{UNDEFINED}} value";
            if (key == "KEY_4") return "Recursive {{KEY_1}} value";
            if (key == "KEY_5") return "Recursive {{KEY_4}} value";
            if (key == "KEY_6") return "Endless {{KEY_7}} part 1";
            if (key == "KEY_7") return "Endless {{KEY_6}} part 2";
            if (key == "KEY_8") return "{{KEY}}";
            return QString();
        }

    signals:
        void attributesChanged() override {}
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace tests
} // namespace librepcb

#endif // ATTRIBUTEPROVIDERDUMMY_H
