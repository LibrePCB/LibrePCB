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

#include <librepcb/core/library/cmp/componentsymbolvariant.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class ComponentGateListModel;
class UndoCommand;
class UndoStack;

/*******************************************************************************
 *  Class ComponentVariantEditor
 ******************************************************************************/

/**
 * @brief The ComponentVariantEditor class
 */
class ComponentVariantEditor : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  ComponentVariantEditor() = delete;
  ComponentVariantEditor(const ComponentVariantEditor& other) = delete;
  explicit ComponentVariantEditor(ComponentSymbolVariant& variant,
                                  QObject* parent = nullptr) noexcept;
  virtual ~ComponentVariantEditor() noexcept;

  // General Methods
  std::optional<ui::ComponentVariantData> getUiData() const;
  void setUiData(const ui::ComponentVariantData& data) noexcept;
  void setUndoStack(UndoStack* stack) noexcept;
  // void apply();

  // Operator Overloadings
  ComponentVariantEditor& operator=(const ComponentVariantEditor& rhs) = delete;

private:
  void execCmd(UndoCommand* cmd);

private:
  ComponentSymbolVariant& mVariant;
  UndoStack* mUndoStack;
  std::shared_ptr<ComponentGateListModel> mGates;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
