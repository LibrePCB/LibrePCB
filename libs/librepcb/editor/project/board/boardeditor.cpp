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
#include "boardeditor.h"

#include "../../3d/openglscenebuilder.h"
#include "../../dialogs/filedialog.h"
#include "../../dialogs/gridsettingsdialog.h"
#include "../../editorcommandset.h"
#include "../../graphics/graphicslayerlist.h"
#include "../../graphics/graphicsscene.h"
#include "../../graphics/primitivepathgraphicsitem.h"
#include "../../project/cmd/cmdboardadd.h"
#include "../../project/cmd/cmdboardremove.h"
#include "../../undostack.h"
#include "../../utils/editortoolbox.h"
#include "../../utils/exclusiveactiongroup.h"
#include "../../utils/menubuilder.h"
#include "../../utils/standardeditorcommandhandler.h"
#include "../../utils/toolbarproxy.h"
#include "../../utils/undostackactiongroup.h"
#include "../../widgets/graphicsview.h"
#include "../../widgets/openglview.h"
#include "../../widgets/rulecheckdock.h"
#include "../../widgets/searchtoolbar.h"
#include "../../workspace/desktopservices.h"
#include "../bomgeneratordialog.h"
#include "../cmd/cmdboardspecctraimport.h"
#include "../outputjobsdialog/outputjobsdialog.h"
#include "../projecteditor.h"
#include "../projectsetupdialog.h"
#include "boardgraphicsscene.h"
#include "boardlayersdock.h"
#include "boardpickplacegeneratordialog.h"
#include "boardsetupdialog.h"
#include "fabricationoutputdialog.h"
#include "fsm/boardeditorfsm.h"
#include "graphicsitems/bgi_device.h"
#include "unplacedcomponentsdock.h"

#include <librepcb/core/3d/stepexport.h>
#include <librepcb/core/application.h>
#include <librepcb/core/attribute/attributesubstitutor.h>
#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/geometry/polygon.h>
#include <librepcb/core/library/pkg/footprint.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/boardd356netlistexport.h>
#include <librepcb/core/project/board/boardpainter.h>
#include <librepcb/core/project/board/boardplanefragmentsbuilder.h>
#include <librepcb/core/project/board/boardspecctraexport.h>
#include <librepcb/core/project/board/drc/boarddesignrulecheck.h>
#include <librepcb/core/project/board/items/bi_device.h>
#include <librepcb/core/project/board/items/bi_footprintpad.h>
#include <librepcb/core/project/board/items/bi_hole.h>
#include <librepcb/core/project/board/items/bi_netline.h>
#include <librepcb/core/project/board/items/bi_netsegment.h>
#include <librepcb/core/project/board/items/bi_plane.h>
#include <librepcb/core/project/board/items/bi_polygon.h>
#include <librepcb/core/project/board/items/bi_via.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/circuit/componentinstance.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/projectattributelookup.h>
#include <librepcb/core/types/layer.h>
#include <librepcb/core/utils/messagelogger.h>
#include <librepcb/core/utils/scopeguard.h>
#include <librepcb/core/utils/toolbox.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>
#include <librepcb/core/workspace/workspacesettings.h>

#include <QSvgGenerator>
#include <QtCore>
#include <QtPrintSupport>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardEditor::BoardEditor(ProjectEditor& projectEditor, Project& project)
  : QMainWindow(0),
    mProjectEditor(projectEditor),
    mProject(project),
    mUi(new Ui::BoardEditor),
    mCommandToolBarProxy(new ToolBarProxy(this)),
    mStandardCommandHandler(new StandardEditorCommandHandler(
        mProjectEditor.getWorkspace().getSettings(), this)),
    mActiveBoard(nullptr),
    mLayers(GraphicsLayerList::boardLayers(
        &projectEditor.getWorkspace().getSettings())),
    mGraphicsScene(),
    mOpenGlSceneBuilder(),
    mOpenGlSceneBuildScheduled(false),
    mTimestampOfLastOpenGlSceneRebuild(0),
    mVisibleSceneRect(),
    mFsm(),
    mPlaneFragmentsBuilder(new BoardPlaneFragmentsBuilder(this)),
    mTimestampOfLastPlaneRebuild(0) {
  mUi->setupUi(this);
  mUi->tabBar->setDocumentMode(true);  // For MacOS
  mUi->lblUnplacedComponentsNote->hide();

  // Workaround for automatically closing window when opening 3D viewer,
  // see https://github.com/LibrePCB/LibrePCB/issues/1363.
  {
    QOpenGLWidget* w = new QOpenGLWidget(this);
    w->hide();
  }

  // Setup graphics view.
  const Theme& theme =
      mProjectEditor.getWorkspace().getSettings().themes.getActive();
  mUi->graphicsView->setSpinnerColor(
      theme.getColor(Theme::Color::sBoardBackground).getSecondaryColor());
  mUi->graphicsView->setInfoBoxColors(
      theme.getColor(Theme::Color::sBoardInfoBox).getPrimaryColor(),
      theme.getColor(Theme::Color::sBoardInfoBox).getSecondaryColor());
  mUi->graphicsView->setUseOpenGl(
      mProjectEditor.getWorkspace().getSettings().useOpenGl.get());
  mUi->graphicsView->setEventHandlerObject(this);
  connect(mUi->graphicsView, &GraphicsView::cursorScenePositionChanged,
          mUi->statusbar, &StatusBar::setAbsoluteCursorPosition);
  connect(mPlaneFragmentsBuilder.data(), &BoardPlaneFragmentsBuilder::started,
          mUi->graphicsView, &GraphicsView::showWaitingSpinner);
  connect(mPlaneFragmentsBuilder.data(), &BoardPlaneFragmentsBuilder::finished,
          mUi->graphicsView, &GraphicsView::hideWaitingSpinner);

  // Setup 3D view.
  connect(mUi->btnShow3D, &QToolButton::clicked, this,
          &BoardEditor::show3DView);
  connect(mUi->btnHide3D, &QToolButton::clicked, this,
          &BoardEditor::hide3DView);
  connect(&mProjectEditor.getUndoStack(), &UndoStack::stateModified, this,
          &BoardEditor::scheduleOpenGlSceneUpdate);

  // Setup status bar.
  mUi->statusbar->setFields(StatusBar::AbsolutePosition |
                            StatusBar::ProgressBar);
  mUi->statusbar->setProgressBarTextFormat(tr("Scanning libraries (%p%)"));
  connect(&mProjectEditor.getWorkspace().getLibraryDb(),
          &WorkspaceLibraryDb::scanProgressUpdate, mUi->statusbar,
          &StatusBar::setProgressBarPercent, Qt::QueuedConnection);
  mUi->statusbar->setProgressBarPercent(
      mProjectEditor.getWorkspace().getLibraryDb().getScanProgressPercent());
  connect(&mProjectEditor, &ProjectEditor::showTemporaryStatusBarMessage,
          mUi->statusbar, &StatusBar::showMessage);

  // Set window title.
  QString filenameStr = mProject.getFilepath().getFilename();
  if (!mProject.getDirectory().isWritable()) {
    filenameStr.append(QStringLiteral(" [Read-Only]"));
  }
  setWindowTitle(tr("%1 - LibrePCB Board Editor").arg(filenameStr));

  // Store layers visibility on save.
  connect(&mProjectEditor, &ProjectEditor::projectAboutToBeSaved, this,
          &BoardEditor::storeLayersVisibility);

  // Build the whole board editor finite state machine.
  BoardEditorFsm::Context fsmContext{mProjectEditor.getWorkspace(),
                                     mProject,
                                     mProjectEditor,
                                     *this,
                                     *mUi->graphicsView,
                                     *mCommandToolBarProxy,
                                     mProjectEditor.getUndoStack()};
  mFsm.reset(new BoardEditorFsm(fsmContext));
  connect(mFsm.data(), &BoardEditorFsm::statusBarMessageChanged, this,
          [this](const QString& message, int timeoutMs) {
            if (timeoutMs < 0) {
              mUi->statusbar->setPermanentMessage(message);
            } else {
              mUi->statusbar->showMessage(message, timeoutMs);
            }
          });

  // Setup plane rebuilder.
  connect(mPlaneFragmentsBuilder.data(), &BoardPlaneFragmentsBuilder::finished,
          this, [this](BoardPlaneFragmentsBuilder::Result result) {
            if (result.applyToBoard() && result.board) {
              // Board has been modified, update air wires & 3D view.
              result.board->forceAirWiresRebuild();
              scheduleOpenGlSceneUpdate();
            }
            mTimestampOfLastPlaneRebuild = QDateTime::currentMSecsSinceEpoch();
          });

  // Create all actions, window menus, toolbars and dock widgets.
  createActions();
  createToolBars();
  createDockWidgets();
  createMenus();  // Depends on dock widgets!
  updateBoardActionGroup();  // Depends on menus!

  // Disable actions which do not work nicely with *.lppz projects yet.
  if (!mProject.getDirectory().isWritable()) {
    mActionGenerateFabricationData->setEnabled(false);
    mActionGenerateBom->setEnabled(false);
    mActionGeneratePickPlace->setEnabled(false);
    mActionOutputJobs->setEnabled(false);
  }

  // Setup "project upgraded" message.
  {
    const QString msg = mProjectEditor.getUpgradeMessageLabelText();
    mUi->msgProjectUpgraded->init(msg, !msg.isEmpty());
    connect(mUi->msgProjectUpgraded, &MessageWidget::linkActivated, this,
            [this]() { mProjectEditor.showUpgradeMessages(this); });
    connect(&mProjectEditor, &ProjectEditor::projectSavedToDisk, this,
            [this]() { mUi->msgProjectUpgraded->setActive(false); });
  }

  // add all boards to the menu and connect to project signals
  mUi->tabBar->setVisible(false);  // hide since there are no boards yet
  for (int i = 0; i < mProject.getBoards().count(); i++) boardAdded(i);
  connect(&mProject, &Project::boardAdded, this, &BoardEditor::boardAdded);
  connect(&mProject, &Project::boardRemoved, this, &BoardEditor::boardRemoved);

  // Restore window geometry.
  QSettings clientSettings;
  restoreGeometry(
      clientSettings.value("board_editor/window_geometry").toByteArray());
  restoreState(
      clientSettings.value("board_editor/window_state_v2").toByteArray());

  // Load first board
  if (mProject.getBoards().count() > 0) setActiveBoardIndex(0);

  // Setup timer for scheduled tasks.
  QTimer* scheduledTasksTimer = new QTimer(this);
  connect(scheduledTasksTimer, &QTimer::timeout, this,
          &BoardEditor::performScheduledTasks);
  scheduledTasksTimer->start(100);

  // Set focus to graphics view (avoid having the focus in some arbitrary
  // widget).
  mUi->graphicsView->setFocus();

  // mGraphicsView->zoomAll(); does not work properly here, should be executed
  // later in the event loop (ugly, but seems to work...)
  QTimer::singleShot(200, mUi->graphicsView, &GraphicsView::zoomAll);
}

