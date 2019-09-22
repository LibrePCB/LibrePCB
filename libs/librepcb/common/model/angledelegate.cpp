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
#include "angledelegate.h"

#include "../toolbox.h"
#include "../units/angle.h"
#include "../widgets/angleedit.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

AngleDelegate::AngleDelegate(QObject* parent) noexcept
  : QStyledItemDelegate(parent) {
}

AngleDelegate::~AngleDelegate() noexcept {
}

/*******************************************************************************
 *  Inherited from QStyledItemDelegate
 ******************************************************************************/

QString AngleDelegate::displayText(const QVariant& value,
                                   const QLocale&  locale) const {
  return Toolbox::floatToString(value.value<Angle>().toDeg(), 10, locale) % "°";
}

QWidget* AngleDelegate::createEditor(QWidget*                    parent,
                                     const QStyleOptionViewItem& option,
                                     const QModelIndex&          index) const {
  Q_UNUSED(option);
  AngleEdit* edt = new AngleEdit(parent);
  edt->setFrame(false);
  edt->setValue(index.data(Qt::EditRole).value<Angle>());
  edt->selectAll();
  return edt;
}

void AngleDelegate::setEditorData(QWidget*           editor,
                                  const QModelIndex& index) const {
  AngleEdit* edt = static_cast<AngleEdit*>(editor);
  edt->setValue(index.data(Qt::EditRole).value<Angle>());
}

void AngleDelegate::setModelData(QWidget* editor, QAbstractItemModel* model,
                                 const QModelIndex& index) const {
  AngleEdit* edt = static_cast<AngleEdit*>(editor);
  model->setData(index, QVariant::fromValue(edt->getValue()), Qt::EditRole);
}

void AngleDelegate::updateEditorGeometry(QWidget*                    editor,
                                         const QStyleOptionViewItem& option,
                                         const QModelIndex& index) const {
  Q_UNUSED(index);
  editor->setGeometry(option.rect);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
