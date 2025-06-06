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

#ifndef LIBREPCB_EDITOR_SCHEMATICEDITORSTATE_ADDTEXT_H
#define LIBREPCB_EDITOR_SCHEMATICEDITORSTATE_ADDTEXT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "schematiceditorstate.h"

#include <librepcb/core/geometry/text.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Layer;
class SI_Text;
class Schematic;

namespace editor {

class CmdTextEdit;
class LayerComboBox;
class PositiveLengthEdit;

/*******************************************************************************
 *  Class SchematicEditorState_AddText
 ******************************************************************************/

/**
 * @brief The SchematicEditorState_AddText class
 */
class SchematicEditorState_AddText final : public SchematicEditorState {
  Q_OBJECT

public:
  // Constructors / Destructor
  SchematicEditorState_AddText() = delete;
  SchematicEditorState_AddText(const SchematicEditorState_AddText& other) =
      delete;
  explicit SchematicEditorState_AddText(const Context& context) noexcept;
  virtual ~SchematicEditorState_AddText() noexcept;

  // General Methods
  virtual bool entry() noexcept override;
  virtual bool exit() noexcept override;

  // Event Handlers
  virtual bool processRotate(const Angle& rotation) noexcept override;
  virtual bool processMirror(Qt::Orientation orientation) noexcept override;
  virtual bool processGraphicsSceneMouseMoved(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonPressed(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonDoubleClicked(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneRightMouseButtonReleased(
      const GraphicsSceneMouseEvent& e) noexcept override;

  // Connection to UI
  QSet<const Layer*> getAvailableLayers() const noexcept;
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

  // Operator Overloadings
  SchematicEditorState_AddText& operator=(
      const SchematicEditorState_AddText& rhs) = delete;

signals:
  void layerChanged(const Layer& layer);
  void heightChanged(const PositiveLength& height);
  void textChanged(const QString& text);

private:  // Methods
  bool addText(const Point& pos) noexcept;
  bool rotateText(const Angle& angle) noexcept;
  bool updatePosition(const Point& pos) noexcept;
  bool fixPosition(const Point& pos) noexcept;
  bool abortCommand(bool showErrMsgBox) noexcept;
  void layerComboBoxLayerChanged(const Layer& layer) noexcept;
  void textComboBoxValueChanged(const QString& value) noexcept;
  void heightEditValueChanged(const PositiveLength& value) noexcept;

private:  // Data
  // State
  bool mIsUndoCmdActive;

  // Current tool settings
  Text mCurrentProperties;

  // Information about the current text to place. Only valid if
  // mIsUndoCmdActive == true.
  SI_Text* mCurrentTextToPlace;
  std::unique_ptr<CmdTextEdit> mCurrentTextEditCmd;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
