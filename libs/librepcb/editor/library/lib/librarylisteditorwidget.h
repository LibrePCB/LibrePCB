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

#ifndef LIBREPCB_EDITOR_LIBRARYLISTEDITORWIDGET_H
#define LIBREPCB_EDITOR_LIBRARYLISTEDITORWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../modelview/editablelistmodel.h"

#include <librepcb/core/types/uuid.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Workspace;

namespace editor {

class SortFilterProxyModel;

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

  typedef EditableListModel<QList<Uuid>> Model;

public:
  // Constructors / Destructor
  LibraryListEditorWidget() = delete;
  explicit LibraryListEditorWidget(const Workspace& ws,
                                   QWidget* parent = nullptr) noexcept;
  LibraryListEditorWidget(const LibraryListEditorWidget& other) = delete;
  ~LibraryListEditorWidget() noexcept;

  // Getters
  QSet<Uuid> getUuids() const noexcept;

  // Setters
  void setReadOnly(bool readOnly) noexcept;
  void setUuids(const QSet<Uuid>& uuids) noexcept;

  // Operator Overloadings
  LibraryListEditorWidget& operator=(const LibraryListEditorWidget& rhs) =
      delete;

signals:
  void edited();

protected:  // Data
  QScopedPointer<Model> mModel;
  QScopedPointer<SortFilterProxyModel> mProxyModel;
  QScopedPointer<Ui::LibraryListEditorWidget> mUi;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
