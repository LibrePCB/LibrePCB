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
#include "searchtoolbar.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SearchToolBar::SearchToolBar(QWidget* parent) noexcept
  : QToolBar(parent),
    mCompleterListFunction(),
    mLineEdit(new QLineEdit()),
    mIndex(0) {
  mLineEdit->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  mLineEdit->setMaxLength(30);  // avoid too large widget in toolbar
  mLineEdit->setClearButtonEnabled(true);  // to quickly clear the search term
  connect(mLineEdit.data(), &QLineEdit::textEdited, this,
          &SearchToolBar::textEdited);
  connect(mLineEdit.data(), &QLineEdit::returnPressed, this,
          &SearchToolBar::enterPressed);
  addWidget(mLineEdit.data());
}

SearchToolBar::~SearchToolBar() noexcept {
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void SearchToolBar::updateCompleter() noexcept {
  QStringList list =
      mCompleterListFunction ? mCompleterListFunction() : QStringList();

  QCollator collator;
  collator.setCaseSensitivity(Qt::CaseInsensitive);
  collator.setIgnorePunctuation(false);
  collator.setNumericMode(true);
  std::sort(list.begin(), list.end(),
            [&collator](const QString& lhs, const QString& rhs) {
              return collator(lhs, rhs);
            });

  QCompleter* completer = new QCompleter(list);
  completer->setCaseSensitivity(Qt::CaseInsensitive);
  mLineEdit->setCompleter(completer);
}

void SearchToolBar::textEdited(const QString& text) noexcept {
  Q_UNUSED(text);
  updateCompleter();
  mIndex = 0;
}

void SearchToolBar::enterPressed() noexcept {
  emit goToTriggered(mLineEdit->text().trimmed(), mIndex++);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
