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

#ifndef LIBREPCB_EDITOR_COMPONENTVARIANTLISTMODEL_H
#define LIBREPCB_EDITOR_COMPONENTVARIANTLISTMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <librepcb/core/library/cmp/componentsymbolvariant.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Component;
class Workspace;

namespace editor {

class ComponentSignalNameListModel;
class ComponentVariantEditor;
class GraphicsLayerList;
class LibraryElementCache;
class UndoCommand;
class UndoStack;

/*******************************************************************************
 *  Class ComponentVariantListModel
 ******************************************************************************/

/**
 * @brief The ComponentVariantListModel class
 */
class ComponentVariantListModel final
  : public QObject,
    public slint::Model<ui::ComponentVariantData> {
  Q_OBJECT

public:
  // Constructors / Destructor
  ComponentVariantListModel() = delete;
  ComponentVariantListModel(const ComponentVariantListModel& other) = delete;
  explicit ComponentVariantListModel(const Workspace& ws,
                                     const GraphicsLayerList& layers,
                                     const LibraryElementCache& cache,
                                     QObject* parent = nullptr) noexcept;
  ~ComponentVariantListModel() noexcept;

  // General Methods
  void setReferences(ComponentSymbolVariantList* list,
                     QPointer<Component> component,
                     const std::shared_ptr<ComponentSignalNameListModel>& sigs,
                     UndoStack* stack, const bool* wizardMode) noexcept;
  slint::Image renderScene(int variant, int gate, float width,
                           float height) noexcept;
  void add() noexcept;

  // Implementations
  std::size_t row_count() const override;
  std::optional<ui::ComponentVariantData> row_data(
      std::size_t i) const override;
  void set_row_data(std::size_t i,
                    const ui::ComponentVariantData& data) noexcept override;

  // Operator Overloadings
  ComponentVariantListModel& operator=(const ComponentVariantListModel& rhs) =
      delete;

private:
  void trigger(int index, std::shared_ptr<ComponentSymbolVariant> obj,
               ui::ComponentVariantAction a) noexcept;
  void listEdited(const ComponentSymbolVariantList& list, int index,
                  const std::shared_ptr<const ComponentSymbolVariant>& variant,
                  ComponentSymbolVariantList::Event event) noexcept;
  void variantUiDataChanged() noexcept;
  void execCmd(UndoCommand* cmd);

private:
  const Workspace& mWorkspace;
  const GraphicsLayerList& mLayers;
  const LibraryElementCache& mCache;
  QPointer<Component> mComponent;
  std::shared_ptr<ComponentSignalNameListModel> mSignals;
  ComponentSymbolVariantList* mList;
  QPointer<UndoStack> mUndoStack;
  const bool* mWizardMode;

  QList<std::shared_ptr<ComponentVariantEditor>> mItems;

  // Slots
  ComponentSymbolVariantList::OnEditedSlot mOnEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
