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

#include "../dialogs/bomgeneratordialog.h"
#include "../dialogs/projectpropertieseditordialog.h"
#include "../docks/ercmsgdock.h"
#include "../projecteditor.h"
#include "boarddesignrulecheckdialog.h"
#include "boarddesignrulecheckmessagesdock.h"
#include "boardlayersdock.h"
#include "boardlayerstacksetupdialog.h"
#include "boardpickplacegeneratordialog.h"
#include "fabricationoutputdialog.h"
#include "fsm/boardeditorfsm.h"
#include "unplacedcomponentsdock.h"

#include <librepcb/common/application.h>
#include <librepcb/common/dialogs/aboutdialog.h>
#include <librepcb/common/dialogs/boarddesignrulesdialog.h>
#include <librepcb/common/dialogs/filedialog.h>
#include <librepcb/common/dialogs/gridsettingsdialog.h>
#include <librepcb/common/fileio/fileutils.h>
#include <librepcb/common/graphics/graphicsscene.h>
#include <librepcb/common/graphics/graphicsview.h>
#include <librepcb/common/graphics/primitivepathgraphicsitem.h>
#include <librepcb/common/gridproperties.h>
#include <librepcb/common/undostack.h>
#include <librepcb/common/utils/exclusiveactiongroup.h>
#include <librepcb/common/utils/undostackactiongroup.h>
#include <librepcb/project/boards/board.h>
#include <librepcb/project/boards/boardlayerstack.h>
#include <librepcb/project/boards/cmd/cmdboardadd.h>
#include <librepcb/project/boards/cmd/cmdboarddesignrulesmodify.h>
#include <librepcb/project/boards/cmd/cmdboardremove.h>
#include <librepcb/project/boards/items/bi_device.h>
#include <librepcb/project/boards/items/bi_footprint.h>
#include <librepcb/project/boards/items/bi_plane.h>
#include <librepcb/project/circuit/circuit.h>
#include <librepcb/project/circuit/componentinstance.h>
#include <librepcb/project/metadata/projectmetadata.h>
#include <librepcb/project/project.h>
#include <librepcb/project/settings/projectsettings.h>
#include <librepcb/workspace/library/workspacelibrarydb.h>
#include <librepcb/workspace/settings/workspacesettings.h>
#include <librepcb/workspace/workspace.h>

