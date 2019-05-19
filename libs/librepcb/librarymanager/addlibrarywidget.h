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

#ifndef LIBREPCB_WORKSPACE_ADDLIBRARYWIDGET_H
#define LIBREPCB_WORKSPACE_ADDLIBRARYWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/exceptions.h>
#include <librepcb/common/fileio/filepath.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

namespace workspace {
class Workspace;
}

namespace library {
namespace manager {

class LibraryDownload;

namespace Ui {
class AddLibraryWidget;
}

/*******************************************************************************
 *  Class AddLibraryWidget
 ******************************************************************************/

/**
 * @brief The AddLibraryWidget class
 */
class AddLibraryWidget final : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  AddLibraryWidget() noexcept;
  AddLibraryWidget(const AddLibraryWidget& other) = delete;
  explicit AddLibraryWidget(workspace::Workspace& ws) noexcept;
  ~AddLibraryWidget() noexcept;

  // General Methods
  void updateRepositoryLibraryList() noexcept;

  // Operator Overloadings
  AddLibraryWidget& operator=(const AddLibraryWidget& rhs) = delete;

signals:
  void libraryAdded(const FilePath& libDir);

private:  // Methods
  void localLibraryNameLineEditTextChanged(QString name) noexcept;
  void downloadZipUrlLineEditTextChanged(QString urlStr) noexcept;
  void createLocalLibraryButtonClicked() noexcept;
  void downloadZippedLibraryButtonClicked() noexcept;
  void downloadZipFinished(bool success, const QString& errMsg) noexcept;
  void repositoryLibraryListReceived(const QJsonArray& libs) noexcept;
  void errorWhileFetchingLibraryList(const QString& errorMsg) noexcept;
  void clearRepositoryLibraryList() noexcept;
  void repoLibraryDownloadCheckedChanged(bool checked) noexcept;
  void downloadLibrariesFromRepositoryButtonClicked() noexcept;

  static QString getTextOrPlaceholderFromQLineEdit(QLineEdit* edit,
                                                   bool isFilename) noexcept;

private:  // Data
  workspace::Workspace&                mWorkspace;
  QScopedPointer<Ui::AddLibraryWidget> mUi;
  QScopedPointer<LibraryDownload>      mManualLibraryDownload;
  QList<QMetaObject::Connection>       mLibraryDownloadConnections;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace manager
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_WORKSPACE_ADDLIBRARYWIDGET_H
