/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "attributeunitcombobox.h"

#include "../attributes/attributetype.h"
#include "../attributes/attributeunit.h"
#include "../attributes/attrtypestring.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

AttributeUnitComboBox::AttributeUnitComboBox(QWidget* parent) noexcept
  : QWidget(parent), mComboBox(new QComboBox(this)), mAttributeType(nullptr) {
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(mComboBox);

  connect(
      mComboBox,
      static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
      this, &AttributeUnitComboBox::currentIndexChanged);

  setAttributeType(AttrTypeString::instance());
}

AttributeUnitComboBox::~AttributeUnitComboBox() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

const AttributeUnit* AttributeUnitComboBox::getCurrentItem() const noexcept {
  int index = mComboBox->currentIndex();
  Q_ASSERT(index >= 0 || mComboBox->count() == 0);
  Q_ASSERT(index < mAttributeType->getAvailableUnits().count());
  return mAttributeType->getAvailableUnits().value(index);
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void AttributeUnitComboBox::setAttributeType(
    const AttributeType& type) noexcept {
  if (&type == mAttributeType) {
    return;
  }

  blockSignals(true);

  mAttributeType = &type;
  mComboBox->clear();
  foreach (const AttributeUnit* unit, type.getAvailableUnits()) {
    mComboBox->addItem(unit->getSymbolTr());
  }
  mComboBox->setCurrentIndex(
      type.getAvailableUnits().indexOf(type.getDefaultUnit()));
  if (mComboBox->count() > 0 && mComboBox->currentIndex() < 0) {
    mComboBox->setCurrentIndex(0);
  }

  blockSignals(false);

  emit currentItemChanged(getCurrentItem());
}

void AttributeUnitComboBox::setCurrentItem(const AttributeUnit* unit) noexcept {
  int index = mAttributeType->getAvailableUnits().indexOf(unit);
  Q_ASSERT((index >= 0) ||
           (mAttributeType->getAvailableUnits().isEmpty() && (!unit)));
  Q_ASSERT(index < mAttributeType->getAvailableUnits().count());
  mComboBox->setCurrentIndex(index);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void AttributeUnitComboBox::currentIndexChanged(int index) noexcept {
  Q_UNUSED(index);
  emit currentItemChanged(getCurrentItem());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
