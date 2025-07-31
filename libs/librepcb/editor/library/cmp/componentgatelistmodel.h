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

#ifndef LIBREPCB_EDITOR_COMPONENTGATELISTMODEL_H
#define LIBREPCB_EDITOR_COMPONENTGATELISTMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <librepcb/core/library/cmp/componentsymbolvariantitem.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Component;
class Workspace;

namespace editor {

class ComponentGateEditor;
class ComponentSignalNameListModel;
class GraphicsLayerList;
class GraphicsScene;
class LibraryElementCache;
class UndoCommand;
class UndoCommandGroup;
class UndoStack;

/*******************************************************************************
 *  Class ComponentGateListModel
 ******************************************************************************/

/**
 * @brief The ComponentGateListModel class
 */
class ComponentGateListModel final
  : public QObject,
    public slint::Model<ui::ComponentGateData> {
  Q_OBJECT

public:
  // Constructors / Destructor
  ComponentGateListModel() = delete;
  ComponentGateListModel(const ComponentGateListModel& other) = delete;
  explicit ComponentGateListModel(const Workspace& ws,
                                  const GraphicsLayerList& layers,
                                  const LibraryElementCache& cache,
                                  QObject* parent = nullptr) noexcept;
  ~ComponentGateListModel() noexcept;

  // General Methods
  void setReferences(ComponentSymbolVariantItemList* list,
                     QPointer<Component> component,
                     QPointer<GraphicsScene> componentScene,
                     const std::shared_ptr<ComponentSignalNameListModel>& sigs,
                     UndoStack* stack, const bool* wizardMode) noexcept;
  slint::Image renderScene(int gate, float width, float height) noexcept;
  void add();

  // Implementations
  std::size_t row_count() const override;
  std::optional<ui::ComponentGateData> row_data(std::size_t i) const override;
  void set_row_data(std::size_t i,
                    const ui::ComponentGateData& data) noexcept override;

  // Operator Overloadings
  ComponentGateListModel& operator=(const ComponentGateListModel& rhs) = delete;

private:
  void trigger(int index, std::shared_ptr<ComponentSymbolVariantItem> obj,
               ui::ComponentGateAction a) noexcept;
  void listEdited(const ComponentSymbolVariantItemList& list, int index,
                  const std::shared_ptr<const ComponentSymbolVariantItem>& item,
                  ComponentSymbolVariantItemList::Event event) noexcept;
  void gateUiDataChanged() noexcept;
  void execCmd(UndoCommand* cmd, bool updateSuffixes);
  std::unique_ptr<UndoCommandGroup> createSuffixUpdateCmd();

private:
  const Workspace& mWorkspace;
  const GraphicsLayerList& mLayers;
  const LibraryElementCache& mCache;
  QPointer<Component> mComponent;
  QPointer<GraphicsScene> mComponentScene;
  std::shared_ptr<ComponentSignalNameListModel> mSignals;
  ComponentSymbolVariantItemList* mList;
  QPointer<UndoStack> mUndoStack;
  const bool* mWizardMode;

  QList<std::shared_ptr<ComponentGateEditor>> mItems;

  // Slots
  ComponentSymbolVariantItemList::OnEditedSlot mOnEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
