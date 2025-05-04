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

#ifndef LIBREPCB_EDITOR_SCHEMATICEDITORSTATE_DRAWPOLYGON_H
#define LIBREPCB_EDITOR_SCHEMATICEDITORSTATE_DRAWPOLYGON_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "schematiceditorstate.h"

#include <librepcb/core/geometry/polygon.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Layer;
class SI_Polygon;
class Schematic;

namespace editor {

class CmdPolygonEdit;
class LayerComboBox;
class UnsignedLengthEdit;

/*******************************************************************************
 *  Class SchematicEditorState_DrawPolygon
 ******************************************************************************/

/**
 * @brief The SchematicEditorState_DrawPolygon class
 */
class SchematicEditorState_DrawPolygon final : public SchematicEditorState {
  Q_OBJECT

public:
  // Constructors / Destructor
  SchematicEditorState_DrawPolygon() = delete;
  SchematicEditorState_DrawPolygon(
      const SchematicEditorState_DrawPolygon& other) = delete;
  explicit SchematicEditorState_DrawPolygon(const Context& context) noexcept;
  virtual ~SchematicEditorState_DrawPolygon() noexcept;

  // General Methods
  virtual bool entry() noexcept override;
  virtual bool exit() noexcept override;

  // Event Handlers
  virtual bool processAbortCommand() noexcept override;
  virtual bool processGraphicsSceneMouseMoved(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonPressed(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonDoubleClicked(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processSwitchToSchematicPage(int index) noexcept override;

  // Connection to UI
  QSet<const Layer*> getAvailableLayers() const noexcept;
  const Layer& getLayer() const noexcept {
    return mCurrentProperties.getLayer();
  }
  void setLayer(const Layer& layer) noexcept;
  const UnsignedLength& getLineWidth() const noexcept {
    return mCurrentProperties.getLineWidth();
  }
  void setLineWidth(const UnsignedLength& width) noexcept;
  bool getFilled() const noexcept { return mCurrentProperties.isFilled(); }
  void setFilled(bool filled) noexcept;

  // Operator Overloadings
  SchematicEditorState_DrawPolygon& operator=(
      const SchematicEditorState_DrawPolygon& rhs) = delete;

signals:
  void layerChanged(const Layer& layer);
  void lineWidthChanged(const UnsignedLength& width);
  void filledChanged(bool filled);

private:  // Methods
  bool startAddPolygon(const Point& pos) noexcept;
  bool addSegment(const Point& pos) noexcept;
  bool updateLastVertexPosition(const Point& pos) noexcept;
  bool abortCommand(bool showErrMsgBox) noexcept;

private:  // Data
  // State
  bool mIsUndoCmdActive;
  Point mLastSegmentPos;

  // Current tool settings
  Polygon mCurrentProperties;

  // Information about the current polygon to place. Only valid if
  // mIsUndoCmdActive == true.
  SI_Polygon* mCurrentPolygon;
  std::unique_ptr<CmdPolygonEdit> mCurrentPolygonEditCmd;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
