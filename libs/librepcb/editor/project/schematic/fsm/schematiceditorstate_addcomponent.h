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
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonPressed(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonDoubleClicked(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneRightMouseButtonReleased(
      const GraphicsSceneMouseEvent& e) noexcept override;

  // Connection to UI
  const QString& getValue() const noexcept { return mCurrentValue; }
  void setValue(const QString& value) noexcept;
  const QStringList& getValueSuggestions() const noexcept {
    return mCurrentValueSuggestions;
  }
  std::optional<AttributeKey> getValueAttributeKey() const noexcept {
    return mCurrentValueAttribute
        ? std::make_optional(mCurrentValueAttribute->getKey())
        : std::nullopt;
  }
  const AttributeType* getValueAttributeType() const noexcept {
    return mCurrentValueAttribute ? &mCurrentValueAttribute->getType()
                                  : nullptr;
  }
  std::optional<QString> getValueAttributeValue() const noexcept {
    return mCurrentValueAttribute
        ? std::make_optional(mCurrentValueAttribute->getValue())
        : std::nullopt;
  }
  void setValueAttributeValue(const QString& value) noexcept;
  const AttributeUnit* getValueAttributeUnit() const noexcept {
    return mCurrentValueAttribute ? mCurrentValueAttribute->getUnit() : nullptr;
  }
  void setValueAttributeUnit(const AttributeUnit* unit) noexcept;

  // Operator Overloadings
  SchematicEditorState_AddComponent& operator=(
      const SchematicEditorState_AddComponent& rhs) = delete;

signals:
  void valueChanged(const QString& value);
  void valueSuggestionsChanged(const QStringList& suggestions);
  void valueAttributeKeyChanged(const std::optional<AttributeKey>& key);
  void valueAttributeTypeChanged(const AttributeType* type);
  void valueAttributeValueChanged(const std::optional<QString>& value);
  void valueAttributeUnitChanged(const AttributeUnit* unit);

private:  // Methods
  void startAddingComponent(
      const std::optional<Uuid>& cmp = std::nullopt,
      const std::optional<Uuid>& symbVar = std::nullopt,
      const std::optional<librepcb::ComponentAssemblyOptionList>& options =
          std::nullopt,
      const QString& searchTerm = QString(), bool keepValue = false);
  bool abortCommand(bool showErrMsgBox) noexcept;
  void applyValueAndAttributeToComponent() noexcept;

private:  // Data
  bool mIsUndoCmdActive;
  bool mUseAddComponentDialog;
  QScopedPointer<AddComponentDialog> mAddComponentDialog;

  // Current tool settings
  Angle mCurrentAngle;
  bool mCurrentMirrored;
  QString mCurrentValue;
  QStringList mCurrentValueSuggestions;
  std::optional<Attribute> mCurrentValueAttribute;

  // Information about the current component/symbol to place
  ComponentInstance* mCurrentComponent;
  int mCurrentSymbVarItemIndex;
  SI_Symbol* mCurrentSymbolToPlace;
  CmdSymbolInstanceEditAll* mCurrentSymbolEditCommand;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
