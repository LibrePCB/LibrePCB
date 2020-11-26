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
#include "packagepadcombobox.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

PackagePadComboBox::PackagePadComboBox(QWidget* parent) noexcept
  : QWidget(parent), mComboBox(new QComboBox(this)) {
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(mComboBox);

  mComboBox->setEditable(false);

  connect(
      mComboBox,
      static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
      this, &PackagePadComboBox::currentIndexChanged);
}

PackagePadComboBox::~PackagePadComboBox() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

tl::optional<Uuid> PackagePadComboBox::getCurrentPad() const noexcept {
  return getPadAtIndex(mComboBox->currentIndex());
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void PackagePadComboBox::setPads(const PackagePadList& pads) noexcept {
  mComboBox->clear();
  mComboBox->addItem(tr("(unconnected)"), QString());
  for (const PackagePad& pad : pads) {
    mComboBox->addItem(*pad.getName(), pad.getUuid().toStr());
  }
  mComboBox->setCurrentIndex(-1);
}

void PackagePadComboBox::setCurrentPad(tl::optional<Uuid> pad) noexcept {
  int index = pad ? mComboBox->findData(pad->toStr(), Qt::UserRole) : -1;
  mComboBox->setCurrentIndex(std::max(index, 0));
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

tl::optional<Uuid> PackagePadComboBox::getPadAtIndex(int index) const noexcept {
  return Uuid::tryFromString(
      mComboBox->itemData(index, Qt::UserRole).toString());
}

void PackagePadComboBox::currentIndexChanged(int index) noexcept {
  emit currentPadChanged(getPadAtIndex(index));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb
