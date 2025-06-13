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

#include "../../dialogs/filedialog.h"
#include "../../editorcommandset.h"
#include "../../graphics/graphicslayerlist.h"
#include "../../graphics/graphicslayersmodel.h"
#include "../../graphics/slintgraphicsview.h"
#include "../../guiapplication.h"
#include "../../library/pkg/footprintgraphicsitem.h"
#include "../../undostack.h"
#include "../../utils/slinthelpers.h"
#include "../../utils/uihelpers.h"
#include "../../workspace/desktopservices.h"
#include "../cmd/cmdadddevicetoboard.h"
#include "../cmd/cmdboardspecctraimport.h"
#include "../projecteditor.h"
#include "boardeditor.h"
#include "boardgraphicsscene.h"
#include "boardpickplacegeneratordialog.h"
#include "fsm/boardeditorfsm.h"
#include "fsm/boardeditorstate_addhole.h"
#include "fsm/boardeditorstate_addstroketext.h"
#include "fsm/boardeditorstate_addvia.h"
#include "fsm/boardeditorstate_drawplane.h"
#include "fsm/boardeditorstate_drawpolygon.h"
#include "fsm/boardeditorstate_drawtrace.h"
#include "fsm/boardeditorstate_drawzone.h"
#include "graphicsitems/bgi_device.h"

#include <librepcb/core/application.h>
#include <librepcb/core/attribute/attributesubstitutor.h>
#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/fileio/transactionaldirectory.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/job/gerberexcellonoutputjob.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/boardd356netlistexport.h>
#include <librepcb/core/project/board/boardpainter.h>
#include <librepcb/core/project/board/boardspecctraexport.h>
#include <librepcb/core/project/board/items/bi_device.h>
#include <librepcb/core/project/board/items/bi_plane.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/circuit/componentinstance.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/projectattributelookup.h>
#include <librepcb/core/project/projectlibrary.h>
#include <librepcb/core/types/layer.h>
#include <librepcb/core/utils/messagelogger.h>
#include <librepcb/core/utils/scopeguard.h>
#include <librepcb/core/workspace/theme.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>
#include <librepcb/core/workspace/workspacesettings.h>

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

