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

#ifndef LIBREPCB_LIBRARY_EDITOR_LIBRARYLISTEDITORWIDGET_H
#define LIBREPCB_LIBRARY_EDITOR_LIBRARYLISTEDITORWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/uuid.h>

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
namespace editor {

namespace Ui {
class LibraryListEditorWidget;
}

/*******************************************************************************
 *  Class LibraryListEditorWidget
 ******************************************************************************/

/**
 * @brief The LibraryListEditorWidget class
 */
class LibraryListEditorWidget final : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  LibraryListEditorWidget() = delete;
  explicit LibraryListEditorWidget(const workspace::Workspace& ws,
                                   QWidget* parent = nullptr) noexcept;
  LibraryListEditorWidget(const LibraryListEditorWidget& other) = delete;
  ~LibraryListEditorWidget() noexcept;

  // Getters
  const QSet<Uuid>& getUuids() const noexcept { return mUuids; }

  // Setters
  void setUuids(const QSet<Uuid>& uuids) noexcept;

  // Operator Overloadings
  LibraryListEditorWidget& operator=(const LibraryListEditorWidget& rhs) =
      delete;

private:
  void btnAddClicked() noexcept;
  void btnRemoveClicked() noexcept;
  void addItem(const Uuid& library) noexcept;

signals:
  void edited();
  void libraryAdded(const Uuid& lib);
  void libraryRemoved(const Uuid& lib);

protected:  // Data
  const workspace::Workspace&                 mWorkspace;
  QScopedPointer<Ui::LibraryListEditorWidget> mUi;
  QSet<Uuid>                                  mUuids;
  QHash<Uuid, QString>                        mLibNames;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_EDITOR_LIBRARYLISTEDITORWIDGET_H
