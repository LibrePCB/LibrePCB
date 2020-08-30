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

#ifndef LIBREPCB_PROJECT_EDITOR_BOARDEDITORSTATE_ADDSTROKETEXT_H
#define LIBREPCB_PROJECT_EDITOR_BOARDEDITORSTATE_ADDSTROKETEXT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "boardeditorstate.h"

#include <librepcb/project/boards/items/bi_stroketext.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class GraphicsLayerComboBox;
class CmdStrokeTextEdit;
class PositiveLengthEdit;

namespace project {

class Board;

namespace editor {

/*******************************************************************************
 *  Class BoardEditorState_AddStrokeText
 ******************************************************************************/

/**
 * @brief The "add stroke text" state/tool of the board editor
 */
class BoardEditorState_AddStrokeText final : public BoardEditorState {
  Q_OBJECT

public:
  // Constructors / Destructor
  BoardEditorState_AddStrokeText() = delete;
  BoardEditorState_AddStrokeText(const BoardEditorState_AddStrokeText& other) =
      delete;
  explicit BoardEditorState_AddStrokeText(const Context& context) noexcept;
  virtual ~BoardEditorState_AddStrokeText() noexcept;

  // General Methods
  virtual bool entry() noexcept override;
  virtual bool exit() noexcept override;

  // Event Handlers
  virtual bool processRotateCw() noexcept override;
  virtual bool processRotateCcw() noexcept override;
  virtual bool processFlipHorizontal() noexcept override;
  virtual bool processFlipVertical() noexcept override;
  virtual bool processGraphicsSceneMouseMoved(
      QGraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonPressed(
      QGraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonDoubleClicked(
      QGraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneRightMouseButtonReleased(
      QGraphicsSceneMouseEvent& e) noexcept override;

  // Operator Overloadings
  BoardEditorState_AddStrokeText& operator       =(
      const BoardEditorState_AddStrokeText& rhs) = delete;

private:  // Methods
  bool addText(Board& board, const Point& pos) noexcept;
  bool rotateText(const Angle& angle) noexcept;
  bool flipText(Qt::Orientation orientation) noexcept;
  bool updatePosition(const Point& pos) noexcept;
  bool fixPosition(const Point& pos) noexcept;
  bool abortCommand(bool showErrMsgBox) noexcept;
  void layerComboBoxLayerChanged(const GraphicsLayerName& layerName) noexcept;
  void textComboBoxValueChanged(const QString& value) noexcept;
  void heightEditValueChanged(const PositiveLength& value) noexcept;
  void mirrorCheckBoxToggled(bool checked) noexcept;
  void makeLayerVisible() noexcept;

private:  // Data
  // State
  bool       mIsUndoCmdActive;
  StrokeText mLastStrokeTextProperties;

  // Information about the current text to place. Only valid if
  // mIsUndoCmdActive == true.
  BI_StrokeText*                    mCurrentTextToPlace;
  QScopedPointer<CmdStrokeTextEdit> mCurrentTextEditCmd;

  // Widgets for the command toolbar
  QScopedPointer<QLabel>                mLayerLabel;
  QScopedPointer<GraphicsLayerComboBox> mLayerComboBox;
  QScopedPointer<QLabel>                mTextLabel;
  QScopedPointer<QComboBox>             mTextComboBox;
  QScopedPointer<QLabel>                mHeightLabel;
  QScopedPointer<PositiveLengthEdit>    mHeightEdit;
  QScopedPointer<QLabel>                mMirrorLabel;
  QScopedPointer<QCheckBox>             mMirrorCheckBox;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif
