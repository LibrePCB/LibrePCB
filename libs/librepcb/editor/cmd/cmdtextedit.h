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

#ifndef LIBREPCB_EDITOR_CMDTEXTEDIT_H
#define LIBREPCB_EDITOR_CMDTEXTEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../undocommand.h"
#include "cmdlistelementinsert.h"
#include "cmdlistelementremove.h"
#include "cmdlistelementsswap.h"

#include <librepcb/core/geometry/text.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class CmdTextEdit
 ******************************************************************************/

/**
 * @brief The CmdTextEdit class
 */
class CmdTextEdit final : public UndoCommand {
public:
  // Constructors / Destructor
  CmdTextEdit() = delete;
  CmdTextEdit(const CmdTextEdit& other) = delete;
  explicit CmdTextEdit(Text& text) noexcept;
  ~CmdTextEdit() noexcept;

  // Setters
  void setLayerName(const GraphicsLayerName& name, bool immediate) noexcept;
  void setText(const QString& text, bool immediate) noexcept;
  void setHeight(const PositiveLength& height, bool immediate) noexcept;
  void setAlignment(const Alignment& align, bool immediate) noexcept;
  void setPosition(const Point& pos, bool immediate) noexcept;
  void translate(const Point& deltaPos, bool immediate) noexcept;
  void snapToGrid(const PositiveLength& gridInterval, bool immediate) noexcept;
  void setRotation(const Angle& angle, bool immediate) noexcept;
  void rotate(const Angle& angle, const Point& center, bool immediate) noexcept;
  void mirror(Qt::Orientation orientation, const Point& center,
              bool immediate) noexcept;

  // Operator Overloadings
  CmdTextEdit& operator=(const CmdTextEdit& rhs) = delete;

private:
  // Private Methods

  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  bool performExecute() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performUndo()
  void performUndo() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performRedo()
  void performRedo() override;

  // Private Member Variables

  // Attributes from the constructor
  Text& mText;

  // General Attributes
  GraphicsLayerName mOldLayerName;
  GraphicsLayerName mNewLayerName;
  QString mOldText;
  QString mNewText;
  Point mOldPosition;
  Point mNewPosition;
  Angle mOldRotation;
  Angle mNewRotation;
  PositiveLength mOldHeight;
  PositiveLength mNewHeight;
  Alignment mOldAlign;
  Alignment mNewAlign;
};

/*******************************************************************************
 *  Undo Commands
 ******************************************************************************/

using CmdTextInsert =
    CmdListElementInsert<Text, TextListNameProvider, Text::Event>;
using CmdTextRemove =
    CmdListElementRemove<Text, TextListNameProvider, Text::Event>;
using CmdTextsSwap =
    CmdListElementsSwap<Text, TextListNameProvider, Text::Event>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
