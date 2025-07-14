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

#ifndef LIBREPCB_EDITOR_PACKAGEEDITORSTATE_DRAWTEXTBASE_H
#define LIBREPCB_EDITOR_PACKAGEEDITORSTATE_DRAWTEXTBASE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "packageeditorstate.h"

#include <librepcb/core/geometry/stroketext.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class CmdStrokeTextEdit;
class LayerComboBox;
class StrokeTextGraphicsItem;

/*******************************************************************************
 *  Class PackageEditorState_DrawTextBase
 ******************************************************************************/

/**
 * @brief The PackageEditorState_DrawTextBase class
 */
class PackageEditorState_DrawTextBase : public PackageEditorState {
  Q_OBJECT

public:
  // Types
  enum class Mode { NAME, VALUE, TEXT };

  // Constructors / Destructor
  PackageEditorState_DrawTextBase() = delete;
  PackageEditorState_DrawTextBase(
      const PackageEditorState_DrawTextBase& other) = delete;
  explicit PackageEditorState_DrawTextBase(Context& context,
                                           Mode mode) noexcept;
  virtual ~PackageEditorState_DrawTextBase() noexcept;

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
  bool processMirror(Qt::Orientation orientation) noexcept override;
  bool processFlip(Qt::Orientation orientation) noexcept override;

  // Connection to UI
  QSet<const Layer*> getAvailableLayers() const noexcept;
  const Layer& getLayer() const noexcept {
    return mCurrentProperties.getLayer();
  }
  void setLayer(const Layer& layer) noexcept;
  const QString& getText() const noexcept {
    return mCurrentProperties.getText();
  }
  void setText(const QString& text) noexcept;
  QStringList getTextSuggestions() const noexcept;
  const PositiveLength& getHeight() const noexcept {
    return mCurrentProperties.getHeight();
  }
  void setHeight(const PositiveLength& height) noexcept;
  const UnsignedLength& getStrokeWidth() const noexcept {
    return mCurrentProperties.getStrokeWidth();
  }
  void setStrokeWidth(const UnsignedLength& width) noexcept;
  const HAlign& getHAlign() const noexcept {
    return mCurrentProperties.getAlign().getH();
  }
  void setHAlign(const HAlign& align) noexcept;
  const VAlign& getVAlign() const noexcept {
    return mCurrentProperties.getAlign().getV();
  }
  void setVAlign(const VAlign& align) noexcept;

  // Operator Overloadings
  PackageEditorState_DrawTextBase& operator=(
      const PackageEditorState_DrawTextBase& rhs) = delete;

signals:
  void layerChanged(const Layer& layer);
  void textChanged(const QString& text);
  void heightChanged(const PositiveLength& height);
  void strokeWidthChanged(const UnsignedLength& width);
  void hAlignChanged(const HAlign& align);
  void vAlignChanged(const VAlign& align);

protected:
  virtual void notifyToolEnter() noexcept = 0;

private:  // Methods
  bool startAddText(const Point& pos) noexcept;
  bool finishAddText(const Point& pos) noexcept;
  bool abortAddText() noexcept;
  void resetToDefaultParameters() noexcept;

private:
  Mode mMode;
  StrokeText mCurrentProperties;

  Point mStartPos;
  std::unique_ptr<CmdStrokeTextEdit> mCurrentEditCmd;
  std::shared_ptr<StrokeText> mCurrentText;
  std::shared_ptr<StrokeTextGraphicsItem> mCurrentGraphicsItem;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
