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

#ifndef LIBREPCB_LIBRARY_EDITOR_FOOTPRINTLISTEDITORWIDGET_H
#define LIBREPCB_LIBRARY_EDITOR_FOOTPRINTLISTEDITORWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/library/pkg/footprint.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class UndoStack;
class EditableTableWidget;

namespace library {
namespace editor {

class FootprintListModel;

/*******************************************************************************
 *  Class FootprintListEditorWidget
 ******************************************************************************/

/**
 * @brief The FootprintListEditorWidget class
 */
class FootprintListEditorWidget final : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit FootprintListEditorWidget(QWidget* parent = nullptr) noexcept;
  FootprintListEditorWidget(const FootprintListEditorWidget& other) = delete;
  ~FootprintListEditorWidget() noexcept;

  // Setters
  void setReadOnly(bool readOnly) noexcept;
  void setReferences(FootprintList& list, UndoStack& stack) noexcept;

  // Operator Overloadings
  FootprintListEditorWidget& operator=(const FootprintListEditorWidget& rhs) =
      delete;

signals:
  void currentFootprintChanged(int index);

private:
  QScopedPointer<FootprintListModel> mModel;
  QScopedPointer<EditableTableWidget> mView;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_EDITOR_FOOTPRINTLISTEDITORWIDGET_H
