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

#ifndef LIBREPCB_EDITOR_CMDBOARDSTROKETEXTEDIT_H
#define LIBREPCB_EDITOR_CMDBOARDSTROKETEXTEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../undocommand.h"

#include <librepcb/core/project/board/boardstroketextdata.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BI_StrokeText;

namespace editor {

/*******************************************************************************
 *  Class CmdBoardStrokeTextEdit
 ******************************************************************************/

/**
 * @brief The CmdBoardStrokeTextEdit class
 */
class CmdBoardStrokeTextEdit final : public UndoCommand {
public:
  // Constructors / Destructor
  explicit CmdBoardStrokeTextEdit(BI_StrokeText& text) noexcept;
  ~CmdBoardStrokeTextEdit() noexcept;

  // Setters
  void setLayer(const Layer& layer, bool immediate) noexcept;
  void setText(const QString& text, bool immediate) noexcept;
  void setHeight(const PositiveLength& height, bool immediate) noexcept;
  void setStrokeWidth(const UnsignedLength& strokeWidth,
                      bool immediate) noexcept;
  void setLetterSpacing(const StrokeTextSpacing& spacing,
                        bool immediate) noexcept;
  void setLineSpacing(const StrokeTextSpacing& spacing,
                      bool immediate) noexcept;
  void setAlignment(const Alignment& align, bool immediate) noexcept;
  void setPosition(const Point& pos, bool immediate) noexcept;
  void translate(const Point& delta, bool immediate) noexcept;
  void snapToGrid(const PositiveLength& gridInterval, bool immediate) noexcept;
  void setRotation(const Angle& angle, bool immediate) noexcept;
  void rotate(const Angle& angle, const Point& center, bool immediate) noexcept;
  void setMirrored(bool mirrored, bool immediate) noexcept;
  void mirrorGeometry(Qt::Orientation orientation, const Point& center,
                      bool immediate) noexcept;
  void mirrorGeometry(const Angle& rotation, const Point& center,
                      bool immediate) noexcept;
  void mirrorLayer(int innerLayerCount, bool immediate) noexcept;
  void setAutoRotate(bool autoRotate, bool immediate) noexcept;
  void setLocked(bool locked) noexcept;

private:  // Methods
  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  bool performExecute() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performUndo()
  void performUndo() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performRedo()
  void performRedo() override;

private:  // Data
  BI_StrokeText& mText;

  BoardStrokeTextData mOldData;
  BoardStrokeTextData mNewData;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
