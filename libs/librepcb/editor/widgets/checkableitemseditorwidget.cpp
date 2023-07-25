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
#include "checkableitemseditorwidget.h"

#include "../utils/editortoolbox.h"

#include <librepcb/core/utils/qtmetatyperegistration.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

// Register Qt meta types.
static QtMetaTypeRegistration<CheckableItemsEditorWidget::ItemList> sMetaType;

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CheckableItemsEditorWidget::CheckableItemsEditorWidget(QWidget* parent) noexcept
  : QFrame(parent), mLayout(new QVBoxLayout()), mItems() {
  setFrameStyle(StyledPanel);
  mLayout->setContentsMargins(3, 0, 3, 0);
  mLayout->setSpacing(3);
  setLayout(mLayout);
}

CheckableItemsEditorWidget::~CheckableItemsEditorWidget() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void CheckableItemsEditorWidget::setItems(const ItemList& items) noexcept {
  mItems = items;
  updateWidgets();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void CheckableItemsEditorWidget::updateWidgets() noexcept {
  while (mLayout->count() > 0) {
    QLayoutItem* item = mLayout->takeAt(0);
    Q_ASSERT(item);
    EditorToolbox::deleteLayoutItemRecursively(item);
  }

  for (Item& item : mItems) {
    QCheckBox* cbx = new QCheckBox(this);
    cbx->setText(std::get<1>(item));
    cbx->setCheckState(std::get<2>(item));
    connect(cbx, &QCheckBox::stateChanged, cbx, [&item](int state) {
      std::get<2>(item) = static_cast<Qt::CheckState>(state);
    });
    mLayout->addWidget(cbx);
  }

  setFixedHeight(sizeHint().height());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
