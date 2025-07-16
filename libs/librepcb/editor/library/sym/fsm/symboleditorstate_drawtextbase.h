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

#ifndef LIBREPCB_EDITOR_SYMBOLEDITORSTATE_DRAWTEXTBASE_H
#define LIBREPCB_EDITOR_SYMBOLEDITORSTATE_DRAWTEXTBASE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "symboleditorstate.h"

#include <librepcb/core/geometry/text.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class CmdTextEdit;
class TextGraphicsItem;

/*******************************************************************************
 *  Class SymbolEditorState_DrawTextBase
 ******************************************************************************/

/**
 * @brief The SymbolEditorState_DrawTextBase class
 */
class SymbolEditorState_DrawTextBase : public SymbolEditorState {
  Q_OBJECT

public:
  // Types
  enum class Mode { NAME, VALUE, TEXT };

  // Constructors / Destructor
  SymbolEditorState_DrawTextBase() = delete;
  SymbolEditorState_DrawTextBase(const SymbolEditorState_DrawTextBase& other) =
      delete;
  explicit SymbolEditorState_DrawTextBase(const Context& context,
                                          Mode mode) noexcept;
  virtual ~SymbolEditorState_DrawTextBase() noexcept;

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
  const HAlign& getHAlign() const noexcept {
    return mCurrentProperties.getAlign().getH();
  }
  void setHAlign(const HAlign& align) noexcept;
  const VAlign& getVAlign() const noexcept {
    return mCurrentProperties.getAlign().getV();
  }
  void setVAlign(const VAlign& align) noexcept;

  // Operator Overloadings
  SymbolEditorState_DrawTextBase& operator=(
      const SymbolEditorState_DrawTextBase& rhs) = delete;

signals:
  void layerChanged(const Layer& layer);
  void textChanged(const QString& text);
  void heightChanged(const PositiveLength& height);
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
  Text mCurrentProperties;

  Point mStartPos;
  std::unique_ptr<CmdTextEdit> mCurrentEditCmd;
  std::shared_ptr<Text> mCurrentText;
  std::shared_ptr<TextGraphicsItem> mCurrentGraphicsItem;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
