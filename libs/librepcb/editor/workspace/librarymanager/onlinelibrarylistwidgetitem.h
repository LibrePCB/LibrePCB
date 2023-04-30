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

#ifndef LIBREPCB_EDITOR_ONLINELIBRARYLISTWIDGETITEM_H
#define LIBREPCB_EDITOR_ONLINELIBRARYLISTWIDGETITEM_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/types/uuid.h>
#include <librepcb/core/types/version.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Workspace;

namespace editor {

class LibraryDownload;

namespace Ui {
class OnlineLibraryListWidgetItem;
}

/*******************************************************************************
 *  Class OnlineLibraryListWidgetItem
 ******************************************************************************/

/**
 * @brief The OnlineLibraryListWidgetItem class
 */
class OnlineLibraryListWidgetItem final : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  OnlineLibraryListWidgetItem() = delete;
  OnlineLibraryListWidgetItem(const OnlineLibraryListWidgetItem& other) =
      delete;
  OnlineLibraryListWidgetItem(Workspace& ws, const QJsonObject& obj) noexcept;
  ~OnlineLibraryListWidgetItem() noexcept;

  // Getters
  const tl::optional<Uuid>& getUuid() const noexcept { return mUuid; }
  const QString& getName() const noexcept { return mName; }
  const QSet<Uuid>& getDependencies() const noexcept { return mDependencies; }
  bool isChecked() const noexcept;

  // Setters
  void setChecked(bool checked) noexcept;

  // General Methods
  void startDownloadIfSelected() noexcept;

  // Operator Overloadings
  OnlineLibraryListWidgetItem& operator=(
      const OnlineLibraryListWidgetItem& rhs) = delete;

signals:
  void checkedChanged(bool checked);

private:  // Methods
  void downloadFinished(bool success, const QString& errMsg) noexcept;
  void iconReceived(const QByteArray& data) noexcept;
  void updateInstalledStatus() noexcept;

private:  // Data
  Workspace& mWorkspace;
  QJsonObject mJsonObject;
  tl::optional<Uuid> mUuid;
  QString mName;
  tl::optional<Version> mVersion;
  bool mIsRecommended;
  QSet<Uuid> mDependencies;
  QScopedPointer<Ui::OnlineLibraryListWidgetItem> mUi;
  QScopedPointer<LibraryDownload> mLibraryDownload;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