static ui::FeatureState toFs(bool enabled) noexcept {
  return enabled ? ui::FeatureState::Enabled : ui::FeatureState::Disabled;
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

Board2dTab::Board2dTab(GuiApplication& app, BoardEditor& editor,
                       QObject* parent) noexcept
  : WindowTab(app, parent),
    onDerivedUiDataChanged(*this),
    mProjectEditor(editor.getProjectEditor()),
    mProject(mProjectEditor.getProject()),
    mBoardEditor(editor),
    mBoard(mBoardEditor.getBoard()),
    mLayers(GraphicsLayerList::boardLayers(&app.getWorkspace().getSettings())),
    mView(new SlintGraphicsView(this)),
    mMsgEmptySchematics(app.getWorkspace(), "EMPTY_BOARD_NO_COMPONENTS"),
    mMsgPlaceDevices(app.getWorkspace(), "EMPTY_BOARD_PLACE_DEVICES"),
    mGridStyle(Theme::GridStyle::None),
    mIgnorePlacementLocks(false),
    mFrameIndex(0),
    mToolFeatures(),
    mTool(ui::EditorTool::Select),
    mToolCursorShape(Qt::ArrowCursor),
    mToolWireMode(BoardEditorState_DrawTrace::WireMode::HV),
    mToolNets(std::make_shared<slint::VectorModel<slint::SharedString>>()),
    mToolNet({true, std::nullopt}),
    mToolLayers(std::make_shared<slint::VectorModel<slint::SharedString>>()),
    mToolLayer(nullptr),
    mToolLineWidth(app.getWorkspace().getSettings()),
    mToolSize(app.getWorkspace().getSettings()),
    mToolDrill(app.getWorkspace().getSettings()),
    mToolFilled(false),
    mToolMirrored(false),
    mToolValueSuggestions(
        std::make_shared<slint::VectorModel<slint::SharedString>>()),
    mToolZoneRules(),
    mUnplacedComponentIndex(0),
    mUnplacedComponentDeviceIndex(0),
    mUnplacedComponentPackageOwned(false),
    mUnplacedComponentFootprintIndex(0) {
  Q_ASSERT(&mBoard.getProject() == &mProject);

  // Load/store layers visibility.
  updateEnabledCopperLayers();
  connect(&mBoard, &Board::innerLayerCountChanged, this,
          &Board2dTab::updateEnabledCopperLayers);
  loadLayersVisibility();
  connect(&mProjectEditor, &ProjectEditor::projectAboutToBeSaved, this,
          &Board2dTab::storeLayersVisibility);

  // Setup graphics view.
  mView->setEventHandler(this);
  connect(mView.get(), &SlintGraphicsView::transformChanged, this,
          &Board2dTab::requestRepaint);
  connect(mView.get(), &SlintGraphicsView::stateChanged, this,
          [this]() { onDerivedUiDataChanged.notify(); });

  // Connect board editor.
  connect(&mBoardEditor, &BoardEditor::uiIndexChanged, this,
          [this]() { onDerivedUiDataChanged.notify(); });
  connect(&mBoardEditor, &BoardEditor::planesRebuildStatusChanged, this,
          [this]() { onDerivedUiDataChanged.notify(); });
  connect(&mBoardEditor, &BoardEditor::planesUpdated, this,
          &Board2dTab::requestRepaint);
  connect(&mBoardEditor, &BoardEditor::drcMessageHighlightRequested, this,
          &Board2dTab::highlightDrcMessage);
  connect(&mBoardEditor, &BoardEditor::aboutToBeDestroyed, this,
          &Board2dTab::closeEnforced);

  // Connect project editor.
  connect(&mProjectEditor, &ProjectEditor::uiIndexChanged, this,
          [this]() { onDerivedUiDataChanged.notify(); });
  connect(&mProjectEditor, &ProjectEditor::abortBlockingToolsInOtherEditors,
          this, [this](const void* source) {
            if (source != this) {
              // Not so nice...
              mFsm->processAbortCommand();
              mFsm->processAbortCommand();
              mFsm->processAbortCommand();
            }
          });

  // Connect undo stack.
  connect(&mProjectEditor.getUndoStack(), &UndoStack::stateModified, this,
          [this]() { onUiDataChanged.notify(); });
  connect(&mProjectEditor, &ProjectEditor::manualModificationsMade, this,
          [this]() { onUiDataChanged.notify(); });

  // Connect search context.
  connect(&mSearchContext, &SearchContext::goToTriggered, this,
          &Board2dTab::goToDevice);

  // Setup messages.
  connect(&mProject.getCircuit(), &Circuit::componentAdded, this,
          &Board2dTab::updateMessages);
  connect(&mProject.getCircuit(), &Circuit::componentRemoved, this,
          &Board2dTab::updateMessages);
  connect(&mBoard, &Board::deviceAdded, this, &Board2dTab::updateMessages);
  connect(&mBoard, &Board::deviceRemoved, this, &Board2dTab::updateMessages);
  connect(&mMsgEmptySchematics, &DismissableMessageContext::visibilityChanged,
          this, [this]() { onDerivedUiDataChanged.notify(); });
  connect(&mMsgPlaceDevices, &DismissableMessageContext::visibilityChanged,
          this, [this]() { onDerivedUiDataChanged.notify(); });
  updateMessages();

  // Build the whole board editor finite state machine.
  BoardEditorFsm::Context fsmContext{
      mApp.getWorkspace(),           mProject, mBoard,
      mProjectEditor.getUndoStack(), *mLayers, *this,
  };
  mFsm.reset(new BoardEditorFsm(fsmContext));

  // Apply theme whenever it has been modified.
  connect(&mApp.getWorkspace().getSettings().themes,
          &WorkspaceSettingsItem_Themes::edited, this, &Board2dTab::applyTheme);
  applyTheme();
}

Board2dTab::~Board2dTab() noexcept {
  clearDrcMarker();  // Avoid dangling pointers.
  setSelectedUnplacedComponent(-1);  // Release memory if needed.
  deactivate();
  mView->setEventHandler(nullptr);

  // Delete FSM as it may trigger some other methods during destruction.
  mFsm.reset();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

int Board2dTab::getProjectIndex() const noexcept {
  return mProjectEditor.getUiIndex();
}

int Board2dTab::getProjectObjectIndex() const noexcept {
  return mProject.getBoardIndex(mBoard);
}

ui::TabData Board2dTab::getUiData() const noexcept {
  ui::TabFeatures features = {};
  features.export_graphics = toFs(mTool == ui::EditorTool::Select);
  features.select = toFs(mTool == ui::EditorTool::Select);
  features.cut = toFs(mToolFeatures.testFlag(Feature::Cut));
  features.copy = toFs(mToolFeatures.testFlag(Feature::Copy));
  features.paste = toFs(mToolFeatures.testFlag(Feature::Paste));
  features.remove = toFs(mToolFeatures.testFlag(Feature::Remove));
  features.rotate = toFs(mToolFeatures.testFlag(Feature::Rotate));
  features.flip = toFs(mToolFeatures.testFlag(Feature::Flip));
  features.snap_to_grid = toFs(mToolFeatures.testFlag(Feature::SnapToGrid));
  features.reset_texts = toFs(mToolFeatures.testFlag(Feature::ResetTexts));
  features.lock = toFs(mToolFeatures.testFlag(Feature::Lock));
  features.unlock = toFs(mToolFeatures.testFlag(Feature::Unlock));
  features.edit_properties = toFs(mToolFeatures.testFlag(Feature::Properties));
  features.modify_line_width =
      toFs(mToolFeatures.testFlag(Feature::ModifyLineWidth));
  features.find = toFs(true);

  return ui::TabData{
      ui::TabType::Board2d,  // Type
      q2s(*mBoard.getName()),  // Title
      features,  // Features
      q2s(mSearchContext.getTerm()),  // Find term
      mSearchContext.getSuggestions(),  // Find suggestions
      mLayersModel,  // Layers
  };
}

void Board2dTab::setUiData(const ui::TabData& data) noexcept {
  mSearchContext.setTerm(s2q(data.find_term));
  WindowTab::setUiData(data);
  onUiDataChanged.notify();
}

ui::Board2dTabData Board2dTab::getDerivedUiData() const noexcept {
  const Theme& theme = mApp.getWorkspace().getSettings().themes.getActive();
  const QColor bgColor =
      theme.getColor(Theme::Color::sBoardBackground).getPrimaryColor();
  const QColor fgColor = (bgColor.lightnessF() >= 0.5) ? Qt::black : Qt::white;

  return ui::Board2dTabData{
      mProjectEditor.getUiIndex(),  // Project index
      mBoardEditor.getUiIndex(),  // Board index
      q2s(bgColor),  // Background color
      q2s(fgColor),  // Foreground color
      q2s(theme.getColor(Theme::Color::sBoardInfoBox)
              .getPrimaryColor()),  // Overlay color
      q2s(theme.getColor(Theme::Color::sBoardInfoBox)
              .getSecondaryColor()),  // Overlay text color
      l2s(mGridStyle),  // Grid style
      l2s(*mBoard.getGridInterval()),  // Grid interval
      l2s(mBoard.getGridUnit()),  // Length unit
      mIgnorePlacementLocks,  // Ignore placement locks
      mBoardEditor.isRebuildingPlanes(),  // Refreshing
      mMsgEmptySchematics.getUiData(),  // Message "empty schematics"
      mMsgPlaceDevices.getUiData(),  // Message "place devices"
      mUnplacedComponentsModel,  // Unplaced components
      mUnplacedComponentIndex,  // Unplaced components index
      mUnplacedComponentDevicesModel,  // Unplaced components devices
      mUnplacedComponentDeviceIndex,  // Unplaced components device index
      mUnplacedComponentFootprintsModel,  // Unplaced components footprints
      mUnplacedComponentFootprintIndex,  // Unplaced components footprint index
      slint::Image(),  // Unplaced components preview
      mTool,  // Tool
      q2s(mView->isPanning() ? Qt::ClosedHandCursor
                             : mToolCursorShape),  // Tool cursor
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
          static_cast<int>(mToolLayersQt.indexOf(mToolLayer)),  // Current
      },
      mToolLineWidth.getUiData(),  // Tool line width
      mToolSize.getUiData(),  // Tool size
      mToolDrill.getUiData(),  // Tool drill
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

void Board2dTab::setDerivedUiData(const ui::Board2dTabData& data) noexcept {
  mSceneImagePos = s2q(data.scene_image_pos);

  mGridStyle = s2l(data.grid_style);
  const std::optional<PositiveLength> interval = s2plength(data.grid_interval);
  if (interval && (*interval != mBoard.getGridInterval())) {
    mBoard.setGridInterval(*interval);
    mProjectEditor.setManualModificationsMade();
  }
  if (mScene) {
    mScene->setGridStyle(mGridStyle);
    mScene->setGridInterval(mBoard.getGridInterval());
  }
  const LengthUnit unit = s2l(data.unit);
  if (unit != mBoard.getGridUnit()) {
    mBoard.setGridUnit(unit);
    mProjectEditor.setManualModificationsMade();
  }

  // Placement locks
  mIgnorePlacementLocks = data.ignore_placement_locks;

  // Messages
  mMsgEmptySchematics.setUiData(data.empty_schematics_msg);
  mMsgPlaceDevices.setUiData(data.place_devices_msg);

  // Unplaced component index
  if (data.unplaced_components_index != mUnplacedComponentIndex) {
    setSelectedUnplacedComponent(data.unplaced_components_index);
  } else if (data.unplaced_components_devices_index !=
             mUnplacedComponentDeviceIndex) {
    setSelectedUnplacedComponentDevice(data.unplaced_components_devices_index);
  } else if (data.unplaced_components_footprints_index !=
             mUnplacedComponentFootprintIndex) {
    setSelectedUnplacedComponentFootprint(
        data.unplaced_components_footprints_index);
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
  mToolLineWidth.setUiData(data.tool_line_width);

  // Tool size
  mToolSize.setUiData(data.tool_size);

  // Tool drill
  mToolDrill.setUiData(data.tool_drill);

  // Tool filled / auto-width
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
  mLayersModel.reset(new GraphicsLayersModel(*mLayers));
  connect(mLayersModel.get(), &GraphicsLayersModel::layersVisibilityChanged,
          &mBoardEditor, &BoardEditor::schedulePlanesRebuild);

  mScene.reset(new BoardGraphicsScene(
      mBoard, *mLayers, mProjectEditor.getHighlightedNetSignals(), this));
  mScene->setGridInterval(mBoard.getGridInterval());
  connect(&mProjectEditor, &ProjectEditor::highlightedNetSignalsChanged,
          mScene.get(), &BoardGraphicsScene::updateHighlightedNetSignals);
  connect(mScene.get(), &GraphicsScene::changed, this,
          &Board2dTab::requestRepaint);

  // Force airwire rebuild immediately and on every project modification.
  mBoard.triggerAirWiresRebuild();
  mActiveConnections.append(connect(&mProjectEditor.getUndoStack(),
                                    &UndoStack::stateModified, &mBoard,
                                    &Board::triggerAirWiresRebuild));

  // Unplaced component state.
  mUnplacedComponentsModel.reset(new slint::VectorModel<slint::SharedString>());
  mUnplacedComponentDevicesModel.reset(
      new slint::VectorModel<slint::SharedString>());
  mUnplacedComponentFootprintsModel.reset(
      new slint::VectorModel<slint::SharedString>());
  mUnplacedComponentLayers =
      GraphicsLayerList::previewLayers(&mApp.getWorkspace().getSettings());
  mUnplacedComponentGraphicsScene.reset(new GraphicsScene());
  mUnplacedComponentGraphicsScene->setOriginCrossVisible(false);
  mUnplacedComponentGraphicsItem.reset();

  // Update unplaced components when needed.
  mUnplacedComponentsUpdateTimer.reset(new QTimer(this));
  mUnplacedComponentsUpdateTimer->setSingleShot(true);
  connect(mUnplacedComponentsUpdateTimer.get(), &QTimer::timeout, this,
          &Board2dTab::updateUnplacedComponents);
  connect(&mProject.getCircuit(), &Circuit::componentAdded, this,
          &Board2dTab::scheduleUnplacedComponentsUpdate);
  connect(&mProject.getCircuit(), &Circuit::componentRemoved, this,
          &Board2dTab::scheduleUnplacedComponentsUpdate);
  mActiveConnections.append(
      connect(&mBoard, &Board::deviceAdded, this,
              &Board2dTab::scheduleUnplacedComponentsUpdate));
  mActiveConnections.append(
      connect(&mBoard, &Board::deviceRemoved, this,
              &Board2dTab::scheduleUnplacedComponentsUpdate));
  scheduleUnplacedComponentsUpdate();

  // Initialize search context.
  mSearchContext.init();

  // Setup input idle timer for planes rebuilding during commands.
  mInputIdleTimer.reset(new QTimer());
  mInputIdleTimer->setInterval(700);
  mInputIdleTimer->setSingleShot(true);
  connect(mInputIdleTimer.get(), &QTimer::timeout, &mBoardEditor,
          &BoardEditor::schedulePlanesRebuild);

  applyTheme();
  mBoardEditor.registerActiveTab(this);
  requestRepaint();
}

void Board2dTab::deactivate() noexcept {
  mInputIdleTimer.reset();
  mBoardEditor.unregisterActiveTab(this);
  while (!mActiveConnections.isEmpty()) {
    disconnect(mActiveConnections.takeLast());
  }
  mSearchContext.deinit();
  mUnplacedComponentGraphicsItem.reset();
  mUnplacedComponentGraphicsScene.reset();
  mUnplacedComponentFootprintsModel.reset();
  mUnplacedComponentDevicesModel.reset();
  mUnplacedComponentsModel.reset();
  mUnplacedComponentLayers.reset();
  mDrcLocationGraphicsItem.reset();
  mScene.reset();
  mLayersModel.reset();
}

void Board2dTab::trigger(ui::TabAction a) noexcept {
  restartIdleTimer();

  switch (a) {
    case ui::TabAction::Print: {
      execGraphicsExportDialog(GraphicsExportDialog::Output::Print, "print");
      break;
    }
    case ui::TabAction::ExportImage: {
      execGraphicsExportDialog(GraphicsExportDialog::Output::Image,
                               "image_export");
      break;
    }
    case ui::TabAction::ExportPdf: {
      execGraphicsExportDialog(GraphicsExportDialog::Output::Pdf, "pdf_export");
      break;
    }
    case ui::TabAction::ExportBom: {
      mProjectEditor.execBomGeneratorDialog(&mBoard);
      break;
    }
    case ui::TabAction::ExportFabricationData: {
      mProjectEditor.execOutputJobsDialog(
          GerberExcellonOutputJob::getTypeName());
      break;
    }
    case ui::TabAction::ExportPickPlace: {
      execPickPlaceExportDialog();
      break;
    }
    case ui::TabAction::ExportD356Netlist: {
      execD356NetlistExportDialog();
      break;
    }
    case ui::TabAction::ExportSpecctra: {
      execSpecctraExportDialog();
      break;
    }
    case ui::TabAction::ImportSpecctra: {
      execSpecctraImportDialog();
      break;
    }
    case ui::TabAction::ImportDxf: {
      mFsm->processImportDxf();
      break;
    }
    case ui::TabAction::PlanesHide: {
      foreach (BI_Plane* p, mBoard.getPlanes()) {
        p->setVisible(false);  // No undo command needed since it is not saved.
      }
      break;
    }
    case ui::TabAction::PlanesShow: {
      foreach (BI_Plane* p, mBoard.getPlanes()) {
        p->setVisible(true);  // No undo command needed since it is not saved.
      }
      break;
    }
    case ui::TabAction::PlanesRebuild: {
      mBoardEditor.startPlanesRebuild(true);
      break;
    }
    case ui::TabAction::SelectAll: {
      mFsm->processSelectAll();
      break;
    }
    case ui::TabAction::Abort: {
      if (mDrcLocationGraphicsItem) {
        clearDrcMarker();
      } else {
        mFsm->processAbortCommand();
      }
      break;
    }
    case ui::TabAction::Cut: {
      mFsm->processCut();
      break;
    }
    case ui::TabAction::Copy: {
      mFsm->processCopy();
      break;
    }
    case ui::TabAction::Paste: {
      mFsm->processPaste();
      break;
    }
    case ui::TabAction::Delete: {
      mFsm->processRemove();
      break;
    }
    case ui::TabAction::RotateCcw: {
      mFsm->processRotate(Angle::deg90());
      break;
    }
    case ui::TabAction::RotateCw: {
      mFsm->processRotate(-Angle::deg90());
      break;
    }
    case ui::TabAction::FlipHorizontally: {
      mFsm->processFlip(Qt::Horizontal);
      break;
    }
    case ui::TabAction::FlipVertically: {
      mFsm->processFlip(Qt::Vertical);
      break;
    }
    case ui::TabAction::MoveLeft: {
      if (!mFsm->processMove(Point(-mBoard.getGridInterval(), 0))) {
        mView->scrollLeft();
      }
      break;
    }
    case ui::TabAction::MoveRight: {
      if (!mFsm->processMove(Point(*mBoard.getGridInterval(), 0))) {
        mView->scrollRight();
      }
      break;
    }
    case ui::TabAction::MoveUp: {
      if (!mFsm->processMove(Point(0, *mBoard.getGridInterval()))) {
        mView->scrollUp();
      }
      break;
    }
    case ui::TabAction::MoveDown: {
      if (!mFsm->processMove(Point(0, -mBoard.getGridInterval()))) {
        mView->scrollDown();
      }
      break;
    }
    case ui::TabAction::SnapToGrid: {
      mFsm->processSnapToGrid();
      break;
    }
    case ui::TabAction::Lock: {
      mFsm->processSetLocked(true);
      break;
    }
    case ui::TabAction::Unlock: {
      mFsm->processSetLocked(false);
      break;
    }
    case ui::TabAction::LineWidthIncrease: {
      mFsm->processChangeLineWidth(1);
      break;
    }
    case ui::TabAction::LineWidthDecrease: {
      mFsm->processChangeLineWidth(-1);
      break;
    }
    case ui::TabAction::LineWidthSet: {
      mFsm->processChangeLineWidth(0);
      break;
    }
    case ui::TabAction::ResetTexts: {
      mFsm->processResetAllTexts();
      break;
    }
    case ui::TabAction::EditProperties: {
      mFsm->processEditProperties();
      break;
    }
    case ui::TabAction::GridIntervalIncrease: {
      mBoard.setGridInterval(PositiveLength(mBoard.getGridInterval() * 2));
      if (mScene) {
        mScene->setGridInterval(mBoard.getGridInterval());
        requestRepaint();
      }
      break;
    }
    case ui::TabAction::GridIntervalDecrease: {
      if ((*mBoard.getGridInterval() % 2) == 0) {
        mBoard.setGridInterval(PositiveLength(mBoard.getGridInterval() / 2));
        if (mScene) {
          mScene->setGridInterval(mBoard.getGridInterval());
          requestRepaint();
        }
      }
      break;
    }
    case ui::TabAction::LayersTop: {
      mLayers->showTop();
      break;
    }
    case ui::TabAction::LayersBottom: {
      mLayers->showBottom();
      break;
    }
    case ui::TabAction::LayersTopBottom: {
      mLayers->showTopAndBottom();
      break;
    }
    case ui::TabAction::LayersAll: {
      mLayers->showAll();
      break;
    }
    case ui::TabAction::LayersNone: {
      mLayers->showNone();
      break;
    }
    case ui::TabAction::ZoomIn: {
      mView->zoomIn();
      break;
    }
    case ui::TabAction::ZoomOut: {
      mView->zoomOut();
      break;
    }
    case ui::TabAction::ZoomFit: {
      if (mScene) mView->zoomToSceneRect(mScene->itemsBoundingRect());
      break;
    }
    case ui::TabAction::FindRefreshSuggestions: {
      QStringList names;
      for (const BI_Device* dev : mBoard.getDeviceInstances()) {
        names.append(*dev->getComponentInstance().getName());
      }
      Toolbox::sortNumeric(names);
      mSearchContext.setSuggestions(names);
      break;
    }
    case ui::TabAction::FindNext: {
      mSearchContext.findNext();
      break;
    }
    case ui::TabAction::FindPrevious: {
      mSearchContext.findPrevious();
      break;
    }
    case ui::TabAction::BoardPlaceComponent: {
      addUnplacedComponentsToBoard(PlaceComponentsMode::Single);
      break;
    }
    case ui::TabAction::BoardPlaceComponentsSimilar: {
      addUnplacedComponentsToBoard(PlaceComponentsMode::Similar);
      break;
    }
    case ui::TabAction::BoardPlaceComponentsAll: {
      addUnplacedComponentsToBoard(PlaceComponentsMode::All);
      break;
    }
    case ui::TabAction::ToolSelect: {
      mFsm->processSelect();
      break;
    }
    case ui::TabAction::ToolWire: {
      mFsm->processDrawTrace();
      break;
    }
    case ui::TabAction::ToolVia: {
      mFsm->processAddVia();
      break;
    }
    case ui::TabAction::ToolPolygon: {
      mFsm->processDrawPolygon();
      break;
    }
    case ui::TabAction::ToolText: {
      mFsm->processAddStrokeText();
      break;
    }
    case ui::TabAction::ToolPlane: {
      mFsm->processDrawPlane();
      break;
    }
    case ui::TabAction::ToolZone: {
      mFsm->processDrawZone();
      break;
    }
    case ui::TabAction::ToolHole: {
      mFsm->processAddHole();
      break;
    }
    case ui::TabAction::ToolMeasure: {
      mFsm->processMeasure();
      break;
    }
    default: {
      WindowTab::trigger(a);
      break;
    }
  }
}

slint::Image Board2dTab::renderScene(float width, float height,
                                     int scene) noexcept {
  if (scene == 1) {
    if (mUnplacedComponentGraphicsScene) {
      SlintGraphicsView view;
      return view.render(*mUnplacedComponentGraphicsScene, width, height);
    } else {
      QPixmap pix(width, height);
      pix.fill(mApp.getWorkspace()
                   .getSettings()
                   .themes.getActive()
                   .getColor(Theme::Color::sBoardBackground)
                   .getPrimaryColor());
      return q2s(pix);
    }
  } else if (mScene) {
    return mView->render(*mScene, width, height);
  } else {
    return slint::Image();
  }
}

bool Board2dTab::processScenePointerEvent(
    const QPointF& pos, slint::private_api::PointerEvent e) noexcept {
  if (mView->pointerEvent(pos, e)) {
    restartIdleTimer();
    return true;
  }
  return false;
}

bool Board2dTab::processSceneScrolled(
    const QPointF& pos, slint::private_api::PointerScrollEvent e) noexcept {
  return mView->scrollEvent(pos, e);
}

bool Board2dTab::processSceneKeyEvent(
    const slint::private_api::KeyEvent& e) noexcept {
  if (mView->keyEvent(e)) {
    restartIdleTimer();
    return true;
  }
  return false;
}

QSet<const Layer*> Board2dTab::getVisibleCopperLayers() const noexcept {
  QSet<const Layer*> layers;
  foreach (const Layer* layer, mBoard.getCopperLayers()) {
    if (auto graphicsLayer = mLayers->get(*layer)) {
      if (graphicsLayer->isVisible()) {
        layers.insert(layer);
      }
    }
  }
  return layers;
}

/*******************************************************************************
 *  IF_GraphicsViewEventHandler Methods
 ******************************************************************************/

bool Board2dTab::graphicsSceneKeyPressed(
    const GraphicsSceneKeyEvent& e) noexcept {
  return mFsm->processKeyPressed(e);
}

bool Board2dTab::graphicsSceneKeyReleased(
    const GraphicsSceneKeyEvent& e) noexcept {
  return mFsm->processKeyReleased(e);
}

bool Board2dTab::graphicsSceneMouseMoved(
    const GraphicsSceneMouseEvent& e) noexcept {
  emit cursorCoordinatesChanged(e.scenePos, mBoard.getGridUnit());
  return mFsm->processGraphicsSceneMouseMoved(e);
}

bool Board2dTab::graphicsSceneLeftMouseButtonPressed(
    const GraphicsSceneMouseEvent& e) noexcept {
  return mFsm->processGraphicsSceneLeftMouseButtonPressed(e);
}

bool Board2dTab::graphicsSceneLeftMouseButtonReleased(
    const GraphicsSceneMouseEvent& e) noexcept {
  return mFsm->processGraphicsSceneLeftMouseButtonReleased(e);
}

bool Board2dTab::graphicsSceneLeftMouseButtonDoubleClicked(
    const GraphicsSceneMouseEvent& e) noexcept {
  return mFsm->processGraphicsSceneLeftMouseButtonDoubleClicked(e);
}

bool Board2dTab::graphicsSceneRightMouseButtonReleased(
    const GraphicsSceneMouseEvent& e) noexcept {
  return mFsm->processGraphicsSceneRightMouseButtonReleased(e);
}

/*******************************************************************************
 *  BoardEditorFsmAdapter Methods
 ******************************************************************************/

BoardGraphicsScene* Board2dTab::fsmGetGraphicsScene() noexcept {
  return mScene.get();
}

bool Board2dTab::fsmGetIgnoreLocks() const noexcept {
  return mIgnorePlacementLocks;
}

void Board2dTab::fsmSetViewCursor(
    const std::optional<Qt::CursorShape>& shape) noexcept {
  if (shape) {
    mToolCursorShape = *shape;
  } else {
    mToolCursorShape = Qt::ArrowCursor;
  }
  onDerivedUiDataChanged.notify();
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
    onDerivedUiDataChanged.notify();
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
  return mView->calcPosWithTolerance(pos, multiplier);
}

Point Board2dTab::fsmMapGlobalPosToScenePos(const QPoint& pos) const noexcept {
  if (QWidget* win = qApp->activeWindow()) {
    return mView->mapToScenePos(win->mapFromGlobal(pos) - mSceneImagePos);
  } else {
    qWarning() << "Failed to map global position to scene position.";
    return Point();
  }
}

void Board2dTab::fsmSetHighlightedNetSignals(
    const QSet<const NetSignal*>& sigs) noexcept {
  mProjectEditor.setHighlightedNetSignals(sigs);
}

void Board2dTab::fsmAbortBlockingToolsInOtherEditors() noexcept {
  mProjectEditor.abortBlockingToolsInOtherEditors(this);
}

void Board2dTab::fsmSetStatusBarMessage(const QString& message,
                                        int timeoutMs) noexcept {
  emit statusBarMessageChanged(message, timeoutMs);
}

void Board2dTab::fsmSetFeatures(Features features) noexcept {
  if (features != mToolFeatures) {
    mToolFeatures = features;
    onUiDataChanged.notify();
  }
}

void Board2dTab::fsmToolLeave() noexcept {
  while (!mFsmStateConnections.isEmpty()) {
    disconnect(mFsmStateConnections.takeLast());
  }
  mTool = ui::EditorTool::Select;
  fsmSetFeatures(Features());
  onDerivedUiDataChanged.notify();
}

void Board2dTab::fsmToolEnter(BoardEditorState_Select& state) noexcept {
  Q_UNUSED(state);

  mTool = ui::EditorTool::Select;
  onDerivedUiDataChanged.notify();
}

void Board2dTab::fsmToolEnter(BoardEditorState_DrawTrace& state) noexcept {
  mTool = ui::EditorTool::Wire;

  // Wire mode
  auto setWireMode = [this](BoardEditorState_DrawTrace::WireMode m) {
    mToolWireMode = m;
    onDerivedUiDataChanged.notify();
  };
  setWireMode(state.getWireMode());
  mFsmStateConnections.append(connect(
      &state, &BoardEditorState_DrawTrace::wireModeChanged, this, setWireMode));
  mFsmStateConnections.append(
      connect(this, &Board2dTab::wireModeRequested, &state,
              &BoardEditorState_DrawTrace::setWireMode));

  // Trace width
  mToolLineWidth.configure(state.getWidth(),
                           LengthEditContext::Steps::generic(),
                           "board_editor/draw_trace/width");
  mFsmStateConnections.append(
      connect(&state, &BoardEditorState_DrawTrace::widthChanged,
              &mToolLineWidth, &LengthEditContext::setValuePositive));
  mFsmStateConnections.append(
      connect(&mToolLineWidth, &LengthEditContext::valueChangedPositive, &state,
              &BoardEditorState_DrawTrace::setWidth));

  // Auto width
  auto setAutoWidth = [this](bool autoWidth) {
    mToolFilled = autoWidth;
    onDerivedUiDataChanged.notify();
  };
  setAutoWidth(state.getAutoWidth());
  mFsmStateConnections.append(
      connect(&state, &BoardEditorState_DrawTrace::autoWidthChanged, this,
              setAutoWidth));
  mFsmStateConnections.append(
      connect(this, &Board2dTab::filledRequested, &state,
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
    onDerivedUiDataChanged.notify();
  };
  setLayer(state.getLayer());
  mFsmStateConnections.append(connect(
      &state, &BoardEditorState_DrawTrace::layerChanged, this, setLayer));
  mFsmStateConnections.append(connect(this, &Board2dTab::layerRequested, &state,
                                      &BoardEditorState_DrawTrace::setLayer));

  // Via size
  mToolSize.configure(state.getViaSize(), LengthEditContext::Steps::generic(),
                      "board_editor/add_via/size");  // From via tool.
  mFsmStateConnections.append(
      connect(&state, &BoardEditorState_DrawTrace::viaSizeChanged, &mToolSize,
              &LengthEditContext::setValuePositive));
  mFsmStateConnections.append(
      connect(&mToolSize, &LengthEditContext::valueChangedPositive, &state,
              &BoardEditorState_DrawTrace::setViaSize));

  // Via drill
  mToolDrill.configure(state.getViaDrillDiameter(),
                       LengthEditContext::Steps::drillDiameter(),
                       "board_editor/add_via/drill");  // From via tool.
  mFsmStateConnections.append(
      connect(&state, &BoardEditorState_DrawTrace::viaDrillDiameterChanged,
              &mToolDrill, &LengthEditContext::setValuePositive));
  mFsmStateConnections.append(
      connect(&mToolDrill, &LengthEditContext::valueChangedPositive, &state,
              &BoardEditorState_DrawTrace::setViaDrillDiameter));

  onDerivedUiDataChanged.notify();
}

void Board2dTab::fsmToolEnter(BoardEditorState_AddVia& state) noexcept {
  mTool = ui::EditorTool::Via;

  // Via size
  mToolSize.configure(state.getSize(), LengthEditContext::Steps::generic(),
                      "board_editor/add_via/size");
  mFsmStateConnections.append(
      connect(&state, &BoardEditorState_AddVia::sizeChanged, &mToolSize,
              &LengthEditContext::setValuePositive));
  mFsmStateConnections.append(
      connect(&mToolSize, &LengthEditContext::valueChangedPositive, &state,
              &BoardEditorState_AddVia::setSize));

  // Via drill
  mToolDrill.configure(state.getDrillDiameter(),
                       LengthEditContext::Steps::drillDiameter(),
                       "board_editor/add_via/drill");
  mFsmStateConnections.append(
      connect(&state, &BoardEditorState_AddVia::drillDiameterChanged,
              &mToolDrill, &LengthEditContext::setValuePositive));
  mFsmStateConnections.append(
      connect(&mToolDrill, &LengthEditContext::valueChangedPositive, &state,
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
    onDerivedUiDataChanged.notify();
  };
  setNet(state.getUseAutoNet(), state.getNet());
  mFsmStateConnections.append(
      connect(&state, &BoardEditorState_AddVia::netChanged, this, setNet));
  mFsmStateConnections.append(connect(this, &Board2dTab::netRequested, &state,
                                      &BoardEditorState_AddVia::setNet));

  onDerivedUiDataChanged.notify();
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
    onDerivedUiDataChanged.notify();
  };
  setLayer(state.getLayer());
  mFsmStateConnections.append(connect(
      &state, &BoardEditorState_DrawPolygon::layerChanged, this, setLayer));
  mFsmStateConnections.append(connect(this, &Board2dTab::layerRequested, &state,
                                      &BoardEditorState_DrawPolygon::setLayer));

  // Line width
  mToolLineWidth.configure(state.getLineWidth(),
                           LengthEditContext::Steps::generic(),
                           "board_editor/draw_polygon/line_width");
  mFsmStateConnections.append(
      connect(&state, &BoardEditorState_DrawPolygon::lineWidthChanged,
              &mToolLineWidth, &LengthEditContext::setValueUnsigned));
  mFsmStateConnections.append(
      connect(&mToolLineWidth, &LengthEditContext::valueChangedUnsigned, &state,
              &BoardEditorState_DrawPolygon::setLineWidth));

  // Filled
  auto setFilled = [this](bool filled) {
    mToolFilled = filled;
    onDerivedUiDataChanged.notify();
  };
  setFilled(state.getFilled());
  mFsmStateConnections.append(connect(
      &state, &BoardEditorState_DrawPolygon::filledChanged, this, setFilled));
  mFsmStateConnections.append(
      connect(this, &Board2dTab::filledRequested, &state,
              &BoardEditorState_DrawPolygon::setFilled));

  onDerivedUiDataChanged.notify();
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
    onDerivedUiDataChanged.notify();
  };
  setLayer(state.getLayer());
  mFsmStateConnections.append(connect(
      &state, &BoardEditorState_AddStrokeText::layerChanged, this, setLayer));
  mFsmStateConnections.append(
      connect(this, &Board2dTab::layerRequested, &state,
              &BoardEditorState_AddStrokeText::setLayer));

  // Height
  mToolSize.configure(state.getHeight(), LengthEditContext::Steps::textHeight(),
                      "board_editor/add_text/size");
  mFsmStateConnections.append(
      connect(&state, &BoardEditorState_AddStrokeText::heightChanged,
              &mToolSize, &LengthEditContext::setValuePositive));
  mFsmStateConnections.append(
      connect(&mToolSize, &LengthEditContext::valueChangedPositive, &state,
              &BoardEditorState_AddStrokeText::setHeight));

  // Text
  auto setText = [this](const QString& text) {
    mToolValue = text;
    onDerivedUiDataChanged.notify();
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
    onDerivedUiDataChanged.notify();
  };
  setMirrored(state.getMirrored());
  mFsmStateConnections.append(
      connect(&state, &BoardEditorState_AddStrokeText::mirroredChanged, this,
              setMirrored));
  mFsmStateConnections.append(
      connect(this, &Board2dTab::mirroredRequested, &state,
              &BoardEditorState_AddStrokeText::setMirrored));

  onDerivedUiDataChanged.notify();
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
    onDerivedUiDataChanged.notify();
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
    onDerivedUiDataChanged.notify();
  };
  setLayer(state.getLayer());
  mFsmStateConnections.append(connect(
      &state, &BoardEditorState_DrawPlane::layerChanged, this, setLayer));
  mFsmStateConnections.append(connect(this, &Board2dTab::layerRequested, &state,
                                      &BoardEditorState_DrawPlane::setLayer));

  onDerivedUiDataChanged.notify();
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
    onDerivedUiDataChanged.notify();
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
    onDerivedUiDataChanged.notify();
  };
  setRules(state.getRules());
  mFsmStateConnections.append(connect(
      &state, &BoardEditorState_DrawZone::rulesChanged, this, setRules));
  mFsmStateConnections.append(connect(this, &Board2dTab::zoneRuleRequested,
                                      &state,
                                      &BoardEditorState_DrawZone::setRule));

  onDerivedUiDataChanged.notify();
}

void Board2dTab::fsmToolEnter(BoardEditorState_AddHole& state) noexcept {
  mTool = ui::EditorTool::Hole;

  // Drill
  mToolDrill.configure(state.getDiameter(),
                       LengthEditContext::Steps::drillDiameter(),
                       "board_editor/add_hole/diameter");
  mFsmStateConnections.append(
      connect(&state, &BoardEditorState_AddHole::diameterChanged, &mToolDrill,
              &LengthEditContext::setValuePositive));
  mFsmStateConnections.append(
      connect(&mToolDrill, &LengthEditContext::valueChangedPositive, &state,
              &BoardEditorState_AddHole::setDiameter));

  onDerivedUiDataChanged.notify();
}

void Board2dTab::fsmToolEnter(BoardEditorState_AddDevice& state) noexcept {
  Q_UNUSED(state);

  mTool = ui::EditorTool::Component;
  onDerivedUiDataChanged.notify();
}

void Board2dTab::fsmToolEnter(BoardEditorState_Measure& state) noexcept {
  Q_UNUSED(state);

  mTool = ui::EditorTool::Measure;
  onDerivedUiDataChanged.notify();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void Board2dTab::updateEnabledCopperLayers() noexcept {
  foreach (const Layer* layer, Layer::innerCopper()) {
    if (std::shared_ptr<GraphicsLayer> gLayer = mLayers->get(*layer)) {
      gLayer->setEnabled(mBoard.getCopperLayers().contains(layer));
    }
  }
}

void Board2dTab::loadLayersVisibility() noexcept {
  foreach (std::shared_ptr<GraphicsLayer> layer, mLayers->all()) {
    if (mBoard.getLayersVisibility().contains(layer->getName())) {
      layer->setVisible(mBoard.getLayersVisibility().value(layer->getName()));
    }
  }
}

void Board2dTab::storeLayersVisibility() noexcept {
  QMap<QString, bool> visibility;
  foreach (std::shared_ptr<GraphicsLayer> layer, mLayers->all()) {
    if (layer->isEnabled()) {
      visibility[layer->getName()] = layer->isVisible();
    }
  }
  mBoard.setLayersVisibility(visibility);
}

void Board2dTab::updateMessages() noexcept {
  bool emptySchematics = true;
  for (auto cmp : mProject.getCircuit().getComponentInstances()) {
    if (!cmp->getLibComponent().isSchematicOnly()) {
      emptySchematics = false;
      break;
    }
  }
  mMsgEmptySchematics.setActive(emptySchematics);
  mMsgPlaceDevices.setActive((!emptySchematics) &&
                             mBoard.getDeviceInstances().isEmpty());
}

void Board2dTab::highlightDrcMessage(
    const std::shared_ptr<const RuleCheckMessage>& msg, bool zoomTo) noexcept {
  if ((!msg) || msg->getLocations().isEmpty()) {
    // Position on board not known.
    clearDrcMarker();
  } else if (mScene) {
    const ThemeColor& color =
        mApp.getWorkspace().getSettings().themes.getActive().getColor(
            Theme::Color::sBoardOverlays);
    QPainterPath path = Path::toQPainterPathPx(msg->getLocations(), true);
    mDrcLocationGraphicsItem.reset(new QGraphicsPathItem());
    mDrcLocationGraphicsItem->setZValue(BoardGraphicsScene::ZValue_AirWires);
    mDrcLocationGraphicsItem->setPen(QPen(color.getPrimaryColor(), 0));
    mDrcLocationGraphicsItem->setBrush(color.getSecondaryColor());
    mDrcLocationGraphicsItem->setPath(path);
    mScene->addItem(*mDrcLocationGraphicsItem.get());

    qreal margin = Length(1000000).toPx();
    QRectF rect = path.boundingRect();
    rect.adjust(-margin, -margin, margin, margin);
    mScene->setSceneRectMarker(rect);
    if (zoomTo) {
      mView->zoomToSceneRect(rect);
    }
  }
}

void Board2dTab::clearDrcMarker() noexcept {
  mDrcLocationGraphicsItem.reset();
  if (mScene) {
    mScene->setSceneRectMarker(QRectF());
  }
}

void Board2dTab::scheduleUnplacedComponentsUpdate() noexcept {
  if (mUnplacedComponentsUpdateTimer) {
    mUnplacedComponentsUpdateTimer->start(100);
  }
}

void Board2dTab::updateUnplacedComponents() noexcept {
  if (!mUnplacedComponentsModel) return;

  mUnplacedComponents.clear();
  mUnplacedComponentsModel->clear();

  QList<ComponentInstance*> components =
      mProject.getCircuit().getComponentInstances().values();
  const QMap<Uuid, BI_Device*> boardDeviceList = mBoard.getDeviceInstances();

  // Sort components manually using numeric sort.
  Toolbox::sortNumeric(
      components,
      [](const QCollator& cmp, const ComponentInstance* lhs,
         const ComponentInstance* rhs) {
        return cmp(*lhs->getName(), *rhs->getName());
      },
      Qt::CaseInsensitive, false);

  foreach (const ComponentInstance* cmp, components) {
    if (boardDeviceList.contains(cmp->getUuid())) continue;
    if (cmp->getLibComponent().isSchematicOnly()) continue;

    // Add component to list.
    ProjectAttributeLookup lookup(*cmp, nullptr,
                                  cmp->getParts(std::nullopt).value(0));
    const QString value =
        AttributeSubstitutor::substitute(lookup("VALUE"), lookup)
            .split("\n", Qt::SkipEmptyParts)
            .join("|");
    const QString libCmpName =
        *cmp->getLibComponent().getNames().value(mProject.getLocaleOrder());
    const QString text =
        QString("%1: %2 %3").arg(*cmp->getName(), value, libCmpName);
    mUnplacedComponents.append(cmp->getUuid());
    mUnplacedComponentsModel->push_back(q2s(text));
  }

  if (mUnplacedComponents.count() > 0) {
    setSelectedUnplacedComponent(
        qBound(0, mUnplacedComponentIndex, mUnplacedComponents.count() - 1));
  } else {
    setSelectedUnplacedComponent(-1);
  }
}

void Board2dTab::restartIdleTimer() noexcept {
  if (mInputIdleTimer) {
    mInputIdleTimer->start();
  }
}

void Board2dTab::setSelectedUnplacedComponent(int index) noexcept {
  mUnplacedComponentIndex = index;
  if ((index >= 0) && (index < mUnplacedComponents.count())) {
    mUnplacedComponent = mProject.getCircuit().getComponentInstanceByUuid(
        mUnplacedComponents.at(index));
  } else {
    mUnplacedComponent = nullptr;
  }
  mUnplacedComponentDevices.clear();
  if (mUnplacedComponentDevicesModel) {
    mUnplacedComponentDevicesModel->clear();
  }

  if (mUnplacedComponent && mUnplacedComponentDevicesModel) {
    std::pair<QList<DeviceMetadata>, int> devices =
        getAvailableDevices(*mUnplacedComponent);
    mUnplacedComponentDevices = devices.first;
    for (int i = 0; i < mUnplacedComponentDevices.count(); ++i) {
      const DeviceMetadata& device = mUnplacedComponentDevices.at(i);
      QString text = device.deviceName;
      if (!text.contains(device.packageName, Qt::CaseInsensitive)) {
        // Package name not contained in device name, so let's show it as well.
        text += " [" % device.packageName % "]";
      }
      if (device.isListedInComponentInstance) {
        text += " ✔";
      }
      mUnplacedComponentDevicesModel->push_back(q2s(text));
    }
    setSelectedUnplacedComponentDevice(devices.second);
  } else {
    setSelectedUnplacedComponentDevice(-1);
  }
}

void Board2dTab::setSelectedUnplacedComponentDevice(int index) noexcept {
  mUnplacedComponentDeviceIndex = index;

  // Abort if index is out of bounds.
  if ((index < 0) || (index >= mUnplacedComponentDevices.count())) {
    setSelectedUnplacedComponentDeviceAndPackage(std::nullopt, nullptr, false);
    return;
  }

  try {
    const DeviceMetadata& device = mUnplacedComponentDevices.at(index);
    bool packageOwned = false;
    // Prefer package in project library for several reasons:
    //  - Allow adding devices even if package not found in workspace library
    //  - Use correct package (version) for preview
    //  - Better performance than loading workspace library elements
    Package* pkg = mProject.getLibrary().getPackage(device.packageUuid);
    if (!pkg) {
      // If package does not exist in project library, use workspace library.
      const FilePath pkgFp =
          mApp.getWorkspace().getLibraryDb().getLatest<Package>(
              device.packageUuid);
      if (pkgFp.isValid()) {
        pkg = Package::open(std::unique_ptr<TransactionalDirectory>(
                                new TransactionalDirectory(
                                    TransactionalFileSystem::openRO(pkgFp))))
                  .release();
        packageOwned = true;
      }
    }
    setSelectedUnplacedComponentDeviceAndPackage(device.deviceUuid, pkg,
                                                 packageOwned);
  } catch (const Exception& e) {
    qCritical() << "Failed to load device & package preview:" << e.getMsg();
    setSelectedUnplacedComponentDeviceAndPackage(std::nullopt, nullptr, false);
  }
}

void Board2dTab::setSelectedUnplacedComponentDeviceAndPackage(
    const std::optional<Uuid>& deviceUuid, Package* package,
    bool packageOwned) noexcept {
  if (mUnplacedComponentFootprintsModel) {
    mUnplacedComponentFootprintsModel->clear();
  }
  if (mUnplacedComponentPackageOwned) {
    delete mUnplacedComponentPackage;
  }
  mUnplacedComponentPackage = nullptr;
  mUnplacedComponentPackageOwned = false;

  int fptIndex = 0;

  if (deviceUuid && package && mUnplacedComponentFootprintsModel) {
    mUnplacedComponentPackage = package;
    mUnplacedComponentPackageOwned = packageOwned;
    for (const Footprint& fpt : mUnplacedComponentPackage->getFootprints()) {
      mUnplacedComponentFootprintsModel->push_back(
          q2s(*fpt.getNames().value(mProject.getLocaleOrder())));
    }
    // Select most relevant footprint.
    if (auto uuid =
            getSuggestedFootprint(mUnplacedComponentPackage->getUuid())) {
      fptIndex = std::max(package->getFootprints().indexOf(*uuid), 0);
    }
  }

  setSelectedUnplacedComponentFootprint(fptIndex);
}

void Board2dTab::setSelectedUnplacedComponentFootprint(int index) noexcept {
  mUnplacedComponentFootprintIndex = index;

  if (mUnplacedComponentGraphicsScene && mUnplacedComponentGraphicsItem) {
    mUnplacedComponentGraphicsScene->removeItem(
        *mUnplacedComponentGraphicsItem);
    mUnplacedComponentGraphicsItem.reset();
  }

  if (mUnplacedComponent && mUnplacedComponentPackage) {
    if (auto fpt = mUnplacedComponentPackage->getFootprints().value(index)) {
      mUnplacedComponentGraphicsItem.reset(new FootprintGraphicsItem(
          fpt, *mUnplacedComponentLayers, Application::getDefaultStrokeFont(),
          &mUnplacedComponentPackage->getPads(),
          &mUnplacedComponent->getLibComponent(), mProject.getLocaleOrder()));
      mUnplacedComponentGraphicsScene->addItem(*mUnplacedComponentGraphicsItem);
    }
  }

  requestRepaint();
}

std::pair<QList<Board2dTab::DeviceMetadata>, int>
    Board2dTab::getAvailableDevices(ComponentInstance& cmp) const noexcept {
  QList<DeviceMetadata> devices;
  Uuid cmpUuid = cmp.getLibComponent().getUuid();
  QStringList localeOrder = mProject.getLocaleOrder();

  // Get matching devices in project library.
  QHash<Uuid, Device*> prjLibDev =
      mProject.getLibrary().getDevicesOfComponent(cmpUuid);
  for (auto i = prjLibDev.constBegin(); i != prjLibDev.constEnd(); ++i) {
    devices.append(
        DeviceMetadata{i.key(), *i.value()->getNames().value(localeOrder),
                       i.value()->getPackageUuid(), QString(), false});
  }

  // Get matching devices in workspace library.
  try {
    QSet<Uuid> wsLibDev =
        mApp.getWorkspace().getLibraryDb().getComponentDevices(
            cmpUuid);  // can throw
    wsLibDev -= Toolbox::toSet(prjLibDev.keys());
    foreach (const Uuid& deviceUuid, wsLibDev) {
      // Get device metadata.
      FilePath devFp = mApp.getWorkspace().getLibraryDb().getLatest<Device>(
          deviceUuid);  // can throw
      if (!devFp.isValid()) continue;
      QString devName;
      mApp.getWorkspace().getLibraryDb().getTranslations<Device>(
          devFp, localeOrder,
          &devName);  // can throw
      Uuid pkgUuid = Uuid::createRandom();  // Temporary.
      mApp.getWorkspace().getLibraryDb().getDeviceMetadata(
          devFp, nullptr,
          &pkgUuid);  // can throw

      devices.append(
          DeviceMetadata{deviceUuid, devName, pkgUuid, QString(), false});
    }
  } catch (const Exception& e) {
    qCritical() << "Failed to list devices in unplaced components dock:"
                << e.getMsg();
  }

  // Determine missing metadata.
  const QSet<Uuid> cmpDevices = cmp.getCompatibleDevices();
  for (DeviceMetadata& device : devices) {
    device.isListedInComponentInstance = cmpDevices.contains(device.deviceUuid);
    if (const Package* package =
            mProject.getLibrary().getPackage(device.packageUuid)) {
      device.packageName = *package->getNames().value(localeOrder);
    } else {
      try {
        FilePath pkgFp = mApp.getWorkspace().getLibraryDb().getLatest<Package>(
            device.packageUuid);  // can throw
        if (!pkgFp.isValid()) continue;
        mApp.getWorkspace().getLibraryDb().getTranslations<Package>(
            pkgFp, localeOrder,
            &device.packageName);  // can throw
      } catch (const Exception& e) {
        qCritical() << "Failed to query packages in unplaced components dock:"
                    << e.getMsg();
      }
    }
  }

  // Sort by device name, using numeric sort.
  Toolbox::sortNumeric(
      devices,
      [](const QCollator& cmp, const DeviceMetadata& lhs,
         const DeviceMetadata& rhs) {
        return cmp(lhs.deviceName, rhs.deviceName);
      },
      Qt::CaseInsensitive, false);

  // Prio 1: Use the device already used for the same component before, if it
  // is chosen in the component instance.
  auto lastDeviceIterator = mLastDeviceOfComponent.find(cmpUuid);
  if ((lastDeviceIterator != mLastDeviceOfComponent.end())) {
    for (int i = 0; i < devices.count(); ++i) {
      if (devices.at(i).isListedInComponentInstance &&
          (devices.at(i).deviceUuid == *lastDeviceIterator)) {
        return std::make_pair(devices, i);
      }
    }
  }

  // Prio 2: Use the first device chosen in the component instance.
  for (int i = 0; i < devices.count(); ++i) {
    if (devices.at(i).isListedInComponentInstance) {
      return std::make_pair(devices, i);
    }
  }

  // Prio 3: Use the device already used for the same component before.
  if ((lastDeviceIterator != mLastDeviceOfComponent.end())) {
    for (int i = 0; i < devices.count(); ++i) {
      if (devices.at(i).deviceUuid == *lastDeviceIterator) {
        return std::make_pair(devices, i);
      }
    }
  }

  // Prio 4: Use the most used device in the current board.
  QHash<Uuid, int> devOccurences;
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    Q_ASSERT(device);
    if (device->getComponentInstance().getLibComponent().getUuid() ==
        cmp.getLibComponent().getUuid()) {
      ++devOccurences[device->getLibDevice().getUuid()];
    }
  }
  auto maxCountIt =
      std::max_element(devOccurences.constBegin(), devOccurences.constEnd());
  if (maxCountIt != devOccurences.constEnd()) {
    for (int i = 0; i < devices.count(); ++i) {
      if (devOccurences.value(devices.at(i).deviceUuid) == (*maxCountIt)) {
        return std::make_pair(devices, i);
      }
    }
  }

  // Prio 5: Use the first device found in the project library.
  for (int i = 0; i < devices.count(); ++i) {
    if (prjLibDev.contains(devices.at(i).deviceUuid)) {
      return std::make_pair(devices, i);
    }
  }

  // Prio 6: Use the first device found in the workspace library.
  return std::make_pair(devices, devices.isEmpty() ? -1 : 0);
}

std::optional<Uuid> Board2dTab::getSuggestedFootprint(
    const Uuid& libPkgUuid) const noexcept {
  // Prio 1: Use the footprint already used for the same device before.
  auto lastFootprintIterator = mLastFootprintOfPackage.find(libPkgUuid);
  if ((lastFootprintIterator != mLastFootprintOfPackage.end())) {
    return *lastFootprintIterator;
  }

  // Prio 2: Use the most used footprint in the current board.
  QHash<Uuid, int> fptOccurences;
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    if (device->getLibPackage().getUuid() == libPkgUuid) {
      ++fptOccurences[device->getLibFootprint().getUuid()];
    }
  }
  auto maxCountIt =
      std::max_element(fptOccurences.constBegin(), fptOccurences.constEnd());
  if (maxCountIt != fptOccurences.constEnd()) {
    QList<Uuid> uuids = fptOccurences.keys(*maxCountIt);
    if (uuids.count() > 0) {
      return uuids.first();
    }
  }

  // Prio 3: Fallback to the default footprint.
  return std::nullopt;
}

void Board2dTab::addUnplacedComponentsToBoard(
    PlaceComponentsMode mode) noexcept {
  // Parse & validate state.
  if ((!mUnplacedComponent) || (!mUnplacedComponentPackage)) {
    return;
  }
  if ((mUnplacedComponentDeviceIndex < 0) ||
      (mUnplacedComponentDeviceIndex >= mUnplacedComponentDevices.count())) {
    return;
  }
  const DeviceMetadata& selectedDevice =
      mUnplacedComponentDevices.at(mUnplacedComponentDeviceIndex);
  std::shared_ptr<const Footprint> selectedFootprint =
      mUnplacedComponentPackage->getFootprints().value(
          mUnplacedComponentFootprintIndex);
  if (!selectedFootprint) {
    return;
  }

  // Release undo stack.
  mProjectEditor.abortBlockingToolsInOtherEditors(this);

  // Memorize selection.
  if (mode != PlaceComponentsMode::All) {
    mLastDeviceOfComponent.insert(
        mUnplacedComponent->getLibComponent().getUuid(),
        selectedDevice.deviceUuid);
    mLastFootprintOfPackage.insert(mUnplacedComponentPackage->getUuid(),
                                   selectedFootprint->getUuid());
  }

  // Single mode is interactive and handled by FSM.
  if (mode == PlaceComponentsMode::Single) {
    mFsm->processAddDevice(*mUnplacedComponent, selectedDevice.deviceUuid,
                           selectedFootprint->getUuid());
    return;
  }

  // Multi-mode is handled here.
  Point nextPos = Point::fromMm(0, -20);
  if (mScene) {
    nextPos += Point::fromPx(mScene->itemsBoundingRect().bottomLeft());
  }
  std::unique_ptr<UndoCommandGroup> cmd(
      new UndoCommandGroup(tr("Add devices to board")));
  for (const Uuid& cmpUuid : mUnplacedComponents) {
    ComponentInstance* cmp =
        mProject.getCircuit().getComponentInstanceByUuid(cmpUuid);
    if (cmp &&
        ((mode == PlaceComponentsMode::All) ||
         (cmp->getLibComponent().getUuid() ==
          mUnplacedComponent->getLibComponent().getUuid()))) {
      const std::pair<QList<DeviceMetadata>, int> devices =
          getAvailableDevices(*cmp);
      if ((devices.second >= 0) && (devices.second < devices.first.count())) {
        const DeviceMetadata& dev = devices.first.at(devices.second);
        const std::optional<Uuid> fptUuid =
            getSuggestedFootprint(dev.packageUuid);
        cmd->appendChild(new CmdAddDeviceToBoard(
            mApp.getWorkspace(), mBoard, *cmp, dev.deviceUuid, fptUuid,
            std::nullopt, nextPos.mapToGrid(mBoard.getGridInterval())));
        if (nextPos.getX() > Length::fromMm(100)) {
          nextPos = Point::fromMm(0, nextPos.getY().toMm() - 10);
        } else {
          nextPos += Point::fromMm(10, 0);
        }
      }
    }
  }
  try {
    mProjectEditor.getUndoStack().execCmd(cmd.release());
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
  }
}

void Board2dTab::execGraphicsExportDialog(GraphicsExportDialog::Output output,
                                          const QString& settingsKey) noexcept {
  try {
    // Determine default file path.
    QString projectName = FilePath::cleanFileName(
        *mProject.getName(), FilePath::ReplaceSpaces | FilePath::KeepCase);
    QString projectVersion = FilePath::cleanFileName(
        *mProject.getVersion(), FilePath::ReplaceSpaces | FilePath::KeepCase);
    QString relativePath =
        QString("output/%1/%2_Board").arg(projectVersion, projectName);
    FilePath defaultFilePath = mProject.getPath().getPathTo(relativePath);

    // Copy board to allow processing it in worker threads.
    QProgressDialog progress(tr("Preparing board..."), tr("Cancel"), 0, 1,
                             qApp->activeWindow());
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(100);
    QList<std::shared_ptr<GraphicsPagePainter>> pages{
        std::make_shared<BoardPainter>(mBoard)};
    progress.setValue(1);
    if (progress.wasCanceled()) {
      return;
    }

    // Show dialog, which will do all the work.
    GraphicsExportDialog dialog(
        GraphicsExportDialog::Mode::Board, output, pages, 0,
        *mProject.getName(), mBoard.getInnerLayerCount(), defaultFilePath,
        mApp.getWorkspace().getSettings().defaultLengthUnit.get(),
        mApp.getWorkspace().getSettings().themes.getActive(),
        "board_editor/" % settingsKey, qApp->activeWindow());
    connect(&dialog, &GraphicsExportDialog::requestOpenFile, this,
            [this](const FilePath& fp) {
              DesktopServices ds(mApp.getWorkspace().getSettings());
              ds.openLocalPath(fp);
            });
    dialog.exec();
  } catch (const Exception& e) {
    QMessageBox::warning(qApp->activeWindow(), tr("Error"), e.getMsg());
  }
}

void Board2dTab::execPickPlaceExportDialog() noexcept {
  BoardPickPlaceGeneratorDialog dialog(mApp.getWorkspace().getSettings(),
                                       mBoard);
  dialog.exec();
}

void Board2dTab::execD356NetlistExportDialog() noexcept {
  try {
    QString path = "output/{{VERSION}}/{{PROJECT}}_Netlist.d356";
    path = AttributeSubstitutor::substitute(
        path, ProjectAttributeLookup(mBoard, nullptr), [&](const QString& str) {
          return FilePath::cleanFileName(
              str, FilePath::ReplaceSpaces | FilePath::KeepCase);
        });
    path = FileDialog::getSaveFileName(
        qApp->activeWindow(), tr("Export IPC D-356A Netlist"),
        mProject.getPath().getPathTo(path).toStr(), "*.d356");
    if (path.isEmpty()) return;
    if (!path.contains(".")) path.append(".d356");

    FilePath fp(path);
    qDebug().nospace() << "Export IPC D-356A netlist to " << fp.toNative()
                       << "...";
    BoardD356NetlistExport exp(mBoard);
    FileUtils::writeFile(fp, exp.generate());  // can throw
    qDebug() << "Successfully exported netlist.";
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
  }
}

void Board2dTab::execSpecctraExportDialog() noexcept {
  try {
    // Default file path.
    QString path = "output/{{VERSION}}/{{PROJECT}}";
    if (mProject.getBoards().count() > 1) {
      path += "_{{BOARD}}";
    }
    path += ".dsn";
    path = AttributeSubstitutor::substitute(
        path, ProjectAttributeLookup(mBoard, nullptr), [&](const QString& str) {
          return FilePath::cleanFileName(
              str, FilePath::ReplaceSpaces | FilePath::KeepCase);
        });

    // Use memorized file path, if board path and version number match.
    QSettings cs;
    const QString csId =
        mBoard.getDirectory().getAbsPath().toStr() + *mProject.getVersion();
    const QString csKey = "board_editor/dsn_export/" %
        QString(QCryptographicHash::hash(csId.toUtf8(), QCryptographicHash::Md5)
                    .toHex());
    path = cs.value(csKey, path).toString();

    // Make file path absolute.
    if (QFileInfo(path).isRelative()) {
      path = mProject.getPath().getPathTo(path).toStr();
    }

    // Choose file path.
    path = FileDialog::getSaveFileName(
        qApp->activeWindow(),
        EditorCommandSet::instance().exportSpecctraDsn.getDisplayText(), path,
        "*.dsn");
    if (path.isEmpty()) return;
    if (!path.contains(".")) path.append(".dsn");
    const FilePath fp(path);

    // Memorize file path.
    cs.setValue(csKey,
                fp.isLocatedInDir(mProject.getPath())
                    ? fp.toRelative(mProject.getPath())
                    : fp.toNative());

    // Perform export.
    qDebug().nospace() << "Export Specctra DSN to " << fp.toNative() << "...";
    BoardSpecctraExport exp(mBoard);
    FileUtils::writeFile(fp, exp.generate());  // can throw
    qDebug() << "Successfully exported Specctra DSN.";
    emit statusBarMessageChanged(tr("Success!"), 3000);
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
  }
}

void Board2dTab::execSpecctraImportDialog() noexcept {
  auto logger = std::make_shared<MessageLogger>();
  logger->warning(
      tr("This is a new feature and we could test it only with very few "
         "external routers. If you experience any compatibility issue with "
         "your router, please let us know!"));
  logger->warning(" → https://librepcb.org/help/");

  try {
    // Use memorized export file path, if board path and version number match.
    QSettings cs;
    const QString csId =
        mBoard.getDirectory().getAbsPath().toStr() + *mProject.getVersion();
    const QString csKey = "board_editor/dsn_export/" %
        QString(QCryptographicHash::hash(csId.toUtf8(), QCryptographicHash::Md5)
                    .toHex());
    QString path = cs.value(csKey).toString().replace(".dsn", ".ses");

    // Make file path absolute.
    if (QFileInfo(path).isRelative()) {
      path = mProject.getPath().getPathTo(path).toStr();
    }

    // Choose file path.
    path = FileDialog::getOpenFileName(
        qApp->activeWindow(),
        EditorCommandSet::instance().importSpecctraSes.getDisplayText(), path,
        "*.ses;;*");
    if (path.isEmpty()) return;
    const FilePath fp(path);

    // Release undo stack.
    mProjectEditor.abortBlockingToolsInOtherEditors(this);

    // Set UI into busy state during the import.
    QApplication::setOverrideCursor(Qt::WaitCursor);
    auto cursorScopeGuard =
        scopeGuard([]() { QApplication::restoreOverrideCursor(); });

    // Perform import.
    qDebug().nospace() << "Import Specctra SES from " << fp.toNative() << "...";
    logger->debug(tr("Parsing Specctra session '%1'...").arg(fp.toNative()));
    const QByteArray content = FileUtils::readFile(fp);  // can throw
    std::unique_ptr<SExpression> root =
        SExpression::parse(content, fp, SExpression::Mode::Permissive);
    mProjectEditor.getUndoStack().execCmd(
        new CmdBoardSpecctraImport(mBoard, *root, logger));  // can throw
    qDebug() << "Successfully imported Specctra SES.";
  } catch (const Exception& e) {
    logger->critical(e.getMsg());
    logger->critical(tr("Import failed, no changes made to the board."));
  }

  // Display messages.
  QDialog dlg(qApp->activeWindow());
  dlg.setWindowTitle(tr("Specctra SES Import"));
  dlg.setMinimumSize(600, 400);
  QVBoxLayout* layout = new QVBoxLayout(&dlg);
  QTextBrowser* txtBrowser = new QTextBrowser(&dlg);
  txtBrowser->setReadOnly(true);
  txtBrowser->setWordWrapMode(QTextOption::WordWrap);
  txtBrowser->setText(logger->getMessagesRichText());
  txtBrowser->verticalScrollBar()->setValue(
      txtBrowser->verticalScrollBar()->maximum());
  layout->addWidget(txtBrowser);
  QPushButton* btnClose = new QPushButton(tr("Close"), &dlg);
  connect(btnClose, &QPushButton::clicked, &dlg, &QDialog::accept);
  layout->addWidget(btnClose);
  dlg.exec();
}

void Board2dTab::goToDevice(const QString& name, int index) noexcept {
  QList<BI_Device*> deviceCandidates;
  foreach (BI_Device* device, mBoard.getDeviceInstances().values()) {
    if (device->getComponentInstance().getName()->startsWith(
            name, Qt::CaseInsensitive)) {
      deviceCandidates.append(device);
    }
  }

  // Sort by name for a natural order of results.
  Toolbox::sortNumeric(
      deviceCandidates,
      [](const QCollator& cmp, const BI_Device* a, const BI_Device* b) {
        return cmp(*a->getComponentInstance().getName(),
                   *b->getComponentInstance().getName());
      },
      Qt::CaseInsensitive, false);

  if (!deviceCandidates.isEmpty()) {
    while (index < 0) {
      index += deviceCandidates.count();
    }
    index %= deviceCandidates.count();
    BI_Device* device = deviceCandidates[index];
    if (mScene) {
      mScene->clearSelection();
      if (auto item = mScene->getDevices().value(device)) {
        item->setSelected(true);
        QRectF rect = item->mapRectToScene(item->childrenBoundingRect());
        // Zoom to a rectangle relative to the maximum graphics item dimension,
        // occupying 1/4th of the screen, but limiting the margin to 10mm.
        const qreal margin =
            std::min(1.5f * std::max(rect.size().width(), rect.size().height()),
                     Length::fromMm(10).toPx());
        rect.adjust(-margin, -margin, margin, margin);
        mView->zoomToSceneRect(rect);
      }
    }
  }
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

  if (mUnplacedComponentGraphicsScene) {
    mUnplacedComponentGraphicsScene->setBackgroundColors(
        theme.getColor(Theme::Color::sBoardBackground).getPrimaryColor(),
        theme.getColor(Theme::Color::sBoardBackground).getSecondaryColor());
  }

  onDerivedUiDataChanged.notify();
}

void Board2dTab::requestRepaint() noexcept {
  ++mFrameIndex;
  onDerivedUiDataChanged.notify();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
