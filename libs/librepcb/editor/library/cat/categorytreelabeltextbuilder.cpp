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
#include "categorytreelabeltextbuilder.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

template <typename ElementType>
CategoryTreeLabelTextBuilder<ElementType>::CategoryTreeLabelTextBuilder(
    const WorkspaceLibraryDb& db, const QStringList& localeOrder,
    bool nulloptIsRootCategory, QLabel& label) noexcept
  : mBuilder(db, localeOrder, nulloptIsRootCategory),
    mLabel(label),
    mOneLine(false),
    mChooseIfEmpty(false) {
}

template <typename ElementType>
CategoryTreeLabelTextBuilder<
    ElementType>::~CategoryTreeLabelTextBuilder() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

template <typename ElementType>
void CategoryTreeLabelTextBuilder<ElementType>::setText(
    const QString& text) noexcept {
  mLabel.setText(text);
  mLabel.setStyleSheet(QString());
}

template <typename ElementType>
void CategoryTreeLabelTextBuilder<ElementType>::setErrorText(
    const QString& error) noexcept {
  mLabel.setText(error);
  mLabel.setStyleSheet("QLabel { color: red; }");
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

template <typename ElementType>
bool CategoryTreeLabelTextBuilder<ElementType>::updateText(
    const std::optional<Uuid>& category) noexcept {
  try {
    QStringList lines = mBuilder.buildTree(category);
    if (lines.isEmpty() && mChooseIfEmpty) {
      setText("<i>" % tr("Please choose a category.") % "</i>");
    } else {
      setText(lines);
    }
    return true;
  } catch (const Exception& e) {
    setErrorText(e.getMsg());
    return false;
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

template <typename ElementType>
void CategoryTreeLabelTextBuilder<ElementType>::setText(
    const QStringList& lines) noexcept {
  QString text;
  for (int i = 0; i < lines.count(); ++i) {
    if (i > 0) {
      QString spaces = QString("&nbsp;").repeated(i * 2);
      text += mOneLine ? QString(" &rArr; ") : QString("<br>%1⤷ ").arg(spaces);
    }
    if ((lines.count() > 1) && (i == lines.count() - 1)) {
      text += "<b>" % lines.value(i) % "</b>";
    } else {
      text += lines.value(i);
    }
  }
  setText(text);
}

/*******************************************************************************
 *  Explicit template instantiations
 ******************************************************************************/
template class CategoryTreeLabelTextBuilder<ComponentCategory>;
template class CategoryTreeLabelTextBuilder<PackageCategory>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
