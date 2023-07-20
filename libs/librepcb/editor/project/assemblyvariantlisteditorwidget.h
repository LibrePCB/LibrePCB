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

#ifndef LIBREPCB_EDITOR_ASSEMBLYVARIANTLISTEDITORWIDGET_H
#define LIBREPCB_EDITOR_ASSEMBLYVARIANTLISTEDITORWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Circuit;

namespace editor {

class AssemblyVariantListModel;
class EditableTableWidget;
class UndoStack;

/*******************************************************************************
 *  Class AssemblyVariantListEditorWidget
 ******************************************************************************/

/**
 * @brief The AssemblyVariantListEditorWidget class
 */
class AssemblyVariantListEditorWidget final : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit AssemblyVariantListEditorWidget(QWidget* parent = nullptr) noexcept;
  AssemblyVariantListEditorWidget(
      const AssemblyVariantListEditorWidget& other) = delete;
  ~AssemblyVariantListEditorWidget() noexcept;

  // Setters
  void setFrameStyle(int style) noexcept;
  void setReadOnly(bool readOnly) noexcept;
  void setReferences(UndoStack* undoStack, Circuit* circuit) noexcept;

  // Operator Overloadings
  AssemblyVariantListEditorWidget& operator=(
      const AssemblyVariantListEditorWidget& rhs) = delete;

signals:
  void currentItemChanged(int index);

private:  // Data
  QScopedPointer<AssemblyVariantListModel> mModel;
  QScopedPointer<EditableTableWidget> mView;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
