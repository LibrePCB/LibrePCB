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

#ifndef LIBREPCB_PROJECT_EDITOR_SCHEMATICEDITORSTATE_ADDCOMPONENT_H
#define LIBREPCB_PROJECT_EDITOR_SCHEMATICEDITORSTATE_ADDCOMPONENT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "schematiceditorstate.h"

#include <librepcb/common/units/angle.h>
#include <librepcb/common/uuid.h>
#include <optional.hpp>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Attribute;
class AttributeUnitComboBox;

namespace project {

class ComponentInstance;
class SI_Symbol;
class CmdSymbolInstanceEdit;

namespace editor {

class AddComponentDialog;

/*******************************************************************************
 *  Class SchematicEditorState_AddComponent
 ******************************************************************************/

/**
 * @brief The SchematicEditorState_AddComponent class
 */
class SchematicEditorState_AddComponent final : public SchematicEditorState {
  Q_OBJECT

public:
  // Constructors / Destructor
  SchematicEditorState_AddComponent() = delete;
  SchematicEditorState_AddComponent(
      const SchematicEditorState_AddComponent& other) = delete;
  explicit SchematicEditorState_AddComponent(const Context& context) noexcept;
  virtual ~SchematicEditorState_AddComponent() noexcept;

  // General Methods
  virtual bool entry() noexcept override;
  virtual bool exit() noexcept override;

  // Event Handlers
  virtual bool processAddComponent() noexcept override;
  virtual bool processAddComponent(const Uuid& cmp,
                                   const Uuid& symbVar) noexcept override;
  virtual bool processRotateCw() noexcept override;
  virtual bool processRotateCcw() noexcept override;
  virtual bool processAbortCommand() noexcept override;
  virtual bool processGraphicsSceneMouseMoved(
      QGraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonPressed(
      QGraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonDoubleClicked(
      QGraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneRightMouseButtonReleased(
      QGraphicsSceneMouseEvent& e) noexcept override;

  // Operator Overloadings
  SchematicEditorState_AddComponent& operator=(
      const SchematicEditorState_AddComponent& rhs) = delete;

private:  // Methods
  void startAddingComponent(const tl::optional<Uuid>& cmp = tl::nullopt,
                            const tl::optional<Uuid>& symbVar = tl::nullopt,
                            const tl::optional<Uuid>& dev = tl::nullopt,
                            bool keepValue = false);
  bool abortCommand(bool showErrMsgBox) noexcept;
  std::shared_ptr<const Attribute> getToolbarAttribute() const noexcept;
  void valueChanged(QString text) noexcept;
  void attributeChanged() noexcept;
  void updateValueToolbar() noexcept;
  void updateAttributeToolbar() noexcept;
  void setFocusToToolbar() noexcept;
  static QString toSingleLine(const QString& text) noexcept;
  static QString toMultiLine(const QString& text) noexcept;

private:  // Data
  bool mIsUndoCmdActive;
  QScopedPointer<AddComponentDialog> mAddComponentDialog;
  Angle mLastAngle;

  // information about the current component/symbol to place
  ComponentInstance* mCurrentComponent;
  int mCurrentSymbVarItemIndex;
  SI_Symbol* mCurrentSymbolToPlace;
  CmdSymbolInstanceEdit* mCurrentSymbolEditCommand;

  // Widgets for the command toolbar
  QLabel* mValueLabel;
  QComboBox* mValueComboBox;
  QLineEdit* mAttributeValueEdit;
  QAction* mAttributeValueEditAction;
  AttributeUnitComboBox* mAttributeUnitComboBox;
  QAction* mAttributeUnitComboBoxAction;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif
