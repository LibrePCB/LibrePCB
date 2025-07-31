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

#ifndef LIBREPCB_EDITOR_COMPONENTGATEEDITOR_H
#define LIBREPCB_EDITOR_COMPONENTGATEEDITOR_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Component;
class ComponentSymbolVariantItem;
class Symbol;
class Workspace;

namespace editor {

class ComponentPinoutListModel;
class ComponentSignalNameListModel;
class GraphicsLayerList;
class GraphicsScene;
class LibraryElementCache;
class SymbolGraphicsItem;
class UndoCommand;
class UndoStack;

/*******************************************************************************
 *  Class ComponentGateEditor
 ******************************************************************************/

/**
 * @brief The ComponentGateEditor class
 */
class ComponentGateEditor final : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  ComponentGateEditor() = delete;
  ComponentGateEditor(const ComponentGateEditor& other) = delete;
  explicit ComponentGateEditor(
      const Workspace& ws, const GraphicsLayerList& layers,
      const LibraryElementCache& cache, QPointer<const Component> component,
      QPointer<GraphicsScene> componentScene,
      const std::shared_ptr<ComponentSignalNameListModel>& sigs,
      std::shared_ptr<ComponentSymbolVariantItem> gate, UndoStack* stack,
      QObject* parent = nullptr) noexcept;
  ~ComponentGateEditor() noexcept;

  // General Methods
  ui::ComponentGateData getUiData() const;
  void setUiData(const ui::ComponentGateData& data) noexcept;
  slint::Image renderScene(float width, float height) noexcept;
  void chooseSymbol();
  void refreshPreview() noexcept;

  // Operator Overloadings
  ComponentGateEditor& operator=(const ComponentGateEditor& rhs) = delete;

signals:
  void uiDataChanged();

private:
  void reloadSymbol() noexcept;
  void execCmd(UndoCommand* cmd);

private:
  const Workspace& mWorkspace;
  const GraphicsLayerList& mLayers;
  const LibraryElementCache& mCache;
  const QPointer<const Component> mComponent;
  QPointer<GraphicsScene> mComponentScene;
  std::shared_ptr<ComponentSignalNameListModel> mSignals;
  std::shared_ptr<ComponentSymbolVariantItem> mGate;
  UndoStack* mUndoStack;
  QCollator mCollator;
  int mFrameIndex;

  std::shared_ptr<const Symbol> mSymbol;
  std::unique_ptr<GraphicsScene> mScene;
  std::unique_ptr<SymbolGraphicsItem> mGraphicsItem;
  std::unique_ptr<SymbolGraphicsItem> mComponentGraphicsItem;
  QMetaObject::Connection mCacheConnection;

  std::shared_ptr<ComponentPinoutListModel> mPinout;
  std::shared_ptr<slint::SortModel<ui::ComponentPinoutData>> mPinoutSorted;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