#include <QSvgGenerator>
#include <QtCore>
#include <QtPrintSupport>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardEditor::BoardEditor(ProjectEditor& projectEditor, Project& project)
  : QMainWindow(0),
    mProjectEditor(projectEditor),
    mProject(project),
    mUi(new Ui::BoardEditor),
    mGraphicsView(nullptr),
    mActiveBoard(nullptr),
    mBoardListActionGroup(this),
    mErcMsgDock(nullptr),
    mUnplacedComponentsDock(nullptr),
    mBoardLayersDock(nullptr),
    mDrcMessagesDock(),
    mFsm(nullptr) {
  mUi->setupUi(this);
  mUi->lblUnplacedComponentsNote->hide();
  mUi->actionProjectSave->setEnabled(mProject.getDirectory().isWritable());

  // set window title
  QString filenameStr = mProject.getFilepath().getFilename();
  if (!mProject.getDirectory().isWritable()) {
    filenameStr.append(QStringLiteral(" [Read-Only]"));
  }
  setWindowTitle(tr("%1 - LibrePCB Board Editor").arg(filenameStr));

  // add graphics view as central widget
  mGraphicsView = new GraphicsView(nullptr, this);
  mGraphicsView->setUseOpenGl(
      mProjectEditor.getWorkspace().getSettings().useOpenGl.get());
  mGraphicsView->setBackgroundBrush(Qt::black);
  mGraphicsView->setForegroundBrush(Qt::white);
  // setCentralWidget(mGraphicsView);
  mUi->centralwidget->layout()->addWidget(mGraphicsView);

  // build the whole board editor finite state machine with all its substate
  // objects
  BoardEditorFsm::Context fsmContext{
      mProjectEditor.getWorkspace(), mProject, *this, *mUi, *mGraphicsView,
      mProjectEditor.getUndoStack()};
  mFsm.reset(new BoardEditorFsm(fsmContext));

  // connect the "tools" toolbar with the state machine
  mToolsActionGroup.reset(new ExclusiveActionGroup());
  mToolsActionGroup->addAction(BoardEditorFsm::State::SELECT,
                               mUi->actionToolSelect);
  mToolsActionGroup->addAction(BoardEditorFsm::State::DRAW_TRACE,
                               mUi->actionToolDrawTrace);
  mToolsActionGroup->addAction(BoardEditorFsm::State::ADD_VIA,
                               mUi->actionToolAddVia);
  mToolsActionGroup->addAction(BoardEditorFsm::State::DRAW_POLYGON,
                               mUi->actionToolDrawPolygon);
  mToolsActionGroup->addAction(BoardEditorFsm::State::DRAW_PLANE,
                               mUi->actionToolAddPlane);
  mToolsActionGroup->addAction(BoardEditorFsm::State::ADD_STROKE_TEXT,
                               mUi->actionToolAddText);
  mToolsActionGroup->addAction(BoardEditorFsm::State::ADD_HOLE,
                               mUi->actionToolAddHole);
  mToolsActionGroup->setCurrentAction(mFsm->getCurrentState());
  connect(mFsm.data(), &BoardEditorFsm::stateChanged, mToolsActionGroup.data(),
          &ExclusiveActionGroup::setCurrentAction);
  connect(mToolsActionGroup.data(),
          &ExclusiveActionGroup::changeRequestTriggered, this,
          &BoardEditor::toolActionGroupChangeTriggered);

  // connect the "command" toolbar with the state machine
  connect(mUi->actionCommandAbort, &QAction::triggered, mFsm.data(),
          &BoardEditorFsm::processAbortCommand);

  // connect the "edit" toolbar with the state machine
  connect(mUi->actionSelectAll, &QAction::triggered, mFsm.data(),
          &BoardEditorFsm::processSelectAll);
  connect(mUi->actionCopy, &QAction::triggered, mFsm.data(),
          &BoardEditorFsm::processCopy);
  connect(mUi->actionCut, &QAction::triggered, mFsm.data(),
          &BoardEditorFsm::processCut);
  connect(mUi->actionPaste, &QAction::triggered, mFsm.data(),
          &BoardEditorFsm::processPaste);
  connect(mUi->actionRotate_CW, &QAction::triggered, mFsm.data(),
          &BoardEditorFsm::processRotateCw);
  connect(mUi->actionRotate_CCW, &QAction::triggered, mFsm.data(),
          &BoardEditorFsm::processRotateCcw);
  connect(mUi->actionFlipHorizontal, &QAction::triggered, mFsm.data(),
          &BoardEditorFsm::processFlipHorizontal);
  connect(mUi->actionFlipVertical, &QAction::triggered, mFsm.data(),
          &BoardEditorFsm::processFlipVertical);
  connect(mUi->actionRemove, &QAction::triggered, mFsm.data(),
          &BoardEditorFsm::processRemove);

  // Add Dock Widgets
  mUnplacedComponentsDock = new UnplacedComponentsDock(mProjectEditor);
  connect(mUnplacedComponentsDock,
          &UnplacedComponentsDock::unplacedComponentsCountChanged, this,
          &BoardEditor::unplacedComponentsCountChanged);
  connect(mUnplacedComponentsDock, &UnplacedComponentsDock::addDeviceTriggered,
          mFsm.data(), &BoardEditorFsm::processAddDevice);
  addDockWidget(Qt::RightDockWidgetArea, mUnplacedComponentsDock, Qt::Vertical);
  mBoardLayersDock = new BoardLayersDock(*this);
  addDockWidget(Qt::RightDockWidgetArea, mBoardLayersDock, Qt::Vertical);
  tabifyDockWidget(mUnplacedComponentsDock, mBoardLayersDock);
  mErcMsgDock = new ErcMsgDock(mProject);
  addDockWidget(Qt::RightDockWidgetArea, mErcMsgDock, Qt::Vertical);
  tabifyDockWidget(mBoardLayersDock, mErcMsgDock);
  mDrcMessagesDock.reset(new BoardDesignRuleCheckMessagesDock(this));
  connect(mDrcMessagesDock.data(),
          &BoardDesignRuleCheckMessagesDock::settingsDialogRequested, this,
          &BoardEditor::on_actionDesignRuleCheck_triggered);
  connect(mDrcMessagesDock.data(),
          &BoardDesignRuleCheckMessagesDock::runDrcRequested, this,
          &BoardEditor::runDrcNonInteractive);
  connect(mDrcMessagesDock.data(),
          &BoardDesignRuleCheckMessagesDock::messageSelected, this,
          &BoardEditor::highlightDrcMessage);
  addDockWidget(Qt::RightDockWidgetArea, mDrcMessagesDock.data());
  tabifyDockWidget(mErcMsgDock, mDrcMessagesDock.data());
  mUnplacedComponentsDock->raise();

  // Add actions to toggle visibility of dock widgets
  mUi->menuView->addSeparator();
  mUi->menuView->addAction(mUnplacedComponentsDock->toggleViewAction());
  mUi->menuView->addAction(mBoardLayersDock->toggleViewAction());
  mUi->menuView->addAction(mErcMsgDock->toggleViewAction());
  mUi->menuView->addAction(mDrcMessagesDock->toggleViewAction());

  // add all boards to the menu and connect to project signals
  mUi->tabBar->setVisible(false);  // hide since there are no boards yet
  for (int i = 0; i < mProject.getBoards().count(); i++) boardAdded(i);
  connect(&mProject, &Project::boardAdded, this, &BoardEditor::boardAdded);
  connect(&mProject, &Project::boardRemoved, this, &BoardEditor::boardRemoved);
  connect(&mBoardListActionGroup, &QActionGroup::triggered, this,
          &BoardEditor::boardListActionGroupTriggered);

  // connect some actions which are created with the Qt Designer
  connect(mUi->actionProjectSave, &QAction::triggered, &mProjectEditor,
          &ProjectEditor::saveProject);
  connect(mUi->actionQuit, &QAction::triggered, this, &BoardEditor::close);
  connect(mUi->actionOpenWebsite, &QAction::triggered,
          []() { QDesktopServices::openUrl(QUrl("https://librepcb.org")); });
  connect(mUi->actionOnlineDocumentation, &QAction::triggered, []() {
    QDesktopServices::openUrl(QUrl("https://docs.librepcb.org"));
  });
  connect(mUi->actionAbout, &QAction::triggered, qApp, &Application::about);
  connect(mUi->actionAboutQt, &QAction::triggered, qApp,
          &QApplication::aboutQt);
  connect(mUi->actionZoomIn, &QAction::triggered, mGraphicsView,
          &GraphicsView::zoomIn);
  connect(mUi->actionZoomOut, &QAction::triggered, mGraphicsView,
          &GraphicsView::zoomOut);
  connect(mUi->actionZoomAll, &QAction::triggered, mGraphicsView,
          &GraphicsView::zoomAll);
  connect(mUi->actionShowControlPanel, &QAction::triggered, &mProjectEditor,
          &ProjectEditor::showControlPanelClicked);
  connect(mUi->actionShowSchematicEditor, &QAction::triggered, &mProjectEditor,
          &ProjectEditor::showSchematicEditor);
  connect(mUi->actionEditNetClasses, &QAction::triggered,
          [this]() { mProjectEditor.execNetClassesEditorDialog(this); });
  connect(mUi->actionProjectSettings, &QAction::triggered,
          [this]() { mProjectEditor.execProjectSettingsDialog(this); });
  connect(mUi->actionExportLppz, &QAction::triggered,
          [this]() { mProjectEditor.execLppzExportDialog(this); });

  // connect the undo/redo actions with the UndoStack of the project
  mUndoStackActionGroup.reset(
      new UndoStackActionGroup(*mUi->actionUndo, *mUi->actionRedo, nullptr,
                               &mProjectEditor.getUndoStack(), this));

  // setup "search" toolbar
  mUi->searchToolbar->setPlaceholderText(tr("Find device..."));
  mUi->searchToolbar->setCompleterListFunction(
      std::bind(&BoardEditor::getSearchToolBarCompleterList, this));
  connect(mUi->searchToolbar, &SearchToolBar::goToTriggered, this,
          &BoardEditor::goToDevice);

  // setup status bar
  mUi->statusbar->setFields(StatusBar::AbsolutePosition |
                            StatusBar::ProgressBar);
  mUi->statusbar->setProgressBarTextFormat(tr("Scanning libraries (%p%)"));
  connect(&mProjectEditor.getWorkspace().getLibraryDb(),
          &workspace::WorkspaceLibraryDb::scanProgressUpdate, mUi->statusbar,
          &StatusBar::setProgressBarPercent, Qt::QueuedConnection);
  connect(mGraphicsView, &GraphicsView::cursorScenePositionChanged,
          mUi->statusbar, &StatusBar::setAbsoluteCursorPosition);

  // Restore Window Geometry
  QSettings clientSettings;
  restoreGeometry(
      clientSettings.value("board_editor/window_geometry").toByteArray());
  restoreState(clientSettings.value("board_editor/window_state").toByteArray());

  // Load first board
  if (mProject.getBoards().count() > 0) setActiveBoardIndex(0);

  // Set focus to graphics view (avoid having the focus in some arbitrary
  // widget).
  mGraphicsView->setFocus();

  // mGraphicsView->zoomAll(); does not work properly here, should be executed
  // later in the event loop (ugly, but seems to work...)
  QTimer::singleShot(200, mGraphicsView, &GraphicsView::zoomAll);
}

