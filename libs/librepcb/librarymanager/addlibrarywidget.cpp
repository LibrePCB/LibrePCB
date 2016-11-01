/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include "addlibrarywidget.h"
#include "ui_addlibrarywidget.h"
#include <librepcb/common/application.h>
#include <librepcb/common/systeminfo.h>
#include <librepcb/common/fileio/fileutils.h>
#include <librepcb/common/network/repository.h>
#include <librepcb/library/library.h>
#include <librepcb/workspace/workspace.h>
#include <librepcb/workspace/settings/workspacesettings.h>
#include "repositorylibrarylistwidgetitem.h"
#include "librarydownload.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace workspace {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

AddLibraryWidget::AddLibraryWidget(Workspace& ws) noexcept :
    QWidget(nullptr), mWorkspace(ws), mUi(new Ui::AddLibraryWidget)
{
    mUi->setupUi(this);
    connect(mUi->tabWidget, &QTabWidget::currentChanged,
            this, &AddLibraryWidget::currentTabChanged);
    connect(mUi->btnDownloadZip, &QPushButton::clicked,
            this, &AddLibraryWidget::downloadZippedLibraryButtonClicked);
    connect(mUi->btnLocalCreate, &QPushButton::clicked,
            this, &AddLibraryWidget::createLocalLibraryButtonClicked);
    connect(mUi->edtLocalName, &QLineEdit::textChanged,
            this, &AddLibraryWidget::localLibraryNameLineEditTextChanged);
    connect(mUi->edtDownloadZipUrl, &QLineEdit::textChanged,
            this, &AddLibraryWidget::downloadZipUrlLineEditTextChanged);
    connect(mUi->btnRepoLibsDownload, &QPushButton::clicked,
            this, &AddLibraryWidget::downloadLibrariesFromRepositoryButtonClicked);

    // tab "create local library": set placeholder texts
    mUi->edtLocalName->setPlaceholderText("My Library");
    mUi->edtLocalAuthor->setPlaceholderText(SystemInfo::getFullUsername());
    mUi->edtLocalVersion->setPlaceholderText("0.1");
    mUi->edtLocalUrl->setPlaceholderText(tr("e.g. the URL to the Git repository (optional)"));
    localLibraryNameLineEditTextChanged(mUi->edtLocalName->text());

    // tab "download ZIP": set placeholder texts and hide widgets
    mUi->edtDownloadZipUrl->setPlaceholderText(tr("e.g. https://github.com/LibrePCB-Libraries/LibrePCB_Base.lplib/archive/master.zip"));
    mUi->prgDownloadZipProgress->setVisible(false);
    mUi->btnDownloadZipAbort->setVisible(false);
    mUi->lblDownloadZipStatusMsg->setText("");

    // select the default tab
    mUi->tabWidget->setCurrentIndex(0);
}

