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
#include "librarydownload.h"
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
  connect(mUi->btnDownloadZip, &QPushButton::clicked, this,
          &AddLibraryWidget::downloadZippedLibraryButtonClicked);
  connect(mUi->btnLocalCreate, &QPushButton::clicked, this,
          &AddLibraryWidget::createLocalLibraryButtonClicked);
  connect(mUi->edtLocalName, &QLineEdit::textChanged, this,
          &AddLibraryWidget::localLibraryNameLineEditTextChanged);
  connect(mUi->edtDownloadZipUrl, &QLineEdit::textChanged, this,
          &AddLibraryWidget::downloadZipUrlLineEditTextChanged);
  connect(mUi->btnOnlineLibrariesDownload, &QPushButton::clicked, this,
          &AddLibraryWidget::downloadOnlineLibrariesButtonClicked);
  connect(mUi->lblLicenseLink, &QLabel::linkActivated, this,
          [this](const QString& url) {
            DesktopServices ds(mWorkspace.getSettings(), this);
            ds.openWebUrl(QUrl(url));
          });
  mUi->lblImportNote->setText(
      mUi->lblImportNote->text().arg("KiCad Import").arg("Eagle Import"));
  connect(mUi->lblImportNote, &QLabel::linkActivated, mUi->edtLocalName,
          &QLineEdit::setText);
  connect(mUi->cbxOnlineLibrariesSelectAll, &QCheckBox::clicked, this,
          [this]() { mManualCheckStateForAllRemoteLibraries = true; });

  // Hide text in library list since text is displayed with custom item
  // widgets, but list item texts are still set for keyboard navigation.
  mUi->lstOnlineLibraries->setStyleSheet(
      "QListWidget::item{"
      "  color: transparent;"
      "  selection-color: transparent;"
      "}");

  // tab "create local library": set placeholder texts
  mUi->edtLocalName->setPlaceholderText("My Library");
  mUi->edtLocalAuthor->setPlaceholderText(
      mWorkspace.getSettings().userName.get());
  mUi->edtLocalVersion->setPlaceholderText("0.1");
  mUi->edtLocalUrl->setPlaceholderText(
      tr("e.g. the URL to the Git repository (optional)"));
  localLibraryNameLineEditTextChanged(mUi->edtLocalName->text());

  // tab "download ZIP": set placeholder texts and hide widgets
  mUi->edtDownloadZipUrl->setPlaceholderText(
      tr("e.g. "
         "https://github.com/LibrePCB-Libraries/LibrePCB_Base.lplib/archive/"
         "master.zip"));
  mUi->prgDownloadZipProgress->setVisible(false);
  mUi->btnDownloadZipAbort->setVisible(false);
  mUi->lblDownloadZipStatusMsg->setText("");

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

void AddLibraryWidget::localLibraryNameLineEditTextChanged(
    QString name) noexcept {
  if (name.isEmpty()) name = mUi->edtLocalName->placeholderText();
  QString dirname = FilePath::cleanFileName(
      name, FilePath::ReplaceSpaces | FilePath::KeepCase);
  if (!dirname.endsWith(".lplib")) dirname.append(".lplib");
  mUi->edtLocalDirectory->setPlaceholderText(dirname);
}

void AddLibraryWidget::downloadZipUrlLineEditTextChanged(
    QString urlStr) noexcept {
  QString left = urlStr.left(urlStr.indexOf(".lplib", Qt::CaseInsensitive));
  QString libName = left.right(left.length() - left.lastIndexOf("/"));
  if (libName == urlStr) {
    libName = QUrl(urlStr).fileName();
  }
  QString dirname = FilePath::cleanFileName(
      libName, FilePath::ReplaceSpaces | FilePath::KeepCase);
  if (dirname.contains(".zip")) {
    dirname.remove(".zip");
  }
  if (!dirname.isEmpty()) {
    dirname.append(".lplib");
  }
  mUi->edtDownloadZipDirectory->setPlaceholderText(dirname);
}