BoardEditor::~BoardEditor() {
  // Save Window Geometry
  QSettings clientSettings;
  clientSettings.setValue("board_editor/window_geometry", saveGeometry());
  clientSettings.setValue("board_editor/window_state", saveState());

  mFsm.reset();
  qDeleteAll(mBoardListActions);
  mBoardListActions.clear();
  delete mBoardLayersDock;
  mBoardLayersDock = nullptr;
  delete mUnplacedComponentsDock;
  mUnplacedComponentsDock = nullptr;
  delete mErcMsgDock;
  mErcMsgDock = nullptr;
  mDrcMessagesDock.reset();
  delete mGraphicsView;
  mGraphicsView = nullptr;
  delete mUi;
  mUi = nullptr;
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
      mActiveBoard->saveViewSceneRect(mGraphicsView->getVisibleSceneRect());
    }
    mActiveBoard = newBoard;
    if (mActiveBoard) {
      // show scene, restore view scene rect, set grid properties
      mActiveBoard->showInView(*mGraphicsView);
      mGraphicsView->setVisibleSceneRect(mActiveBoard->restoreViewSceneRect());
      mGraphicsView->setGridProperties(mActiveBoard->getGridProperties());
      mUi->statusbar->setLengthUnit(
          mActiveBoard->getGridProperties().getUnit());
      // force airwire rebuild immediately and on every project modification
      mActiveBoard->triggerAirWiresRebuild();
      connect(&mProjectEditor.getUndoStack(), &UndoStack::stateModified,
              mActiveBoard.data(), &Board::triggerAirWiresRebuild);
    } else {
      mGraphicsView->setScene(nullptr);
    }

    // update dock widgets
    mUnplacedComponentsDock->setBoard(mActiveBoard);
    mBoardLayersDock->setActiveBoard(mActiveBoard);
    mDrcMessagesDock->setInteractive(mActiveBoard != nullptr);
    mDrcMessagesDock->setMessages(mActiveBoard
                                      ? mDrcMessages[mActiveBoard->getUuid()]
                                      : QList<BoardDesignRuleCheckMessage>());

    // update toolbars
    mUi->actionGrid->setEnabled(mActiveBoard != nullptr);
  }

  // update GUI
  mUi->tabBar->setCurrentIndex(index);
  for (int i = 0; i < mBoardListActions.count(); ++i) {
    mBoardListActions.at(i)->setChecked(i == index);
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

  QAction* actionBefore = mBoardListActions.value(newIndex - 1);
  QAction* newAction = new QAction(*board->getName(), this);
  newAction->setCheckable(true);
  mUi->menuBoard->insertAction(actionBefore, newAction);
  mBoardListActions.insert(newIndex, newAction);
  mBoardListActionGroup.addAction(newAction);

  mUi->tabBar->insertTab(newIndex, *board->getName());

  // To avoid wasting space, only show the tab bar if there are multiple boards.
  mUi->tabBar->setVisible(mUi->tabBar->count() > 1);
}

