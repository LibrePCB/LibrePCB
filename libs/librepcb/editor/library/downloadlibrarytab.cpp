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
#include "downloadlibrarytab.h"

#include "guiapplication.h"
#include "utils/slinthelpers.h"

#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>
#include <librepcb/editor/workspace/librarymanager/librarydownload.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

DownloadLibraryTab::DownloadLibraryTab(GuiApplication& app,
                                       QObject* parent) noexcept
  : WindowTab(app, parent),
    onDerivedUiDataChanged(*this),
    mUiData{
        slint::SharedString(),  // URL
        slint::SharedString(),  // URL error
        slint::SharedString(),  // URL suggestion
        slint::SharedString(),  // Directory
        slint::SharedString(),  // Directory default
        slint::SharedString(),  // Directory error
        false,  // Valid
        false,  // Download running
        0,  // Download progress
        slint::SharedString(),  // Download error
    } {
  validate();
}

DownloadLibraryTab::~DownloadLibraryTab() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

ui::TabData DownloadLibraryTab::getUiData() const noexcept {
  return ui::TabData{
      ui::TabType::DownloadLibrary,  // Type
      q2s(tr("Download Library")),  // Title
      ui::Action::None,  // Action
  };
}

void DownloadLibraryTab::setDerivedUiData(
    const ui::DownloadLibraryTabData& data) noexcept {
  mUiData = data;
  validate();
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

void DownloadLibraryTab::triggerAsync(ui::Action a) noexcept {
  switch (a) {
    case ui::Action::TabCancel: {
      if (mDownload) {
        mDownload.reset();
        mUiData.download_running = false;
        mUiData.download_progress = 0;
        mUiData.download_status = slint::SharedString();
        onDerivedUiDataChanged.notify();
      } else {
        emit closeRequested();
      }
      break;
    }

    case ui::Action::TabOk: {
      try {
        if ((!mUrl) || (!mDirectory.isValid()) || mDownload) {
          throw LogicError(__FILE__, __LINE__);
        }

        mUiData.download_running = true;
        mUiData.download_progress = 0;
        onDerivedUiDataChanged.notify();

        mDownload.reset(new LibraryDownload(*mUrl, mDirectory));
        connect(mDownload.get(), &LibraryDownload::progressState,
                [this](const QString& state) {
                  mUiData.download_status = q2s(state);
                  onDerivedUiDataChanged.notify();
                });
        connect(mDownload.get(), &LibraryDownload::progressPercent,
                [this](int percent) {
                  mUiData.download_progress = percent;
                  onDerivedUiDataChanged.notify();
                });
        connect(mDownload.get(), &LibraryDownload::finished, this,
                &DownloadLibraryTab::downloadFinished);
        mDownload->start();
      } catch (const Exception& e) {
        mUiData.download_status = q2s(e.getMsg());
        onDerivedUiDataChanged.notify();
      }
      break;
    }

    default: {
      WindowTab::triggerAsync(a);
      break;
    }
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void DownloadLibraryTab::validate() noexcept {
  const QString urlStr = s2q(mUiData.url);
  mUrl = validateUrl(urlStr, mUiData.url_error, false);

  mUiData.url_suggestion = slint::SharedString();
  if (mUrl && (!mUrl->toString().endsWith(".zip"))) {
    auto urlWithSuffix = [this](const QString& suffix) {
      QString url = mUrl->toString();
      while ((!url.isEmpty()) && (url.endsWith("/"))) {
        url.chop(1);
      }
      return url + suffix;
    };

    QString suggestion;
    if (mUrl->host().toLower().contains("github")) {
      suggestion = urlWithSuffix("/archive/refs/heads/master.zip");
    } else if (mUrl->host().toLower().contains("gitlab")) {
      const QString repo = mUrl->path().split("/", Qt::SkipEmptyParts).last();
      suggestion = urlWithSuffix("/-/archive/master/" % repo % "-master.zip");
    }
    mUiData.url_suggestion = q2s(QUrl(suggestion).toString());
  }

  QString left = urlStr.left(urlStr.indexOf(".lplib", Qt::CaseInsensitive));
  QString libName = left.right(left.length() - left.lastIndexOf("/"));
  if (libName == urlStr) {
    libName = mUrl ? mUrl->fileName() : QString();
  }
  libName.remove("-master");
  libName.remove("-main");
  QString dirDefault = FilePath::cleanFileName(
      libName, FilePath::ReplaceSpaces | FilePath::KeepCase);
  if (dirDefault.contains(".zip")) {
    dirDefault.remove(".zip");
  }
  if (!dirDefault.isEmpty()) {
    dirDefault.append(".lplib");
  }
  mUiData.directory_default = q2s(dirDefault);

  QString dirStr = s2q(mUiData.directory).trimmed();
  if (dirStr.isEmpty()) {
    dirStr = s2q(mUiData.directory_default);
  }
  const std::optional<FileProofName> dirName =
      validateFileProofName(dirStr, mUiData.directory_error, ".lplib");
  mDirectory = dirName
      ? mApp.getWorkspace().getLibrariesPath().getPathTo("local/" % *dirName)
      : FilePath();
  if (mDirectory.isValid() &&
      (mDirectory.isExistingFile() || mDirectory.isExistingDir())) {
    mDirectory = FilePath();
    mUiData.directory_error = q2s(tr("Exists already"));
  }

  mUiData.valid = mUrl && mDirectory.isValid();
  onDerivedUiDataChanged.notify();
}

void DownloadLibraryTab::downloadFinished(bool success,
                                          const QString& errMsg) noexcept {
  mDownload.reset();

  if (success) {
    // Force rescan to index the new library.
    mApp.getWorkspace().getLibraryDb().startLibraryRescan();
    emit closeRequested();
  } else {
    mUiData.download_status = q2s(errMsg);
    mUiData.download_running = false;
    mUiData.download_progress = 0;
    onDerivedUiDataChanged.notify();
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