void AddLibraryWidget::createLocalLibraryButtonClicked() noexcept {
  // get attributes
  QString name = getTextOrPlaceholderFromQLineEdit(mUi->edtLocalName, false);
  QString desc =
      getTextOrPlaceholderFromQLineEdit(mUi->edtLocalDescription, false);
  QString author =
      getTextOrPlaceholderFromQLineEdit(mUi->edtLocalAuthor, false);
  QString versionStr =
      getTextOrPlaceholderFromQLineEdit(mUi->edtLocalVersion, false);
  tl::optional<Version> version = Version::tryFromString(versionStr);
  QString urlStr = mUi->edtLocalUrl->text().trimmed();
  QUrl url = QUrl::fromUserInput(urlStr);
  bool useCc0License = mUi->cbxLocalCc0License->isChecked();
  QString directoryStr =
      getTextOrPlaceholderFromQLineEdit(mUi->edtLocalDirectory, true);
  if ((!directoryStr.isEmpty()) && (!directoryStr.endsWith(".lplib"))) {
    directoryStr.append(".lplib");
  }
  FilePath directory =
      mWorkspace.getLibrariesPath().getPathTo("local/" % directoryStr);

  // check attributes validity
  if (name.isEmpty()) {
    QMessageBox::critical(this, tr("Invalid Input"),
                          tr("Please enter a name."));
    return;
  }
  if (author.isEmpty()) {
    QMessageBox::critical(this, tr("Invalid Input"),
                          tr("Please enter an author."));
    return;
  }
  if (!version) {
    QMessageBox::critical(this, tr("Invalid Input"),
                          tr("The specified version number is not valid."));
    return;
  }
  if (!url.isValid() && !urlStr.isEmpty()) {
    QMessageBox::critical(this, tr("Invalid Input"),
                          tr("The specified URL is not valid."));
    return;
  }
  if (directoryStr.isEmpty()) {
    QMessageBox::critical(this, tr("Invalid Input"),
                          tr("Please enter a directory name."));
    return;
  }
  if (directory.isExistingFile() || directory.isExistingDir()) {
    QMessageBox::critical(this, tr("Invalid Input"),
                          tr("The specified directory exists already."));
    return;
  }

  try {
    // create transactional file system
    std::shared_ptr<TransactionalFileSystem> fs =
        TransactionalFileSystem::openRW(directory);
    TransactionalDirectory dir(fs);

    // create the new library
    QScopedPointer<Library> lib(new Library(Uuid::createRandom(), *version,
                                            author, ElementName(name), desc,
                                            QString("")));  // can throw
    lib->setUrl(url);
    try {
      lib->setIcon(FileUtils::readFile(Application::getResourcesDir().getPathTo(
          "library/default_image.png")));
    } catch (const Exception& e) {
      qCritical() << "Could not open the library image:" << e.getMsg();
    }
    lib->moveTo(dir);  // can throw

    // copy license file
    if (useCc0License) {
      try {
        FilePath source =
            Application::getResourcesDir().getPathTo("licenses/cc0-1.0.txt");
        fs->write("LICENSE.txt", FileUtils::readFile(source));  // can throw
      } catch (Exception& e) {
        qCritical() << "Could not copy the license file:" << e.getMsg();
      }
    }

    // copy readme file
    try {
      FilePath source =
          Application::getResourcesDir().getPathTo("library/readme_template");
      QByteArray content = FileUtils::readFile(source);  // can throw
      content.replace("{LIBRARY_NAME}", name.toUtf8());
      if (useCc0License) {
        content.replace("{LICENSE_TEXT}",
                        "Creative Commons (CC0-1.0). For the "
                        "license text, see [LICENSE.txt](LICENSE.txt).");
      } else {
        content.replace("{LICENSE_TEXT}", "No license set.");
      }
      fs->write("README.md", content);  // can throw
    } catch (Exception& e) {
      qCritical() << "Could not copy the readme file:" << e.getMsg();
    }

    // copy .gitignore
    try {
      FilePath source = Application::getResourcesDir().getPathTo(
          "library/gitignore_template");
      fs->write(".gitignore", FileUtils::readFile(source));  // can throw
    } catch (Exception& e) {
      qCritical() << "Could not copy the .gitignore file:" << e.getMsg();
    }

    // copy .gitattributes
    try {
      FilePath source = Application::getResourcesDir().getPathTo(
          "library/gitattributes_template");
      fs->write(".gitattributes", FileUtils::readFile(source));  // can throw
    } catch (Exception& e) {
      qCritical() << "Could not copy the .gitattributes file:" << e.getMsg();
    }

    // save file system
    fs->save();  // can throw

    // library successfully added! reset input fields and emit signal
    mUi->edtLocalName->clear();
    mUi->edtLocalDescription->clear();
    mUi->edtLocalAuthor->clear();
    mUi->edtLocalVersion->clear();
    mUi->edtLocalUrl->clear();
    mUi->cbxLocalCc0License->setChecked(false);
    mUi->edtLocalDirectory->clear();
    emit libraryAdded(directory);
  } catch (Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
  }
}