void BoardEditor::boardRemoved(int oldIndex) {
  QAction* action = mBoardListActions.takeAt(oldIndex);
  Q_ASSERT(action);
  mBoardListActionGroup.removeAction(action);
  delete action;

  mUi->tabBar->removeTab(oldIndex);  // calls setActiveBoardIndex() if needed

  // To avoid wasting space, only show the tab bar if there are multiple boards.
  mUi->tabBar->setVisible(mUi->tabBar->count() > 1);
}

/*******************************************************************************
 *  Actions
 ******************************************************************************/

void BoardEditor::on_actionProjectClose_triggered() {
  mProjectEditor.closeAndDestroy(true, this);
}

void BoardEditor::on_actionNewBoard_triggered() {
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

void BoardEditor::on_actionCopyBoard_triggered() {
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

void BoardEditor::on_actionRemoveBoard_triggered() {
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

void BoardEditor::on_actionGrid_triggered() {
  if (Board* activeBoard = getActiveBoard()) {
    GridSettingsDialog dialog(activeBoard->getGridProperties(), this);
    connect(&dialog, &GridSettingsDialog::gridPropertiesChanged,
            [this](const GridProperties& grid) {
              mGraphicsView->setGridProperties(grid);
              mUi->statusbar->setLengthUnit(grid.getUnit());
            });
    if (dialog.exec()) {
      // In contrast to schematics, apply the grid only on the currently active
      // board instead of all, so we can use different grids for each board.
      activeBoard->setGridProperties(dialog.getGrid());
    }
  }
}

void BoardEditor::on_actionPrint_triggered() {
  try {
    Board* board = getActiveBoard();
    if (!board) {
      throw Exception(__FILE__, __LINE__, tr("No board selected."));
    }
    QPrinter printer(QPrinter::HighResolution);
    printer.setPaperSize(QPrinter::A4);
    printer.setOrientation(QPrinter::Landscape);
    printer.setCreator(QString("LibrePCB %1").arg(qApp->applicationVersion()));
    printer.setDocName(*mProject.getMetadata().getName());
    QPrintDialog printDialog(&printer, this);
    printDialog.setOption(QAbstractPrintDialog::PrintSelection, false);
    printDialog.setMinMax(1, 1);
    if (printDialog.exec() == QDialog::Accepted) {
      board->print(printer);  // can throw
    }
  } catch (Exception& e) {
    QMessageBox::warning(this, tr("Error"), e.getMsg());
  }
}

void BoardEditor::on_actionExportAsPdf_triggered() {
  try {
    Board* board = getActiveBoard();
    if (!board) {
      throw Exception(__FILE__, __LINE__, tr("No board selected."));
    }
    QString projectName =
        FilePath::cleanFileName(*mProject.getMetadata().getName(),
                                FilePath::ReplaceSpaces | FilePath::KeepCase);
    QString projectVersion =
        FilePath::cleanFileName(mProject.getMetadata().getVersion(),
                                FilePath::ReplaceSpaces | FilePath::KeepCase);
    QString relativePath =
        QString("output/%1/%2_Board.pdf").arg(projectVersion, projectName);
    FilePath defaultFilePath = mProject.getPath().getPathTo(relativePath);
    QDir().mkpath(defaultFilePath.getParentDir().toStr());
    QString filename = FileDialog::getSaveFileName(
        this, tr("PDF Export"), defaultFilePath.toNative(), "*.pdf");
    if (filename.isEmpty()) return;
    if (!filename.endsWith(".pdf")) filename.append(".pdf");
    FilePath filepath(filename);

    // Create output directory first because QPrinter silently fails if it
    // doesn't exist.
    FileUtils::makePath(filepath.getParentDir());  // can throw

    // Print (use local block scope to ensure the PDF is fully written & closed
    // after leaving the block - without this, opening the PDF could fail)
    {
      QPrinter printer(QPrinter::HighResolution);
      printer.setPaperSize(QPrinter::A4);
      printer.setOrientation(QPrinter::Landscape);
      printer.setOutputFormat(QPrinter::PdfFormat);
      printer.setCreator(
          QString("LibrePCB %1").arg(qApp->applicationVersion()));
      printer.setOutputFileName(filepath.toStr());
      board->print(printer);  // can throw
    }

    QDesktopServices::openUrl(QUrl::fromLocalFile(filepath.toNative()));
  } catch (Exception& e) {
    QMessageBox::warning(this, tr("Error"), e.getMsg());
  }
}

void BoardEditor::on_actionExportAsSvg_triggered() {
  try {
    Board* board = getActiveBoard();
    if (!board) {
      throw Exception(__FILE__, __LINE__, tr("No board selected."));
    }
    QString projectName =
        FilePath::cleanFileName(*mProject.getMetadata().getName(),
                                FilePath::ReplaceSpaces | FilePath::KeepCase);
    QString projectVersion =
        FilePath::cleanFileName(mProject.getMetadata().getVersion(),
                                FilePath::ReplaceSpaces | FilePath::KeepCase);
    QString relativePath =
        QString("output/%1/%2_Board.svg").arg(projectVersion, projectName);
    FilePath defaultFilePath = mProject.getPath().getPathTo(relativePath);
    QDir().mkpath(defaultFilePath.getParentDir().toStr());
    QString filename = FileDialog::getSaveFileName(
        this, tr("SVG Export"), defaultFilePath.toNative(), "*.svg");
    if (filename.isEmpty()) return;
    if (!filename.endsWith(".svg")) filename.append(".svg");
    FilePath filepath(filename);

    // Create output directory first because QSvgGenerator might not create it.
    FileUtils::makePath(filepath.getParentDir());  // can throw

    // Export
    int dpi = 254;
    QRectF rectPx = board->getGraphicsScene().itemsBoundingRect();
    QRectF rectSvg(Length::fromPx(rectPx.left()).toInch() * dpi,
                   Length::fromPx(rectPx.top()).toInch() * dpi,
                   Length::fromPx(rectPx.width()).toInch() * dpi,
                   Length::fromPx(rectPx.height()).toInch() * dpi);
    rectSvg.moveTo(0, 0);  // seems to be required for the SVG viewbox
    QSvgGenerator generator;
    generator.setTitle(filepath.getFilename());
    generator.setDescription(*mProject.getMetadata().getName());
    generator.setFileName(filepath.toStr());
    generator.setSize(rectSvg.toAlignedRect().size());
    generator.setViewBox(rectSvg);
    generator.setResolution(dpi);
    QPainter painter(&generator);
    board->renderToQPainter(painter, dpi);
  } catch (Exception& e) {
    QMessageBox::warning(this, tr("Error"), e.getMsg());
  }
}

void BoardEditor::on_actionGenerateFabricationData_triggered() {
  Board* board = getActiveBoard();
  if (!board) return;

  FabricationOutputDialog dialog(*board, this);
  dialog.exec();
}

void BoardEditor::on_actionGenerateBom_triggered() {
  BomGeneratorDialog dialog(mProject, getActiveBoard(), this);
  dialog.exec();
}

void BoardEditor::on_actionGeneratePickPlace_triggered() {
  Board* board = getActiveBoard();
  if (!board) return;

  BoardPickPlaceGeneratorDialog dialog(*board);
  dialog.exec();
}

void BoardEditor::on_actionProjectProperties_triggered() {
  ProjectPropertiesEditorDialog dialog(mProject.getMetadata(),
                                       mProjectEditor.getUndoStack(), this);
  dialog.exec();
}

void BoardEditor::on_actionUpdateLibrary_triggered() {
  // ugly hack until we have a *real* project library updater...
  emit mProjectEditor.openProjectLibraryUpdaterClicked(mProject.getFilepath());
}

void BoardEditor::on_actionLayerStackSetup_triggered() {
  Board* board = getActiveBoard();
  if (!board) return;

  try {
    BoardLayerStackSetupDialog dialog(board->getLayerStack(),
                                      mProjectEditor.getUndoStack(), this);
    dialog.exec();
  } catch (Exception& e) {
    QMessageBox::warning(this, tr("Error"), e.getMsg());
  }
}

void BoardEditor::on_actionModifyDesignRules_triggered() {
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

void BoardEditor::on_actionDesignRuleCheck_triggered() {
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
      mDrcMessagesDock->show();
      mDrcMessagesDock->raise();
    }
  }
}

void BoardEditor::on_actionRebuildPlanes_triggered() {
  Board* board = getActiveBoard();
  if (board) {
    board->rebuildAllPlanes();
    board->forceAirWiresRebuild();
  }
}

void BoardEditor::on_actionShowAllPlanes_triggered() {
  if (Board* board = getActiveBoard()) {
    foreach (BI_Plane* plane, board->getPlanes()) {
      plane->setVisible(true);  // No undo command needed since it is not saved
    }
  }
}

void BoardEditor::on_actionHideAllPlanes_triggered() {
  if (Board* board = getActiveBoard()) {
    foreach (BI_Plane* plane, board->getPlanes()) {
      plane->setVisible(false);  // No undo command needed since it is not saved
    }
  }
}

void BoardEditor::on_tabBar_currentChanged(int index) {
  setActiveBoardIndex(index);
}

void BoardEditor::on_lblUnplacedComponentsNote_linkActivated() {
  mUnplacedComponentsDock->show();
  mUnplacedComponentsDock->raise();
}

void BoardEditor::boardListActionGroupTriggered(QAction* action) {
  setActiveBoardIndex(mBoardListActions.indexOf(action));
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

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
      mFsm->processKeyPressed(*e);
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
    default:
      Q_ASSERT(false);
      qCritical() << "Unknown tool triggered!";
      break;
  }
}

void BoardEditor::unplacedComponentsCountChanged(int count) noexcept {
  mUi->lblUnplacedComponentsNote->setVisible(count > 0);
}

void BoardEditor::runDrcNonInteractive() noexcept {
  Board* board = getActiveBoard();
  if (!board) return;

  bool wasInteractive = mDrcMessagesDock->setInteractive(false);

  try {
    BoardDesignRuleCheck drc(*board, mDrcOptions);
    connect(&drc, &BoardDesignRuleCheck::progressPercent,
            mDrcMessagesDock.data(),
            &BoardDesignRuleCheckMessagesDock::setProgressPercent);
    connect(&drc, &BoardDesignRuleCheck::progressStatus,
            mDrcMessagesDock.data(),
            &BoardDesignRuleCheckMessagesDock::setProgressStatus);
    drc.execute();  // can throw
    updateBoardDrcMessages(*board, drc.getMessages());
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
  }

  mDrcMessagesDock->setInteractive(wasInteractive);
}

void BoardEditor::updateBoardDrcMessages(
    const Board& board,
    const QList<BoardDesignRuleCheckMessage>& messages) noexcept {
  clearDrcMarker();
  mDrcMessages.insert(board.getUuid(), messages);
  if (&board == getActiveBoard()) {
    mDrcMessagesDock->setMessages(messages);
  }
}

void BoardEditor::highlightDrcMessage(const BoardDesignRuleCheckMessage& msg,
                                      bool zoomTo) noexcept {
  if (QGraphicsScene* scene = mGraphicsView->scene()) {
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
    mGraphicsView->setSceneRectMarker(rect);
    if (zoomTo) {
      mGraphicsView->zoomToRect(rect);
    }
  }
}

void BoardEditor::clearDrcMarker() noexcept {
  mDrcLocationGraphicsItem.reset();
  mGraphicsView->setSceneRectMarker(QRectF());
}

QList<BI_Device*> BoardEditor::getSearchCandidates() noexcept {
  QList<BI_Device*> candidates = {};
  if (Board* board = getActiveBoard()) {
    candidates += board->getDeviceInstances().values();
    std::sort(candidates.begin(), candidates.end(),
              [](BI_Device* a, BI_Device* b) {
                return a->getComponentInstance().getName() <
                    b->getComponentInstance().getName();
              });
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

void BoardEditor::goToDevice(const QString& name, unsigned int index) noexcept {
  QList<BI_Device*> deviceCandidates = {};
  foreach (BI_Device* device, getSearchCandidates()) {
    if (device->getComponentInstance().getName()->startsWith(
            name, Qt::CaseInsensitive)) {
      deviceCandidates.append(device);
    }
  }

  if (deviceCandidates.count()) {
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
    mGraphicsView->zoomToRect(rect);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
