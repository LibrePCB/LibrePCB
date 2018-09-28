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
  : QWidget(parent),
    mPackage(nullptr),
    mFootprint(nullptr),
    mComboBox(new QComboBox(this)) {
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
  setPackage(nullptr);
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

PackagePad* PackagePadComboBox::getCurrentPad() const noexcept {
  tl::optional<Uuid> uuid = Uuid::tryFromString(
      mComboBox->itemData(mComboBox->currentIndex(), Qt::UserRole).toString());
  if (mPackage && uuid) {
    return mPackage->getPads().find(*uuid).get();
  } else {
    return nullptr;
  }
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void PackagePadComboBox::setPackage(Package*   package,
                                    Footprint* footprint) noexcept {
  mPackage   = package;
  mFootprint = footprint;
  updatePads();
}

void PackagePadComboBox::setCurrentPad(PackagePad* pad) noexcept {
  if (mPackage && pad) {
    mComboBox->setCurrentIndex(
        mComboBox->findData(*pad->getName(), Qt::UserRole));
  } else {
    mComboBox->setCurrentIndex(0);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void PackagePadComboBox::currentIndexChanged(int index) noexcept {
  Q_UNUSED(index);
  emit currentPadChanged(getCurrentPad());
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void PackagePadComboBox::updatePads() noexcept {
  mComboBox->clear();
  mComboBox->addItem(tr("(unconnected)"), QString());

  if (mPackage) {
    for (const PackagePad& pad : mPackage->getPads()) {
      if ((!mFootprint) || (!mFootprint->getPads().contains(pad.getUuid()))) {
        mComboBox->addItem(*pad.getName(), pad.getUuid().toStr());
      }
    }
  }

  mComboBox->setCurrentIndex(mComboBox->count() > 1 ? 1 : 0);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb
