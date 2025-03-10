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

#ifndef LIBREPCB_EDITOR_BOARDEDITORSTATE_ADDSTROKETEXT_H
#define LIBREPCB_EDITOR_BOARDEDITORSTATE_ADDSTROKETEXT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "boardeditorstate.h"

#include <librepcb/core/project/board/items/bi_stroketext.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Board;

namespace editor {

class CmdBoardStrokeTextEdit;

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
  virtual bool processRotate(const Angle& rotation) noexcept override;
  virtual bool processFlip(Qt::Orientation orientation) noexcept override;
  virtual bool processGraphicsSceneMouseMoved(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonPressed(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonDoubleClicked(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneRightMouseButtonReleased(
      const GraphicsSceneMouseEvent& e) noexcept override;

  // Connection to UI
  QSet<const Layer*> getAvailableLayers() noexcept;
  const Layer& getLayer() const noexcept {
    return mCurrentProperties.getLayer();
  }
  void setLayer(const Layer& layer) noexcept;
  const PositiveLength& getHeight() const noexcept {
    return mCurrentProperties.getHeight();
  }
  void setHeight(const PositiveLength& height) noexcept;
  const QString& getText() const noexcept {
    return mCurrentProperties.getText();
  }
  QStringList getTextSuggestions() const noexcept;
  void setText(const QString& text) noexcept;
  bool getMirrored() const noexcept { return mCurrentProperties.getMirrored(); }
  void setMirrored(bool mirrored) noexcept;

  // Operator Overloadings
  BoardEditorState_AddStrokeText& operator=(
      const BoardEditorState_AddStrokeText& rhs) = delete;

signals:
  void layerChanged(const Layer& layer);
  void heightChanged(const PositiveLength& height);
  void textChanged(const QString& text);
  void mirroredChanged(bool mirrored);

private:  // Methods
  bool addText(const Point& pos) noexcept;
  bool rotateText(const Angle& angle) noexcept;
  bool flipText(Qt::Orientation orientation) noexcept;
  bool updatePosition(const Point& pos) noexcept;
  bool fixPosition(const Point& pos) noexcept;
  bool abortCommand(bool showErrMsgBox) noexcept;

private:  // Data
  // State
  bool mIsUndoCmdActive;

  // Current tool settings
  BoardStrokeTextData mCurrentProperties;

  // Information about the current text to place. Only valid if
  // mIsUndoCmdActive == true.
  BI_StrokeText* mCurrentTextToPlace;
  std::unique_ptr<CmdBoardStrokeTextEdit> mCurrentTextEditCmd;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
