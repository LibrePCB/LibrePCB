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
#include "checkableitemsdelegate.h"

#include "../widgets/checkableitemseditorwidget.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CheckableItemsDelegate::CheckableItemsDelegate(QObject* parent) noexcept
  : QStyledItemDelegate(parent) {
}

CheckableItemsDelegate::~CheckableItemsDelegate() noexcept {
}

/*******************************************************************************
 *  Inherited from QStyledItemDelegate
 ******************************************************************************/

QWidget* CheckableItemsDelegate::createEditor(
    QWidget* parent, const QStyleOptionViewItem& option,
    const QModelIndex& index) const {
  Q_UNUSED(option);
  auto items =
      index.data(Qt::UserRole).value<CheckableItemsEditorWidget::ItemList>();
  CheckableItemsEditorWidget* edt = new CheckableItemsEditorWidget(parent);
  edt->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
  return edt;
}

void CheckableItemsDelegate::setEditorData(QWidget* editor,
                                           const QModelIndex& index) const {
  CheckableItemsEditorWidget* edt =
      static_cast<CheckableItemsEditorWidget*>(editor);
  edt->setItems(
      index.data(Qt::UserRole).value<CheckableItemsEditorWidget::ItemList>());
}

void CheckableItemsDelegate::setModelData(QWidget* editor,
                                          QAbstractItemModel* model,
                                          const QModelIndex& index) const {
  CheckableItemsEditorWidget* edt =
      static_cast<CheckableItemsEditorWidget*>(editor);
  model->setData(index, QVariant::fromValue(edt->getItems()), Qt::UserRole);
}

void CheckableItemsDelegate::updateEditorGeometry(
    QWidget* editor, const QStyleOptionViewItem& option,
    const QModelIndex& index) const {
  Q_UNUSED(index);

  CheckableItemsEditorWidget* edt =
      static_cast<CheckableItemsEditorWidget*>(editor);

  QRect rect = option.rect;
  rect.setTopLeft(editor->parentWidget()->mapToGlobal(rect.topLeft()));
  rect.setWidth(option.rect.width());
  rect.setHeight(edt->height());
  edt->setGeometry(rect);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
