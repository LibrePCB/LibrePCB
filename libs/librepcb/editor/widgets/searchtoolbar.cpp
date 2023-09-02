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

#include <librepcb/core/utils/toolbox.h>

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

SearchToolBar::SearchToolBar(QWidget* parent) noexcept
  : QToolBar(parent),
    mCompleterListFunction(),
    mLineEdit(new QLineEdit()),
    mForward(true),
    mIndex(0) {
  mLineEdit->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  mLineEdit->setMaxLength(30);  // avoid too large widget in toolbar
  mLineEdit->setClearButtonEnabled(true);  // to quickly clear the search term
  connect(mLineEdit.data(), &QLineEdit::textChanged, this,
          &SearchToolBar::textChangedHandler);
  connect(mLineEdit.data(), &QLineEdit::returnPressed, this,
          &SearchToolBar::findNext);
  addWidget(mLineEdit.data());
  setFocusPolicy(mLineEdit->focusPolicy());
  setFocusProxy(mLineEdit.data());
  setWindowTitle(tr("Search"));
}

SearchToolBar::~SearchToolBar() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SearchToolBar::clear() noexcept {
  mLineEdit->clear();
}

void SearchToolBar::selectAllAndSetFocus() noexcept {
  mLineEdit->selectAll();
  mLineEdit->setFocus();
}

void SearchToolBar::findNext() noexcept {
  if (!mForward) {
    mForward = true;
    mIndex += 2;
  }
  emit goToTriggered(mLineEdit->text().trimmed(), mIndex);
  ++mIndex;
}

void SearchToolBar::findPrevious() noexcept {
  if (mForward) {
    mForward = false;
    mIndex -= 2;
  }
  emit goToTriggered(mLineEdit->text().trimmed(), mIndex);
  --mIndex;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void SearchToolBar::updateCompleter() noexcept {
  QStringList list =
      mCompleterListFunction ? mCompleterListFunction() : QStringList();
  Toolbox::sortNumeric(list, Qt::CaseInsensitive, false);

  QCompleter* completer = new QCompleter(list);
  completer->setCaseSensitivity(Qt::CaseInsensitive);
  mLineEdit->setCompleter(completer);
}

void SearchToolBar::textChangedHandler(const QString& text) noexcept {
  updateCompleter();
  mIndex = 0;
  mForward = true;
  emit textChanged(text);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