BoardEditor::~BoardEditor() {
  // Save Window Geometry
  QSettings clientSettings;
  clientSettings.setValue("board_editor/window_geometry", saveGeometry());
  clientSettings.setValue("board_editor/window_state_v2", saveState());

  // Important: Release command toolbar proxy since otherwise the actions will
  // be deleted first.
  mCommandToolBarProxy->setToolBar(nullptr);

  mFsm.reset();
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

bool BoardEditor::getIgnoreLocks() const noexcept {
  return mActionIgnoreLocks && mActionIgnoreLocks->isChecked();
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

bool BoardEditor::setActiveBoardIndex(int index) noexcept {
  Board* newBoard = mProject.getBoardByIndex(index);

  if (newBoard != mActiveBoard) {
    // "Ask" the FSM if changing the scene is allowed at the moment.
    // If the FSM accepts the event, we can switch to the specified board.
    if (!mFsm->processSwitchToBoard(index)) {
      return false;  // changing the board is not allowed!
    }

    if (mActiveBoard) {
      // stop airwire rebuild on every project modification (for performance
      // reasons)
      disconnect(&mProjectEditor.getUndoStack(), &UndoStack::stateModified,
                 mActiveBoard.data(), &Board::triggerAirWiresRebuild);
      // Save current view scene rect.
      mVisibleSceneRect[mActiveBoard->getUuid()] =
          mUi->graphicsView->getVisibleSceneRect();
      // Save layers visibility.
      storeLayersVisibility();
    }

    clearDrcMarker();  // Avoid dangling pointers.
    mUi->graphicsView->setScene(nullptr);
    mGraphicsScene.reset();
    mActiveBoard = newBoard;

    if (mActiveBoard) {
      // Update layers.
      connect(mActiveBoard, &Board::innerLayerCountChanged, this,
              &BoardEditor::updateEnabledCopperLayers);
      updateEnabledCopperLayers();
      loadLayersVisibility();
      // show scene, restore view scene rect, set grid properties
      const Theme& theme =
          mProjectEditor.getWorkspace().getSettings().themes.getActive();
      mGraphicsScene.reset(
          new BoardGraphicsScene(*mActiveBoard, *mLayers.get(),
                                 mProjectEditor.getHighlightedNetSignals()));
      mGraphicsScene->setBackgroundColors(
          theme.getColor(Theme::Color::sBoardBackground).getPrimaryColor(),
          theme.getColor(Theme::Color::sBoardBackground).getSecondaryColor());
      mGraphicsScene->setOverlayColors(
          theme.getColor(Theme::Color::sBoardOverlays).getPrimaryColor(),
          theme.getColor(Theme::Color::sBoardOverlays).getSecondaryColor());
      mGraphicsScene->setSelectionRectColors(
          theme.getColor(Theme::Color::sBoardSelection).getPrimaryColor(),
          theme.getColor(Theme::Color::sBoardSelection).getSecondaryColor());
      mGraphicsScene->setGridStyle(theme.getBoardGridStyle());
      mGraphicsScene->setGridInterval(mActiveBoard->getGridInterval());
      connect(&mProjectEditor, &ProjectEditor::highlightedNetSignalsChanged,
              mGraphicsScene.data(),
              &BoardGraphicsScene::updateHighlightedNetSignals);
      mUi->graphicsView->setScene(mGraphicsScene.data());
      const QRectF sceneRect = mVisibleSceneRect.value(mActiveBoard->getUuid());
      if (!sceneRect.isEmpty()) {
        mUi->graphicsView->setVisibleSceneRect(sceneRect);
      }
      mUi->statusbar->setLengthUnit(mActiveBoard->getGridUnit());
      // force airwire rebuild immediately and on every project modification
      mActiveBoard->triggerAirWiresRebuild();
      connect(&mProjectEditor.getUndoStack(), &UndoStack::stateModified,
              mActiveBoard.data(), &Board::triggerAirWiresRebuild);
    } else {
      mUi->graphicsView->setScene(nullptr);
    }

    // update dock widgets
    mDockUnplacedComponents->setBoard(mActiveBoard);
    mDockDrc->setInteractive(mActiveBoard != nullptr);
    mDockDrc->setMessages(mActiveBoard ? mDrcMessages[mActiveBoard->getUuid()]
                                       : std::nullopt);
    mDockDrc->setApprovals(mActiveBoard ? mActiveBoard->getDrcMessageApprovals()
                                        : QSet<SExpression>());

    // update toolbars
    mActionGridProperties->setEnabled(mActiveBoard != nullptr);
    mActionGridIncrease->setEnabled(mActiveBoard != nullptr);
    mActionGridDecrease->setEnabled(mActiveBoard != nullptr);

    // Update 3D view.
    scheduleOpenGlSceneUpdate();
  }

  // update GUI
  mFsm->processSwitchedBoard();
  mUi->tabBar->setCurrentIndex(index);
  if (QAction* action = mBoardActionGroup->actions().value(index)) {
    action->setChecked(true);
  }

  return true;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BoardEditor::abortAllCommands() noexcept {
  // ugly... ;-)
  mFsm->processAbortCommand();
  mFsm->processAbortCommand();
  mFsm->processAbortCommand();
}

void BoardEditor::abortBlockingToolsInOtherEditors() noexcept {
  mProjectEditor.abortBlockingToolsInOtherEditors(this);
}

/*******************************************************************************
 *  Inherited Methods
 ******************************************************************************/

void BoardEditor::closeEvent(QCloseEvent* event) noexcept {
  if (!mProjectEditor.windowIsAboutToClose(*this))
    event->ignore();
  else
    QMainWindow::closeEvent(event);
}

/*******************************************************************************
 *  Public Slots
 ******************************************************************************/

void BoardEditor::boardAdded(int newIndex) {
  Board* board = mProject.getBoardByIndex(newIndex);
  Q_ASSERT(board);
  if (!board) return;

  mUi->tabBar->insertTab(newIndex, *board->getName());

  // To avoid wasting space, only show the tab bar if there are multiple boards.
  mUi->tabBar->setVisible(mUi->tabBar->count() > 1);
}

void BoardEditor::boardRemoved(int oldIndex) {
  mUi->tabBar->removeTab(oldIndex);  // calls setActiveBoardIndex() if needed

  // To avoid wasting space, only show the tab bar if there are multiple boards.
  mUi->tabBar->setVisible(mUi->tabBar->count() > 1);
}

/*******************************************************************************
 *  Actions
 ******************************************************************************/

void BoardEditor::on_tabBar_currentChanged(int index) {
  setActiveBoardIndex(index);
}

void BoardEditor::on_lblUnplacedComponentsNote_linkActivated() {
  mDockUnplacedComponents->show();
  mDockUnplacedComponents->raise();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BoardEditor::updateEnabledCopperLayers() noexcept {
  if (Board* board = getActiveBoard()) {
    foreach (const Layer* layer, Layer::innerCopper()) {
      if (std::shared_ptr<GraphicsLayer> gLayer = mLayers->get(*layer)) {
        gLayer->setEnabled(board->getCopperLayers().contains(layer));
      }
    }
  }
}

void BoardEditor::loadLayersVisibility() noexcept {
  if (Board* board = getActiveBoard()) {
    foreach (std::shared_ptr<GraphicsLayer> layer, mLayers->all()) {
      if (board->getLayersVisibility().contains(layer->getName())) {
        layer->setVisible(board->getLayersVisibility().value(layer->getName()));
      }
    }
  }
}

void BoardEditor::storeLayersVisibility() noexcept {
  if (Board* board = getActiveBoard()) {
    QMap<QString, bool> visibility;
    foreach (std::shared_ptr<GraphicsLayer> layer, mLayers->all()) {
      if (layer->isEnabled()) {
        visibility[layer->getName()] = layer->isVisible();
      }
    }
    board->setLayersVisibility(visibility);
  }
}

void BoardEditor::createActions() noexcept {
  const EditorCommandSet& cmd = EditorCommandSet::instance();

  mActionAboutLibrePcb.reset(cmd.aboutLibrePcb.createAction(
      this, &mProjectEditor, &ProjectEditor::aboutLibrePcbRequested));
  mActionAboutQt.reset(
      cmd.aboutQt.createAction(this, qApp, &QApplication::aboutQt));
  mActionOnlineDocumentation.reset(cmd.documentationOnline.createAction(
      this, mStandardCommandHandler.data(),
      &StandardEditorCommandHandler::onlineDocumentation));
  mActionKeyboardShortcutsReference.reset(
      cmd.keyboardShortcutsReference.createAction(
          this, mStandardCommandHandler.data(),
          &StandardEditorCommandHandler::shortcutsReference));
  mActionWebsite.reset(
      cmd.website.createAction(this, mStandardCommandHandler.data(),
                               &StandardEditorCommandHandler::website));
  mActionSaveProject.reset(cmd.projectSave.createAction(
      this, &mProjectEditor, &ProjectEditor::saveProject));
  mActionSaveProject->setEnabled(mProject.getDirectory().isWritable());
  mActionCloseProject.reset(cmd.projectClose.createAction(
      this, this, [this]() { mProjectEditor.closeAndDestroy(true, this); }));
  mActionCloseWindow.reset(
      cmd.windowClose.createAction(this, this, &BoardEditor::close));
  mActionQuit.reset(cmd.applicationQuit.createAction(
      this, qApp, &QApplication::closeAllWindows,
      EditorCommand::ActionFlag::QueuedConnection));
  mActionFileManager.reset(cmd.fileManager.createAction(this, this, [this]() {
    mStandardCommandHandler->fileManager(mProject.getPath());
  }));
  mActionSchematicEditor.reset(cmd.schematicEditor.createAction(
      this, &mProjectEditor, &ProjectEditor::showSchematicEditor));
  mActionControlPanel.reset(cmd.controlPanel.createAction(
      this, &mProjectEditor, &ProjectEditor::showControlPanelClicked));
  mActionProjectSetup.reset(cmd.projectSetup.createAction(this, this, [this]() {
    abortBlockingToolsInOtherEditors();  // Release undo stack.
    ProjectSetupDialog dialog(mProject, mProjectEditor.getUndoStack(), this);
    dialog.exec();
  }));
  mActionUpdateLibrary.reset(
      cmd.projectLibraryUpdate.createAction(this, this, [this]() {
        // Ugly hack until we have a *real* project library updater...
        emit mProjectEditor.openProjectLibraryUpdaterClicked(
            mProject.getFilepath());
      }));
  mActionBoardSetup.reset(cmd.boardSetup.createAction(
      this, this, &BoardEditor::execBoardSetupDialog));
  mActionRunQuickCheck.reset(
      cmd.runQuickCheck.createAction(this, this, [this]() { runDrc(true); }));
  mActionRunDesignRuleCheck.reset(cmd.runDesignRuleCheck.createAction(
      this, this, [this]() { runDrc(false); }));
  mActionImportDxf.reset(cmd.importDxf.createAction(
      this, mFsm.data(), &BoardEditorFsm::processImportDxf));
  mActionImportSpecctra.reset(cmd.importSpecctraSes.createAction(
      this, this, &BoardEditor::execSpecctraImportDialog));
  mActionExportLppz.reset(cmd.exportLppz.createAction(
      this, this, [this]() { mProjectEditor.execLppzExportDialog(this); }));
  mActionExportImage.reset(cmd.exportImage.createAction(this, this, [this]() {
    execGraphicsExportDialog(GraphicsExportDialog::Output::Image,
                             "image_export");
  }));
  mActionExportPdf.reset(cmd.exportPdf.createAction(this, this, [this]() {
    execGraphicsExportDialog(GraphicsExportDialog::Output::Pdf, "pdf_export");
  }));
  mActionExportStep.reset(cmd.exportStep.createAction(
      this, this, &BoardEditor::execStepExportDialog));
  mActionExportSpecctra.reset(cmd.exportSpecctraDsn.createAction(
      this, this, &BoardEditor::execSpecctraExportDialog));
  mActionPrint.reset(cmd.print.createAction(this, this, [this]() {
    execGraphicsExportDialog(GraphicsExportDialog::Output::Print, "print");
  }));
  mActionGenerateBom.reset(cmd.generateBom.createAction(this, this, [this]() {
    BomGeneratorDialog dialog(mProjectEditor.getWorkspace().getSettings(),
                              mProject, getActiveBoard(), this);
    connect(&dialog, &BomGeneratorDialog::projectSettingsModified,
            &mProjectEditor, &ProjectEditor::setManualModificationsMade);
    dialog.exec();
  }));
  mActionGenerateFabricationData.reset(
      cmd.generateFabricationData.createAction(this, this, [this]() {
        if (Board* board = getActiveBoard()) {
          FabricationOutputDialog dialog(
              mProjectEditor.getWorkspace().getSettings(), *board, this);
          connect(&dialog, &FabricationOutputDialog::orderPcbDialogTriggered,
                  this, [this, &dialog]() {
                    mProjectEditor.execOrderPcbDialog(&dialog);
                  });
          dialog.exec();
        }
      }));
  mActionGeneratePickPlace.reset(
      cmd.generatePickPlace.createAction(this, this, [this]() {
        if (Board* board = getActiveBoard()) {
          BoardPickPlaceGeneratorDialog dialog(
              mProjectEditor.getWorkspace().getSettings(), *board);
          dialog.exec();
        }
      }));
  mActionGenerateD356Netlist.reset(cmd.generateD356Netlist.createAction(
      this, this, &BoardEditor::execD356NetlistExportDialog));
  mActionOutputJobs.reset(cmd.outputJobs.createAction(this, this, [this]() {
    OutputJobsDialog dialog(mProjectEditor.getWorkspace().getSettings(),
                            mProject, mProjectEditor.getUndoStack(), this);
    connect(&dialog, &OutputJobsDialog::orderPcbDialogTriggered, this,
            [this, &dialog]() { mProjectEditor.execOrderPcbDialog(&dialog); });
    dialog.exec();
  }));
  mActionOrderPcb.reset(cmd.orderPcb.createAction(
      this, this, [this]() { mProjectEditor.execOrderPcbDialog(this); }));
  mActionNewBoard.reset(
      cmd.boardNew.createAction(this, this, &BoardEditor::newBoard));
  mActionCopyBoard.reset(
      cmd.boardCopy.createAction(this, this, &BoardEditor::copyBoard));
  mActionRemoveBoard.reset(
      cmd.boardRemove.createAction(this, this, &BoardEditor::removeBoard));
  mActionNextPage.reset(cmd.pageNext.createAction(this, this, [this]() {
    const int newIndex = mUi->tabBar->currentIndex() + 1;
    if (newIndex < mUi->tabBar->count()) {
      mUi->tabBar->setCurrentIndex(newIndex);
    }
  }));
  addAction(mActionNextPage.data());
  mActionPreviousPage.reset(cmd.pagePrevious.createAction(this, this, [this]() {
    const int newIndex = mUi->tabBar->currentIndex() - 1;
    if (newIndex >= 0) {
      mUi->tabBar->setCurrentIndex(newIndex);
    }
  }));
  addAction(mActionPreviousPage.data());
  mActionFind.reset(cmd.find.createAction(this));
  mActionFindNext.reset(cmd.findNext.createAction(this));
  mActionFindPrevious.reset(cmd.findPrevious.createAction(this));
  mActionSelectAll.reset(cmd.selectAll.createAction(
      this, mFsm.data(), &BoardEditorFsm::processSelectAll));
  mActionGridProperties.reset(cmd.gridProperties.createAction(
      this, this, &BoardEditor::execGridPropertiesDialog));
  mActionGridIncrease.reset(cmd.gridIncrease.createAction(this, this, [this]() {
    const Board* board = getActiveBoard();
    if (board && mGraphicsScene) {
      const Length interval = board->getGridInterval() * 2;
      setGridProperties(PositiveLength(interval), board->getGridUnit(),
                        mGraphicsScene->getGridStyle(), true);
    }
  }));
  mActionGridDecrease.reset(cmd.gridDecrease.createAction(this, this, [this]() {
    const Board* board = getActiveBoard();
    if (board && mGraphicsScene) {
      const Length interval = *board->getGridInterval();
      if ((interval % 2) == 0) {
        setGridProperties(PositiveLength(interval / 2), board->getGridUnit(),
                          mGraphicsScene->getGridStyle(), true);
      }
    }
  }));
  mActionIgnoreLocks.reset(cmd.ignoreLocks.createAction(this));
  mActionIgnoreLocks->setCheckable(true);
  mActionZoomFit.reset(cmd.zoomFitContent.createAction(this, this, [this]() {
    if (mOpenGlView && mOpenGlView->isVisible()) {
      mOpenGlView->zoomAll();
    } else {
      mUi->graphicsView->zoomAll();
    }
  }));
  mActionZoomIn.reset(cmd.zoomIn.createAction(this, this, [this]() {
    if (mOpenGlView && mOpenGlView->isVisible()) {
      mOpenGlView->zoomIn();
    } else {
      mUi->graphicsView->zoomIn();
    }
  }));
  mActionZoomOut.reset(cmd.zoomOut.createAction(this, this, [this]() {
    if (mOpenGlView && mOpenGlView->isVisible()) {
      mOpenGlView->zoomOut();
    } else {
      mUi->graphicsView->zoomOut();
    }
  }));
  mActionToggle3D.reset(cmd.toggle3d.createAction(this, this, [this]() {
    if (!show3DView()) {
      hide3DView();
      hide3DView();
    }
  }));
  mActionUndo.reset(cmd.undo.createAction(this));
  mActionRedo.reset(cmd.redo.createAction(this));
  mActionCut.reset(cmd.clipboardCut.createAction(this, mFsm.data(),
                                                 &BoardEditorFsm::processCut));
  mActionCopy.reset(cmd.clipboardCopy.createAction(
      this, mFsm.data(), &BoardEditorFsm::processCopy));
  mActionPaste.reset(cmd.clipboardPaste.createAction(
      this, mFsm.data(), &BoardEditorFsm::processPaste));
  mActionMoveLeft.reset(cmd.moveLeft.createAction(this, this, [this]() {
    if (const Board* board = getActiveBoard()) {
      if (!mFsm->processMove(Point(-board->getGridInterval(), 0))) {
        // Workaround for consumed keyboard shortcuts for scrolling.
        mUi->graphicsView->horizontalScrollBar()->triggerAction(
            QScrollBar::SliderSingleStepSub);
      }
    }
  }));
  addAction(mActionMoveLeft.data());
  mActionMoveRight.reset(cmd.moveRight.createAction(this, this, [this]() {
    if (const Board* board = getActiveBoard()) {
      if (!mFsm->processMove(Point(*board->getGridInterval(), 0))) {
        // Workaround for consumed keyboard shortcuts for scrolling.
        mUi->graphicsView->horizontalScrollBar()->triggerAction(
            QScrollBar::SliderSingleStepAdd);
      }
    }
  }));
  addAction(mActionMoveRight.data());
  mActionMoveUp.reset(cmd.moveUp.createAction(this, this, [this]() {
    if (const Board* board = getActiveBoard()) {
      if (!mFsm->processMove(Point(0, *board->getGridInterval()))) {
        // Workaround for consumed keyboard shortcuts for scrolling.
        mUi->graphicsView->verticalScrollBar()->triggerAction(
            QScrollBar::SliderSingleStepSub);
      }
    }
  }));
  addAction(mActionMoveUp.data());
  mActionMoveDown.reset(cmd.moveDown.createAction(this, this, [this]() {
    if (const Board* board = getActiveBoard()) {
      if (!mFsm->processMove(Point(0, -board->getGridInterval()))) {
        // Workaround for consumed keyboard shortcuts for scrolling.
        mUi->graphicsView->verticalScrollBar()->triggerAction(
            QScrollBar::SliderSingleStepAdd);
      }
    }
  }));
  addAction(mActionMoveDown.data());
  mActionRotateCcw.reset(cmd.rotateCcw.createAction(
      this, this, [this]() { mFsm->processRotate(Angle::deg90()); }));
  mActionRotateCw.reset(cmd.rotateCw.createAction(
      this, this, [this]() { mFsm->processRotate(-Angle::deg90()); }));
  mActionFlipHorizontal.reset(cmd.flipHorizontal.createAction(
      this, this, [this]() { mFsm->processFlip(Qt::Horizontal); }));
  mActionFlipVertical.reset(cmd.flipVertical.createAction(
      this, this, [this]() { mFsm->processFlip(Qt::Vertical); }));
  mActionSnapToGrid.reset(cmd.snapToGrid.createAction(
      this, mFsm.data(), &BoardEditorFsm::processSnapToGrid));
  mActionLock.reset(cmd.lock.createAction(
      this, this, [this]() { mFsm->processSetLocked(true); }));
  mActionUnlock.reset(cmd.unlock.createAction(
      this, this, [this]() { mFsm->processSetLocked(false); }));
  mActionResetAllTexts.reset(cmd.deviceResetTextAll.createAction(
      this, mFsm.data(), &BoardEditorFsm::processResetAllTexts));
  mActionIncreaseLineWidth.reset(cmd.lineWidthIncrease.createAction(
      this, this, [this]() { mFsm->processChangeLineWidth(1); }));
  mActionDecreaseLineWidth.reset(cmd.lineWidthDecrease.createAction(
      this, this, [this]() { mFsm->processChangeLineWidth(-1); }));
  mActionChangeLineWidth.reset(cmd.setLineWidth.createAction(
      this, this, [this]() { mFsm->processChangeLineWidth(0); }));
  mActionProperties.reset(cmd.properties.createAction(
      this, mFsm.data(), &BoardEditorFsm::processEditProperties));
  mActionRemove.reset(cmd.remove.createAction(this, mFsm.data(),
                                              &BoardEditorFsm::processRemove));
  mActionShowPlanes.reset(cmd.planeShowAll.createAction(this, this, [this]() {
    if (Board* board = getActiveBoard()) {
      foreach (BI_Plane* plane, board->getPlanes()) {
        // No undo command needed since it is not saved.
        plane->setVisible(true);
      }
    }
  }));
  mActionHidePlanes.reset(cmd.planeHideAll.createAction(this, this, [this]() {
    if (Board* board = getActiveBoard()) {
      foreach (BI_Plane* plane, board->getPlanes()) {
        // No undo command needed since it is not saved.
        plane->setVisible(false);
      }
    }
  }));
  mActionRebuildPlanes.reset(cmd.planeRebuildAll.createAction(
      this, this, [this]() { startPlaneRebuild(true); }));
  mActionAbort.reset(cmd.abort.createAction(
      this, mFsm.data(), &BoardEditorFsm::processAbortCommand));
  mActionToolSelect.reset(cmd.toolSelect.createAction(this));
  mActionToolTrace.reset(cmd.toolTrace.createAction(this));
  mActionToolVia.reset(cmd.toolVia.createAction(this));
  mActionToolPolygon.reset(cmd.toolPolygon.createAction(this));
  mActionToolText.reset(cmd.toolText.createAction(this));
  mActionToolPlane.reset(cmd.toolPlane.createAction(this));
  mActionToolZone.reset(cmd.toolZone.createAction(this));
  mActionToolHole.reset(cmd.toolHole.createAction(this));
  mActionToolMeasure.reset(cmd.toolMeasure.createAction(this));
  mActionDockErc.reset(cmd.dockErc.createAction(this, this, [this]() {
    mDockErc->show();
    mDockErc->raise();
    mDockErc->setFocus();
  }));
  mActionDockDrc.reset(cmd.dockDrc.createAction(this, this, [this]() {
    mDockDrc->show();
    mDockDrc->raise();
    mDockDrc->setFocus();
  }));
  mActionDockLayers.reset(cmd.dockLayers.createAction(this, this, [this]() {
    mDockLayers->show();
    mDockLayers->raise();
    mDockLayers->setFocus();
  }));
  mActionDockPlaceDevices.reset(
      cmd.dockPlaceDevices.createAction(this, this, [this]() {
        mDockUnplacedComponents->show();
        mDockUnplacedComponents->raise();
        mDockUnplacedComponents->setFocus();
      }));

  // Widget shortcuts.
  mUi->graphicsView->addAction(cmd.commandToolBarFocus.createAction(
      this, this,
      [this]() {
        mCommandToolBarProxy->startTabFocusCycle(*mUi->graphicsView);
      },
      EditorCommand::ActionFlag::WidgetShortcut));

  // Undo stack action group.
  mUndoStackActionGroup.reset(
      new UndoStackActionGroup(*mActionUndo, *mActionRedo, nullptr,
                               &mProjectEditor.getUndoStack(), this));

  // Tools action group.
  mToolsActionGroup.reset(new ExclusiveActionGroup());
  mToolsActionGroup->addAction(mActionToolSelect.data(),
                               BoardEditorFsm::State::SELECT);
  mToolsActionGroup->addAction(mActionToolTrace.data(),
                               BoardEditorFsm::State::DRAW_TRACE);
  mToolsActionGroup->addAction(mActionToolVia.data(),
                               BoardEditorFsm::State::ADD_VIA);
  mToolsActionGroup->addAction(mActionToolPolygon.data(),
                               BoardEditorFsm::State::DRAW_POLYGON);
  mToolsActionGroup->addAction(mActionToolPlane.data(),
                               BoardEditorFsm::State::DRAW_PLANE);
  mToolsActionGroup->addAction(mActionToolZone.data(),
                               BoardEditorFsm::State::DRAW_ZONE);
  mToolsActionGroup->addAction(mActionToolText.data(),
                               BoardEditorFsm::State::ADD_STROKE_TEXT);
  mToolsActionGroup->addAction(mActionToolHole.data(),
                               BoardEditorFsm::State::ADD_HOLE);
  mToolsActionGroup->addAction(mActionToolMeasure.data(),
                               BoardEditorFsm::State::MEASURE);
  mToolsActionGroup->setCurrentAction(mFsm->getCurrentState());
  connect(mFsm.data(), &BoardEditorFsm::stateChanged, mToolsActionGroup.data(),
          &ExclusiveActionGroup::setCurrentAction);
  connect(mToolsActionGroup.data(), &ExclusiveActionGroup::actionTriggered,
          this, &BoardEditor::toolRequested);
}

void BoardEditor::createToolBars() noexcept {
  // File.
  mToolBarFile.reset(new QToolBar(tr("File"), this));
  mToolBarFile->setObjectName("toolBarFile");
  mToolBarFile->addAction(mActionCloseProject.data());
  mToolBarFile->addSeparator();
  mToolBarFile->addAction(mActionNewBoard.data());
  mToolBarFile->addAction(mActionSaveProject.data());
  mToolBarFile->addAction(mActionPrint.data());
  mToolBarFile->addAction(mActionExportPdf.data());
  mToolBarFile->addAction(mActionOutputJobs.data());
  mToolBarFile->addAction(mActionOrderPcb.data());
  mToolBarFile->addSeparator();
  mToolBarFile->addAction(mActionControlPanel.data());
  mToolBarFile->addAction(mActionSchematicEditor.data());
  mToolBarFile->addSeparator();
  mToolBarFile->addAction(mActionUndo.data());
  mToolBarFile->addAction(mActionRedo.data());
  addToolBar(Qt::TopToolBarArea, mToolBarFile.data());

  // Edit.
  mToolBarEdit.reset(new QToolBar(tr("Edit"), this));
  mToolBarEdit->setObjectName("toolBarEdit");
  mToolBarEdit->addAction(mActionCut.data());
  mToolBarEdit->addAction(mActionCopy.data());
  mToolBarEdit->addAction(mActionPaste.data());
  mToolBarEdit->addAction(mActionRemove.data());
  mToolBarEdit->addAction(mActionRotateCcw.data());
  mToolBarEdit->addAction(mActionRotateCw.data());
  mToolBarEdit->addAction(mActionFlipHorizontal.data());
  mToolBarEdit->addAction(mActionFlipVertical.data());
  addToolBar(Qt::TopToolBarArea, mToolBarEdit.data());

  // View.
  mToolBarView.reset(new QToolBar(tr("View"), this));
  mToolBarView->setObjectName("toolBarView");
  mToolBarView->addAction(mActionGridProperties.data());
  mToolBarView->addAction(mActionIgnoreLocks.data());
  mToolBarView->addAction(mActionZoomIn.data());
  mToolBarView->addAction(mActionZoomOut.data());
  mToolBarView->addAction(mActionZoomFit.data());
  mToolBarView->addAction(mActionToggle3D.data());
  addToolBar(Qt::TopToolBarArea, mToolBarView.data());

  // Search.
  mToolBarSearch.reset(new SearchToolBar(this));
  mToolBarSearch->setObjectName("toolBarSearch");
  mToolBarSearch->setPlaceholderText(tr("Find device..."));
  mToolBarSearch->setCompleterListFunction(
      std::bind(&BoardEditor::getSearchToolBarCompleterList, this));
  connect(mActionFind.data(), &QAction::triggered, mToolBarSearch.data(),
          &SearchToolBar::selectAllAndSetFocus);
  connect(mActionFindNext.data(), &QAction::triggered, mToolBarSearch.data(),
          &SearchToolBar::findNext);
  connect(mActionFindPrevious.data(), &QAction::triggered,
          mToolBarSearch.data(), &SearchToolBar::findPrevious);
  addToolBar(Qt::TopToolBarArea, mToolBarSearch.data());
  connect(mToolBarSearch.data(), &SearchToolBar::goToTriggered, this,
          &BoardEditor::goToDevice);

  // Command.
  mToolBarCommand.reset(new QToolBar(tr("Command"), this));
  mToolBarCommand->setObjectName("toolBarCommand");
  mToolBarCommand->addAction(mActionAbort.data());
  mToolBarCommand->addSeparator();
  addToolBarBreak(Qt::TopToolBarArea);
  addToolBar(Qt::TopToolBarArea, mToolBarCommand.data());
  mCommandToolBarProxy->setToolBar(mToolBarCommand.data());

  // Tools.
  mToolBarTools.reset(new QToolBar(tr("Tools"), this));
  mToolBarTools->setObjectName("toolBarTools");
  mToolBarTools->addAction(mActionToolSelect.data());
  mToolBarTools->addAction(mActionToolTrace.data());
  mToolBarTools->addAction(mActionToolVia.data());
  mToolBarTools->addAction(mActionToolPolygon.data());
  mToolBarTools->addAction(mActionToolText.data());
  mToolBarTools->addAction(mActionToolPlane.data());
  mToolBarTools->addAction(mActionToolZone.data());
  mToolBarTools->addAction(mActionToolHole.data());
  mToolBarTools->addSeparator();
  mToolBarTools->addAction(mActionToolMeasure.data());
  mToolBarTools->addAction(mActionRunQuickCheck.data());
  mToolBarTools->addAction(mActionRunDesignRuleCheck.data());
  addToolBar(Qt::LeftToolBarArea, mToolBarTools.data());
}

void BoardEditor::createDockWidgets() noexcept {
  // Unplaced components.
  mDockUnplacedComponents.reset(new UnplacedComponentsDock(mProjectEditor));
  connect(mDockUnplacedComponents.data(),
          &UnplacedComponentsDock::unplacedComponentsCountChanged, this,
          &BoardEditor::unplacedComponentsCountChanged);
  connect(mDockUnplacedComponents.data(),
          &UnplacedComponentsDock::addDeviceTriggered, mFsm.data(),
          &BoardEditorFsm::processAddDevice);
  addDockWidget(Qt::RightDockWidgetArea, mDockUnplacedComponents.data(),
                Qt::Vertical);

  // Layers.
  mDockLayers.reset(new BoardLayersDock(*mLayers));
  addDockWidget(Qt::RightDockWidgetArea, mDockLayers.data(), Qt::Vertical);
  tabifyDockWidget(mDockUnplacedComponents.data(), mDockLayers.data());

  // ERC Messages.
  mDockErc.reset(
      new RuleCheckDock(RuleCheckDock::Mode::ElectricalRuleCheck, this));
  mDockErc->setObjectName("dockErc");
  mDockErc->setApprovals(mProject.getErcMessageApprovals());
  connect(&mProject, &Project::ercMessageApprovalsChanged, mDockErc.data(),
          &RuleCheckDock::setApprovals);
  connect(mDockErc.data(), &RuleCheckDock::messageApprovalRequested,
          &mProjectEditor, &ProjectEditor::setErcMessageApproved);
  connect(&mProjectEditor, &ProjectEditor::ercFinished, mDockErc.data(),
          &RuleCheckDock::setMessages);
  addDockWidget(Qt::RightDockWidgetArea, mDockErc.data(), Qt::Vertical);
  tabifyDockWidget(mDockLayers.data(), mDockErc.data());

  // DRC Messages.
  mDockDrc.reset(
      new RuleCheckDock(RuleCheckDock::Mode::BoardDesignRuleCheck, this));
  mDockDrc->setObjectName("dockDrc");
  mDockDrc->setInteractive(false);
  connect(mDockDrc.data(), &RuleCheckDock::settingsDialogRequested, this,
          [this]() { execBoardSetupDialog(true); });
  connect(mDockDrc.data(), &RuleCheckDock::runDrcRequested, this,
          [this]() { runDrc(false); });
  connect(mDockDrc.data(), &RuleCheckDock::runQuickCheckRequested, this,
          [this]() { runDrc(true); });
  connect(mDockDrc.data(), &RuleCheckDock::messageSelected, this,
          &BoardEditor::highlightDrcMessage);
  connect(mDockDrc.data(), &RuleCheckDock::messageApprovalRequested, this,
          &BoardEditor::setDrcMessageApproved);
  addDockWidget(Qt::RightDockWidgetArea, mDockDrc.data());
  tabifyDockWidget(mDockErc.data(), mDockDrc.data());

  // By default, open the unplaced components dock.
  mDockUnplacedComponents->raise();
}

void BoardEditor::createMenus() noexcept {
  MenuBuilder mb(mUi->menuBar);

  // File.
  mb.newMenu(&MenuBuilder::createFileMenu);
  mb.addAction(mActionSaveProject);
  mb.addAction(mActionFileManager);
  mb.addSeparator();
  {
    MenuBuilder smb(mb.addSubMenu(&MenuBuilder::createImportMenu));
    smb.addAction(mActionImportDxf);
    smb.addAction(mActionImportSpecctra);
  }
  {
    MenuBuilder smb(mb.addSubMenu(&MenuBuilder::createExportMenu));
    smb.addAction(mActionExportPdf);
    smb.addAction(mActionExportImage);
    smb.addAction(mActionExportStep);
    smb.addAction(mActionExportSpecctra);
    smb.addAction(mActionExportLppz);
  }
  {
    MenuBuilder smb(mb.addSubMenu(&MenuBuilder::createProductionDataMenu));
    smb.addAction(mActionGenerateBom);
    smb.addAction(mActionGenerateFabricationData);
    smb.addAction(mActionGeneratePickPlace);
    smb.addAction(mActionGenerateD356Netlist);
  }
  mb.addAction(mActionOutputJobs);
  mb.addSeparator();
  mb.addAction(mActionPrint);
  mb.addAction(mActionOrderPcb);
  mb.addSeparator();
  mb.addAction(mActionCloseWindow);
  mb.addAction(mActionCloseProject);
  mb.addSeparator();
  mb.addAction(mActionQuit);

  // Edit.
  mb.newMenu(&MenuBuilder::createEditMenu);
  mb.addAction(mActionUndo);
  mb.addAction(mActionRedo);
  mb.addSeparator();
  mb.addAction(mActionSelectAll);
  mb.addSeparator();
  mb.addAction(mActionCut);
  mb.addAction(mActionCopy);
  mb.addAction(mActionPaste);
  mb.addAction(mActionRemove);
  mb.addSeparator();
  mb.addAction(mActionRotateCcw);
  mb.addAction(mActionRotateCw);
  mb.addAction(mActionFlipHorizontal);
  mb.addAction(mActionFlipVertical);
  mb.addAction(mActionSnapToGrid);
  mb.addAction(mActionLock);
  mb.addAction(mActionUnlock);
  mb.addAction(mActionResetAllTexts);
  {
    MenuBuilder smb(mb.addSubMenu(&MenuBuilder::createLineWidthMenu));
    smb.addAction(mActionIncreaseLineWidth);
    smb.addAction(mActionDecreaseLineWidth);
    smb.addAction(mActionChangeLineWidth);
  }
  mb.addSeparator();
  mb.addAction(mActionFind);
  mb.addAction(mActionFindNext);
  mb.addAction(mActionFindPrevious);
  mb.addSeparator();
  mb.addAction(mActionProperties);

  // View.
  mb.newMenu(&MenuBuilder::createViewMenu);
  mb.addAction(mActionGridProperties);
  mb.addAction(mActionGridIncrease.data());
  mb.addAction(mActionGridDecrease.data());
  mb.addSeparator();
  mb.addAction(mActionIgnoreLocks);
  mb.addSeparator();
  mb.addAction(mActionHidePlanes);
  mb.addAction(mActionShowPlanes);
  mb.addSeparator();
  mb.addAction(mActionZoomIn);
  mb.addAction(mActionZoomOut);
  mb.addAction(mActionZoomFit);
  mb.addSeparator();
  mb.addAction(mActionToggle3D);
  mb.addSeparator();
  {
    MenuBuilder smb(mb.addSubMenu(&MenuBuilder::createGoToDockMenu));
    smb.addAction(mActionDockErc);
    smb.addAction(mActionDockDrc);
    smb.addAction(mActionDockLayers);
    smb.addAction(mActionDockPlaceDevices);
  }
  {
    MenuBuilder smb(mb.addSubMenu(&MenuBuilder::createDocksVisibilityMenu));
    smb.addAction(mDockUnplacedComponents->toggleViewAction());
    smb.addAction(mDockLayers->toggleViewAction());
    smb.addAction(mDockErc->toggleViewAction());
    smb.addAction(mDockDrc->toggleViewAction());
  }

  // Board.
  mMenuBoard = mb.newMenu(&MenuBuilder::createBoardMenu);
  mb.addAction(mActionBoardSetup);
  mb.addSeparator();
  mb.addAction(mActionRebuildPlanes);
  mb.addAction(mActionRunQuickCheck);
  mb.addAction(mActionRunDesignRuleCheck);
  mb.addSeparator();
  mb.addAction(mActionNewBoard);
  mb.addAction(mActionCopyBoard);
  mb.addAction(mActionRemoveBoard);
  mb.addSection(tr("Boards"));
  // Boards will be added here, see updateBoardActionGroup().

  // Project.
  mb.newMenu(&MenuBuilder::createProjectMenu);
  mb.addAction(mActionProjectSetup);
  mb.addSeparator();
  mb.addAction(mActionUpdateLibrary);

  // Tools.
  mb.newMenu(&MenuBuilder::createToolsMenu);
  mb.addAction(mActionToolSelect);
  mb.addAction(mActionToolTrace);
  mb.addAction(mActionToolVia);
  mb.addAction(mActionToolPolygon);
  mb.addAction(mActionToolText);
  mb.addAction(mActionToolPlane);
  mb.addAction(mActionToolZone);
  mb.addAction(mActionToolHole);
  mb.addSeparator();
  mb.addAction(mActionToolMeasure);

  // Help.
  mb.newMenu(&MenuBuilder::createHelpMenu);
  mb.addAction(mActionOnlineDocumentation);
  mb.addAction(mActionKeyboardShortcutsReference);
  mb.addAction(mActionWebsite);
  mb.addSeparator();
  mb.addAction(mActionAboutLibrePcb);
  mb.addAction(mActionAboutQt);
}

void BoardEditor::updateBoardActionGroup() noexcept {
  mBoardActionGroup.reset(new QActionGroup(this));
  connect(&mProject, &Project::boardAdded, this,
          &BoardEditor::updateBoardActionGroup);
  connect(&mProject, &Project::boardRemoved, this,
          &BoardEditor::updateBoardActionGroup);
  for (int i = 0; i < mProject.getBoards().count(); ++i) {
    const Board* board = mProject.getBoardByIndex(i);
    QAction* action = mBoardActionGroup->addAction(*board->getName());
    action->setCheckable(true);
    action->setChecked(board == mActiveBoard.data());
    mMenuBoard->addAction(action);
  }
  connect(mBoardActionGroup.data(), &QActionGroup::triggered, this,
          [this](QAction* action) {
            setActiveBoardIndex(mBoardActionGroup->actions().indexOf(action));
          });
}

bool BoardEditor::graphicsViewEventHandler(QEvent* event) {
  Q_ASSERT(event);
  switch (event->type()) {
    case QEvent::GraphicsSceneMouseMove: {
      auto* e = dynamic_cast<QGraphicsSceneMouseEvent*>(event);
      Q_ASSERT(e);
      mFsm->processGraphicsSceneMouseMoved(*e);
      break;
    }

    case QEvent::GraphicsSceneMousePress: {
      auto* e = dynamic_cast<QGraphicsSceneMouseEvent*>(event);
      Q_ASSERT(e);
      switch (e->button()) {
        case Qt::LeftButton: {
          clearDrcMarker();  // clear DRC location on click
          mFsm->processGraphicsSceneLeftMouseButtonPressed(*e);
          break;
        }
        default: {
          break;
        }
      }
      break;
    }

    case QEvent::GraphicsSceneMouseRelease: {
      auto* e = dynamic_cast<QGraphicsSceneMouseEvent*>(event);
      Q_ASSERT(e);
      switch (e->button()) {
        case Qt::LeftButton: {
          mFsm->processGraphicsSceneLeftMouseButtonReleased(*e);
          break;
        }
        case Qt::RightButton: {
          mFsm->processGraphicsSceneRightMouseButtonReleased(*e);
          break;
        }
        default: {
          break;
        }
      }
      break;
    }

    case QEvent::GraphicsSceneMouseDoubleClick: {
      auto* e = dynamic_cast<QGraphicsSceneMouseEvent*>(event);
      Q_ASSERT(e);
      switch (e->button()) {
        case Qt::LeftButton: {
          mFsm->processGraphicsSceneLeftMouseButtonDoubleClicked(*e);
          break;
        }
        default: {
          break;
        }
      }
      break;
    }

    case QEvent::KeyPress: {
      auto* e = dynamic_cast<QKeyEvent*>(event);
      Q_ASSERT(e);
      if (mFsm->processKeyPressed(*e)) {
        return true;
      }
      switch (e->key()) {
        case Qt::Key_Left:
        case Qt::Key_Right:
        case Qt::Key_Up:
        case Qt::Key_Down:
          // Allow handling these keys by the graphics view for scrolling.
          return false;
        default:
          break;
      }
      break;
    }

    case QEvent::KeyRelease: {
      auto* e = dynamic_cast<QKeyEvent*>(event);
      Q_ASSERT(e);
      mFsm->processKeyReleased(*e);
      break;
    }

    default: {
      break;
    }
  }

  // Always accept graphics scene events, even if we do not react on some of
  // the events! This will give us the full control over the graphics scene.
  // Otherwise, the graphics scene can react on some events and disturb our
  // state machine. Only the wheel event is ignored because otherwise the
  // view will not allow to zoom with the mouse wheel.
  return (event->type() != QEvent::GraphicsSceneWheel);
}

void BoardEditor::toolRequested(const QVariant& newTool) noexcept {
  switch (newTool.toInt()) {
    case BoardEditorFsm::State::SELECT:
      mFsm->processSelect();
      break;
    case BoardEditorFsm::State::DRAW_TRACE:
      mFsm->processDrawTrace();
      break;
    case BoardEditorFsm::State::DRAW_POLYGON:
      mFsm->processDrawPolygon();
      break;
    case BoardEditorFsm::State::DRAW_PLANE:
      mFsm->processDrawPlane();
      break;
    case BoardEditorFsm::State::DRAW_ZONE:
      mFsm->processDrawZone();
      break;
    case BoardEditorFsm::State::ADD_VIA:
      mFsm->processAddVia();
      break;
    case BoardEditorFsm::State::ADD_STROKE_TEXT:
      mFsm->processAddStrokeText();
      break;
    case BoardEditorFsm::State::ADD_HOLE:
      mFsm->processAddHole();
      break;
    case BoardEditorFsm::State::MEASURE:
      mFsm->processMeasure();
      break;
    default:
      qCritical() << "Unhandled switch-case in "
                     "BoardEditor::toolActionGroupChangeTriggered():"
                  << newTool;
      break;
  }
}

void BoardEditor::unplacedComponentsCountChanged(int count) noexcept {
  mUi->lblUnplacedComponentsNote->setVisible(count > 0);
}

void BoardEditor::runDrc(bool quick) noexcept {
  try {
    Board* board = getActiveBoard();
    if (!board) return;

    // Make sure the DRC dock is visible because of the progress bar.
    mDockDrc->show();
    mDockDrc->raise();

    // Set UI into busy state during the checks.
    setCursor(Qt::WaitCursor);
    bool wasInteractive = mDockDrc->setInteractive(false);
    auto busyScopeGuard = scopeGuard([this, wasInteractive]() {
      mDockDrc->setInteractive(wasInteractive);
      unsetCursor();
    });

    // Run the DRC.
    BoardDesignRuleCheck drc;
    connect(&drc, &BoardDesignRuleCheck::progressPercent, mDockDrc.data(),
            &RuleCheckDock::setProgressPercent);
    connect(&drc, &BoardDesignRuleCheck::progressStatus, mDockDrc.data(),
            &RuleCheckDock::setProgressStatus);
    drc.start(*board, board->getDrcSettings(), quick);  // can throw
    const BoardDesignRuleCheck::Result result = drc.waitForFinished();

    // Update DRC messages.
    clearDrcMarker();
    mDrcMessages.insert(board->getUuid(), result.messages);
    mDockDrc->setMessages(result.messages);

    // Detect & remove disappeared messages.
    const QSet<SExpression> approvals =
        RuleCheckMessage::getAllApprovals(result.messages);
    if (board->updateDrcMessageApprovals(approvals, quick)) {
      mDockDrc->setApprovals(board->getDrcMessageApprovals());
      mProjectEditor.setManualModificationsMade();
    }
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
  }
}

void BoardEditor::highlightDrcMessage(const RuleCheckMessage& msg,
                                      bool zoomTo) noexcept {
  if (msg.getLocations().isEmpty()) {
    // Position on board not known.
    clearDrcMarker();
  } else if (mGraphicsScene) {
    const ThemeColor& color =
        mProjectEditor.getWorkspace().getSettings().themes.getActive().getColor(
            Theme::Color::sBoardOverlays);
    QPainterPath path = Path::toQPainterPathPx(msg.getLocations(), true);
    mDrcLocationGraphicsItem.reset(new QGraphicsPathItem());
    mDrcLocationGraphicsItem->setZValue(BoardGraphicsScene::ZValue_AirWires);
    mDrcLocationGraphicsItem->setPen(QPen(color.getPrimaryColor(), 0));
    mDrcLocationGraphicsItem->setBrush(color.getSecondaryColor());
    mDrcLocationGraphicsItem->setPath(path);
    mGraphicsScene->addItem(*mDrcLocationGraphicsItem.data());

    qreal margin = Length(1000000).toPx();
    QRectF rect = path.boundingRect();
    rect.adjust(-margin, -margin, margin, margin);
    mGraphicsScene->setSceneRectMarker(rect);
    if (zoomTo) {
      mUi->graphicsView->zoomToRect(rect);
    }
  }
}

void BoardEditor::setDrcMessageApproved(const RuleCheckMessage& msg,
                                        bool approved) noexcept {
  if (Board* board = getActiveBoard()) {
    board->setDrcMessageApproved(msg.getApproval(), approved);
    mDockDrc->setApprovals(board->getDrcMessageApprovals());
    mProjectEditor.setManualModificationsMade();
  }
}

void BoardEditor::clearDrcMarker() noexcept {
  mDrcLocationGraphicsItem.reset();
  if (mGraphicsScene) {
    mGraphicsScene->setSceneRectMarker(QRectF());
  }
}

QList<BI_Device*> BoardEditor::getSearchCandidates() noexcept {
  QList<BI_Device*> candidates;
  if (Board* board = getActiveBoard()) {
    candidates += board->getDeviceInstances().values();
  }
  return candidates;
}

QStringList BoardEditor::getSearchToolBarCompleterList() noexcept {
  QStringList list;
  foreach (const BI_Device* device, getSearchCandidates()) {
    list.append(*device->getComponentInstance().getName());
  }
  return list;
}

void BoardEditor::goToDevice(const QString& name, int index) noexcept {
  QList<BI_Device*> deviceCandidates;
  foreach (BI_Device* device, getSearchCandidates()) {
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

  if ((!deviceCandidates.isEmpty()) && mGraphicsScene) {
    mGraphicsScene->clearSelection();
    while (index < 0) {
      index += deviceCandidates.count();
    }
    index %= deviceCandidates.count();
    BI_Device* device = deviceCandidates[index];
    if (auto item = mGraphicsScene->getDevices().value(device)) {
      item->setSelected(true);
      QRectF rect = item->mapRectToScene(item->childrenBoundingRect());
      // Zoom to a rectangle relative to the maximum graphics item dimension,
      // occupying 1/4th of the screen, but limiting the margin to 10mm.
      const qreal margin =
          std::min(1.5f * std::max(rect.size().width(), rect.size().height()),
                   Length::fromMm(10).toPx());
      rect.adjust(-margin, -margin, margin, margin);
      mUi->graphicsView->zoomToRect(rect);
    }
  }
}

void BoardEditor::scheduleOpenGlSceneUpdate() noexcept {
  mOpenGlSceneBuildScheduled = true;
}

void BoardEditor::performScheduledTasks() noexcept {
  const bool commandActive =
      mProjectEditor.getUndoStack().isCommandGroupActive() ||
      mUi->graphicsView->isMouseButtonPressed(Qt::LeftButton |
                                              Qt::MiddleButton);
  const bool userInputIdle = (mUi->graphicsView->getIdleTimeMs() >= 700);
  const bool updateAllowedInCurrentState = ((!commandActive) || userInputIdle);

  // Rebuild planes, if needed. Depending on various conditions to avoid too
  // high CPU load caused by too frequent plane rebuilds.
  const qint64 planeBuildPauseMs =
      QDateTime::currentMSecsSinceEpoch() - mTimestampOfLastPlaneRebuild;
  if (mPlaneFragmentsBuilder && (!mPlaneFragmentsBuilder->isBusy()) &&
      updateAllowedInCurrentState && (planeBuildPauseMs >= 1000) &&
      isActiveTopLevelWindow()) {
    startPlaneRebuild(false);
  }

  // Update 3D scene, if needed.
  const bool planesRebuilding =
      mPlaneFragmentsBuilder && mPlaneFragmentsBuilder->isBusy();
  const qint64 openGlBuildPauseMs =
      QDateTime::currentMSecsSinceEpoch() - mTimestampOfLastOpenGlSceneRebuild;
  if ((!planesRebuilding) && (mOpenGlSceneBuildScheduled || commandActive) &&
      mOpenGlView && mOpenGlSceneBuilder && (!mOpenGlSceneBuilder->isBusy()) &&
      updateAllowedInCurrentState && (openGlBuildPauseMs >= 1000) &&
      isActiveTopLevelWindow()) {
    std::shared_ptr<SceneData3D> data;
    if (Board* board = getActiveBoard()) {
      auto av = mProject.getCircuit().getAssemblyVariants().value(0);
      data = board->buildScene3D(av ? std::make_optional(av->getUuid())
                                    : std::nullopt);
    } else {
      data = std::make_shared<SceneData3D>();
    }
    mOpenGlSceneBuildScheduled = false;
    mOpenGlSceneBuilder->start(data);
  }
}

void BoardEditor::startPlaneRebuild(bool full) noexcept {
  Board* board = getActiveBoard();
  if (board && mPlaneFragmentsBuilder) {
    if (full) {
      // Forced rebuild -> all layers.
      mPlaneFragmentsBuilder->start(*board);
    } else {
      // Automatic rebuild -> only modified & visible layers. However, if the
      // 3D view is open, all planes on outer layers are visible!
      QSet<const Layer*> layers;
      foreach (const Layer* layer, board->getCopperLayers()) {
        if (auto graphicsLayer = mLayers->get(*layer)) {
          if (graphicsLayer->isVisible() ||
              (mOpenGlView && (layer->isTop() || layer->isBottom()))) {
            layers.insert(layer);
          }
        }
      }
      mPlaneFragmentsBuilder->start(*board, &layers);
    }
  }
}

bool BoardEditor::isActiveTopLevelWindow() const noexcept {
  if (isActiveWindow()) {
    return true;
  }
  QWidget* w = QApplication::activeWindow();
  while (w) {
    if (w == this) {
      return true;
    }
    w = w->parentWidget();
  }
  if ((mUi->graphicsView->getIdleTimeMs() < 2000) ||
      (mOpenGlView && (mOpenGlView->getIdleTimeMs() < 2000))) {
    return true;  // Safe fallback if active window detection is not reliable.
  }
  return false;
}

void BoardEditor::newBoard() noexcept {
  bool ok = false;
  QString name =
      QInputDialog::getText(this, tr("Add New Board"), tr("Choose a name:"),
                            QLineEdit::Normal, tr("new_board"), &ok);
  if (!ok) return;

  try {
    const QString dirName = FilePath::cleanFileName(
        name, FilePath::ReplaceSpaces | FilePath::ToLowerCase);
    if (dirName.isEmpty()) {
      throw RuntimeError(__FILE__, __LINE__,
                         tr("Invalid name: '%1'").arg(name));
    }

    abortBlockingToolsInOtherEditors();  // Release undo stack.
    CmdBoardAdd* cmd =
        new CmdBoardAdd(mProject, dirName, ElementName(name));  // can throw
    mProjectEditor.getUndoStack().execCmd(cmd);
    setActiveBoardIndex(mProject.getBoardIndex(*cmd->getBoard()));
  } catch (Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
  }
}

void BoardEditor::copyBoard() noexcept {
  Board* board = getActiveBoard();
  if (!board) return;

  bool ok = false;
  QString name = QInputDialog::getText(
      this, tr("Copy Board"), tr("Choose a name:"), QLineEdit::Normal,
      tr("copy_of_%1").arg(*board->getName()), &ok);
  if (!ok) return;

  try {
    const QString dirName = FilePath::cleanFileName(
        name, FilePath::ReplaceSpaces | FilePath::ToLowerCase);
    if (dirName.isEmpty()) {
      throw RuntimeError(__FILE__, __LINE__,
                         tr("Invalid name: '%1'").arg(name));
    }

    abortBlockingToolsInOtherEditors();  // Release undo stack.
    CmdBoardAdd* cmd = new CmdBoardAdd(mProject, dirName, ElementName(name),
                                       board);  // can throw
    mProjectEditor.getUndoStack().execCmd(cmd);
    setActiveBoardIndex(mProject.getBoardIndex(*cmd->getBoard()));
  } catch (Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
  }
}

void BoardEditor::removeBoard() noexcept {
  Board* board = getActiveBoard();
  if (!board) return;

  QMessageBox::StandardButton btn = QMessageBox::question(
      this, tr("Remove board"),
      tr("Are you really sure to remove the board \"%1\"?")
          .arg(*board->getName()));
  if (btn != QMessageBox::Yes) return;

  try {
    abortBlockingToolsInOtherEditors();  // Release undo stack.
    mProjectEditor.getUndoStack().execCmd(new CmdBoardRemove(*board));
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
  }
}

void BoardEditor::setGridProperties(const PositiveLength& interval,
                                    const LengthUnit& unit,
                                    Theme::GridStyle style,
                                    bool applyToBoard) noexcept {
  if (mGraphicsScene) {
    mGraphicsScene->setGridInterval(interval);
    mGraphicsScene->setGridStyle(style);
  }

  mUi->statusbar->setLengthUnit(unit);

  // In contrast to schematics, apply the grid only on the currently active
  // board instead of all, so we can use different grids for each board.
  Board* activeBoard = getActiveBoard();
  if (activeBoard && applyToBoard) {
    activeBoard->setGridInterval(interval);
    activeBoard->setGridUnit(unit);
  }
}

void BoardEditor::execGridPropertiesDialog() noexcept {
  Board* board = getActiveBoard();
  if (board && mGraphicsScene) {
    GridSettingsDialog dialog(board->getGridInterval(), board->getGridUnit(),
                              mGraphicsScene->getGridStyle(), this);
    connect(&dialog, &GridSettingsDialog::gridPropertiesChanged,
            [this](const PositiveLength& interval, const LengthUnit& unit,
                   Theme::GridStyle style) {
              setGridProperties(interval, unit, style, false);
            });
    if (dialog.exec()) {
      setGridProperties(dialog.getInterval(), dialog.getUnit(),
                        dialog.getStyle(), true);
    }
  }
}

void BoardEditor::execBoardSetupDialog(bool switchToDrcSettings) noexcept {
  if (Board* board = getActiveBoard()) {
    abortBlockingToolsInOtherEditors();  // Release undo stack.
    BoardSetupDialog dialog(*board, mProjectEditor.getUndoStack(), this);
    if (switchToDrcSettings) {
      dialog.openDrcSettingsTab();
    }
    dialog.exec();
  }
}

void BoardEditor::execGraphicsExportDialog(
    GraphicsExportDialog::Output output, const QString& settingsKey) noexcept {
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
    QList<std::shared_ptr<GraphicsPagePainter>> pages;
    if (mActiveBoard) {
      QProgressDialog progress(tr("Preparing board..."), tr("Cancel"), 0, 1,
                               this);
      progress.setWindowModality(Qt::WindowModal);
      progress.setMinimumDuration(100);
      pages.append(std::make_shared<BoardPainter>(*mActiveBoard));
      progress.setValue(1);
      if (progress.wasCanceled()) {
        return;
      }
    }

    // Show dialog, which will do all the work.
    GraphicsExportDialog dialog(
        GraphicsExportDialog::Mode::Board, output, pages, 0,
        *mProject.getName(),
        mActiveBoard ? mActiveBoard->getInnerLayerCount() : 0, defaultFilePath,
        mProjectEditor.getWorkspace().getSettings().defaultLengthUnit.get(),
        mProjectEditor.getWorkspace().getSettings().themes.getActive(),
        "board_editor/" % settingsKey, this);
    connect(&dialog, &GraphicsExportDialog::requestOpenFile, this,
            [this](const FilePath& fp) {
              DesktopServices ds(mProjectEditor.getWorkspace().getSettings());
              ds.openLocalPath(fp);
            });
    dialog.exec();
  } catch (const Exception& e) {
    QMessageBox::warning(this, tr("Error"), e.getMsg());
  }
}

void BoardEditor::execStepExportDialog() noexcept {
  Board* board = getActiveBoard();
  if (!board) return;

  // Determine default file path.
  const QString projectName = FilePath::cleanFileName(
      *mProject.getName(), FilePath::ReplaceSpaces | FilePath::KeepCase);
  const QString projectVersion = FilePath::cleanFileName(
      *mProject.getVersion(), FilePath::ReplaceSpaces | FilePath::KeepCase);
  const FilePath defaultFilePath = mProject.getPath().getPathTo(
      QString("output/%1/%2.step").arg(projectVersion, projectName));

  // Ask for file path.
  const FilePath fp(FileDialog::getSaveFileName(this, tr("Export STEP Model"),
                                                defaultFilePath.toStr(),
                                                "STEP Models (*.step *.stp)"));
  if (!fp.isValid()) {
    return;
  }

  // Build data.
  auto av = mProject.getCircuit().getAssemblyVariants().value(0);
  auto data = board->buildScene3D(av ? std::make_optional(av->getUuid())
                                     : std::nullopt);

  // Start export.
  StepExport exp;
  QProgressDialog dlg(this);
  dlg.setAutoClose(false);
  dlg.setAutoReset(false);
  connect(&exp, &StepExport::progressStatus, &dlg,
          &QProgressDialog::setLabelText);
  connect(&exp, &StepExport::progressPercent, &dlg, &QProgressDialog::setValue);
  connect(&exp, &StepExport::finished, &dlg, &QProgressDialog::close);
  connect(&dlg, &QProgressDialog::canceled, &exp, &StepExport::cancel);
  exp.start(data, fp, 700);
  dlg.exec();
  const QString errorMsg = exp.waitForFinished();
  if (!errorMsg.isEmpty()) {
    QMessageBox::critical(this, tr("STEP Export Failure"), errorMsg);
  }
}

void BoardEditor::execD356NetlistExportDialog() noexcept {
  Board* board = getActiveBoard();
  if (!board) return;

  try {
    QString path = "output/{{VERSION}}/{{PROJECT}}_Netlist.d356";
    path = AttributeSubstitutor::substitute(
        path, ProjectAttributeLookup(*board, nullptr), [&](const QString& str) {
          return FilePath::cleanFileName(
              str, FilePath::ReplaceSpaces | FilePath::KeepCase);
        });
    path = FileDialog::getSaveFileName(
        this, tr("Export IPC D-356A Netlist"),
        mProject.getPath().getPathTo(path).toStr(), "*.d356");
    if (path.isEmpty()) return;
    if (!path.contains(".")) path.append(".d356");

    FilePath fp(path);
    qDebug().nospace() << "Export IPC D-356A netlist to " << fp.toNative()
                       << "...";
    BoardD356NetlistExport exp(*board);
    FileUtils::writeFile(fp, exp.generate());  // can throw
    qDebug() << "Successfully exported netlist.";
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
  }
}

void BoardEditor::execSpecctraExportDialog() noexcept {
  Board* board = getActiveBoard();
  if (!board) return;

  try {
    // Default file path.
    QString path = "output/{{VERSION}}/{{PROJECT}}";
    if (mProject.getBoards().count() > 1) {
      path += "_{{BOARD}}";
    }
    path += ".dsn";
    path = AttributeSubstitutor::substitute(
        path, ProjectAttributeLookup(*board, nullptr), [&](const QString& str) {
          return FilePath::cleanFileName(
              str, FilePath::ReplaceSpaces | FilePath::KeepCase);
        });

    // Use memorized file path, if board path and version number match.
    QSettings cs;
    const QString csId =
        board->getDirectory().getAbsPath().toStr() + *mProject.getVersion();
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
        this, EditorCommandSet::instance().exportSpecctraDsn.getDisplayText(),
        path, "*.dsn");
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
    BoardSpecctraExport exp(*board);
    FileUtils::writeFile(fp, exp.generate());  // can throw
    qDebug() << "Successfully exported Specctra DSN.";
    mUi->statusbar->showMessage(tr("Success!"), 3000);
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
  }
}

void BoardEditor::execSpecctraImportDialog() noexcept {
  Board* board = getActiveBoard();
  if (!board) return;

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
        board->getDirectory().getAbsPath().toStr() + *mProject.getVersion();
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
        this, EditorCommandSet::instance().importSpecctraSes.getDisplayText(),
        path, "*.ses;;*");
    if (path.isEmpty()) return;
    const FilePath fp(path);

    // Set UI into busy state during the import.
    setCursor(Qt::WaitCursor);
    auto busyScopeGuard = scopeGuard([this]() { unsetCursor(); });

    // Perform import.
    qDebug().nospace() << "Import Specctra SES from " << fp.toNative() << "...";
    logger->debug(tr("Parsing Specctra session '%1'...").arg(fp.toNative()));
    const QByteArray content = FileUtils::readFile(fp);  // can throw
    std::unique_ptr<SExpression> root =
        SExpression::parse(content, fp, SExpression::Mode::Permissive);
    mProjectEditor.getUndoStack().execCmd(
        new CmdBoardSpecctraImport(*board, *root, logger));  // can throw
    qDebug() << "Successfully imported Specctra SES.";
  } catch (const Exception& e) {
    logger->critical(e.getMsg());
    logger->critical(tr("Import failed, no changes made to the board."));
  }

  // Display messages.
  QDialog dlg(this);
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

bool BoardEditor::show3DView() noexcept {
  if (!mOpenGlView) {
    mOpenGlView.reset(new OpenGlView(this));
    mUi->mainLayout->insertWidget(2, mOpenGlView.data(), 1);
    mOpenGlSceneBuilder.reset(new OpenGlSceneBuilder());
    connect(mOpenGlSceneBuilder.data(), &OpenGlSceneBuilder::started,
            mOpenGlView.data(), &OpenGlView::startSpinning);
    connect(mOpenGlSceneBuilder.data(), &OpenGlSceneBuilder::finished,
            mOpenGlView.data(), &OpenGlView::stopSpinning);
    connect(mOpenGlSceneBuilder.data(), &OpenGlSceneBuilder::finished, this,
            [this]() {
              mTimestampOfLastOpenGlSceneRebuild =
                  QDateTime::currentMSecsSinceEpoch();
            });
    connect(mOpenGlSceneBuilder.data(), &OpenGlSceneBuilder::objectAdded,
            mOpenGlView.data(), &OpenGlView::addObject);
    connect(mOpenGlSceneBuilder.data(), &OpenGlSceneBuilder::objectRemoved,
            mOpenGlView.data(), &OpenGlView::removeObject);
    connect(mOpenGlSceneBuilder.data(), &OpenGlSceneBuilder::objectUpdated,
            mOpenGlView.data(),
            static_cast<void (OpenGlView::*)()>(&OpenGlView::update));
    scheduleOpenGlSceneUpdate();
    mUi->btnHide3D->setEnabled(true);
    return true;
  } else if (mUi->graphicsView->isVisible()) {
    mUi->graphicsView->hide();
    mUi->btnShow3D->setEnabled(false);
    return true;
  }
  return false;
}

void BoardEditor::hide3DView() noexcept {
  if (!mUi->graphicsView->isVisible()) {
    mUi->graphicsView->show();
    mUi->btnShow3D->setEnabled(true);
  } else {
    mOpenGlView.reset();
    mUi->btnHide3D->setEnabled(false);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
