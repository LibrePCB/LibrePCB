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

#ifndef LIBREPCB_EDITOR_COMPONENTVARIANTEDITOR_H
#define LIBREPCB_EDITOR_COMPONENTVARIANTEDITOR_H

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
class ComponentSymbolVariant;
class Workspace;

namespace editor {

class ComponentGateListModel;
class ComponentSignalNameListModel;
class GraphicsLayerList;
class GraphicsScene;
class LibraryElementCache;
class UndoCommand;
class UndoStack;

/*******************************************************************************
 *  Class ComponentVariantEditor
 ******************************************************************************/

/**
 * @brief The ComponentVariantEditor class
 */
class ComponentVariantEditor final : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  ComponentVariantEditor() = delete;
  ComponentVariantEditor(const ComponentVariantEditor& other) = delete;
  explicit ComponentVariantEditor(
      const Workspace& ws, const GraphicsLayerList& layers,
      const LibraryElementCache& cache, QPointer<Component> component,
      const std::shared_ptr<ComponentSignalNameListModel>& sigs,
      std::shared_ptr<ComponentSymbolVariant> variant, UndoStack* stack,
      const bool* wizardMode, QObject* parent = nullptr) noexcept;
  ~ComponentVariantEditor() noexcept;

  // General Methods
  ui::ComponentVariantData getUiData() const;
  void setUiData(const ui::ComponentVariantData& data) noexcept;
  slint::Image renderScene(int gate, float width, float height) noexcept;
  void addGate();
  void autoConnectPins();
  void updateUnassignedSignals() noexcept;

  // Operator Overloadings
  ComponentVariantEditor& operator=(const ComponentVariantEditor& rhs) = delete;

signals:
  void uiDataChanged();

private:
  void execCmd(UndoCommand* cmd);
  static QString appendNumberToSignalName(QString name, int number) noexcept;

private:
  const Workspace& mWorkspace;
  const GraphicsLayerList& mLayers;
  const LibraryElementCache& mCache;
  const QPointer<Component> mComponent;
  std::shared_ptr<ComponentSymbolVariant> mVariant;
  QPointer<UndoStack> mUndoStack;
  const bool* mWizardMode;

  std::unique_ptr<GraphicsScene> mScene;
  int mFrameIndex;

  std::shared_ptr<ComponentGateListModel> mGates;
  bool mHasUnassignedSignals;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
