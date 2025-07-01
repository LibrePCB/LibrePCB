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

#include <librepcb/core/library/cmp/componentsymbolvariantitem.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Symbol;
class Workspace;

namespace editor {

class ComponentPinoutListModel;
class UndoCommand;
class UndoStack;
class GraphicsScene;
class SymbolGraphicsItem;
class GraphicsLayerList;

/*******************************************************************************
 *  Class ComponentGateEditor
 ******************************************************************************/

/**
 * @brief The ComponentGateEditor class
 */
class ComponentGateEditor : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  ComponentGateEditor() = delete;
  ComponentGateEditor(const ComponentGateEditor& other) = delete;
  explicit ComponentGateEditor(const Workspace& ws,
                               const GraphicsLayerList& layers,
                               std::shared_ptr<ComponentSymbolVariantItem> item,
                               QObject* parent = nullptr) noexcept;
  virtual ~ComponentGateEditor() noexcept;

  // General Methods
  ui::ComponentGateData getUiData() const;
  void setUiData(const ui::ComponentGateData& data) noexcept;
  void setUndoStack(UndoStack* stack) noexcept;
  // void apply();
  slint::Image renderScene(float width, float height) noexcept;

  // Operator Overloadings
  ComponentGateEditor& operator=(const ComponentGateEditor& rhs) = delete;

private:
  void execCmd(UndoCommand* cmd);
  void loadSymbol() noexcept;

private:
  const Workspace& mWorkspace;
  const GraphicsLayerList& mLayers;

  std::shared_ptr<ComponentSymbolVariantItem> mItem;
  UndoStack* mUndoStack;
  std::shared_ptr<ComponentPinoutListModel> mPinout;

  std::unique_ptr<Symbol> mSymbol;
  std::unique_ptr<GraphicsScene> mScene;
  std::unique_ptr<SymbolGraphicsItem> mGraphicsItem;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
