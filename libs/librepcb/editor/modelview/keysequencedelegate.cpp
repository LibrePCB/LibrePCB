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
#include "keysequencedelegate.h"

#include "../widgets/keysequenceseditorwidget.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

KeySequenceDelegate::KeySequenceDelegate(QObject* parent) noexcept
  : QStyledItemDelegate(parent) {
}

KeySequenceDelegate::~KeySequenceDelegate() noexcept {
}

/*******************************************************************************
 *  Inherited from QStyledItemDelegate
 ******************************************************************************/

QWidget* KeySequenceDelegate::createEditor(QWidget* parent,
                                           const QStyleOptionViewItem& option,
                                           const QModelIndex& index) const {
  Q_UNUSED(option);
  auto defaultSequences = index.data(Qt::UserRole).value<QList<QKeySequence>>();
  KeySequencesEditorWidget* edt =
      new KeySequencesEditorWidget(defaultSequences, parent);
  edt->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
  KeySequenceDelegate* mutableThis = const_cast<KeySequenceDelegate*>(this);
  connect(
      edt, &KeySequencesEditorWidget::applyTriggered, this,
      [mutableThis, edt]() {
        emit mutableThis->commitData(edt);
        emit mutableThis->closeEditor(edt);
      },
      Qt::QueuedConnection);
  connect(
      edt, &KeySequencesEditorWidget::cancelTriggered, this,
      [mutableThis, edt]() { emit mutableThis->closeEditor(edt); },
      Qt::QueuedConnection);
  return edt;
}

void KeySequenceDelegate::setEditorData(QWidget* editor,
                                        const QModelIndex& index) const {
  KeySequencesEditorWidget* edt =
      static_cast<KeySequencesEditorWidget*>(editor);
  QVariant overrides = index.data(Qt::EditRole);
  if (overrides.isNull()) {
    edt->setOverrides(tl::nullopt);
  } else {
    edt->setOverrides(overrides.value<QList<QKeySequence>>());
  }
}

void KeySequenceDelegate::setModelData(QWidget* editor,
                                       QAbstractItemModel* model,
                                       const QModelIndex& index) const {
  KeySequencesEditorWidget* edt =
      static_cast<KeySequencesEditorWidget*>(editor);
  const auto& override = edt->getOverrides();
  model->setData(index, override ? QVariant::fromValue(*override) : QVariant(),
                 Qt::EditRole);
}

void KeySequenceDelegate::updateEditorGeometry(
    QWidget* editor, const QStyleOptionViewItem& option,
    const QModelIndex& index) const {
  Q_UNUSED(index);

  KeySequencesEditorWidget* edt =
      static_cast<KeySequencesEditorWidget*>(editor);
  edt->setRowHeight(option.rect.height());

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
