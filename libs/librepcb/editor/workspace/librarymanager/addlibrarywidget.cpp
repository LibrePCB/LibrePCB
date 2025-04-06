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
#include "addlibrarywidget.h"

#include "../../widgets/waitingspinnerwidget.h"
#include "onlinelibrarylistwidgetitem.h"
#include "ui_addlibrarywidget.h"

#include <librepcb/core/exceptions.h>
#include <librepcb/core/network/apiendpoint.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacesettings.h>

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

AddLibraryWidget::AddLibraryWidget(Workspace& ws) noexcept
  : QWidget(nullptr),
    mWorkspace(ws),
    mUi(new Ui::AddLibraryWidget),
    mManualCheckStateForAllRemoteLibraries(false) {
  mUi->setupUi(this);
  connect(mUi->btnOnlineLibrariesDownload, &QPushButton::clicked, this,
          &AddLibraryWidget::downloadOnlineLibrariesButtonClicked);
  connect(mUi->cbxOnlineLibrariesSelectAll, &QCheckBox::clicked, this,
          [this]() { mManualCheckStateForAllRemoteLibraries = true; });

  // Hide text in library list since text is displayed with custom item
  // widgets, but list item texts are still set for keyboard navigation.
  mUi->lstOnlineLibraries->setStyleSheet(
      "QListWidget::item{"
      "  color: transparent;"
      "  selection-color: transparent;"
      "}");

  // select the default tab
  mUi->tabWidget->setCurrentIndex(0);
}

AddLibraryWidget::~AddLibraryWidget() noexcept {
  clearOnlineLibraryList();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void AddLibraryWidget::updateOnlineLibraryList() noexcept {
  clearOnlineLibraryList();
  foreach (const QUrl& url, mWorkspace.getSettings().apiEndpoints.get()) {
    std::shared_ptr<ApiEndpoint> repo = std::make_shared<ApiEndpoint>(url);
    connect(repo.get(), &ApiEndpoint::libraryListReceived, this,
            &AddLibraryWidget::onlineLibraryListReceived);
    connect(repo.get(), &ApiEndpoint::errorWhileFetchingLibraryList, this,
            &AddLibraryWidget::errorWhileFetchingLibraryList);

    // Add waiting spinner during library list download.
    WaitingSpinnerWidget* spinner =
        new WaitingSpinnerWidget(mUi->lstOnlineLibraries);
    connect(repo.get(), &ApiEndpoint::libraryListReceived, spinner,
            &WaitingSpinnerWidget::deleteLater);
    connect(repo.get(), &ApiEndpoint::errorWhileFetchingLibraryList, spinner,
            &WaitingSpinnerWidget::deleteLater);
    spinner->show();

    repo->requestLibraryList();
    mApiEndpoints.append(repo);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void AddLibraryWidget::onlineLibraryListReceived(
    const QJsonArray& libs) noexcept {
  for (const QJsonValue& libVal : libs) {
    OnlineLibraryListWidgetItem* widget =
        new OnlineLibraryListWidgetItem(mWorkspace, libVal.toObject());
    if (mManualCheckStateForAllRemoteLibraries) {
      widget->setChecked(mUi->cbxOnlineLibrariesSelectAll->isChecked());
    }
    connect(mUi->cbxOnlineLibrariesSelectAll, &QCheckBox::clicked, widget,
            &OnlineLibraryListWidgetItem::setChecked);
    connect(widget, &OnlineLibraryListWidgetItem::checkedChanged, this,
            &AddLibraryWidget::repoLibraryDownloadCheckedChanged);
    QListWidgetItem* item = new QListWidgetItem(mUi->lstOnlineLibraries);
    // Set item text to make searching by keyboard working (type to find
    // library). However, the text would mess up the look, thus it is made
    // hidden with a stylesheet set in the constructor (see above).
    item->setText(widget->getName());
    item->setSizeHint(widget->sizeHint());
    mUi->lstOnlineLibraries->setItemWidget(item, widget);
  }
}

void AddLibraryWidget::errorWhileFetchingLibraryList(
    const QString& errorMsg) noexcept {
  QListWidgetItem* item =
      new QListWidgetItem(errorMsg, mUi->lstOnlineLibraries);
  item->setBackground(Qt::red);
  item->setForeground(Qt::white);
}

void AddLibraryWidget::clearOnlineLibraryList() noexcept {
  mApiEndpoints.clear();  // disconnects all signal/slot connections
  for (int i = mUi->lstOnlineLibraries->count() - 1; i >= 0; i--) {
    QListWidgetItem* item = mUi->lstOnlineLibraries->item(i);
    Q_ASSERT(item);
    delete mUi->lstOnlineLibraries->itemWidget(item);
    delete item;
  }
  Q_ASSERT(mUi->lstOnlineLibraries->count() == 0);
}

void AddLibraryWidget::repoLibraryDownloadCheckedChanged(
    bool checked) noexcept {
  if (checked) {
    // one more library is checked, check all dependencies too
    QSet<Uuid> libs;
    for (int i = 0; i < mUi->lstOnlineLibraries->count(); i++) {
      QListWidgetItem* item = mUi->lstOnlineLibraries->item(i);
      Q_ASSERT(item);
      auto* widget = dynamic_cast<OnlineLibraryListWidgetItem*>(
          mUi->lstOnlineLibraries->itemWidget(item));
      if (widget && widget->isChecked()) {
        libs.unite(widget->getDependencies());
      }
    }
    for (int i = 0; i < mUi->lstOnlineLibraries->count(); i++) {
      QListWidgetItem* item = mUi->lstOnlineLibraries->item(i);
      Q_ASSERT(item);
      auto* widget = dynamic_cast<OnlineLibraryListWidgetItem*>(
          mUi->lstOnlineLibraries->itemWidget(item));
      if (widget && widget->getUuid() && (libs.contains(*widget->getUuid()))) {
        widget->setChecked(true);
      }
    }
  } else {
    // one library was unchecked, uncheck all libraries with missing
    // dependencies
    QSet<Uuid> libs;
    for (int i = 0; i < mUi->lstOnlineLibraries->count(); i++) {
      QListWidgetItem* item = mUi->lstOnlineLibraries->item(i);
      Q_ASSERT(item);
      auto* widget = dynamic_cast<OnlineLibraryListWidgetItem*>(
          mUi->lstOnlineLibraries->itemWidget(item));
      if (widget && widget->isChecked() && widget->getUuid()) {
        libs.insert(*widget->getUuid());
      }
    }
    for (int i = 0; i < mUi->lstOnlineLibraries->count(); i++) {
      QListWidgetItem* item = mUi->lstOnlineLibraries->item(i);
      Q_ASSERT(item);
      auto* widget = dynamic_cast<OnlineLibraryListWidgetItem*>(
          mUi->lstOnlineLibraries->itemWidget(item));
      if (widget && (!libs.contains(widget->getDependencies()))) {
        widget->setChecked(false);
      }
    }
  }
}

void AddLibraryWidget::downloadOnlineLibrariesButtonClicked() noexcept {
  for (int i = 0; i < mUi->lstOnlineLibraries->count(); i++) {
    QListWidgetItem* item = mUi->lstOnlineLibraries->item(i);
    Q_ASSERT(item);
    auto* widget = dynamic_cast<OnlineLibraryListWidgetItem*>(
        mUi->lstOnlineLibraries->itemWidget(item));
    if (widget) {
      widget->startDownloadIfSelected();
    } else {
      qWarning() << "Invalid item widget detected in library manager.";
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
