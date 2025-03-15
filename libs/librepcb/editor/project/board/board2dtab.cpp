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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "board2dtab.h"

#include "../../guiapplication.h"
#include "../../notification.h"
#include "../../notificationsmodel.h"
#include "../../rulecheck/rulecheckmessagesmodel.h"
#include "../../utils/slinthelpers.h"
#include "../../utils/uihelpers.h"
#include "../projecteditor2.h"
#include "../projectsmodel.h"
#include "fsm/boardeditorfsm.h"
#include "fsm/boardeditorstate_addhole.h"
#include "fsm/boardeditorstate_addstroketext.h"
#include "fsm/boardeditorstate_addvia.h"
#include "fsm/boardeditorstate_drawplane.h"
#include "fsm/boardeditorstate_drawpolygon.h"
#include "fsm/boardeditorstate_drawtrace.h"
#include "fsm/boardeditorstate_drawzone.h"

#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/boardplanefragmentsbuilder.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/types/layer.h>
#include <librepcb/core/utils/scopeguard.h>
#include <librepcb/core/workspace/theme.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacesettings.h>
#include <librepcb/editor/project/board/boardgraphicsscene.h>
#include <librepcb/editor/undostack.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

static QString toSingleLine(const QString& s) {
  return QString(s).replace("\n", "\\n");
}

static QString toMultiLine(const QString& s) {
  return s.trimmed().replace("\\n", "\n");
}

static ui::WireMode l2s(BoardEditorState_DrawTrace::WireMode v) noexcept {
  if (v == BoardEditorState_DrawTrace::WireMode::HV) {
    return ui::WireMode::HV;
  } else if (v == BoardEditorState_DrawTrace::WireMode::VH) {
    return ui::WireMode::VH;
  } else if (v == BoardEditorState_DrawTrace::WireMode::Deg9045) {
    return ui::WireMode::Deg9045;
  } else if (v == BoardEditorState_DrawTrace::WireMode::Deg4590) {
    return ui::WireMode::Deg4590;
  } else if (v == BoardEditorState_DrawTrace::WireMode::Straight) {
    return ui::WireMode::Straight;
  } else {
    return ui::WireMode::HV;
  }
}

