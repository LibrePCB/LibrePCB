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
#include "lengthdelegate.h"

#include "../widgets/lengthedit.h"

#include <librepcb/core/types/length.h>
#include <librepcb/core/utils/toolbox.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

LengthDelegate::LengthDelegate(QObject* parent) noexcept
  : QStyledItemDelegate(parent), mUnit(LengthUnit::millimeters()) {
}

LengthDelegate::~LengthDelegate() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void LengthDelegate::setUnit(const LengthUnit& unit) noexcept {
  mUnit = unit;
}

/*******************************************************************************
 *  Inherited from QStyledItemDelegate
 ******************************************************************************/

QString LengthDelegate::displayText(const QVariant& value,
                                    const QLocale& locale) const {
  qreal converted = mUnit.convertToUnit(value.value<Length>());
  return Toolbox::floatToString(converted,
                                mUnit.getReasonableNumberOfDecimals(), locale) %
      " " % mUnit.toShortStringTr();
}

QWidget* LengthDelegate::createEditor(QWidget* parent,
                                      const QStyleOptionViewItem& option,
                                      const QModelIndex& index) const {
  Q_UNUSED(option);
  LengthEdit* edt = new LengthEdit(parent);
  edt->setFrame(false);
  edt->setButtonSymbols(QAbstractSpinBox::NoButtons);
  edt->setChangeUnitActionVisible(false);  // avoid wasting space
  edt->setDefaultUnit(mUnit);
  edt->setValue(index.data(Qt::EditRole).value<Length>());
  edt->selectAll();

  // Manually close the editor if editing is finished, because for some reason
  // the view does not receive the "focus out" event from our own editor widget.
  // Queued connection is needed to avoid receiving the "enter pressed" key
  // event if editing was finished by pressing enter.
  connect(edt, &LengthEdit::editingFinished, this,
          &LengthDelegate::editingFinished, Qt::QueuedConnection);

  return edt;
}

void LengthDelegate::setEditorData(QWidget* editor,
                                   const QModelIndex& index) const {
  LengthEdit* edt = static_cast<LengthEdit*>(editor);
  edt->setValue(index.data(Qt::EditRole).value<Length>());
}

void LengthDelegate::setModelData(QWidget* editor, QAbstractItemModel* model,
                                  const QModelIndex& index) const {
  LengthEdit* edt = static_cast<LengthEdit*>(editor);
  model->setData(index, QVariant::fromValue(edt->getValue()), Qt::EditRole);
}

void LengthDelegate::updateEditorGeometry(QWidget* editor,
                                          const QStyleOptionViewItem& option,
                                          const QModelIndex& index) const {
  Q_UNUSED(index);
  editor->setGeometry(option.rect);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void LengthDelegate::editingFinished() noexcept {
  LengthEdit* edt = static_cast<LengthEdit*>(sender());
  commitData(edt);
  closeEditor(edt);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
