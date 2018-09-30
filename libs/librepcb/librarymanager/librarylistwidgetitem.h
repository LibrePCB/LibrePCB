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

#ifndef LIBREPCB_WORKSPACE_LIBRARYLISTWIDGETITEM_H
#define LIBREPCB_WORKSPACE_LIBRARYLISTWIDGETITEM_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/exceptions.h>

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

namespace Ui {
class LibraryListWidgetItem;
}

/*******************************************************************************
 *  Class LibraryListWidgetItem
 ******************************************************************************/

/**
 * @brief The LibraryListWidgetItem class
 *
 * @author ubruhin
 * @date 2016-08-03
 */
class LibraryListWidgetItem final : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  LibraryListWidgetItem() noexcept;
  LibraryListWidgetItem(const LibraryListWidgetItem& other) = delete;
  LibraryListWidgetItem(workspace::Workspace&   ws,
                        QSharedPointer<Library> lib) noexcept;
  ~LibraryListWidgetItem() noexcept;

  // Getters
  QSharedPointer<Library> getLibrary() const noexcept { return mLib; }
  QString                 getName() const noexcept;
  bool                    isRemoteLibrary() const noexcept;

  // Operator Overloadings
  LibraryListWidgetItem& operator=(const LibraryListWidgetItem& rhs) = delete;

protected:  // Methods
  void mouseDoubleClickEvent(QMouseEvent* e) noexcept override;

signals:
  void openLibraryEditorTriggered(QSharedPointer<Library> lib);

private:  // Data
  QScopedPointer<Ui::LibraryListWidgetItem> mUi;
  workspace::Workspace&                     mWorkspace;
  QSharedPointer<Library>                   mLib;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace manager
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_WORKSPACE_LIBRARYLISTWIDGETITEM_H
