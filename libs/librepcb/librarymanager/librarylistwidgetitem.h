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

#ifndef LIBREPCB_LIBRARYMANAGER_LIBRARYLISTWIDGETITEM_H
#define LIBREPCB_LIBRARYMANAGER_LIBRARYLISTWIDGETITEM_H

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
namespace manager {

namespace Ui {
class LibraryListWidgetItem;
}

/*******************************************************************************
 *  Class LibraryListWidgetItem
 ******************************************************************************/

/**
 * @brief The LibraryListWidgetItem class
 */
class LibraryListWidgetItem final : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  LibraryListWidgetItem() noexcept;
  LibraryListWidgetItem(const LibraryListWidgetItem& other) = delete;
  LibraryListWidgetItem(workspace::Workspace& ws, const FilePath& libDir,
                        const QString& name = "",
                        const QString& description = "",
                        const QPixmap& icon = QPixmap()) noexcept;
  ~LibraryListWidgetItem() noexcept;

  // Getters
  const FilePath& getLibraryFilePath() const noexcept { return mLibDir; }
  QString getName() const noexcept;
  bool isRemoteLibrary() const noexcept { return mIsRemoteLibrary; }

  // Operator Overloadings
  LibraryListWidgetItem& operator=(const LibraryListWidgetItem& rhs) = delete;

protected:  // Methods
  void mouseDoubleClickEvent(QMouseEvent* e) noexcept override;

signals:
  void openLibraryEditorTriggered(const FilePath& libDir);

private:  // Data
  QScopedPointer<Ui::LibraryListWidgetItem> mUi;
  FilePath mLibDir;
  bool mIsRemoteLibrary;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace manager
}  // namespace library
}  // namespace librepcb

#endif
