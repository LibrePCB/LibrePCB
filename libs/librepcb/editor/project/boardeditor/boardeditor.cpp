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

#include "../../dialogs/filedialog.h"
#include "../../dialogs/gridsettingsdialog.h"
#include "../../editorcommandset.h"
#include "../../project/cmd/cmdboardadd.h"
#include "../../project/cmd/cmdboarddesignrulesmodify.h"
#include "../../project/cmd/cmdboardremove.h"
#include "../../undostack.h"
#include "../../utils/exclusiveactiongroup.h"
#include "../../utils/menubuilder.h"
#include "../../utils/standardeditorcommandhandler.h"
#include "../../utils/toolbarproxy.h"
#include "../../utils/undostackactiongroup.h"
#include "../../widgets/graphicsview.h"
#include "../../widgets/searchtoolbar.h"
#include "../../workspace/desktopservices.h"
#include "../bomgeneratordialog.h"
#include "../erc/ercmsgdock.h"
#include "../projecteditor.h"
#include "../projectpropertieseditordialog.h"
#include "boarddesignrulecheckdialog.h"
#include "boarddesignrulecheckmessagesdock.h"
#include "boarddesignrulesdialog.h"
#include "boardlayersdock.h"
#include "boardlayerstacksetupdialog.h"
#include "boardpickplacegeneratordialog.h"
#include "fabricationoutputdialog.h"
#include "fsm/boardeditorfsm.h"
#include "unplacedcomponentsdock.h"

#include <librepcb/core/application.h>
#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/graphics/graphicsscene.h>
#include <librepcb/core/graphics/primitivepathgraphicsitem.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/boardlayerstack.h>
#include <librepcb/core/project/board/boardpainter.h>
#include <librepcb/core/project/board/items/bi_device.h>
#include <librepcb/core/project/board/items/bi_footprint.h>
#include <librepcb/core/project/board/items/bi_plane.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/circuit/componentinstance.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/projectmetadata.h>
#include <librepcb/core/project/projectsettings.h>
#include <librepcb/core/types/gridproperties.h>
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
    mFsm() {
  mUi->setupUi(this);
  mUi->tabBar->setDocumentMode(true);  // For MacOS
  mUi->lblUnplacedComponentsNote->hide();

  // Setup graphics view.
  mUi->graphicsView->setBackgroundBrush(Qt::black);
  mUi->graphicsView->setForegroundBrush(Qt::white);
  mUi->graphicsView->setOverlayColor(Qt::yellow);
  mUi->graphicsView->setRulerColor(Qt::yellow);
  mUi->graphicsView->setUseOpenGl(
      mProjectEditor.getWorkspace().getSettings().useOpenGl.get());
  mUi->graphicsView->setEventHandlerObject(this);
  connect(mUi->graphicsView, &GraphicsView::cursorScenePositionChanged,
          mUi->statusbar, &StatusBar::setAbsoluteCursorPosition);

  // Setup status bar.
  mUi->statusbar->setFields(StatusBar::AbsolutePosition |
                            StatusBar::ProgressBar);
  mUi->statusbar->setProgressBarTextFormat(tr("Scanning libraries (%p%)"));
  connect(&mProjectEditor.getWorkspace().getLibraryDb(),
          &WorkspaceLibraryDb::scanProgressUpdate, mUi->statusbar,
          &StatusBar::setProgressBarPercent, Qt::QueuedConnection);

  // Set window title.
  QString filenameStr = mProject.getFilepath().getFilename();
  if (!mProject.getDirectory().isWritable()) {
    filenameStr.append(QStringLiteral(" [Read-Only]"));
  }
  setWindowTitle(tr("%1 - LibrePCB Board Editor").arg(filenameStr));

  // Build the whole board editor finite state machine.
  BoardEditorFsm::Context fsmContext{mProjectEditor.getWorkspace(),
                                     mProject,
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

  // Create all actions, window menus, toolbars and dock widgets.
  createActions();
  createToolBars();
  createDockWidgets();
  createMenus();  // Depends on dock widgets!
  updateBoardActionGroup();  // Depends on menus!

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
      // save current view scene rect
      mActiveBoard->saveViewSceneRect(mUi->graphicsView->getVisibleSceneRect());
    }
    mActiveBoard = newBoard;
    if (mActiveBoard) {
      // show scene, restore view scene rect, set grid properties
      mUi->graphicsView->setScene(&mActiveBoard->getGraphicsScene());
      mUi->graphicsView->setVisibleSceneRect(
          mActiveBoard->restoreViewSceneRect());
      mUi->graphicsView->setGridProperties(mActiveBoard->getGridProperties());
      mUi->statusbar->setLengthUnit(
          mActiveBoard->getGridProperties().getUnit());
      // force airwire rebuild immediately and on every project modification
      mActiveBoard->triggerAirWiresRebuild();
      connect(&mProjectEditor.getUndoStack(), &UndoStack::stateModified,
              mActiveBoard.data(), &Board::triggerAirWiresRebuild);
    } else {
      mUi->graphicsView->setScene(nullptr);
    }

    // update dock widgets
    mDockUnplacedComponents->setBoard(mActiveBoard);
    mDockLayers->setActiveBoard(mActiveBoard);
    mDockDrc->setInteractive(mActiveBoard != nullptr);
    mDockDrc->setMessages(mActiveBoard ? mDrcMessages[mActiveBoard->getUuid()]
                                       : QList<BoardDesignRuleCheckMessage>());

    // update toolbars
    mActionGridProperties->setEnabled(mActiveBoard != nullptr);
    mActionGridIncrease->setEnabled(mActiveBoard != nullptr);
    mActionGridDecrease->setEnabled(mActiveBoard != nullptr);
  }

  // update GUI
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

