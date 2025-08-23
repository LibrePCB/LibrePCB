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

#ifndef LIBREPCB_EDITOR_SCHEMATICTAB_H
#define LIBREPCB_EDITOR_SCHEMATICTAB_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../dialogs/graphicsexportdialog.h"
#include "../../utils/dismissablemessagecontext.h"
#include "../../utils/lengtheditcontext.h"
#include "../../utils/searchcontext.h"
#include "../../widgets/if_graphicsvieweventhandler.h"
#include "fsm/schematiceditorfsmadapter.h"
#include "fsm/schematiceditorstate_drawwire.h"
#include "windowtab.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class AttributeUnit;
class ErcMsgBase;

namespace editor {

class GraphicsLayer;
class GraphicsLayerList;
class ProjectEditor;
class SchematicEditor;
class SchematicEditorFsm;
class SlintGraphicsView;

/*******************************************************************************
 *  Class SchematicTab
 ******************************************************************************/

/**
 * @brief The SchematicTab class
 */
class SchematicTab final : public WindowTab,
                           public SchematicEditorFsmAdapter,
                           public IF_GraphicsViewEventHandler {
  Q_OBJECT

public:
  // Signals
  Signal<SchematicTab> onDerivedUiDataChanged;

  // Constructors / Destructor
  SchematicTab() = delete;
  SchematicTab(const SchematicTab& other) = delete;
  explicit SchematicTab(GuiApplication& app, SchematicEditor& editor,
                        QObject* parent = nullptr) noexcept;
  ~SchematicTab() noexcept;

  // General Methods
  int getProjectIndex() const noexcept;
  int getProjectObjectIndex() const noexcept;
  ui::TabData getUiData() const noexcept override;
  void setUiData(const ui::TabData& data) noexcept override;
  ui::SchematicTabData getDerivedUiData() const noexcept;
  void setDerivedUiData(const ui::SchematicTabData& data) noexcept;
  void highlightErcMessage(const std::shared_ptr<const ErcMsgBase>& msg,
                           bool zoomTo) noexcept;
  void activate() noexcept override;
  void deactivate() noexcept override;
  void trigger(ui::TabAction a) noexcept override;
  slint::Image renderScene(float width, float height,
                           int scene) noexcept override;
  bool processScenePointerEvent(
      const QPointF& pos, slint::private_api::PointerEvent e) noexcept override;
  bool processSceneScrolled(
      const QPointF& pos,
      slint::private_api::PointerScrollEvent e) noexcept override;
  bool processSceneKeyEvent(
      const slint::private_api::KeyEvent& e) noexcept override;

  // IF_GraphicsViewEventHandler
  bool graphicsSceneKeyPressed(
      const GraphicsSceneKeyEvent& e) noexcept override;
  bool graphicsSceneKeyReleased(
      const GraphicsSceneKeyEvent& e) noexcept override;
  bool graphicsSceneMouseMoved(
      const GraphicsSceneMouseEvent& e) noexcept override;
  bool graphicsSceneLeftMouseButtonPressed(
      const GraphicsSceneMouseEvent& e) noexcept override;
  bool graphicsSceneLeftMouseButtonReleased(
      const GraphicsSceneMouseEvent& e) noexcept override;
  bool graphicsSceneLeftMouseButtonDoubleClicked(
      const GraphicsSceneMouseEvent& e) noexcept override;
  bool graphicsSceneRightMouseButtonReleased(
      const GraphicsSceneMouseEvent& e) noexcept override;

  // SchematicEditorFsmAdapter
  SchematicGraphicsScene* fsmGetGraphicsScene() noexcept override;
  bool fsmGetIgnoreLocks() const noexcept override;
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
  void fsmZoomToSceneRect(const QRectF& r) noexcept override;
  void fsmSetHighlightedNetSignals(
      const QSet<const NetSignal*>& sigs) noexcept override;
  void fsmAbortBlockingToolsInOtherEditors() noexcept override;
  void fsmSetStatusBarMessage(const QString& message,
                              int timeoutMs = -1) noexcept override;
  void fsmSetFeatures(Features features) noexcept override;
  void fsmToolLeave() noexcept override;
  void fsmToolEnter(SchematicEditorState_Select& state) noexcept override;
  void fsmToolEnter(SchematicEditorState_DrawWire& state) noexcept override;
  void fsmToolEnter(SchematicEditorState_AddNetLabel& state) noexcept override;
  void fsmToolEnter(SchematicEditorState_AddComponent& state) noexcept override;
  void fsmToolEnter(SchematicEditorState_DrawPolygon& state) noexcept override;
  void fsmToolEnter(SchematicEditorState_AddText& state) noexcept override;
  void fsmToolEnter(SchematicEditorState_AddImage& state) noexcept override;
  void fsmToolEnter(SchematicEditorState_Measure& state) noexcept override;

  // Operator Overloadings
  SchematicTab& operator=(const SchematicTab& rhs) = delete;

signals:
  void wireModeRequested(SchematicEditorState_DrawWire::WireMode mode);
  void layerRequested(const Layer& layer);
  void filledRequested(bool filled);
  void valueRequested(const QString& value);
  void attributeValueRequested(const QString& value);
  void attributeUnitRequested(const AttributeUnit* unit);

private:
  void updateMessages() noexcept;
  void clearErcMarker() noexcept;
  void execGraphicsExportDialog(GraphicsExportDialog::Output output,
                                const QString& settingsKey) noexcept;
  void goToSymbol(const QString& name, int index) noexcept;
  void applyTheme() noexcept;
  void requestRepaint() noexcept;

private:
  // References
  ProjectEditor& mProjectEditor;
  Project& mProject;
  SchematicEditor& mSchematicEditor;
  Schematic& mSchematic;
  std::unique_ptr<GraphicsLayerList> mLayers;
  std::shared_ptr<GraphicsLayer> mPinNumbersLayer;
  std::unique_ptr<SlintGraphicsView> mView;

  // Message handles
  DismissableMessageContext mMsgInstallLibraries;
  DismissableMessageContext mMsgAddDrawingFrame;

  // State
  SearchContext mSearchContext;
  Theme::GridStyle mGridStyle;
  QPointF mSceneImagePos;
  bool mIgnorePlacementLocks;
  int mFrameIndex;

  // Current tool
  Features mToolFeatures;
  ui::EditorTool mTool;
  Qt::CursorShape mToolCursorShape;
  QString mToolOverlayText;
  SchematicEditorState_DrawWire::WireMode mToolWireMode;
  QVector<const Layer*> mToolLayersQt;
  std::shared_ptr<slint::VectorModel<slint::SharedString>> mToolLayers;
  const Layer* mToolLayer;
  LengthEditContext mToolLineWidth;
  LengthEditContext mToolSize;
  bool mToolFilled;
  QString mToolValue;
  std::shared_ptr<slint::VectorModel<slint::SharedString>>
      mToolValueSuggestions;
  std::optional<QString> mToolAttributeValue;
  QString mToolAttributeValuePlaceholder;
  QList<const AttributeUnit*> mToolAttributeUnitsQt;
  std::shared_ptr<slint::VectorModel<slint::SharedString>> mToolAttributeUnits;
  const AttributeUnit* mToolAttributeUnit;

  // FSM
  QVector<QMetaObject::Connection> mFsmStateConnections;
  QScopedPointer<SchematicEditorFsm> mFsm;
  std::unique_ptr<QGraphicsPathItem> mErcLocationGraphicsItem;

  // Objects in active state
  std::unique_ptr<SchematicGraphicsScene> mScene;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
