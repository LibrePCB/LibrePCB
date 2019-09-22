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

#include "../toolbox.h"
#include "../units/length.h"
#include "../widgets/lengthedit.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

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
                                    const QLocale&  locale) const {
  return Toolbox::floatToString(value.value<Length>().toMm(), 10, locale) %
         " " % mUnit.toShortStringTr();
}

QWidget* LengthDelegate::createEditor(QWidget*                    parent,
                                      const QStyleOptionViewItem& option,
                                      const QModelIndex&          index) const {
  Q_UNUSED(option);
  LengthEdit* edt = new LengthEdit(parent);
  edt->setFrame(false);
  edt->setUnit(mUnit);
  edt->setValue(index.data(Qt::EditRole).value<Length>());
  edt->selectAll();
  return edt;
}

void LengthDelegate::setEditorData(QWidget*           editor,
                                   const QModelIndex& index) const {
  LengthEdit* edt = static_cast<LengthEdit*>(editor);
  edt->setValue(index.data(Qt::EditRole).value<Length>());
}

void LengthDelegate::setModelData(QWidget* editor, QAbstractItemModel* model,
                                  const QModelIndex& index) const {
  LengthEdit* edt = static_cast<LengthEdit*>(editor);
  model->setData(index, QVariant::fromValue(edt->getValue()), Qt::EditRole);
}

void LengthDelegate::updateEditorGeometry(QWidget*                    editor,
                                          const QStyleOptionViewItem& option,
                                          const QModelIndex& index) const {
  Q_UNUSED(index);
  editor->setGeometry(option.rect);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
