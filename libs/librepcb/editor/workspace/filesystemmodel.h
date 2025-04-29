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

#ifndef LIBREPCB_EDITOR_FILESYSTEMMODEL_H
#define LIBREPCB_EDITOR_FILESYSTEMMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <librepcb/core/fileio/filepath.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Workspace;

namespace editor {

class QuickAccessModel;

/*******************************************************************************
 *  Class FileSystemModel
 ******************************************************************************/

/**
 * @brief The FileSystemModel class
 */
class FileSystemModel : public QObject,
                        public slint::Model<ui::TreeViewItemData> {
  Q_OBJECT

public:
  // Constructors / Destructor
  FileSystemModel() = delete;
  FileSystemModel(const FileSystemModel& other) = delete;
  explicit FileSystemModel(const Workspace& ws, const FilePath& root,
                           const QString& settingsPrefix,
                           QuickAccessModel* quickAccessModel = nullptr,
                           QObject* parent = nullptr) noexcept;
  virtual ~FileSystemModel() noexcept;

  // Implementations
  std::size_t row_count() const override;
  std::optional<ui::TreeViewItemData> row_data(std::size_t i) const override;
  void set_row_data(std::size_t i,
                    const ui::TreeViewItemData& data) noexcept override;

  // Operator Overloadings
  FileSystemModel& operator=(const FileSystemModel& rhs) = delete;

signals:
  void openFileTriggered(const FilePath& fp);
  void newProjectTriggered(const FilePath& fp);

private:
  void expandDir(const FilePath& fp, std::size_t index, int level) noexcept;
  void collapseDir(const FilePath& fp, std::size_t index, int level) noexcept;
  void directoryChanged(const QString& dir) noexcept;
  void favoriteProjectChanged(const FilePath& fp, bool favorite) noexcept;
  void trigger(const FilePath& fp, ui::TreeViewItemAction a) noexcept;
  void removeFileOrDirectory(const FilePath& fp) noexcept;

  const Workspace& mWorkspace;
  const FilePath mRoot;
  const QString mSettingsPrefix;
  QPointer<QuickAccessModel> mQuickAccess;
  std::vector<ui::TreeViewItemData> mItems;
  QFileSystemWatcher mWatcher;
  QSet<FilePath> mExpandedDirs;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
