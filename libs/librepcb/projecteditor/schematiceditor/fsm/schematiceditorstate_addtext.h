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

#ifndef LIBREPCB_PROJECT_EDITOR_SCHEMATICEDITORSTATE_ADDTEXT_H
#define LIBREPCB_PROJECT_EDITOR_SCHEMATICEDITORSTATE_ADDTEXT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "schematiceditorstate.h"

#include <librepcb/common/geometry/text.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class CmdTextEdit;
class PositiveLengthEdit;
class GraphicsLayerComboBox;

namespace project {

class Schematic;
class SI_Text;

namespace editor {

/*******************************************************************************
 *  Class SchematicEditorState_AddText
 ******************************************************************************/

/**
 * @brief The SchematicEditorState_AddText class
 */
class SchematicEditorState_AddText final : public SchematicEditorState {
  Q_OBJECT

public:
  // Constructors / Destructor
  SchematicEditorState_AddText() = delete;
  SchematicEditorState_AddText(const SchematicEditorState_AddText& other) =
      delete;
  explicit SchematicEditorState_AddText(const Context& context) noexcept;
  virtual ~SchematicEditorState_AddText() noexcept;

  // General Methods
  virtual bool entry() noexcept override;
  virtual bool exit() noexcept override;

  // Event Handlers
  virtual bool processRotateCw() noexcept override;
  virtual bool processRotateCcw() noexcept override;
  virtual bool processGraphicsSceneMouseMoved(
      QGraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonPressed(
      QGraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonDoubleClicked(
      QGraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneRightMouseButtonReleased(
      QGraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processSwitchToSchematicPage(int index) noexcept override;

  // Operator Overloadings
  SchematicEditorState_AddText& operator=(
      const SchematicEditorState_AddText& rhs) = delete;

private:  // Methods
  bool addText(Schematic& schematic, const Point& pos) noexcept;
  bool rotateText(const Angle& angle) noexcept;
  bool updatePosition(const Point& pos) noexcept;
  bool fixPosition(const Point& pos) noexcept;
  bool abortCommand(bool showErrMsgBox) noexcept;
  void layerComboBoxLayerChanged(const GraphicsLayerName& layerName) noexcept;
  void textComboBoxValueChanged(const QString& value) noexcept;
  void heightEditValueChanged(const PositiveLength& value) noexcept;
  void setFocusToTextEdit() noexcept;

private:  // Data
  // State
  bool mIsUndoCmdActive;
  Text mLastTextProperties;

  // Information about the current text to place. Only valid if
  // mIsUndoCmdActive == true.
  SI_Text* mCurrentTextToPlace;
  QScopedPointer<CmdTextEdit> mCurrentTextEditCmd;

  // Widgets for the command toolbar
  QScopedPointer<QLabel> mLayerLabel;
  QScopedPointer<GraphicsLayerComboBox> mLayerComboBox;
  QScopedPointer<QLabel> mTextLabel;
  QScopedPointer<QComboBox> mTextComboBox;
  QScopedPointer<QLabel> mHeightLabel;
  QScopedPointer<PositiveLengthEdit> mHeightEdit;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif
