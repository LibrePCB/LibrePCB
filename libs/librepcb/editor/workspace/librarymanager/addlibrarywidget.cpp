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
#include "../desktopservices.h"
#include "onlinelibrarylistwidgetitem.h"
#include "ui_addlibrarywidget.h"

#include <librepcb/core/application.h>
#include <librepcb/core/exceptions.h>
#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/library.h>
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
  connect(mUi->lblLicenseLink, &QLabel::linkActivated, this,
          [this](const QString& url) {
            DesktopServices ds(mWorkspace.getSettings());
            ds.openWebUrl(QUrl(url));
          });
  mUi->lblImportNote->setText(
      mUi->lblImportNote->text().arg("KiCad Import").arg("Eagle Import"));
  connect(mUi->lblImportNote, &QLabel::linkActivated, mUi->edtLocalName,
          &QLineEdit::setText);
  connect(mUi->cbxOnlineLibrariesSelectAll, &QCheckBox::clicked, this,
          [this]() { mManualCheckStateForAllRemoteLibraries = true; });
}

AddLibraryWidget::~AddLibraryWidget() noexcept {
  clearOnlineLibraryList();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void AddLibraryWidget::onlineLibraryListReceived(
    QList<ApiEndpoint::Library> libs) noexcept {
  for (const ApiEndpoint::Library& lib : libs) {
    OnlineLibraryListWidgetItem* widget =
        new OnlineLibraryListWidgetItem(mWorkspace, lib);
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
    item->setText(widget->getLibrary().name);
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
        libs.unite(widget->getLibrary().dependencies);
      }
    }
    for (int i = 0; i < mUi->lstOnlineLibraries->count(); i++) {
      QListWidgetItem* item = mUi->lstOnlineLibraries->item(i);
      Q_ASSERT(item);
      auto* widget = dynamic_cast<OnlineLibraryListWidgetItem*>(
          mUi->lstOnlineLibraries->itemWidget(item));
      if (widget && libs.contains(widget->getLibrary().uuid)) {
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
      if (widget && widget->isChecked()) {
        libs.insert(widget->getLibrary().uuid);
      }
    }
    for (int i = 0; i < mUi->lstOnlineLibraries->count(); i++) {
      QListWidgetItem* item = mUi->lstOnlineLibraries->item(i);
      Q_ASSERT(item);
      auto* widget = dynamic_cast<OnlineLibraryListWidgetItem*>(
          mUi->lstOnlineLibraries->itemWidget(item));
      if (widget && (!libs.contains(widget->getLibrary().dependencies))) {
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
 *  Private Static Methods
 ******************************************************************************/

QString AddLibraryWidget::getTextOrPlaceholderFromQLineEdit(
    QLineEdit* edit, bool isFilename) noexcept {
  if (edit) {
    QString text = edit->text().trimmed();
    QString placeholder = edit->placeholderText().trimmed();
    QString retval = (text.length() > 0) ? text : placeholder;
    if (isFilename) {
      return FilePath::cleanFileName(
          retval, FilePath::ReplaceSpaces | FilePath::KeepCase);
    } else {
      return retval;
    }
  } else {
    return QString("");
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
