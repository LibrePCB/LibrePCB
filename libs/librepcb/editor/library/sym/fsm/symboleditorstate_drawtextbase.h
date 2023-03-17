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

#include <librepcb/core/graphics/graphicslayername.h>
#include <librepcb/core/types/alignment.h>
#include <librepcb/core/types/angle.h>
#include <librepcb/core/types/length.h>
#include <librepcb/core/types/point.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Text;

namespace editor {

class CmdTextEdit;
class HAlignActionGroup;
class TextGraphicsItem;
class VAlignActionGroup;

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
  QSet<EditorWidgetBase::Feature> getAvailableFeatures() const
      noexcept override;

  // Event Handlers
  bool processGraphicsSceneMouseMoved(
      QGraphicsSceneMouseEvent& e) noexcept override;
  bool processGraphicsSceneLeftMouseButtonPressed(
      QGraphicsSceneMouseEvent& e) noexcept override;
  bool processGraphicsSceneRightMouseButtonReleased(
      QGraphicsSceneMouseEvent& e) noexcept override;
  bool processRotate(const Angle& rotation) noexcept override;
  bool processMirror(Qt::Orientation orientation) noexcept override;

  // Operator Overloadings
  SymbolEditorState_DrawTextBase& operator=(
      const SymbolEditorState_DrawTextBase& rhs) = delete;

private:  // Methods
  bool startAddText(const Point& pos) noexcept;
  bool finishAddText(const Point& pos) noexcept;
  bool abortAddText() noexcept;
  void resetToDefaultParameters() noexcept;
  void layerComboBoxValueChanged(const GraphicsLayerName& layerName) noexcept;
  void heightEditValueChanged(const PositiveLength& value) noexcept;
  void textComboBoxValueChanged(const QString& value) noexcept;
  void hAlignActionGroupValueChanged(const HAlign& value) noexcept;
  void vAlignActionGroupValueChanged(const VAlign& value) noexcept;

private:  // Types / Data
  Mode mMode;
  Point mStartPos;
  QScopedPointer<CmdTextEdit> mEditCmd;
  std::shared_ptr<Text> mCurrentText;
  std::shared_ptr<TextGraphicsItem> mCurrentGraphicsItem;
  QPointer<HAlignActionGroup> mHAlignActionGroup;
  QPointer<VAlignActionGroup> mVAlignActionGroup;

  // parameter memory
  GraphicsLayerName mLastLayerName;
  Angle mLastRotation;
  PositiveLength mLastHeight;
  Alignment mLastAlignment;
  QString mLastText;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
