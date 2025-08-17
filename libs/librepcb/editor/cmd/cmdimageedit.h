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

#ifndef LIBREPCB_EDITOR_CMDIMAGEEDIT_H
#define LIBREPCB_EDITOR_CMDIMAGEEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../undocommand.h"

#include <librepcb/core/geometry/image.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class CmdImageEdit
 ******************************************************************************/

/**
 * @brief The CmdImageEdit class
 */
class CmdImageEdit final : public UndoCommand {
public:
  // Constructors / Destructor
  CmdImageEdit() = delete;
  CmdImageEdit(const CmdImageEdit& other) = delete;
  explicit CmdImageEdit(Image& image) noexcept;
  ~CmdImageEdit() noexcept;

  // Setters
  void setFileName(const FileProofName& name, bool immediate) noexcept;
  void setPosition(const Point& pos, bool immediate) noexcept;
  void translate(const Point& deltaPos, bool immediate) noexcept;
  void snapToGrid(const PositiveLength& gridInterval, bool immediate) noexcept;
  void setRotation(const Angle& angle, bool immediate) noexcept;
  void rotate(const Angle& angle, const Point& center, bool immediate) noexcept;
  void mirror(Qt::Orientation orientation, const Point& center,
              bool immediate) noexcept;
  void mirror(const Angle& rotation, const Point& center,
              bool immediate) noexcept;
  void setWidth(const PositiveLength& width, bool immediate) noexcept;
  void setHeight(const PositiveLength& height, bool immediate) noexcept;
  void setBorderWidth(const std::optional<UnsignedLength>& width,
                      bool immediate) noexcept;

  // Operator Overloadings
  CmdImageEdit& operator=(const CmdImageEdit& rhs) = delete;

private:  // Methods
  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  bool performExecute() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performUndo()
  void performUndo() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performRedo()
  void performRedo() override;

private:  // Data
  Image& mImage;

  // General Attributes
  FileProofName mOldFileName;
  FileProofName mNewFileName;
  Point mOldPosition;
  Point mNewPosition;
  Angle mOldRotation;
  Angle mNewRotation;
  PositiveLength mOldWidth;
  PositiveLength mNewWidth;
  PositiveLength mOldHeight;
  PositiveLength mNewHeight;
  std::optional<UnsignedLength> mOldBorderWidth;
  std::optional<UnsignedLength> mNewBorderWidth;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
