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

#ifndef LIBREPCB_EDITOR_BOARD2DTAB_H
#define LIBREPCB_EDITOR_BOARD2DTAB_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "fsm/boardeditorfsmadapter.h"
#include "fsm/boardeditorstate_drawtrace.h"
#include "graphicsscenetab.h"

#include <librepcb/core/geometry/zone.h>
#include <librepcb/core/project/board/drc/boarddesignrulecheck.h>
#include <librepcb/core/types/lengthunit.h>

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BoardPlaneFragmentsBuilder;

namespace editor {

class BoardEditorFsm;
class GraphicsScene;
class GuiApplication;
class IF_GraphicsLayerProvider;
class Notification;
class RuleCheckMessagesModel;

/*******************************************************************************
 *  Class Board2dTab
 ******************************************************************************/

/**
 * @brief The Board2dTab class
 */
class Board2dTab final : public GraphicsSceneTab, public BoardEditorFsmAdapter {
  Q_OBJECT

public:
  // Signals
  Signal<Board2dTab> onDerivedUiDataChanged;

  // Constructors / Destructor
  Board2dTab() = delete;
  Board2dTab(const Board2dTab& other) = delete;
  explicit Board2dTab(GuiApplication& app, std::shared_ptr<ProjectEditor2> prj,
                      int boardIndex, QObject* parent = nullptr) noexcept;
  virtual ~Board2dTab() noexcept;

  // General Methods
  ui::TabData getUiData() const noexcept override;
  ui::Board2dTabData getDerivedUiData() const noexcept;
  void setDerivedUiData(const ui::Board2dTabData& data) noexcept;
  void activate() noexcept override;
  void deactivate() noexcept override;
  void triggerAsync(ui::Action a) noexcept override;
  bool processScenePointerEvent(
      const QPointF& pos, const QPointF& globalPos,
      slint::private_api::PointerEvent e) noexcept override;

  // BoardEditorFsmAdapter
  Board* fsmGetActiveBoard() noexcept override;
  BoardGraphicsScene* fsmGetGraphicsScene() noexcept override;
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
  void fsmSetHighlightedNetSignals(
      const QSet<const NetSignal*>& sigs) noexcept override;
  void fsmAbortBlockingToolsInOtherEditors() noexcept override;
  void fsmSetStatusBarMessage(const QString& message,
                              int timeoutMs = -1) noexcept override;
  void fsmToolLeave() noexcept override;
  void fsmToolEnter(BoardEditorState_Select& state) noexcept override;
  void fsmToolEnter(BoardEditorState_DrawTrace& state) noexcept override;
  void fsmToolEnter(BoardEditorState_AddVia& state) noexcept override;
  void fsmToolEnter(BoardEditorState_DrawPolygon& state) noexcept override;
  void fsmToolEnter(BoardEditorState_AddStrokeText& state) noexcept override;
  void fsmToolEnter(BoardEditorState_DrawPlane& state) noexcept override;
  void fsmToolEnter(BoardEditorState_DrawZone& state) noexcept override;
  void fsmToolEnter(BoardEditorState_AddHole& state) noexcept override;
  void fsmToolEnter(BoardEditorState_AddDevice& state) noexcept override;
  void fsmToolEnter(BoardEditorState_Measure& state) noexcept override;
  void fsmSetFeatures(Features features) noexcept override;

  // Operator Overloadings
  Board2dTab& operator=(const Board2dTab& rhs) = delete;

signals:
  void wireModeRequested(BoardEditorState_DrawTrace::WireMode mode);
  void netRequested(bool autoNet, const std::optional<Uuid>& net);
  void layerRequested(const Layer& layer);
  void autoWidthRequested(bool autoWidth);
  void lineWidthRequested(const UnsignedLength& width);
  void traceWidthRequested(const PositiveLength& width);
  void sizeRequested(const PositiveLength& height);
  void drillRequested(const PositiveLength& height);
  void filledRequested(bool filled);
  void mirroredRequested(bool mirrored);
  void valueRequested(const QString& value);
  void zoneRuleRequested(Zone::Rule rule, bool enable);

protected:
  const LengthUnit* getCurrentUnit() const noexcept override;
  void requestRepaint() noexcept override;

private:
  void startDrc(bool quick) noexcept;
  void setDrcResult(const BoardDesignRuleCheck::Result& result) noexcept;
  void applyTheme() noexcept;

private:
  std::shared_ptr<ProjectEditor2> mEditor;
  const int mObjIndex;
  Theme::GridStyle mGridStyle;
  QPointF mSceneImagePos;
  int mFrameIndex;

  // Current tool
  Features mToolFeatures;
  ui::EditorTool mTool;
  Qt::CursorShape mToolCursorShape;
  QString mToolOverlayText;
  BoardEditorState_DrawTrace::WireMode mToolWireMode;
  QVector<std::pair<bool, std::optional<Uuid>>> mToolNetsQt;
  std::shared_ptr<slint::VectorModel<slint::SharedString>> mToolNets;
  std::pair<bool, std::optional<Uuid>> mToolNet;
  QVector<const Layer*> mToolLayersQt;
  std::shared_ptr<slint::VectorModel<slint::SharedString>> mToolLayers;
  const Layer* mToolLayer;
  bool mToolAutoWidth;
  UnsignedLength mToolLineWidth;
  LengthUnit mToolLineWidthUnit;
  PositiveLength mToolSize;
  LengthUnit mToolSizeUnit;
  PositiveLength mToolDrill;
  LengthUnit mToolDrillUnit;
  bool mToolFilled;
  bool mToolMirrored;
  QString mToolValue;
  std::shared_ptr<slint::VectorModel<slint::SharedString>>
      mToolValueSuggestions;
  Zone::Rules mToolZoneRules;

  std::unique_ptr<BoardDesignRuleCheck> mDrc;
  std::shared_ptr<Notification> mDrcNotification;
  uint mDrcUndoStackState;
  std::shared_ptr<RuleCheckMessagesModel> mDrcMessages;
  QString mDrcExecutionError;
  std::unique_ptr<BoardPlaneFragmentsBuilder> mPlaneBuilder;

  QVector<QMetaObject::Connection> mFsmStateConnections;
  QScopedPointer<BoardEditorFsm> mFsm;

  QVector<QMetaObject::Connection> mActiveConnections;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
