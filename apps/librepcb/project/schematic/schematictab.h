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

#ifndef LIBREPCB_PROJECT_SCHEMATICTAB_H
#define LIBREPCB_PROJECT_SCHEMATICTAB_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "graphicsscenetab.h"

#include <librepcb/core/types/lengthunit.h>
#include <librepcb/editor/dialogs/graphicsexportdialog.h>
#include <librepcb/editor/project/schematiceditor/fsm/schematiceditorfsmadapter.h>
#include <librepcb/editor/project/schematiceditor/fsm/schematiceditorstate_drawwire.h>

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class GraphicsScene;
class IF_GraphicsLayerProvider;
class SchematicEditorFsm;

namespace app {

class GuiApplication;
class ProjectEditor;

/*******************************************************************************
 *  Class SchematicTab
 ******************************************************************************/

/**
 * @brief The SchematicTab class
 */
class SchematicTab final : public GraphicsSceneTab,
                           public SchematicEditorFsmAdapter {
  Q_OBJECT

public:
  // Constructors / Destructor
  SchematicTab() = delete;
  SchematicTab(const SchematicTab& other) = delete;
  explicit SchematicTab(GuiApplication& app, std::shared_ptr<ProjectEditor> prj,
                        int schematicIndex, QObject* parent = nullptr) noexcept;
  virtual ~SchematicTab() noexcept;

  // General Methods
  ui::TabData getBaseUiData() const noexcept override;
  ui::SchematicTabData getUiData() const noexcept;
  void setUiData(const ui::SchematicTabData& data) noexcept;
  void activate() noexcept override;
  void deactivate() noexcept override;
  bool actionTriggered(ui::ActionId id) noexcept override;
  bool processScenePointerEvent(
      const QPointF& pos, const QPointF& globalPos,
      slint::private_api::PointerEvent e) noexcept override;

  // SchematicEditorFsmAdapter
  Schematic* fsmGetActiveSchematic() noexcept override;
  SchematicGraphicsScene* fsmGetGraphicsScene() noexcept override;
  void fsmSetViewCursor(
      const std::optional<Qt::CursorShape>& shape) noexcept override;
  void fsmSetViewGrayOut(bool grayOut) noexcept override;
  void fsmSetViewInfoBoxText(const QString& text) noexcept override;
  void fsmSetViewRuler(
      const std::optional<std::pair<Point, Point>>& pos) noexcept override;
  void fsmSetSceneCursor(const Point& pos, bool cross,
                         bool circle) noexcept override;
  QPainterPath fsmCalcPosWithTolerance(
      const Point& pos, qreal multiplier) const noexcept override;
  Point fsmMapGlobalPosToScenePos(const QPoint& pos) const noexcept override;
  void fsmSetHighlightedNetSignals(
      const QSet<const NetSignal*>& sigs) noexcept override;
  void fsmAbortBlockingToolsInOtherEditors() noexcept override;
  void fsmSetStatusBarMessage(const QString& message,
                              int timeoutMs = -1) noexcept override;
  void fsmSetTool(Tool tool, SchematicEditorState* state) noexcept override;

  // Operator Overloadings
  SchematicTab& operator=(const SchematicTab& rhs) = delete;

signals:
  void wireModeRequested(SchematicEditorState_DrawWire::WireMode mode);
  void layerRequested(const Layer& layer);
  void lineWidthRequested(const UnsignedLength& width);
  void heightRequested(const PositiveLength& height);
  void filledRequested(bool filled);
  void valueRequested(const QString& value);

protected:
  const LengthUnit* getCurrentUnit() const noexcept override;
  void requestRepaint() noexcept override;

private:
  void execGraphicsExportDialog(GraphicsExportDialog::Output output,
                                const QString& settingsKey) noexcept;
  void applyTheme() noexcept;

private:
  std::shared_ptr<ProjectEditor> mEditor;
  QScopedPointer<SchematicEditorFsm> mFsm;

  Theme::GridStyle mGridStyle;

  // Current tool
  Tool mTool;
  Qt::CursorShape mToolCursorShape;
  QString mToolOverlayText;
  SchematicEditorState_DrawWire::WireMode mToolWireMode;
  QVector<const Layer*> mToolLayersQt;
  std::shared_ptr<slint::VectorModel<slint::SharedString>> mToolLayers;
  const Layer* mToolLayer;
  UnsignedLength mToolLineWidth;
  LengthUnit mToolLineWidthUnit;
  PositiveLength mToolHeight;
  LengthUnit mToolHeightUnit;
  bool mToolFilled;
  QString mToolValue;
  std::shared_ptr<slint::VectorModel<slint::SharedString>>
      mToolValueSuggestions;

  QHash<Qt::MouseButton, QPointF> mMouseButtonDownScenePos;
  QHash<Qt::MouseButton, QPoint> mMouseButtonDownScreenPos;

  QVector<QMetaObject::Connection> mFsmStateConnections;

  int mFrameIndex;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb

#endif