static BoardEditorState_DrawTrace::WireMode s2l(ui::WireMode v) noexcept {
  if (v == ui::WireMode::HV) {
    return BoardEditorState_DrawTrace::WireMode::HV;
  } else if (v == ui::WireMode::VH) {
    return BoardEditorState_DrawTrace::WireMode::VH;
  } else if (v == ui::WireMode::Deg9045) {
    return BoardEditorState_DrawTrace::WireMode::Deg9045;
  } else if (v == ui::WireMode::Deg4590) {
    return BoardEditorState_DrawTrace::WireMode::Deg4590;
  } else if (v == ui::WireMode::Straight) {
    return BoardEditorState_DrawTrace::WireMode::Straight;
  } else {
    return BoardEditorState_DrawTrace::WireMode::HV;
  }
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Board2dTab::Board2dTab(GuiApplication& app, std::shared_ptr<ProjectEditor2> prj,
                       int boardIndex, QObject* parent) noexcept
  : GraphicsSceneTab(app, prj, boardIndex, parent),
    mGridStyle(Theme::GridStyle::None),
    mSceneImagePos(),
    mFrameIndex(0),
    mToolFeatures(),
    mTool(ui::EditorTool::Select),
    mToolCursorShape(Qt::ArrowCursor),
    mToolOverlayText(),
    mToolWireMode(BoardEditorState_DrawTrace::WireMode::HV),
    mToolNetsQt(),
    mToolNets(std::make_shared<slint::VectorModel<slint::SharedString>>()),
    mToolNet({true, std::nullopt}),
    mToolLayersQt(),
    mToolLayers(std::make_shared<slint::VectorModel<slint::SharedString>>()),
    mToolLayer(nullptr),
    mToolAutoWidth(false),
    mToolLineWidth(0),
    mToolLineWidthUnit(
        app.getWorkspace().getSettings().defaultLengthUnit.get()),
    mToolSize(1),
    mToolSizeUnit(app.getWorkspace().getSettings().defaultLengthUnit.get()),
    mToolDrill(1),
    mToolDrillUnit(app.getWorkspace().getSettings().defaultLengthUnit.get()),
    mToolFilled(false),
    mToolMirrored(false),
    mToolValue(),
    mToolValueSuggestions(
        std::make_shared<slint::VectorModel<slint::SharedString>>()),
    mToolZoneRules(),
    mDrc(new BoardDesignRuleCheck(this)),
    mDrcNotification(new Notification(ui::NotificationType::Progress, QString(),
                                      QString(), QString(), QString(), true)),
    mDrcUndoStackState(mEditor->getUndoStack().getUniqueStateId()),
    mDrcMessages(),
    mDrcExecutionError(),
    mPlaneBuilder(),
    mFsm() {
  // Connect undo stack.
  connect(&prj->getUndoStack(), &UndoStack::stateModified, this, [this]() {
    if (mDrcMessages) {
      emit baseUiDataChanged();
    }
  });
  connect(&mEditor->getUndoStack(), &UndoStack::stateModified, this,
          &Board2dTab::tabUiDataChanged);
  connect(mEditor.get(), &ProjectEditor2::manualModificationsMade, this,
          &Board2dTab::tabUiDataChanged);

  // Connect DRC.
  connect(mDrc.get(), &BoardDesignRuleCheck::progressPercent,
          mDrcNotification.get(), &Notification::setProgress);
  connect(mDrc.get(), &BoardDesignRuleCheck::progressStatus,
          mDrcNotification.get(), &Notification::setDescription);
  connect(mDrc.get(), &BoardDesignRuleCheck::finished, this,
          &Board2dTab::setDrcResult);

  // Build the whole board editor finite state machine.
  Q_ASSERT(qApp->activeWindow());
  BoardEditorFsm::Context fsmContext{
      mApp.getWorkspace(),   mEditor->getProject(), prj->getUndoStack(),
      *qApp->activeWindow(), *mLayerProvider,       *this,
  };
  mFsm.reset(new BoardEditorFsm(fsmContext));

  // Apply theme whenever it has been modified.
  connect(&mApp.getWorkspace().getSettings().themes,
          &WorkspaceSettingsItem_Themes::edited, this, &Board2dTab::applyTheme);
  applyTheme();
}

Board2dTab::~Board2dTab() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

ui::TabData Board2dTab::getBaseUiData() const noexcept {
  auto brd = mEditor->getProject().getBoardByIndex(mObjIndex);

  ui::RuleCheckState drcState;
  if (mDrc->isRunning()) {
    drcState = ui::RuleCheckState::Running;
  } else if (!mDrcMessages) {
    drcState = ui::RuleCheckState::NotRunYet;
  } else if (mDrcUndoStackState == mEditor->getUndoStack().getUniqueStateId()) {
    drcState = ui::RuleCheckState::UpToDate;
  } else {
    drcState = ui::RuleCheckState::Outdated;
  }

  return ui::TabData{
      ui::TabType::Board2d,  // Type
      q2s(brd ? *brd->getName() : QString()),  // Title
      mApp.getProjects().getIndexOf(mEditor),  // Project index
      drcState,  // Rule check state
      mDrcMessages,  // Rule check messages
      mDrcMessages ? mDrcMessages->getUnapprovedCount()
                   : 0,  // Rule check unapproved messages
      q2s(mDrcExecutionError),  // Rule check execution error
      mEditor->getProject().getDirectory().isWritable(),  // Can save
      true,  // Can export graphics
      mEditor->getUndoStack().canUndo(),  // Can undo
      q2s(mEditor->getUndoStack().getUndoCmdText()),  // Undo text
      mEditor->getUndoStack().canRedo(),  // Can redo
      q2s(mEditor->getUndoStack().getRedoCmdText()),  // Redo text
      mToolFeatures.testFlag(Feature::Cut),  // Can cut
      mToolFeatures.testFlag(Feature::Copy),  // Can copy
      mToolFeatures.testFlag(Feature::Paste),  // Can paste
      mToolFeatures.testFlag(Feature::Remove),  // Can remove
      mToolFeatures.testFlag(Feature::Rotate),  // Can rotate
      mToolFeatures.testFlag(Feature::Mirror),  // Can mirror
      mToolFeatures.testFlag(Feature::MoveAlign),  // Can move/align
      mToolFeatures.testFlag(Feature::SnapToGrid),  // Can snap to grid
      mToolFeatures.testFlag(Feature::ResetTexts),  // Can reset texts
      mToolFeatures.testFlag(Feature::LockPlacement),  // Can lock placement
      mToolFeatures.testFlag(Feature::Properties),  // Can edit properties
  };
}

ui::Board2dTabData Board2dTab::getUiData() const noexcept {
  const Theme& theme = mApp.getWorkspace().getSettings().themes.getActive();
  auto brd = mEditor->getProject().getBoardByIndex(mObjIndex);

  QString gridIntervalStr;
  if (brd) {
    const LengthUnit& unit = brd->getGridUnit();
    gridIntervalStr = Toolbox::floatToString(
        unit.convertToUnit(*brd->getGridInterval()), 10, QLocale());
  }

  return ui::Board2dTabData{
      q2s(theme.getColor(Theme::Color::sBoardInfoBox)
              .getPrimaryColor()),  // Overlay color
      q2s(theme.getColor(Theme::Color::sBoardInfoBox)
              .getSecondaryColor()),  // Overlay text color
      l2s(mGridStyle),  // Grid style
      q2s(gridIntervalStr),  // Grid interval
      brd ? l2s(brd->getGridUnit())
          : ui::LengthUnit::Millimeters,  // Length unit
      mTool,  // Tool
      q2s(mToolCursorShape),  // Tool cursor
      q2s(mToolOverlayText),  // Tool overlay text
      l2s(mToolWireMode),  // Tool wire mode
      ui::ComboBoxData{
          // Tool net
          mToolNets,  // Items,
          static_cast<int>(mToolNetsQt.indexOf(mToolNet)),  // Current index
      },
      ui::ComboBoxData{
          // Tool layer
          mToolLayers,  // Items
          static_cast<int>(mToolLayersQt.indexOf(mToolLayer)),  // Current index
      },
      ui::LengthEditData{
          // Tool line width
          l2s(*mToolLineWidth),  // Value
          l2s(mToolLineWidthUnit),  // Unit
          *mToolLineWidth % 2 == 0,  // Can decrease
          false,  // Increase
          false,  // Decrease
      },
      ui::LengthEditData{
          // Tool size
          l2s(*mToolSize),  // Value
          l2s(mToolSizeUnit),  // Unit
          *mToolSize % 2 == 0,  // Can decrease
          false,  // Increase
          false,  // Decrease
      },
      ui::LengthEditData{
          // Tool drill
          l2s(*mToolDrill),  // Value
          l2s(mToolDrillUnit),  // Unit
          *mToolDrill % 2 == 0,  // Can decrease
          false,  // Increase
          false,  // Decrease
      },
      mToolFilled,  // Tool filled
      mToolMirrored,  // Tool mirrored
      ui::LineEditData{
          // Tool value
          true,  // Enabled
          q2s(toSingleLine(mToolValue)),  // Text
          slint::SharedString(),  // Placeholder
          mToolValueSuggestions,  // Suggestions
      },
      mToolZoneRules.testFlag(Zone::Rule::NoCopper),  // Tool no copper
      mToolZoneRules.testFlag(Zone::Rule::NoPlanes),  // Tool no planes
      mToolZoneRules.testFlag(Zone::Rule::NoExposure),  // Tool no exposure
      mToolZoneRules.testFlag(Zone::Rule::NoDevices),  // Tool no devices
      q2s(mSceneImagePos),  // Scene image position
      mFrameIndex,  // Frame index
  };
}

void Board2dTab::setUiData(const ui::Board2dTabData& data) noexcept {
  auto brd = mEditor->getProject().getBoardByIndex(mObjIndex);

  mSceneImagePos = s2q(data.scene_image_pos);

  mGridStyle = s2l(data.grid_style);
  if (mScene) {
    mScene->setGridStyle(mGridStyle);
  }
  const LengthUnit unit = s2l(data.unit);
  if (brd && (unit != brd->getGridUnit())) {
    brd->setGridUnit(unit);
    mEditor->setManualModificationsMade();
  }

  // Tool net
  const auto netCfg = mToolNetsQt.value(data.tool_net.current_index);
  emit netRequested(netCfg.first, netCfg.second);

  // Tool layer
  if (const Layer* layer = mToolLayersQt.value(data.tool_layer.current_index)) {
    emit layerRequested(*layer);
  }

  // Tool wire mode
  emit wireModeRequested(s2l(data.tool_wire_mode));

  // Tool line width
  mToolLineWidthUnit = s2l(data.tool_line_width.unit);
  if (auto l = s2ulength(data.tool_line_width.value)) {
    emit lineWidthRequested(*l);
  }
  if (auto l = s2plength(data.tool_line_width.value)) {
    emit traceWidthRequested(*l);
  }
  if (data.tool_line_width.increase) {
    const auto w = mToolLineWidth * 2;
    if (w >= 0) emit lineWidthRequested(UnsignedLength(w));
    if (w > 0) emit traceWidthRequested(PositiveLength(w));
  } else if (data.tool_line_width.decrease && (*mToolLineWidth % 2 == 0)) {
    const auto w = mToolLineWidth / 2;
    if (w >= 0) emit lineWidthRequested(UnsignedLength(w));
    if (w > 0) emit traceWidthRequested(PositiveLength(w));
  }

  // Tool size
  mToolSizeUnit = s2l(data.tool_size.unit);
  if (auto s = s2plength(data.tool_size.value)) {
    emit sizeRequested(*s);
  }
  if (data.tool_size.increase) {
    emit sizeRequested(PositiveLength(mToolSize * 2));
  } else if (data.tool_size.decrease && (mToolSize > 1) &&
             (*mToolSize % 2 == 0)) {
    emit sizeRequested(PositiveLength(mToolSize / 2));
  }

  // Tool drill
  mToolDrillUnit = s2l(data.tool_drill.unit);
  if (auto d = s2plength(data.tool_drill.value)) {
    emit drillRequested(*d);
  }
  if (data.tool_drill.increase) {
    emit drillRequested(PositiveLength(mToolDrill * 2));
  } else if (data.tool_drill.decrease && (mToolDrill > 1) &&
             (*mToolDrill % 2 == 0)) {
    emit drillRequested(PositiveLength(mToolDrill / 2));
  }

  // Tool filled
  emit filledRequested(data.tool_filled);

  // Tool mirrored
  emit mirroredRequested(data.tool_mirrored);

  // Tool value
  emit valueRequested(toMultiLine(s2q(data.tool_value.text)));

  // Tool zone rules
  emit zoneRuleRequested(Zone::Rule::NoCopper, data.tool_no_copper);
  emit zoneRuleRequested(Zone::Rule::NoPlanes, data.tool_no_planes);
  emit zoneRuleRequested(Zone::Rule::NoExposure, data.tool_no_exposures);
  emit zoneRuleRequested(Zone::Rule::NoDevices, data.tool_no_devices);

  requestRepaint();
}

void Board2dTab::activate() noexcept {
  if (auto brd = mEditor->getProject().getBoardByIndex(mObjIndex)) {
    mPlaneBuilder.reset(new BoardPlaneFragmentsBuilder(this));
    connect(mPlaneBuilder.get(), &BoardPlaneFragmentsBuilder::finished, this,
            [this](BoardPlaneFragmentsBuilder::Result result) {
              if (result.applyToBoard() && result.board) {
                result.board->forceAirWiresRebuild();
                requestRepaint();
              }
            });
    mPlaneBuilder->start(*brd);
    mScene.reset(new BoardGraphicsScene(
        *brd, *mLayerProvider, std::make_shared<QSet<const NetSignal*>>(),
        this));
    mScene->setGridInterval(brd->getGridInterval());
    connect(mScene.get(), &GraphicsScene::changed, this,
            &Board2dTab::requestRepaint);
    applyTheme();

    // Force airwire rebuild immediately and on every project modification.
    brd->triggerAirWiresRebuild();
    mActiveConnections.append(connect(&mEditor->getUndoStack(),
                                      &UndoStack::stateModified, brd,
                                      &Board::triggerAirWiresRebuild));

    requestRepaint();
  }
}

void Board2dTab::deactivate() noexcept {
  while (!mActiveConnections.isEmpty()) {
    disconnect(mActiveConnections.takeLast());
  }
  mPlaneBuilder.reset();
  mScene.reset();
}

bool Board2dTab::trigger(ui::Action a) noexcept {
  if (a == ui::Action::Save) {
    mEditor->saveProject();
    return true;
  } else if (a == ui::Action::SectionGridIntervalIncrease) {
    if (auto brd = mEditor->getProject().getBoardByIndex(mObjIndex)) {
      brd->setGridInterval(PositiveLength(brd->getGridInterval() * 2));
      if (mScene) {
        mScene->setGridInterval(brd->getGridInterval());
        requestRepaint();
      }
    }
    return true;
  } else if (a == ui::Action::SectionGridIntervalDecrease) {
    if (auto brd = mEditor->getProject().getBoardByIndex(mObjIndex)) {
      if ((*brd->getGridInterval() % 2) == 0) {
        brd->setGridInterval(PositiveLength(brd->getGridInterval() / 2));
        if (mScene) {
          mScene->setGridInterval(brd->getGridInterval());
          requestRepaint();
        }
      }
    }
    return true;
  } else if (a == ui::Action::RunQuickCheck) {
    startDrc(true);
    return true;
  } else if (a == ui::Action::RunDrc) {
    startDrc(false);
    return true;
  } else if (a == ui::Action::Cut) {
    return mFsm->processCut();
  } else if (a == ui::Action::Copy) {
    return mFsm->processCopy();
  } else if (a == ui::Action::Paste) {
    return mFsm->processPaste();
  } else if (a == ui::Action::Delete) {
    return mFsm->processRemove();
  } else if (a == ui::Action::RotateCcw) {
    return mFsm->processRotate(Angle::deg90());
  } else if (a == ui::Action::RotateCw) {
    return mFsm->processRotate(-Angle::deg90());
  } else if (a == ui::Action::MirrorHorizontally) {
    return mFsm->processFlip(Qt::Horizontal);
  } else if (a == ui::Action::MirrorVertically) {
    return mFsm->processFlip(Qt::Vertical);
    //} else if (a ==ui::Action::MoveAlign) {
    //  return mFsm->process();
    //} else if (a ==ui::Action::SnapToGrid) {
    //  return mFsm->process();
  } else if (a == ui::Action::ResetTexts) {
    return mFsm->processResetAllTexts();
    //} else if (a ==ui::Action::LockUnlockPlacement) {
    //  return mFsm->process();
  } else if (a == ui::Action::EditProperties) {
    return mFsm->processEditProperties();
  } else if (a == ui::Action::ToolSelect) {
    return mFsm->processSelect();
  } else if (a == ui::Action::ToolWire) {
    return mFsm->processDrawTrace();
  } else if (a == ui::Action::ToolVia) {
    return mFsm->processAddVia();
  } else if (a == ui::Action::ToolPolygon) {
    return mFsm->processDrawPolygon();
  } else if (a == ui::Action::ToolText) {
    return mFsm->processAddStrokeText();
  } else if (a == ui::Action::ToolPlane) {
    return mFsm->processDrawPlane();
  } else if (a == ui::Action::ToolZone) {
    return mFsm->processDrawZone();
  } else if (a == ui::Action::ToolHole) {
    return mFsm->processAddHole();
  } else if (a == ui::Action::ToolMeasure) {
    return mFsm->processMeasure();
  } else if (a == ui::Action::FocusLost) {
    if (mMouseEvent.buttons.testFlag(Qt::LeftButton)) {
      mMouseEvent.buttons.setFlag(Qt::LeftButton, false);
      mFsm->processGraphicsSceneLeftMouseButtonReleased(mMouseEvent);
    }
    if (mMouseEvent.buttons.testFlag(Qt::RightButton)) {
      mMouseEvent.buttons.setFlag(Qt::RightButton, false);
    }
    return true;
  }

  return GraphicsSceneTab::trigger(a);
}

bool Board2dTab::processScenePointerEvent(
    const QPointF& pos, const QPointF& globalPos,
    slint::private_api::PointerEvent e) noexcept {
  if (GraphicsSceneTab::processScenePointerEvent(pos, globalPos, e)) {
    return true;
  }

  using slint::private_api::PointerEventButton;
  using slint::private_api::PointerEventKind;

  bool handled = false;
  if (mMouseEventIsDoubleClick && (e.button == PointerEventButton::Left)) {
    handled =
        mFsm->processGraphicsSceneLeftMouseButtonDoubleClicked(mMouseEvent);
  } else if ((e.button == PointerEventButton::Left) &&
             (e.kind == PointerEventKind::Down)) {
    handled = mFsm->processGraphicsSceneLeftMouseButtonPressed(mMouseEvent);
  } else if ((e.button == PointerEventButton::Left) &&
             (e.kind == PointerEventKind::Up)) {
    handled = mFsm->processGraphicsSceneLeftMouseButtonReleased(mMouseEvent);
  } else if ((e.button == PointerEventButton::Right) &&
             (e.kind == PointerEventKind::Up)) {
    handled = mFsm->processGraphicsSceneRightMouseButtonReleased(mMouseEvent);
  } else if (e.kind == PointerEventKind::Move) {
    handled = mFsm->processGraphicsSceneMouseMoved(mMouseEvent);
  }

  if (handled) {
    requestRepaint();
  }

  return handled;
}

/*******************************************************************************
 *  BoardEditorFsmAdapter Methods
 ******************************************************************************/

Board* Board2dTab::fsmGetActiveBoard() noexcept {
  return mEditor->getProject().getBoardByIndex(mObjIndex);
}

BoardGraphicsScene* Board2dTab::fsmGetGraphicsScene() noexcept {
  return qobject_cast<BoardGraphicsScene*>(mScene.get());
}

bool Board2dTab::fsmGetIgnoreLocks() const noexcept {
  return false;  // TODO
}

void Board2dTab::fsmSetViewCursor(
    const std::optional<Qt::CursorShape>& shape) noexcept {
  if (shape) {
    mToolCursorShape = *shape;
  } else {
    mToolCursorShape = Qt::ArrowCursor;
  }
  emit tabUiDataChanged();
}

void Board2dTab::fsmSetViewGrayOut(bool grayOut) noexcept {
  if (mScene) {
    mScene->setGrayOut(grayOut);
  }
}

void Board2dTab::fsmSetViewInfoBoxText(const QString& text) noexcept {
  QString t = text;
  t.replace("&nbsp;", " ");
  t.replace("<br>", "\n");
  t.replace("<b>", "");
  t.replace("</b>", "");

  if (t != mToolOverlayText) {
    mToolOverlayText = t;
    emit tabUiDataChanged();
  }
}

void Board2dTab::fsmSetViewRuler(
    const std::optional<std::pair<Point, Point>>& pos) noexcept {
  if (mScene) {
    mScene->setRulerPositions(pos);
  }
}

void Board2dTab::fsmSetSceneCursor(const Point& pos, bool cross,
                                   bool circle) noexcept {
  if (mScene) {
    mScene->setSceneCursor(pos, cross, circle);
  }
}

QPainterPath Board2dTab::fsmCalcPosWithTolerance(
    const Point& pos, qreal multiplier) const noexcept {
  return calcPosWithTolerance(pos, multiplier);
}

Point Board2dTab::fsmMapGlobalPosToScenePos(const QPoint& pos) const noexcept {
  if (QWidget* win = qApp->activeWindow()) {
    return mapToScenePos(win->mapFromGlobal(pos) - mSceneImagePos);
  } else {
    qWarning() << "Failed to map global position to scene position.";
    return Point();
  }
}

void Board2dTab::fsmSetHighlightedNetSignals(
    const QSet<const NetSignal*>& sigs) noexcept {
  mEditor->setHighlightedNetSignals(sigs);
}

void Board2dTab::fsmAbortBlockingToolsInOtherEditors() noexcept {
  /* TODO */
}

void Board2dTab::fsmSetStatusBarMessage(const QString& message,
                                        int timeoutMs) noexcept {
  emit statusBarMessageChanged(message, timeoutMs);
}

void Board2dTab::fsmToolLeave() noexcept {
  while (!mFsmStateConnections.isEmpty()) {
    disconnect(mFsmStateConnections.takeLast());
  }
  mTool = ui::EditorTool::Select;
  fsmSetFeatures(Features());
  emit tabUiDataChanged();
}

void Board2dTab::fsmToolEnter(BoardEditorState_Select& state) noexcept {
  Q_UNUSED(state);

  mTool = ui::EditorTool::Select;
  emit tabUiDataChanged();
}

void Board2dTab::fsmToolEnter(BoardEditorState_DrawTrace& state) noexcept {
  mTool = ui::EditorTool::Wire;

  // Wire mode
  auto setWireMode = [this](BoardEditorState_DrawTrace::WireMode m) {
    mToolWireMode = m;
    emit tabUiDataChanged();
  };
  setWireMode(state.getWireMode());
  mFsmStateConnections.append(connect(
      &state, &BoardEditorState_DrawTrace::wireModeChanged, this, setWireMode));
  mFsmStateConnections.append(
      connect(this, &Board2dTab::wireModeRequested, &state,
              &BoardEditorState_DrawTrace::setWireMode));

  // Trace width
  auto setTraceWidth = [this](const PositiveLength& width) {
    mToolLineWidth = positiveToUnsigned(width);
    emit tabUiDataChanged();
  };
  setTraceWidth(state.getWidth());
  mFsmStateConnections.append(connect(
      &state, &BoardEditorState_DrawTrace::widthChanged, this, setTraceWidth));
  mFsmStateConnections.append(connect(this, &Board2dTab::traceWidthRequested,
                                      &state,
                                      &BoardEditorState_DrawTrace::setWidth));

  // Auto width
  auto setAutoWidth = [this](bool autoWidth) {
    mToolAutoWidth = autoWidth;
    emit tabUiDataChanged();
  };
  setAutoWidth(state.getAutoWidth());
  mFsmStateConnections.append(
      connect(&state, &BoardEditorState_DrawTrace::autoWidthChanged, this,
              setAutoWidth));
  mFsmStateConnections.append(
      connect(this, &Board2dTab::autoWidthRequested, &state,
              &BoardEditorState_DrawTrace::setAutoWidth));

  // Layers
  mToolLayersQt = Toolbox::sortedQSet(state.getAvailableLayers());
  mToolLayers->clear();
  for (const Layer* layer : mToolLayersQt) {
    mToolLayers->push_back(q2s(layer->getNameTr()));
  }

  // Layer
  auto setLayer = [this](const Layer& layer) {
    mToolLayer = &layer;
    emit tabUiDataChanged();
  };
  setLayer(state.getLayer());
  mFsmStateConnections.append(connect(
      &state, &BoardEditorState_DrawTrace::layerChanged, this, setLayer));
  mFsmStateConnections.append(connect(this, &Board2dTab::layerRequested, &state,
                                      &BoardEditorState_DrawTrace::setLayer));

  // Via size
  auto setViaSize = [this](const PositiveLength& size) {
    mToolSize = size;
    emit tabUiDataChanged();
  };
  setViaSize(state.getViaSize());
  mFsmStateConnections.append(connect(
      &state, &BoardEditorState_DrawTrace::viaSizeChanged, this, setViaSize));
  mFsmStateConnections.append(connect(this, &Board2dTab::sizeRequested, &state,
                                      &BoardEditorState_DrawTrace::setViaSize));

  // Via drill
  auto setViaDrill = [this](const PositiveLength& drill) {
    mToolDrill = drill;
    emit tabUiDataChanged();
  };
  setViaDrill(state.getViaDrillDiameter());
  mFsmStateConnections.append(
      connect(&state, &BoardEditorState_DrawTrace::viaDrillDiameterChanged,
              this, setViaDrill));
  mFsmStateConnections.append(
      connect(this, &Board2dTab::drillRequested, &state,
              &BoardEditorState_DrawTrace::setViaDrillDiameter));

  emit tabUiDataChanged();
}

void Board2dTab::fsmToolEnter(BoardEditorState_AddVia& state) noexcept {
  mTool = ui::EditorTool::Via;

  // Via size
  auto setViaSize = [this](const PositiveLength& size) {
    mToolSize = size;
    emit tabUiDataChanged();
  };
  setViaSize(state.getSize());
  mFsmStateConnections.append(
      connect(&state, &BoardEditorState_AddVia::sizeChanged, this, setViaSize));
  mFsmStateConnections.append(connect(this, &Board2dTab::sizeRequested, &state,
                                      &BoardEditorState_AddVia::setSize));

  // Via drill
  auto setViaDrill = [this](const PositiveLength& drill) {
    mToolDrill = drill;
    emit tabUiDataChanged();
  };
  setViaDrill(state.getDrillDiameter());
  mFsmStateConnections.append(
      connect(&state, &BoardEditorState_AddVia::drillDiameterChanged, this,
              setViaDrill));
  mFsmStateConnections.append(
      connect(this, &Board2dTab::drillRequested, &state,
              &BoardEditorState_AddVia::setDrillDiameter));

  // Nets
  mToolNetsQt.clear();
  mToolNets->clear();
  mToolNetsQt.append(std::make_pair(true, std::nullopt));
  mToolNets->push_back(q2s("[" % tr("Auto") % "]"));
  mToolNetsQt.append(std::make_pair(false, std::nullopt));
  mToolNets->push_back(q2s("[" % tr("None") % "]"));
  for (const auto& item : state.getAvailableNets()) {
    mToolNetsQt.append(std::make_pair(false, item.first));
    mToolNets->push_back(q2s(item.second));
  }

  // Net
  auto setNet = [this](bool autoNet, const std::optional<Uuid>& net) {
    if (autoNet) {
      mToolNet = std::make_pair(true, std::nullopt);
    } else {
      mToolNet = std::make_pair(false, net);
    }
    emit tabUiDataChanged();
  };
  setNet(state.getUseAutoNet(), state.getNet());
  mFsmStateConnections.append(
      connect(&state, &BoardEditorState_AddVia::netChanged, this, setNet));
  mFsmStateConnections.append(connect(this, &Board2dTab::netRequested, &state,
                                      &BoardEditorState_AddVia::setNet));

  emit tabUiDataChanged();
}

void Board2dTab::fsmToolEnter(BoardEditorState_DrawPolygon& state) noexcept {
  mTool = ui::EditorTool::Polygon;

  // Layers
  mToolLayersQt = Toolbox::sortedQSet(state.getAvailableLayers());
  mToolLayers->clear();
  for (const Layer* layer : mToolLayersQt) {
    mToolLayers->push_back(q2s(layer->getNameTr()));
  }

  // Layer
  auto setLayer = [this](const Layer& layer) {
    mToolLayer = &layer;
    emit tabUiDataChanged();
  };
  setLayer(state.getLayer());
  mFsmStateConnections.append(connect(
      &state, &BoardEditorState_DrawPolygon::layerChanged, this, setLayer));
  mFsmStateConnections.append(connect(this, &Board2dTab::layerRequested, &state,
                                      &BoardEditorState_DrawPolygon::setLayer));

  // Line width
  auto setLineWidth = [this](const UnsignedLength& width) {
    mToolLineWidth = width;
    emit tabUiDataChanged();
  };
  setLineWidth(state.getLineWidth());
  mFsmStateConnections.append(
      connect(&state, &BoardEditorState_DrawPolygon::lineWidthChanged, this,
              setLineWidth));
  mFsmStateConnections.append(
      connect(this, &Board2dTab::lineWidthRequested, &state,
              &BoardEditorState_DrawPolygon::setLineWidth));

  // Filled
  auto setFilled = [this](bool filled) {
    mToolFilled = filled;
    emit tabUiDataChanged();
  };
  setFilled(state.getFilled());
  mFsmStateConnections.append(connect(
      &state, &BoardEditorState_DrawPolygon::filledChanged, this, setFilled));
  mFsmStateConnections.append(
      connect(this, &Board2dTab::filledRequested, &state,
              &BoardEditorState_DrawPolygon::setFilled));

  emit tabUiDataChanged();
}

void Board2dTab::fsmToolEnter(BoardEditorState_AddStrokeText& state) noexcept {
  mTool = ui::EditorTool::Text;

  // Layers
  mToolLayersQt = Toolbox::sortedQSet(state.getAvailableLayers());
  mToolLayers->clear();
  for (const Layer* layer : mToolLayersQt) {
    mToolLayers->push_back(q2s(layer->getNameTr()));
  }

  // Layer
  auto setLayer = [this](const Layer& layer) {
    mToolLayer = &layer;
    emit tabUiDataChanged();
  };
  setLayer(state.getLayer());
  mFsmStateConnections.append(connect(
      &state, &BoardEditorState_AddStrokeText::layerChanged, this, setLayer));
  mFsmStateConnections.append(
      connect(this, &Board2dTab::layerRequested, &state,
              &BoardEditorState_AddStrokeText::setLayer));

  // Height
  auto setHeight = [this](const PositiveLength& height) {
    mToolSize = height;
    emit tabUiDataChanged();
  };
  setHeight(state.getHeight());
  mFsmStateConnections.append(connect(
      &state, &BoardEditorState_AddStrokeText::heightChanged, this, setHeight));
  mFsmStateConnections.append(
      connect(this, &Board2dTab::sizeRequested, &state,
              &BoardEditorState_AddStrokeText::setHeight));

  // Text
  auto setText = [this](const QString& text) {
    mToolValue = text;
    emit tabUiDataChanged();
  };
  setText(state.getText());
  mFsmStateConnections.append(connect(
      &state, &BoardEditorState_AddStrokeText::textChanged, this, setText));
  mFsmStateConnections.append(
      connect(this, &Board2dTab::valueRequested, &state,
              &BoardEditorState_AddStrokeText::setText));

  // Text suggestions
  mToolValueSuggestions->clear();
  for (const QString& v : state.getTextSuggestions()) {
    mToolValueSuggestions->push_back(q2s(v));
  }

  // Mirrored
  auto setMirrored = [this](bool mirrored) {
    mToolMirrored = mirrored;
    emit tabUiDataChanged();
  };
  setMirrored(state.getMirrored());
  mFsmStateConnections.append(
      connect(&state, &BoardEditorState_AddStrokeText::mirroredChanged, this,
              setMirrored));
  mFsmStateConnections.append(
      connect(this, &Board2dTab::mirroredRequested, &state,
              &BoardEditorState_AddStrokeText::setMirrored));

  emit tabUiDataChanged();
}

void Board2dTab::fsmToolEnter(BoardEditorState_DrawPlane& state) noexcept {
  mTool = ui::EditorTool::Plane;

  // Nets
  mToolNetsQt.clear();
  mToolNets->clear();
  mToolNetsQt.append(std::make_pair(false, std::nullopt));
  mToolNets->push_back(q2s("[" % tr("None") % "]"));
  for (const auto& item : state.getAvailableNets()) {
    mToolNetsQt.append(std::make_pair(false, item.first));
    mToolNets->push_back(q2s(item.second));
  }

  // Net
  auto setNet = [this](const std::optional<Uuid>& net) {
    mToolNet = std::make_pair(false, net);
    emit tabUiDataChanged();
  };
  setNet(state.getNet());
  mFsmStateConnections.append(
      connect(&state, &BoardEditorState_DrawPlane::netChanged, this, setNet));
  mFsmStateConnections.append(
      connect(this, &Board2dTab::netRequested, &state,
              [&state](bool autoNet, const std::optional<Uuid>& net) {
                Q_UNUSED(autoNet);
                state.setNet(net);
              }));

  // Layers
  mToolLayersQt = Toolbox::sortedQSet(state.getAvailableLayers());
  mToolLayers->clear();
  for (const Layer* layer : mToolLayersQt) {
    mToolLayers->push_back(q2s(layer->getNameTr()));
  }

  // Layer
  auto setLayer = [this](const Layer& layer) {
    mToolLayer = &layer;
    emit tabUiDataChanged();
  };
  setLayer(state.getLayer());
  mFsmStateConnections.append(connect(
      &state, &BoardEditorState_DrawPlane::layerChanged, this, setLayer));
  mFsmStateConnections.append(connect(this, &Board2dTab::layerRequested, &state,
                                      &BoardEditorState_DrawPlane::setLayer));

  emit tabUiDataChanged();
}

void Board2dTab::fsmToolEnter(BoardEditorState_DrawZone& state) noexcept {
  mTool = ui::EditorTool::Zone;

  // Available layers
  mToolLayersQt = Toolbox::sortedQSet(state.getAvailableLayers());
  mToolLayers->clear();
  for (const Layer* layer : mToolLayersQt) {
    mToolLayers->push_back(q2s(layer->getNameTr()));
  }

  // Layers
  auto setLayers = [this](const QSet<const Layer*>& layers) {
    if (!layers.isEmpty()) {
      mToolLayer = layers.values().first();
    }
    emit tabUiDataChanged();
  };
  setLayers(state.getLayers());
  mFsmStateConnections.append(connect(
      &state, &BoardEditorState_DrawZone::layersChanged, this, setLayers));
  mFsmStateConnections.append(
      connect(this, &Board2dTab::layerRequested, &state,
              [&state](const Layer& layer) { state.setLayers({&layer}); }));

  // Rules
  auto setRules = [this](Zone::Rules rules) {
    mToolZoneRules = rules;
    emit tabUiDataChanged();
  };
  setRules(state.getRules());
  mFsmStateConnections.append(connect(
      &state, &BoardEditorState_DrawZone::rulesChanged, this, setRules));
  mFsmStateConnections.append(connect(this, &Board2dTab::zoneRuleRequested,
                                      &state,
                                      &BoardEditorState_DrawZone::setRule));

  emit tabUiDataChanged();
}

void Board2dTab::fsmToolEnter(BoardEditorState_AddHole& state) noexcept {
  mTool = ui::EditorTool::Hole;

  // Drill
  auto setDrill = [this](const PositiveLength& drill) {
    mToolDrill = drill;
    emit tabUiDataChanged();
  };
  setDrill(state.getDiameter());
  mFsmStateConnections.append(connect(
      &state, &BoardEditorState_AddHole::diameterChanged, this, setDrill));
  mFsmStateConnections.append(connect(this, &Board2dTab::drillRequested, &state,
                                      &BoardEditorState_AddHole::setDiameter));

  emit tabUiDataChanged();
}

void Board2dTab::fsmToolEnter(BoardEditorState_AddDevice& state) noexcept {
  Q_UNUSED(state);
}

void Board2dTab::fsmToolEnter(BoardEditorState_Measure& state) noexcept {
  Q_UNUSED(state);

  mTool = ui::EditorTool::Measure;
  emit tabUiDataChanged();
}

void Board2dTab::fsmSetFeatures(Features features) noexcept {
  if (features != mToolFeatures) {
    mToolFeatures = features;
    emit baseUiDataChanged();
  }
}

const LengthUnit* Board2dTab::getCurrentUnit() const noexcept {
  if (auto brd = mEditor->getProject().getBoardByIndex(mObjIndex)) {
    return &brd->getGridUnit();
  } else {
    return nullptr;
  }
}

void Board2dTab::requestRepaint() noexcept {
  ++mFrameIndex;
  emit tabUiDataChanged();
}

void Board2dTab::startDrc(bool quick) noexcept {
  auto board = mEditor->getProject().getBoardByIndex(mObjIndex);
  if (!board) return;

  // Abort any ongoing run.
  mDrc->cancel();

  // Show progress notification during the run.
  mDrcNotification->setTitle(
      (quick ? tr("Running Quick Check") : tr("Running Design Rule Check")) %
      "...");
  mApp.getNotifications().push(mDrcNotification);

  // Run the DRC.
  mDrcUndoStackState = mEditor->getUndoStack().getUniqueStateId();
  mDrc->start(*board, board->getDrcSettings(), quick);  // can throw
  emit baseUiDataChanged();
}

void Board2dTab::setDrcResult(
    const BoardDesignRuleCheck::Result& result) noexcept {
  auto board = mEditor->getProject().getBoardByIndex(mObjIndex);
  if (!board) return;

  // Detect & remove disappeared messages.
  const QSet<SExpression> approvals =
      RuleCheckMessage::getAllApprovals(result.messages);
  if (board && board->updateDrcMessageApprovals(approvals, result.quick)) {
    mEditor->setManualModificationsMade();
  }

  // Update UI.
  if (!mDrcMessages) {
    mDrcMessages.reset(new RuleCheckMessagesModel());
    connect(mDrcMessages.get(), &RuleCheckMessagesModel::unapprovedCountChanged,
            this, &Board2dTab::baseUiDataChanged);
    connect(mDrcMessages.get(), &RuleCheckMessagesModel::approvalChanged, board,
            &Board::setDrcMessageApproved);
    connect(mDrcMessages.get(), &RuleCheckMessagesModel::approvalChanged,
            mEditor.get(), &ProjectEditor2::setManualModificationsMade);
  }
  mDrcMessages->setMessages(result.messages, board->getDrcMessageApprovals());
  mDrcExecutionError = result.errors.join("\n\n");
  mDrcNotification->dismiss();
  emit baseUiDataChanged();
}

void Board2dTab::applyTheme() noexcept {
  const Theme& theme = mApp.getWorkspace().getSettings().themes.getActive();
  mGridStyle = theme.getBoardGridStyle();

  if (mScene) {
    mScene->setBackgroundColors(
        theme.getColor(Theme::Color::sBoardBackground).getPrimaryColor(),
        theme.getColor(Theme::Color::sBoardBackground).getSecondaryColor());
    mScene->setOverlayColors(
        theme.getColor(Theme::Color::sBoardOverlays).getPrimaryColor(),
        theme.getColor(Theme::Color::sBoardOverlays).getSecondaryColor());
    mScene->setSelectionRectColors(
        theme.getColor(Theme::Color::sBoardSelection).getPrimaryColor(),
        theme.getColor(Theme::Color::sBoardSelection).getSecondaryColor());
    mScene->setGridStyle(mGridStyle);
  }

  emit tabUiDataChanged();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
