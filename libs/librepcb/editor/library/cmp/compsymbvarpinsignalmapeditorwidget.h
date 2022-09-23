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

#ifndef LIBREPCB_EDITOR_COMPSYMBVARPINSIGNALMAPEDITORWIDGET_H
#define LIBREPCB_EDITOR_COMPSYMBVARPINSIGNALMAPEDITORWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/library/cmp/componentsignal.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class ComponentSymbolVariant;

namespace editor {

class ComponentPinSignalMapModel;
class LibraryElementCache;
class SortFilterProxyModel;
class UndoStack;

/*******************************************************************************
 *  Class CompSymbVarPinSignalMapEditorWidget
 ******************************************************************************/

/**
 * @brief The CompSymbVarPinSignalMapEditorWidget class
 */
class CompSymbVarPinSignalMapEditorWidget final : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit CompSymbVarPinSignalMapEditorWidget(
      QWidget* parent = nullptr) noexcept;
  CompSymbVarPinSignalMapEditorWidget(
      const CompSymbVarPinSignalMapEditorWidget& other) = delete;
  ~CompSymbVarPinSignalMapEditorWidget() noexcept;

  // General Methods
  void setReadOnly(bool readOnly) noexcept;
  void setReferences(
      ComponentSymbolVariant* variant,
      const std::shared_ptr<const LibraryElementCache>& symbolCache,
      const ComponentSignalList* sigs, UndoStack* undoStack) noexcept;
  void resetReferences() noexcept;

  // Operator Overloadings
  CompSymbVarPinSignalMapEditorWidget& operator=(
      const CompSymbVarPinSignalMapEditorWidget& rhs) = delete;

private:
  QScopedPointer<ComponentPinSignalMapModel> mModel;
  QScopedPointer<SortFilterProxyModel> mProxy;
  QScopedPointer<QTableView> mView;
  QScopedPointer<QPushButton> mBtnAutoAssign;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