void AddLibraryWidget::downloadZippedLibraryButtonClicked() noexcept {
  if (mManualLibraryDownload) {
    QMessageBox::critical(this, tr("Busy"),
                          tr("A download is already running."));
    return;
  }

  // get attributes
  QUrl url = QUrl::fromUserInput(mUi->edtDownloadZipUrl->text().trimmed());
  QString dirStr =
      getTextOrPlaceholderFromQLineEdit(mUi->edtDownloadZipDirectory, true);
  if ((!dirStr.isEmpty()) && (!dirStr.endsWith(".lplib"))) {
    dirStr.append(".lplib");
  }
  FilePath extractToDir =
      mWorkspace.getLibrariesPath().getPathTo("local/" % dirStr);

  // check attributes validity
  if (!url.isValid()) {
    QMessageBox::critical(this, tr("Invalid Input"),
                          tr("Please enter a valid URL."));
    return;
  }
  if ((dirStr.isEmpty()) || (!extractToDir.isValid())) {
    QMessageBox::critical(this, tr("Invalid Input"),
                          tr("Please enter a valid directory."));
    return;
  }
  if (extractToDir.isExistingFile() || extractToDir.isExistingDir()) {
    QMessageBox::critical(this, tr("Directory exists already"),
                          tr("The directory \"%1\" exists already.")
                              .arg(extractToDir.toNative()));
    return;
  }

  // update widgets
  mUi->btnDownloadZip->setEnabled(false);
  mUi->btnDownloadZipAbort->setVisible(true);
  mUi->prgDownloadZipProgress->setVisible(true);
  mUi->prgDownloadZipProgress->setValue(0);
  mUi->lblDownloadZipStatusMsg->setText("");
  mUi->lblDownloadZipStatusMsg->setStyleSheet("");

  // download library
  mManualLibraryDownload.reset(new LibraryDownload(url, extractToDir));
  connect(mManualLibraryDownload.data(), &LibraryDownload::progressState,
          mUi->lblDownloadZipStatusMsg, &QLabel::setText);
  connect(mManualLibraryDownload.data(), &LibraryDownload::progressPercent,
          mUi->prgDownloadZipProgress, &QProgressBar::setValue);
  connect(mManualLibraryDownload.data(), &LibraryDownload::finished, this,
          &AddLibraryWidget::downloadZipFinished);
  connect(mUi->btnDownloadZipAbort, &QPushButton::clicked,
          mManualLibraryDownload.data(), &LibraryDownload::abort);
  mManualLibraryDownload->start();
}

void AddLibraryWidget::downloadZipFinished(bool success,
                                           const QString& errMsg) noexcept {
  Q_ASSERT(mManualLibraryDownload);

  if (success) {
    mUi->lblDownloadZipStatusMsg->setText("");
    emit libraryAdded(mManualLibraryDownload->getDestinationDir());
  } else {
    mUi->lblDownloadZipStatusMsg->setText(errMsg);
  }

  // update widgets
  mUi->btnDownloadZip->setEnabled(true);
  mUi->btnDownloadZipAbort->setVisible(false);
  mUi->prgDownloadZipProgress->setVisible(false);
  mUi->lblDownloadZipStatusMsg->setStyleSheet("QLabel {color: red;}");

  // delete download helper
  mManualLibraryDownload.reset();
}

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
