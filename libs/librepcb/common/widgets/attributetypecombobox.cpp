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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include "attributetypecombobox.h"
#include "../attributes/attributetype.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

AttributeTypeComboBox::AttributeTypeComboBox(QWidget* parent) noexcept :
    QWidget(parent), mComboBox(new QComboBox(this))
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(mComboBox);

    foreach (const AttributeType* type, AttributeType::getAllTypes()) {
        mComboBox->addItem(type->getNameTr());
    }
    mComboBox->setCurrentIndex(0);
    connect(mComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &AttributeTypeComboBox::currentIndexChanged);
}

AttributeTypeComboBox::~AttributeTypeComboBox() noexcept
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

const AttributeType& AttributeTypeComboBox::getCurrentItem() const noexcept
{
    int index = mComboBox->currentIndex();
    Q_ASSERT(index >= 0);
    Q_ASSERT(index < AttributeType::getAllTypes().count());
    return *AttributeType::getAllTypes().value(index);
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void AttributeTypeComboBox::setCurrentItem(const AttributeType& type) noexcept
{
    int index = AttributeType::getAllTypes().indexOf(&type);
    Q_ASSERT(index >= 0);
    Q_ASSERT(index < AttributeType::getAllTypes().count());
    mComboBox->setCurrentIndex(index);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void AttributeTypeComboBox::currentIndexChanged(int index) noexcept
{
    Q_UNUSED(index);
    emit currentItemChanged(&getCurrentItem()); // passing a reference does not work with Qt5.2
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
