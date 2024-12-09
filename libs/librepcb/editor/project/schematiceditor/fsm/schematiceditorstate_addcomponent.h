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

#ifndef LIBREPCB_EDITOR_SCHEMATICEDITORSTATE_ADDCOMPONENT_H
#define LIBREPCB_EDITOR_SCHEMATICEDITORSTATE_ADDCOMPONENT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "schematiceditorstate.h"

#include <librepcb/core/project/circuit/componentassemblyoption.h>
#include <librepcb/core/types/angle.h>
#include <librepcb/core/types/uuid.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>
#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Attribute;
class ComponentInstance;
class SI_Symbol;

namespace editor {

class AddComponentDialog;
class AttributeUnitComboBox;
class CmdSymbolInstanceEditAll;

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
  virtual bool processAddComponent(
      const QString& searchTerm = QString()) noexcept override;
  virtual bool processAddComponent(const Uuid& cmp,
                                   const Uuid& symbVar) noexcept override;
  virtual bool processRotate(const Angle& rotation) noexcept override;
  virtual bool processMirror(Qt::Orientation orientation) noexcept override;
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
  void startAddingComponent(
      const std::optional<Uuid>& cmp = std::nullopt,
      const std::optional<Uuid>& symbVar = std::nullopt,
      const std::optional<librepcb::ComponentAssemblyOptionList>& options =
          std::nullopt,
      const QString& searchTerm = QString(), bool keepValue = false);
  bool abortCommand(bool showErrMsgBox) noexcept;
  std::shared_ptr<const Attribute> getToolbarAttribute() const noexcept;
  void valueChanged(QString text) noexcept;
  void attributeChanged() noexcept;
  void updateValueToolbar() noexcept;
  void updateAttributeToolbar() noexcept;
  static QString toSingleLine(const QString& text) noexcept;
  static QString toMultiLine(const QString& text) noexcept;

private:  // Data
  bool mIsUndoCmdActive;
  bool mUseAddComponentDialog;
  QScopedPointer<AddComponentDialog> mAddComponentDialog;
  Angle mLastAngle;
  bool mLastMirrored;

  // information about the current component/symbol to place
  ComponentInstance* mCurrentComponent;
  int mCurrentSymbVarItemIndex;
  SI_Symbol* mCurrentSymbolToPlace;
  CmdSymbolInstanceEditAll* mCurrentSymbolEditCommand;

  // Widgets for the command toolbar
  QPointer<QComboBox> mValueComboBox;
  QPointer<QLineEdit> mAttributeValueEdit;
  QPointer<QAction> mAttributeValueEditAction;
  QPointer<AttributeUnitComboBox> mAttributeUnitComboBox;
  QPointer<QAction> mAttributeUnitComboBoxAction;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
