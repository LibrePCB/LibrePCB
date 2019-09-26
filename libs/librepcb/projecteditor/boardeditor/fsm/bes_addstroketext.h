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

#ifndef LIBREPCB_PROJECT_EDITOR_BES_ADDSTROKETEXT_H
#define LIBREPCB_PROJECT_EDITOR_BES_ADDSTROKETEXT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "bes_base.h"

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
 *  Class BES_AddStrokeText
 ******************************************************************************/

/**
 * @brief The BES_AddStrokeText class
 */
class BES_AddStrokeText final : public BES_Base {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit BES_AddStrokeText(BoardEditor& editor, Ui::BoardEditor& editorUi,
                             GraphicsView& editorGraphicsView,
                             UndoStack&    undoStack);
  ~BES_AddStrokeText();

  // General Methods
  ProcRetVal process(BEE_Base* event) noexcept override;
  bool       entry(BEE_Base* event) noexcept override;
  bool       exit(BEE_Base* event) noexcept override;

private:
  // Private Methods
  ProcRetVal processSceneEvent(BEE_Base* event) noexcept;
  ProcRetVal processRotateEvent(const Angle& angle) noexcept;
  ProcRetVal processFlipEvent(Qt::Orientation orientation) noexcept;
  bool       addText(Board& board, const Point& pos) noexcept;
  void       updateTextPosition(const Point& pos) noexcept;
  bool       fixText(const Point& pos) noexcept;
  void layerComboBoxLayerChanged(const GraphicsLayerName& layerName) noexcept;
  void textComboBoxValueChanged(const QString& value) noexcept;
  void heightEditValueChanged(const PositiveLength& value) noexcept;
  void mirrorCheckBoxToggled(bool checked) noexcept;
  void makeSelectedLayerVisible() noexcept;

  // State
  bool                              mUndoCmdActive;
  BI_StrokeText*                    mText;
  QScopedPointer<CmdStrokeTextEdit> mEditCmd;
  GraphicsLayerName                 mCurrentLayerName;
  QString                           mCurrentText;
  PositiveLength                    mCurrentHeight;
  bool                              mCurrentMirror;
  Angle                             mCurrentRotation;

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

#endif  // LIBREPCB_PROJECT_EDITOR_BES_ADDSTROKETEXT_H
