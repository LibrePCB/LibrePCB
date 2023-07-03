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

#ifndef LIBREPCB_EDITOR_COMPONENTSYMBOLVARIANTITEMLISTEDITORWIDGET_H
#define LIBREPCB_EDITOR_COMPONENTSYMBOLVARIANTITEMLISTEDITORWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/library/cmp/componentsymbolvariantitem.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Workspace;

namespace editor {

class ComponentSymbolVariantItemListModel;
class EditableTableWidget;
class IF_GraphicsLayerProvider;
class LibraryElementCache;
class UndoStack;

/*******************************************************************************
 *  Class ComponentSymbolVariantItemListEditorWidget
 ******************************************************************************/

/**
 * @brief The ComponentSymbolVariantItemListEditorWidget class
 */
class ComponentSymbolVariantItemListEditorWidget final : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit ComponentSymbolVariantItemListEditorWidget(
      QWidget* parent = nullptr) noexcept;
  ComponentSymbolVariantItemListEditorWidget(
      const ComponentSymbolVariantItemListEditorWidget& other) = delete;
  ~ComponentSymbolVariantItemListEditorWidget() noexcept;

  // Setters
  void setReadOnly(bool readOnly) noexcept;
  void setReferences(
      const Workspace& ws, const IF_GraphicsLayerProvider& layerProvider,
      ComponentSymbolVariantItemList& items,
      const std::shared_ptr<const LibraryElementCache>& symbolCache,
      UndoStack* undoStack) noexcept;
  void resetReferences() noexcept;

  // Operator Overloadings
  ComponentSymbolVariantItemListEditorWidget& operator=(
      const ComponentSymbolVariantItemListEditorWidget& rhs) = delete;

signals:
  void edited();
  void triggerGraphicsItemsUpdate();
  void triggerGraphicsItemsTextsUpdate();

private:  // Methods
  void itemListEdited(
      const ComponentSymbolVariantItemList& list, int index,
      const std::shared_ptr<const ComponentSymbolVariantItem>& item,
      ComponentSymbolVariantItemList::Event event) noexcept;
  void itemEdited(const ComponentSymbolVariantItemList& list, int index,
                  const std::shared_ptr<const ComponentSymbolVariantItem>& item,
                  ComponentSymbolVariantItem::Event event) noexcept;
  void btnSymbolBrowseClicked(const QPersistentModelIndex& itemIndex) noexcept;

private:  // Data
  QScopedPointer<ComponentSymbolVariantItemListModel> mModel;
  QScopedPointer<EditableTableWidget> mView;
  const Workspace* mWorkspace;
  const IF_GraphicsLayerProvider* mLayerProvider;

  // Slots
  ComponentSymbolVariantItemList::OnEditedSlot mOnItemListEditedSlot;
  ComponentSymbolVariantItemList::OnElementEditedSlot mOnItemEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