AddLibraryWidget::~AddLibraryWidget() noexcept
{
    clearRepositoryLibraryList();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void AddLibraryWidget::updateInstalledStatusOfRepositoryLibraries() noexcept
{
    for (int i = 0; i < mUi->lstRepoLibs->count(); i++) {
        QListWidgetItem* item = mUi->lstRepoLibs->item(i); Q_ASSERT(item);
        auto* widget = dynamic_cast<RepositoryLibraryListWidgetItem*>(
                           mUi->lstRepoLibs->itemWidget(item));
        if (widget) widget->updateInstalledStatus();
    }
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void AddLibraryWidget::currentTabChanged(int index) noexcept
{
    if (mUi->tabWidget->widget(index) == mUi->tabDownloadFromRepo) {
        // tab "download from repository": request list of libraries
        clearRepositoryLibraryList();
        QList<const Repository*> repos = mWorkspace.getSettings().getRepositories().getRepositories();
        foreach (const Repository* repo, repos) { Q_ASSERT(repo);
            connect(repo, &Repository::libraryListReceived,
                    this, &AddLibraryWidget::repositoryLibraryListReceived);
            connect(repo, &Repository::errorWhileFetchingLibraryList,
                    this, &AddLibraryWidget::errorWhileFetchingLibraryList);
            repo->requestLibraryList();
        }
    }
}

void AddLibraryWidget::localLibraryNameLineEditTextChanged(QString name) noexcept
{
    if (name.isEmpty()) name = mUi->edtLocalName->placeholderText();
    QString dirname = FilePath::cleanFileName(name, FilePath::ReplaceSpaces | FilePath::KeepCase);
    if (!dirname.endsWith(".lplib")) dirname.append(".lplib");
    mUi->edtLocalDirectory->setPlaceholderText(dirname);
}

void AddLibraryWidget::downloadZipUrlLineEditTextChanged(QString urlStr) noexcept
{
    QString left = urlStr.left(urlStr.indexOf(".lplib", Qt::CaseInsensitive));
    QString libName = left.right(left.length() - left.lastIndexOf("/"));
    if (libName == urlStr) {libName = QUrl(urlStr).fileName();}
    QString dirname = FilePath::cleanFileName(libName, FilePath::ReplaceSpaces | FilePath::KeepCase);
    if (dirname.contains(".zip")) {dirname.remove(".zip");}
    if (!dirname.isEmpty()) {dirname.append(".lplib");}
    mUi->edtDownloadZipDirectory->setPlaceholderText(dirname);
}

void AddLibraryWidget::createLocalLibraryButtonClicked() noexcept
{
    // get attributes
    QString name = getTextOrPlaceholderFromQLineEdit(mUi->edtLocalName, false);
    QString desc = getTextOrPlaceholderFromQLineEdit(mUi->edtLocalDescription, false);
    QString author = getTextOrPlaceholderFromQLineEdit(mUi->edtLocalAuthor, false);
    QString versionStr = getTextOrPlaceholderFromQLineEdit(mUi->edtLocalVersion, false);
    Version version(versionStr);
    QString urlStr = mUi->edtLocalUrl->text().trimmed();
    QUrl url = QUrl::fromUserInput(urlStr);
    bool useCc0License = mUi->cbxLocalCc0License->isChecked();
    QString directoryStr = getTextOrPlaceholderFromQLineEdit(mUi->edtLocalDirectory, true);
    if ((!directoryStr.isEmpty()) && (!directoryStr.endsWith(".lplib"))) {
        directoryStr.append(".lplib");
    }
    FilePath directory = mWorkspace.getLibrariesPath().getPathTo("local/" % directoryStr);

    // check attributes validity
    if (name.isEmpty()) {
        QMessageBox::critical(this, tr("Invalid Input"), tr("Please enter a name."));
        return;
    }
    if (author.isEmpty()) {
        QMessageBox::critical(this, tr("Invalid Input"), tr("Please enter an author."));
        return;
    }
    if (!version.isValid()) {
        QMessageBox::critical(this, tr("Invalid Input"), tr("The specified version number is not valid."));
        return;
    }
    if (!url.isValid() && !urlStr.isEmpty()) {
        QMessageBox::critical(this, tr("Invalid Input"), tr("The specified URL is not valid."));
        return;
    }
    if (directoryStr.isEmpty()) {
        QMessageBox::critical(this, tr("Invalid Input"), tr("Please enter a directory name."));
        return;
    }
    if (directory.isExistingFile() || directory.isExistingDir()) {
        QMessageBox::critical(this, tr("Invalid Input"), tr("The specified directory exists already."));
        return;
    }

    try {
        // create the new library
        QScopedPointer<library::Library> lib(new library::Library(
            Uuid::createRandom(), version, author, name, desc, QString("")));
        lib->setUrl(url);
        lib->saveTo(directory); // can throw

        // copy license file
        if (useCc0License) {
            try {
                FilePath source = qApp->getResourcesDir().getPathTo("licenses/cc0-1.0.txt");
                FilePath destination = directory.getPathTo("LICENSE.txt");
                FileUtils::copyFile(source, destination); // can throw
            } catch (Exception& e) {
                qCritical() << "Could not copy the license file:" << e.getUserMsg();
            }
        }

        // copy readme file
        try {
            FilePath source = qApp->getResourcesDir().getPathTo("library/readme_template");
            FilePath destination = directory.getPathTo("README.md");
            QByteArray content = FileUtils::readFile(source); // can throw
            content.replace("{LIBRARY_NAME}", name.toUtf8());
            if (useCc0License) {
                content.replace("{LICENSE_TEXT}", "Creative Commons (CC0-1.0). For the "
                                "license text, see [LICENSE.txt](LICENSE.txt).");
            } else {
                content.replace("{LICENSE_TEXT}", "No license set.");
            }
            FileUtils::writeFile(destination, content); // can throw
        } catch (Exception& e) {
            qCritical() << "Could not copy the readme file:" << e.getUserMsg();
        }

        // copy .gitignore
        try {
            FilePath source = qApp->getResourcesDir().getPathTo("library/gitignore_template");
            FilePath destination = directory.getPathTo(".gitignore");
            FileUtils::copyFile(source, destination); // can throw
        } catch (Exception& e) {
            qCritical() << "Could not copy the .gitignore file:" << e.getUserMsg();
        }

        // copy .gitattributes
        try {
            FilePath source = qApp->getResourcesDir().getPathTo("library/gitattributes_template");
            FilePath destination = directory.getPathTo(".gitattributes");
            FileUtils::copyFile(source, destination); // can throw
        } catch (Exception& e) {
            qCritical() << "Could not copy the .gitattributes file:" << e.getUserMsg();
        }

        // add the new library to the workspace
        mWorkspace.addLocalLibrary(directory.getFilename()); // can throw

        // library successfully added! reset input fields and emit signal
        mUi->edtLocalName->clear();
        mUi->edtLocalDescription->clear();
        mUi->edtLocalAuthor->clear();
        mUi->edtLocalVersion->clear();
        mUi->edtLocalUrl->clear();
        mUi->cbxLocalCc0License->setChecked(false);
        mUi->edtLocalDirectory->clear();
        emit libraryAdded(directory, true);
    } catch (Exception& e) {
        QMessageBox::critical(this, tr("Error"), e.getUserMsg());
    }
}

void AddLibraryWidget::downloadZippedLibraryButtonClicked() noexcept
{
    if (mManualLibraryDownload) {
        QMessageBox::critical(this, tr("Busy"), tr("A download is already running."));
        return;
    }

    // get attributes
    QUrl url = QUrl::fromUserInput(mUi->edtDownloadZipUrl->text().trimmed());
    QString dirStr = getTextOrPlaceholderFromQLineEdit(mUi->edtDownloadZipDirectory, true);
    if ((!dirStr.isEmpty()) && (!dirStr.endsWith(".lplib"))) {
        dirStr.append(".lplib");
    }
    FilePath extractToDir = mWorkspace.getLibrariesPath().getPathTo("local/" % dirStr);

    // check attributes validity
    if (!url.isValid()) {
        QMessageBox::critical(this, tr("Invalid Input"), tr("Please enter a valid URL."));
        return;
    }
    if ((dirStr.isEmpty()) || (!extractToDir.isValid())) {
        QMessageBox::critical(this, tr("Invalid Input"), tr("Please enter a valid directory."));
        return;
    }
    if (extractToDir.isExistingFile() || extractToDir.isExistingDir()) {
        QMessageBox::critical(this, tr("Directory exists already"),
            QString(tr("The directory \"%1\" exists already.")).arg(extractToDir.toNative()));
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
    connect(mManualLibraryDownload.data(), &LibraryDownload::finished,
            this, &AddLibraryWidget::downloadZipFinished);
    connect(mUi->btnDownloadZipAbort, &QPushButton::clicked,
            mManualLibraryDownload.data(), &LibraryDownload::abort);
    mManualLibraryDownload->start();
}

void AddLibraryWidget::downloadZipFinished(bool success, const QString& errMsg) noexcept
{
    Q_ASSERT(mManualLibraryDownload);

    if (success) {
        try {
            // add library to workspace
            mWorkspace.addLocalLibrary(mManualLibraryDownload->getDestinationDir().getFilename());

            // finish
            mUi->lblDownloadZipStatusMsg->setText("");
            emit libraryAdded(mManualLibraryDownload->getDestinationDir(), true);
        } catch (const Exception& e) {
            mUi->lblDownloadZipStatusMsg->setText(e.getUserMsg());
        }
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

void AddLibraryWidget::repositoryLibraryListReceived(const QJsonArray& libs) noexcept
{
    foreach (const QJsonValue& libVal, libs) {
        RepositoryLibraryListWidgetItem* widget = new RepositoryLibraryListWidgetItem(
                                                      mWorkspace, libVal.toObject());
        connect(widget, &RepositoryLibraryListWidgetItem::checkedChanged,
                this, &AddLibraryWidget::repoLibraryDownloadCheckedChanged);
        connect(widget, &RepositoryLibraryListWidgetItem::libraryAdded,
                this, &AddLibraryWidget::libraryAdded);
        QListWidgetItem* item = new QListWidgetItem(mUi->lstRepoLibs);
        item->setSizeHint(widget->sizeHint());
        mUi->lstRepoLibs->setItemWidget(item, widget);
    }
}

void AddLibraryWidget::errorWhileFetchingLibraryList(const QString& errorMsg) noexcept
{
    QListWidgetItem* item = new QListWidgetItem(errorMsg, mUi->lstRepoLibs);
    item->setBackgroundColor(Qt::red);
    item->setForeground(Qt::white);
}

void AddLibraryWidget::clearRepositoryLibraryList() noexcept
{
    QList<const Repository*> repos = mWorkspace.getSettings().getRepositories().getRepositories();
    foreach (const Repository* repo, repos) { Q_ASSERT(repo);
        disconnect(repo, &Repository::libraryListReceived,
                   this, &AddLibraryWidget::repositoryLibraryListReceived);
    }
    for (int i = mUi->lstRepoLibs->count()-1; i >= 0; i--) {
        QListWidgetItem* item = mUi->lstRepoLibs->item(i); Q_ASSERT(item);
        delete mUi->lstRepoLibs->itemWidget(item);
        delete item;
    }
    Q_ASSERT(mUi->lstRepoLibs->count() == 0);
}

void AddLibraryWidget::repoLibraryDownloadCheckedChanged(bool checked) noexcept
{
    if (checked) {
        // one more library is checked, check all dependencies too
        QSet<Uuid> libs;
        for (int i = 0; i < mUi->lstRepoLibs->count(); i++) {
            QListWidgetItem* item = mUi->lstRepoLibs->item(i); Q_ASSERT(item);
            auto* widget = dynamic_cast<RepositoryLibraryListWidgetItem*>(
                               mUi->lstRepoLibs->itemWidget(item));
            if (widget && widget->isChecked()) {
                libs.unite(widget->getDependencies());
            }
        }
        for (int i = 0; i < mUi->lstRepoLibs->count(); i++) {
            QListWidgetItem* item = mUi->lstRepoLibs->item(i); Q_ASSERT(item);
            auto* widget = dynamic_cast<RepositoryLibraryListWidgetItem*>(
                               mUi->lstRepoLibs->itemWidget(item));
            if (widget && (libs.contains(widget->getUuid()))) {
                widget->setChecked(true);
            }
        }
    } else {
        // one library was unchecked, uncheck all libraries with missing dependencies
        QSet<Uuid> libs;
        for (int i = 0; i < mUi->lstRepoLibs->count(); i++) {
            QListWidgetItem* item = mUi->lstRepoLibs->item(i); Q_ASSERT(item);
            auto* widget = dynamic_cast<RepositoryLibraryListWidgetItem*>(
                               mUi->lstRepoLibs->itemWidget(item));
            if (widget && widget->isChecked()) {
                libs.insert(widget->getUuid());
            }
        }
        for (int i = 0; i < mUi->lstRepoLibs->count(); i++) {
            QListWidgetItem* item = mUi->lstRepoLibs->item(i); Q_ASSERT(item);
            auto* widget = dynamic_cast<RepositoryLibraryListWidgetItem*>(
                               mUi->lstRepoLibs->itemWidget(item));
            if (widget && (!libs.contains(widget->getDependencies()))) {
                widget->setChecked(false);
            }
        }
    }
}

void AddLibraryWidget::downloadLibrariesFromRepositoryButtonClicked() noexcept
{
    for (int i = 0; i < mUi->lstRepoLibs->count(); i++) {
        QListWidgetItem* item = mUi->lstRepoLibs->item(i); Q_ASSERT(item);
        auto* widget = dynamic_cast<RepositoryLibraryListWidgetItem*>(
                           mUi->lstRepoLibs->itemWidget(item));
        if (widget) {
            widget->startDownloadIfSelected();
        } else {
            qWarning() << "Invalid item widget detected.";
        }
    }
}

/*****************************************************************************************
 *  Private Static Methods
 ****************************************************************************************/

QString AddLibraryWidget::getTextOrPlaceholderFromQLineEdit(QLineEdit* edit, bool isFilename) noexcept
{
    if (edit) {
        QString text = edit->text().trimmed();
        QString placeholder = edit->placeholderText().trimmed();
        QString retval = (text.length() > 0) ? text : placeholder;
        if (isFilename) {
            return FilePath::cleanFileName(retval, FilePath::ReplaceSpaces | FilePath::KeepCase);
        } else {
            return retval;
        }
    } else {
        return QString("");
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace workspace
} // namespace librepcb
