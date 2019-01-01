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

#ifndef LIBREPCB_WORKSPACE_LIBRARYMANAGER_H
#define LIBREPCB_WORKSPACE_LIBRARYMANAGER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
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

class Library;

namespace manager {

class AddLibraryWidget;
class LibraryListWidgetItem;

namespace Ui {
class LibraryManager;
}

/*******************************************************************************
 *  Class LibraryManager
 ******************************************************************************/

/**
 * @brief The LibraryManager class
 *
 * @author ubruhin
 * @date 2016-08-03
 */
class LibraryManager final : public QMainWindow {
  Q_OBJECT

public:
  // Constructors / Destructor
  LibraryManager()                            = delete;
  LibraryManager(const LibraryManager& other) = delete;
  LibraryManager(workspace::Workspace& ws, QWidget* parent = nullptr) noexcept;
  ~LibraryManager() noexcept;

  // General Methods
  void updateRepositoryLibraryList() noexcept;

  // Operator Overloadings
  LibraryManager& operator=(const LibraryManager& rhs) = delete;

private:  // Methods
  void closeEvent(QCloseEvent* event) noexcept override;
  void clearLibraryList() noexcept;
  void updateLibraryList() noexcept;
  void currentListItemChanged(QListWidgetItem* current,
                              QListWidgetItem* previous) noexcept;
  void libraryAddedSlot(const FilePath& libDir) noexcept;

  static bool widgetsLessThan(const LibraryListWidgetItem* a,
                              const LibraryListWidgetItem* b) noexcept;

signals:
  void openLibraryEditorTriggered(const FilePath& libDir);

private:  // Data
  workspace::Workspace&              mWorkspace;
  QScopedPointer<Ui::LibraryManager> mUi;
  QScopedPointer<AddLibraryWidget>   mAddLibraryWidget;
  QWidget*                           mCurrentWidget;
  FilePath                           mSelectedLibrary;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace manager
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_WORKSPACE_LIBRARYMANAGER_H
