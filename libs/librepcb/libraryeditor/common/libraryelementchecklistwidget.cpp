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
#include "libraryelementchecklistwidget.h"

#include <algorithm>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

LibraryElementCheckListWidget::LibraryElementCheckListWidget(
    QWidget* parent) noexcept
  : QWidget(parent), mListWidget(new QListWidget(this)), mHandler(nullptr) {
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(mListWidget.data());
  connect(mListWidget.data(), &QListWidget::itemDoubleClicked, this,
          &LibraryElementCheckListWidget::itemDoubleClicked);
  updateList();  // adds the "looks good" message
}

LibraryElementCheckListWidget::~LibraryElementCheckListWidget() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void LibraryElementCheckListWidget::setHandler(
    IF_LibraryElementCheckHandler* handler) noexcept {
  mHandler = handler;
}

void LibraryElementCheckListWidget::setMessages(
    LibraryElementCheckMessageList messages) noexcept {
  // Sort by severity and message.
  std::sort(messages.begin(), messages.end(),
            [](std::shared_ptr<const LibraryElementCheckMessage> a,
               std::shared_ptr<const LibraryElementCheckMessage> b) {
              return (a && b) ? (*b) < (*a) : false;
            });

  // Detect if messages have changed.
  bool isSame = (mMessages.count() == messages.count());
  if (isSame) {
    for (int i = 0; i < mMessages.count(); ++i) {
      auto m1 = mMessages.value(i);
      auto m2 = messages.value(i);
      if ((!m1) || (!m2) || (*m1 != *m2)) {
        isSame = false;
      }
    }
  }

  // Only update if messages have changed (avoid GUI flickering).
  if (!isSame) {
    mMessages = messages;
    updateList();
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void LibraryElementCheckListWidget::updateList() noexcept {
  mListWidget->clear();
  foreach (const auto& msg, mMessages) {
    QListWidgetItem* item = new QListWidgetItem();
    mListWidget->addItem(item);
    LibraryElementCheckListItemWidget* widget =
        new LibraryElementCheckListItemWidget(msg, *this);
    mListWidget->setItemWidget(item, widget);
  }
  if (mListWidget->count() == 0) {
    mListWidget->setEnabled(false);
    mListWidget->addItem(tr("Looks good so far :-)"));
  } else {
    mListWidget->setEnabled(true);
  }
}

void LibraryElementCheckListWidget::itemDoubleClicked(
    QListWidgetItem* item) noexcept {
  std::shared_ptr<const LibraryElementCheckMessage> msg =
      mMessages.value(mListWidget->row(item));
  if (msg && mHandler) {
    if (mHandler->libraryElementCheckFixAvailable(msg)) {
      mHandler->libraryElementCheckFixRequested(msg);
    } else {
      mHandler->libraryElementCheckDescriptionRequested(msg);
    }
  }
}

bool LibraryElementCheckListWidget::libraryElementCheckFixAvailable(
    std::shared_ptr<const LibraryElementCheckMessage> msg) noexcept {
  if (mHandler) {
    return mHandler->libraryElementCheckFixAvailable(msg);
  } else {
    return false;
  }
}

void LibraryElementCheckListWidget::libraryElementCheckFixRequested(
    std::shared_ptr<const LibraryElementCheckMessage> msg) noexcept {
  if (mHandler) {
    mHandler->libraryElementCheckFixRequested(msg);
  }
}

void LibraryElementCheckListWidget::libraryElementCheckDescriptionRequested(
    std::shared_ptr<const LibraryElementCheckMessage> msg) noexcept {
  if (mHandler) {
    mHandler->libraryElementCheckDescriptionRequested(msg);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb
