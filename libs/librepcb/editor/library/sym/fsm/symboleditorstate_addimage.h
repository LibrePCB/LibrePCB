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

#ifndef LIBREPCB_EDITOR_SYMBOLEDITORSTATE_ADDIMAGE_H
#define LIBREPCB_EDITOR_SYMBOLEDITORSTATE_ADDIMAGE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "symboleditorstate.h"

#include <librepcb/core/geometry/image.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class CmdImageEdit;
class ImageGraphicsItem;

/*******************************************************************************
 *  Class SymbolEditorState_AddImage
 ******************************************************************************/

/**
 * @brief The SymbolEditorState_AddImage class
 */
class SymbolEditorState_AddImage final : public SymbolEditorState {
  Q_OBJECT

public:
  // Constructors / Destructor
  SymbolEditorState_AddImage() = delete;
  SymbolEditorState_AddImage(const SymbolEditorState_AddImage& other) = delete;
  explicit SymbolEditorState_AddImage(const Context& context) noexcept;
  ~SymbolEditorState_AddImage() noexcept;

  // General Methods
  bool entry() noexcept override;
  bool exit() noexcept override;

  // Event Handlers
  bool processGraphicsSceneMouseMoved(
      const GraphicsSceneMouseEvent& e) noexcept override;
  bool processGraphicsSceneLeftMouseButtonPressed(
      const GraphicsSceneMouseEvent& e) noexcept override;
  bool processGraphicsSceneRightMouseButtonReleased(
      const GraphicsSceneMouseEvent& e) noexcept override;
  bool processRotate(const Angle& rotation) noexcept override;
  bool processAddImage(const QByteArray& data, const QString& format,
                       const QString& basename) noexcept override;

  // Operator Overloadings
  SymbolEditorState_AddImage& operator=(const SymbolEditorState_AddImage& rhs) =
      delete;

private:  // Methods
  bool start(const Point& pos, QByteArray data, QString format,
             QString basename) noexcept;
  void updateSize(const Point& pos) noexcept;
  bool finish(const Point& pos) noexcept;
  bool abort(bool showErrMsgBox) noexcept;

private:
  enum class State { Positioning, Resizing };

  // State
  State mState;
  bool mUndoCmdActive;

  // Current tool settings
  Image mCurrentProperties;

  // Information about the current image to place
  std::unique_ptr<CmdImageEdit> mCurrentEditCmd;
  std::shared_ptr<Image> mCurrentImage;
  qreal mCurrentImageAspectRatio;
  std::shared_ptr<ImageGraphicsItem> mCurrentGraphicsItem;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
