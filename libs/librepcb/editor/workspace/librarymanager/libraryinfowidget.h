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

#ifndef LIBREPCB_EDITOR_LIBRARYINFOWIDGET_H
#define LIBREPCB_EDITOR_LIBRARYINFOWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/fileio/filepath.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Library;
class Workspace;

namespace editor {

namespace Ui {
class LibraryInfoWidget;
}

/*******************************************************************************
 *  Class LibraryInfoWidget
 ******************************************************************************/

/**
 * @brief The LibraryInfoWidget class
 */
class LibraryInfoWidget final : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  LibraryInfoWidget() = delete;
  LibraryInfoWidget(const LibraryInfoWidget& other) = delete;
  LibraryInfoWidget(Workspace& ws, const FilePath& libDir);
  ~LibraryInfoWidget() noexcept;

  // Getters
  QString getName() const noexcept;

  // Operator Overloadings
  LibraryInfoWidget& operator=(const LibraryInfoWidget& rhs) = delete;

signals:
  void openLibraryEditorTriggered(const FilePath& libDir);

private:  // Methods
  void btnOpenLibraryEditorClicked() noexcept;
  void btnRemoveLibraryClicked() noexcept;
  bool isRemoteLibrary() const noexcept;

private:  // Data
  QScopedPointer<Ui::LibraryInfoWidget> mUi;
  Workspace& mWorkspace;
  FilePath mLibDir;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