/*******************************************************************************
 *  Inherited Methods
 ******************************************************************************/

void BoardEditor::closeEvent(QCloseEvent* event) {
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

void BoardEditor::createActions() noexcept {
  const EditorCommandSet& cmd = EditorCommandSet::instance();

  mActionAboutLibrePcb.reset(cmd.aboutLibrePcb.createAction(
      this, mStandardCommandHandler.data(),
      &StandardEditorCommandHandler::aboutLibrePcb));
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
      this, qApp, &Application::quitTriggered));
  mActionFileManager.reset(cmd.fileManager.createAction(this, this, [this]() {
    mStandardCommandHandler->fileManager(mProject.getPath());
  }));
  mActionSchematicEditor.reset(cmd.schematicEditor.createAction(
      this, &mProjectEditor, &ProjectEditor::showSchematicEditor));
  mActionControlPanel.reset(cmd.controlPanel.createAction(
      this, &mProjectEditor, &ProjectEditor::showControlPanelClicked));
  mActionProjectProperties.reset(
      cmd.projectProperties.createAction(this, this, [this]() {
        ProjectPropertiesEditorDialog dialog(
            mProject.getMetadata(), mProjectEditor.getUndoStack(), this);
        dialog.exec();
      }));
  mActionProjectSettings.reset(cmd.projectSettings.createAction(
      this, this,
      [this]() { mProjectEditor.execProjectSettingsDialog(this); }));
  mActionNetClasses.reset(cmd.netClasses.createAction(this, this, [this]() {
    mProjectEditor.execNetClassesEditorDialog(this);
  }));
  mActionUpdateLibrary.reset(
      cmd.projectLibraryUpdate.createAction(this, this, [this]() {
        // Ugly hack until we have a *real* project library updater...
        emit mProjectEditor.openProjectLibraryUpdaterClicked(
            mProject.getFilepath());
      }));
  mActionLayerStack.reset(cmd.layerStack.createAction(this, this, [this]() {
    try {
      if (Board* board = getActiveBoard()) {
        BoardLayerStackSetupDialog dialog(board->getLayerStack(),
                                          mProjectEditor.getUndoStack(), this);
        dialog.exec();
      }
    } catch (Exception& e) {
      QMessageBox::warning(this, tr("Error"), e.getMsg());
    }
  }));
  mActionDesignRules.reset(cmd.designRules.createAction(
      this, this, &BoardEditor::execDesignRulesDialog));
  mActionDesignRuleCheck.reset(cmd.designRuleCheck.createAction(
      this, this, &BoardEditor::execDesignRuleCheckDialog));
  mActionImportDxf.reset(cmd.importDxf.createAction(
      this, mFsm.data(), &BoardEditorFsm::processImportDxf));
  mActionExportLppz.reset(cmd.exportLppz.createAction(
      this, this, [this]() { mProjectEditor.execLppzExportDialog(this); }));
  mActionExportImage.reset(cmd.exportImage.createAction(this, this, [this]() {
    execGraphicsExportDialog(GraphicsExportDialog::Output::Image,
                             "image_export");
  }));
  mActionExportPdf.reset(cmd.exportPdf.createAction(this, this, [this]() {
    execGraphicsExportDialog(GraphicsExportDialog::Output::Pdf, "pdf_export");
  }));
  mActionPrint.reset(cmd.print.createAction(this, this, [this]() {
    execGraphicsExportDialog(GraphicsExportDialog::Output::Print, "print");
  }));
  mActionGenerateBom.reset(cmd.generateBom.createAction(this, this, [this]() {
    BomGeneratorDialog dialog(mProjectEditor.getWorkspace().getSettings(),
                              mProject, getActiveBoard(), this);
    dialog.exec();
  }));
  mActionGenerateFabricationData.reset(
      cmd.generateFabricationData.createAction(this, this, [this]() {
        if (Board* board = getActiveBoard()) {
          FabricationOutputDialog dialog(
              mProjectEditor.getWorkspace().getSettings(), *board, this);
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
  mActionOrderPcb.reset(cmd.orderPcb.createAction(this, this, [this]() {
    mProjectEditor.execOrderPcbDialog(nullptr, this);
  }));
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
    if (const Board* board = getActiveBoard()) {
      GridProperties grid = board->getGridProperties();
      grid.setInterval(PositiveLength(grid.getInterval() * 2));
      setGridProperties(grid, true);
    }
  }));
  mActionGridDecrease.reset(cmd.gridDecrease.createAction(this, this, [this]() {
    if (const Board* board = getActiveBoard()) {
      GridProperties grid = board->getGridProperties();
      if ((*grid.getInterval()) % 2 == 0) {
        grid.setInterval(PositiveLength(grid.getInterval() / 2));
        setGridProperties(grid, true);
      }
    }
  }));
  mActionZoomFit.reset(cmd.zoomFitContent.createAction(this, mUi->graphicsView,
                                                       &GraphicsView::zoomAll));
  mActionZoomIn.reset(
      cmd.zoomIn.createAction(this, mUi->graphicsView, &GraphicsView::zoomIn));
  mActionZoomOut.reset(cmd.zoomOut.createAction(this, mUi->graphicsView,
                                                &GraphicsView::zoomOut));
  mActionUndo.reset(cmd.undo.createAction(this));
  mActionRedo.reset(cmd.redo.createAction(this));
  mActionCut.reset(cmd.clipboardCut.createAction(this, mFsm.data(),
                                                 &BoardEditorFsm::processCut));
  mActionCopy.reset(cmd.clipboardCopy.createAction(
      this, mFsm.data(), &BoardEditorFsm::processCopy));
  mActionPaste.reset(cmd.clipboardPaste.createAction(
      this, mFsm.data(), &BoardEditorFsm::processPaste));
  mActionMoveLeft.reset(cmd.moveLeft.createAction(this, this, [this]() {
    if (!mFsm->processMove(
            Point(-mUi->graphicsView->getGridProperties().getInterval(), 0))) {
      // Workaround for consumed keyboard shortcuts for scrolling.
      mUi->graphicsView->horizontalScrollBar()->triggerAction(
          QScrollBar::SliderSingleStepSub);
    }
  }));
  addAction(mActionMoveLeft.data());
  mActionMoveRight.reset(cmd.moveRight.createAction(this, this, [this]() {
    if (!mFsm->processMove(
            Point(*mUi->graphicsView->getGridProperties().getInterval(), 0))) {
      // Workaround for consumed keyboard shortcuts for scrolling.
      mUi->graphicsView->horizontalScrollBar()->triggerAction(
          QScrollBar::SliderSingleStepAdd);
    }
  }));
  addAction(mActionMoveRight.data());
  mActionMoveUp.reset(cmd.moveUp.createAction(this, this, [this]() {
    if (!mFsm->processMove(
            Point(0, *mUi->graphicsView->getGridProperties().getInterval()))) {
      // Workaround for consumed keyboard shortcuts for scrolling.
      mUi->graphicsView->verticalScrollBar()->triggerAction(
          QScrollBar::SliderSingleStepSub);
    }
  }));
  addAction(mActionMoveUp.data());
  mActionMoveDown.reset(cmd.moveDown.createAction(this, this, [this]() {
    if (!mFsm->processMove(
            Point(0, -mUi->graphicsView->getGridProperties().getInterval()))) {
      // Workaround for consumed keyboard shortcuts for scrolling.
      mUi->graphicsView->verticalScrollBar()->triggerAction(
          QScrollBar::SliderSingleStepAdd);
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
  mActionResetAllTexts.reset(cmd.deviceResetTextAll.createAction(
      this, mFsm.data(), &BoardEditorFsm::processResetAllTexts));
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
  mActionRebuildPlanes.reset(
      cmd.planeRebuildAll.createAction(this, this, [this]() {
        if (Board* board = getActiveBoard()) {
          board->rebuildAllPlanes();
          board->forceAirWiresRebuild();
        }
      }));
  mActionAbort.reset(cmd.abort.createAction(
      this, mFsm.data(), &BoardEditorFsm::processAbortCommand));
  mActionToolSelect.reset(cmd.toolSelect.createAction(this));
  mActionToolTrace.reset(cmd.toolTrace.createAction(this));
  mActionToolVia.reset(cmd.toolVia.createAction(this));
  mActionToolPolygon.reset(cmd.toolPolygon.createAction(this));
  mActionToolText.reset(cmd.toolText.createAction(this));
  mActionToolPlane.reset(cmd.toolPlane.createAction(this));
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
  mToolsActionGroup->addAction(BoardEditorFsm::State::SELECT,
                               mActionToolSelect.data());
  mToolsActionGroup->addAction(BoardEditorFsm::State::DRAW_TRACE,
                               mActionToolTrace.data());
  mToolsActionGroup->addAction(BoardEditorFsm::State::ADD_VIA,
                               mActionToolVia.data());
  mToolsActionGroup->addAction(BoardEditorFsm::State::DRAW_POLYGON,
                               mActionToolPolygon.data());
  mToolsActionGroup->addAction(BoardEditorFsm::State::DRAW_PLANE,
                               mActionToolPlane.data());
  mToolsActionGroup->addAction(BoardEditorFsm::State::ADD_STROKE_TEXT,
                               mActionToolText.data());
  mToolsActionGroup->addAction(BoardEditorFsm::State::ADD_HOLE,
                               mActionToolHole.data());
  mToolsActionGroup->addAction(BoardEditorFsm::State::MEASURE,
                               mActionToolMeasure.data());
  mToolsActionGroup->setCurrentAction(mFsm->getCurrentState());
  connect(mFsm.data(), &BoardEditorFsm::stateChanged, mToolsActionGroup.data(),
          &ExclusiveActionGroup::setCurrentAction);
  connect(mToolsActionGroup.data(),
          &ExclusiveActionGroup::changeRequestTriggered, this,
          &BoardEditor::toolActionGroupChangeTriggered);
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
  mToolBarView->addAction(mActionZoomIn.data());
  mToolBarView->addAction(mActionZoomOut.data());
  mToolBarView->addAction(mActionZoomFit.data());
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
  mToolBarTools->addAction(mActionToolHole.data());
  mToolBarTools->addSeparator();
  mToolBarTools->addAction(mActionToolMeasure.data());
  mToolBarTools->addAction(mActionRebuildPlanes.data());
  mToolBarTools->addAction(mActionDesignRuleCheck.data());
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

  // Layers-
  mDockLayers.reset(new BoardLayersDock(*this));
  addDockWidget(Qt::RightDockWidgetArea, mDockLayers.data(), Qt::Vertical);
  tabifyDockWidget(mDockUnplacedComponents.data(), mDockLayers.data());

  // ERC Messages.
  mDockErc.reset(new ErcMsgDock(mProject));
  addDockWidget(Qt::RightDockWidgetArea, mDockErc.data(), Qt::Vertical);
  tabifyDockWidget(mDockLayers.data(), mDockErc.data());

  // DRC Messages.
  mDockDrc.reset(new BoardDesignRuleCheckMessagesDock(this));
  connect(mDockDrc.data(),
          &BoardDesignRuleCheckMessagesDock::settingsDialogRequested, this,
          &BoardEditor::execDesignRuleCheckDialog);
  connect(mDockDrc.data(), &BoardDesignRuleCheckMessagesDock::runDrcRequested,
          this, &BoardEditor::runDrcNonInteractive);
  connect(mDockDrc.data(), &BoardDesignRuleCheckMessagesDock::messageSelected,
          this, &BoardEditor::highlightDrcMessage);
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
  }
  {
    MenuBuilder smb(mb.addSubMenu(&MenuBuilder::createExportMenu));
    smb.addAction(mActionExportPdf);
    smb.addAction(mActionExportImage);
    smb.addAction(mActionExportLppz);
  }
  {
    MenuBuilder smb(mb.addSubMenu(&MenuBuilder::createProductionDataMenu));
    smb.addAction(mActionGenerateFabricationData);
    smb.addAction(mActionGeneratePickPlace);
    smb.addAction(mActionGenerateBom);
  }
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
  mb.addAction(mActionResetAllTexts);
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
  mb.addAction(mActionHidePlanes);
  mb.addAction(mActionShowPlanes);
  mb.addSeparator();
  mb.addAction(mActionZoomIn);
  mb.addAction(mActionZoomOut);
  mb.addAction(mActionZoomFit);
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
  mb.addAction(mActionLayerStack);
  mb.addSeparator();
  mb.addAction(mActionDesignRules);
  mb.addAction(mActionDesignRuleCheck);
  mb.addSeparator();
  mb.addAction(mActionRebuildPlanes);
  mb.addSeparator();
  mb.addAction(mActionNewBoard);
  mb.addAction(mActionCopyBoard);
  mb.addAction(mActionRemoveBoard);
  mb.addSection(tr("Boards"));
  // Boards will be added here, see updateBoardActionGroup().

  // Project.
  mb.newMenu(&MenuBuilder::createProjectMenu);
  mb.addAction(mActionNetClasses);
  mb.addAction(mActionProjectProperties);
  mb.addAction(mActionProjectSettings);
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
        default: { break; }
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
        default: { break; }
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
        default: { break; }
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

    default: { break; }
  }

  // Always accept graphics scene events, even if we do not react on some of
  // the events! This will give us the full control over the graphics scene.
  // Otherwise, the graphics scene can react on some events and disturb our
  // state machine. Only the wheel event is ignored because otherwise the
  // view will not allow to zoom with the mouse wheel.
  return (event->type() != QEvent::GraphicsSceneWheel);
}

void BoardEditor::toolActionGroupChangeTriggered(
    const QVariant& newTool) noexcept {
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

void BoardEditor::runDrcNonInteractive() noexcept {
  Board* board = getActiveBoard();
  if (!board) return;

  bool wasInteractive = mDockDrc->setInteractive(false);

  try {
    BoardDesignRuleCheck drc(*board, mDrcOptions);
    connect(&drc, &BoardDesignRuleCheck::progressPercent, mDockDrc.data(),
            &BoardDesignRuleCheckMessagesDock::setProgressPercent);
    connect(&drc, &BoardDesignRuleCheck::progressStatus, mDockDrc.data(),
            &BoardDesignRuleCheckMessagesDock::setProgressStatus);
    drc.execute();  // can throw
    updateBoardDrcMessages(*board, drc.getMessages());
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
  }

  mDockDrc->setInteractive(wasInteractive);
}

void BoardEditor::updateBoardDrcMessages(
    const Board& board,
    const QList<BoardDesignRuleCheckMessage>& messages) noexcept {
  clearDrcMarker();
  mDrcMessages.insert(board.getUuid(), messages);
  if (&board == getActiveBoard()) {
    mDockDrc->setMessages(messages);
  }
}

void BoardEditor::highlightDrcMessage(const BoardDesignRuleCheckMessage& msg,
                                      bool zoomTo) noexcept {
  if (QGraphicsScene* scene = mUi->graphicsView->scene()) {
    QPainterPath path = Path::toQPainterPathPx(msg.getLocations(), true);
    mDrcLocationGraphicsItem.reset(new QGraphicsPathItem());
    mDrcLocationGraphicsItem->setZValue(Board::ZValue_AirWires);
    mDrcLocationGraphicsItem->setPen(Qt::NoPen);
    mDrcLocationGraphicsItem->setBrush(QColor::fromRgb(255, 127, 0));
    mDrcLocationGraphicsItem->setPath(path);
    scene->addItem(mDrcLocationGraphicsItem.data());

    qreal margin = Length(1000000).toPx();
    QRectF rect = path.boundingRect();
    rect.adjust(-margin, -margin, margin, margin);
    mUi->graphicsView->setSceneRectMarker(rect);
    if (zoomTo) {
      mUi->graphicsView->zoomToRect(rect);
    }
  }
}

void BoardEditor::clearDrcMarker() noexcept {
  mDrcLocationGraphicsItem.reset();
  mUi->graphicsView->setSceneRectMarker(QRectF());
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
  QCollator collator;
  collator.setCaseSensitivity(Qt::CaseInsensitive);
  collator.setIgnorePunctuation(false);
  collator.setNumericMode(true);
  std::sort(deviceCandidates.begin(), deviceCandidates.end(),
            [&collator](const BI_Device* a, const BI_Device* b) {
              return collator(*a->getComponentInstance().getName(),
                              *b->getComponentInstance().getName());
            });

  if (deviceCandidates.count()) {
    while (index < 0) {
      index += deviceCandidates.count();
    }
    index %= deviceCandidates.count();
    BI_Device* device = deviceCandidates[index];
    Board* board = getActiveBoard();
    Q_ASSERT(board);
    board->clearSelection();
    device->setSelected(true);
    QRectF rect = device->getFootprint().getBoundingRect();
    // Zoom to a rectangle relative to the maximum device dimension. The
    // device is 1/4th of the screen.
    qreal margin = 1.5f * std::max(rect.size().width(), rect.size().height());
    rect.adjust(-margin, -margin, margin, margin);
    mUi->graphicsView->zoomToRect(rect);
  }
}

void BoardEditor::newBoard() noexcept {
  bool ok = false;
  QString name =
      QInputDialog::getText(this, tr("Add New Board"), tr("Choose a name:"),
                            QLineEdit::Normal, tr("new_board"), &ok);
  if (!ok) return;

  try {
    CmdBoardAdd* cmd =
        new CmdBoardAdd(mProject, ElementName(name));  // can throw
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
    CmdBoardAdd* cmd =
        new CmdBoardAdd(mProject, *board, ElementName(name));  // can throw
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
    mProjectEditor.getUndoStack().execCmd(new CmdBoardRemove(*board));
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
  }
}

void BoardEditor::setGridProperties(const GridProperties& grid,
                                    bool applyToBoard) noexcept {
  mUi->graphicsView->setGridProperties(grid);
  mUi->statusbar->setLengthUnit(grid.getUnit());

  Board* activeBoard = getActiveBoard();
  if (activeBoard && applyToBoard) {
    // In contrast to schematics, apply the grid only on the currently active
    // board instead of all, so we can use different grids for each board.
    activeBoard->setGridProperties(grid);
  }
}

void BoardEditor::execGridPropertiesDialog() noexcept {
  if (Board* activeBoard = getActiveBoard()) {
    GridSettingsDialog dialog(activeBoard->getGridProperties(), this);
    connect(
        &dialog, &GridSettingsDialog::gridPropertiesChanged,
        [this](const GridProperties& grid) { setGridProperties(grid, false); });
    if (dialog.exec()) {
      setGridProperties(dialog.getGrid(), true);
    }
  }
}

void BoardEditor::execDesignRulesDialog() noexcept {
  Board* board = getActiveBoard();
  if (!board) return;

  try {
    BoardDesignRules originalRules = board->getDesignRules();
    BoardDesignRulesDialog dialog(board->getDesignRules(),
                                  mProjectEditor.getDefaultLengthUnit(),
                                  "board_editor/design_rules_dialog", this);
    connect(&dialog, &BoardDesignRulesDialog::rulesChanged,
            [&](const BoardDesignRules& rules) {
              board->getDesignRules() = rules;
              emit board->attributesChanged();
            });
    int result = dialog.exec();
    board->getDesignRules() = originalRules;  // important hack ;)
    if (result == QDialog::Accepted) {
      CmdBoardDesignRulesModify* cmd =
          new CmdBoardDesignRulesModify(*board, dialog.getDesignRules());
      mProjectEditor.getUndoStack().execCmd(cmd);
    }
  } catch (Exception& e) {
    QMessageBox::warning(this, tr("Error"), e.getMsg());
  }
}

void BoardEditor::execDesignRuleCheckDialog() noexcept {
  Board* board = getActiveBoard();
  if (!board) return;

  BoardDesignRuleCheckDialog dialog(*board, mDrcOptions,
                                    mProjectEditor.getDefaultLengthUnit(),
                                    "board_editor/drc_dialog", this);
  dialog.exec();
  mDrcOptions = dialog.getOptions();
  if (auto messages = dialog.getMessages()) {
    updateBoardDrcMessages(*board, *messages);
    if (messages->count() > 0) {
      mDockDrc->show();
      mDockDrc->raise();
    }
  }
}

void BoardEditor::execGraphicsExportDialog(
    GraphicsExportDialog::Output output, const QString& settingsKey) noexcept {
  try {
    // Determine default file path.
    QString projectName =
        FilePath::cleanFileName(*mProject.getMetadata().getName(),
                                FilePath::ReplaceSpaces | FilePath::KeepCase);
    QString projectVersion =
        FilePath::cleanFileName(mProject.getMetadata().getVersion(),
                                FilePath::ReplaceSpaces | FilePath::KeepCase);
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
        *mProject.getMetadata().getName(),
        mActiveBoard ? mActiveBoard->getLayerStack().getInnerLayerCount() : 0,
        defaultFilePath,
        mProjectEditor.getWorkspace().getSettings().defaultLengthUnit.get(),
        "board_editor/" % settingsKey, this);
    connect(&dialog, &GraphicsExportDialog::requestOpenFile, this,
            [this](const FilePath& fp) {
              DesktopServices services(
                  mProjectEditor.getWorkspace().getSettings(), this);
              services.openLocalPath(fp);
            });
    dialog.exec();
  } catch (const Exception& e) {
    QMessageBox::warning(this, tr("Error"), e.getMsg());
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
