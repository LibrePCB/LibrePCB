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

#ifndef LIBREPCB_PROJECT_SES_ADDCOMPONENT_H
#define LIBREPCB_PROJECT_SES_ADDCOMPONENT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "ses_base.h"

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Attribute;
class AttributeUnitComboBox;

namespace library {
class Component;
class ComponentSymbolVariant;
class ComponentSymbolVariantItem;
}  // namespace library

namespace project {

class ComponentInstance;
class SI_Symbol;
class CmdSymbolInstanceEdit;

namespace editor {

class AddComponentDialog;

/*******************************************************************************
 *  Class SES_AddComponent
 ******************************************************************************/

/**
 * @brief The SES_AddComponent class
 */
class SES_AddComponent final : public SES_Base {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit SES_AddComponent(SchematicEditor&     editor,
                            Ui::SchematicEditor& editorUi,
                            GraphicsView&        editorGraphicsView,
                            UndoStack&           undoStack);
  ~SES_AddComponent();

  // General Methods
  ProcRetVal process(SEE_Base* event) noexcept override;
  bool       entry(SEE_Base* event) noexcept override;
  bool       exit(SEE_Base* event) noexcept override;

private:
  // Private Methods
  ProcRetVal processSceneEvent(SEE_Base* event) noexcept;
  void       startAddingComponent(const tl::optional<Uuid>& cmp = tl::nullopt,
                                  const tl::optional<Uuid>& symbVar = tl::nullopt,
                                  const tl::optional<Uuid>& dev = tl::nullopt,
                                  bool                      keepValue = false);
  bool       abortCommand(bool showErrMsgBox) noexcept;
  std::shared_ptr<const Attribute> getToolbarAttribute() const noexcept;
  void                             valueChanged(QString text) noexcept;
  void                             attributeChanged() noexcept;
  void                             updateValueToolbar() noexcept;
  void                             updateAttributeToolbar() noexcept;
  void                             setFocusToToolbar() noexcept;
  static QString                   toSingleLine(const QString& text) noexcept;
  static QString                   toMultiLine(const QString& text) noexcept;

  // Attributes
  bool                mIsUndoCmdActive;
  AddComponentDialog* mAddComponentDialog;
  Angle               mLastAngle;

  // information about the current component/symbol to place
  ComponentInstance*     mCurrentComponent;
  int                    mCurrentSymbVarItemIndex;
  SI_Symbol*             mCurrentSymbolToPlace;
  CmdSymbolInstanceEdit* mCurrentSymbolEditCommand;

  // Widgets for the command toolbar
  QLabel*                mValueLabel;
  QComboBox*             mValueComboBox;
  QLineEdit*             mAttributeValueEdit;
  QAction*               mAttributeValueEditAction;
  AttributeUnitComboBox* mAttributeUnitComboBox;
  QAction*               mAttributeUnitComboBoxAction;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_SES_ADDCOMPONENT_H
