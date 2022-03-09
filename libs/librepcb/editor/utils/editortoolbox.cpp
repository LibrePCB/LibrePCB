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
#include "editortoolbox.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

void EditorToolbox::removeFormLayoutRow(QLabel& label) noexcept {
  if (auto l = dynamic_cast<QFormLayout*>(label.parentWidget()->layout())) {
    for (int i = 0; i < l->rowCount(); ++i) {
      QLayoutItem* labelItem = l->itemAt(i, QFormLayout::LabelRole);
      QLayoutItem* fieldItem = l->itemAt(i, QFormLayout::FieldRole);
      if ((labelItem) && (labelItem->widget() == &label) && (fieldItem)) {
        hideLayoutItem(*labelItem);
        hideLayoutItem(*fieldItem);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 8, 0))
        l->takeRow(i);  // Avoid ugly space caused by the empty layout rows.
#endif
        return;
      }
    }
  }
  qWarning().nospace()
      << "EditorToolbox::removeFormLayoutRow() failed to remove row "
      << label.objectName() << ".";
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void EditorToolbox::hideLayoutItem(QLayoutItem& item) noexcept {
  if (QWidget* widget = item.widget()) {
    widget->hide();
  } else if (QLayout* layout = item.layout()) {
    for (int i = 0; i < layout->count(); ++i) {
      if (QLayoutItem* child = layout->itemAt(i)) {
        hideLayoutItem(*child);
      }
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
