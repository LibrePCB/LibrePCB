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
#include "alignmentselector.h"

#include "ui_alignmentselector.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

AlignmentSelector::AlignmentSelector(QWidget* parent) noexcept
  : QWidget(parent), mUi(new Ui::AlignmentSelector()) {
  mUi->setupUi(this);
  mLookupTable.insert(mUi->tl, Alignment(HAlign::left(), VAlign::top()));
  mLookupTable.insert(mUi->cl, Alignment(HAlign::left(), VAlign::center()));
  mLookupTable.insert(mUi->bl, Alignment(HAlign::left(), VAlign::bottom()));
  mLookupTable.insert(mUi->tc, Alignment(HAlign::center(), VAlign::top()));
  mLookupTable.insert(mUi->cc, Alignment(HAlign::center(), VAlign::center()));
  mLookupTable.insert(mUi->bc, Alignment(HAlign::center(), VAlign::bottom()));
  mLookupTable.insert(mUi->tr, Alignment(HAlign::right(), VAlign::top()));
  mLookupTable.insert(mUi->cr, Alignment(HAlign::right(), VAlign::center()));
  mLookupTable.insert(mUi->br, Alignment(HAlign::right(), VAlign::bottom()));
  setAlignment(Alignment());
}

AlignmentSelector::~AlignmentSelector() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void AlignmentSelector::setReadOnly(bool readOnly) noexcept {
  mUi->tl->setDisabled(readOnly);
  mUi->tc->setDisabled(readOnly);
  mUi->tr->setDisabled(readOnly);
  mUi->cl->setDisabled(readOnly);
  mUi->cc->setDisabled(readOnly);
  mUi->cr->setDisabled(readOnly);
  mUi->bl->setDisabled(readOnly);
  mUi->bc->setDisabled(readOnly);
  mUi->br->setDisabled(readOnly);
}

Alignment AlignmentSelector::getAlignment() const noexcept {
  foreach (QRadioButton* btn, mLookupTable.keys()) {
    if (btn->isChecked()) return mLookupTable.value(btn);
  }
  Q_ASSERT(false);
  return Alignment();
}

void AlignmentSelector::setAlignment(const Alignment& align) noexcept {
  QRadioButton* btn = mLookupTable.key(align, nullptr);
  Q_ASSERT(btn);
  if (btn) btn->setChecked(true);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
