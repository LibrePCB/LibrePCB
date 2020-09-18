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
#include "schematiceditor.h"

#include "../dialogs/bomgeneratordialog.h"
#include "../dialogs/projectpropertieseditordialog.h"
#include "../docks/ercmsgdock.h"
#include "../projecteditor.h"
#include "fsm/schematiceditorfsm.h"
#include "schematicpagesdock.h"
#include "ui_schematiceditor.h"

#include <librepcb/common/application.h>
#include <librepcb/common/dialogs/aboutdialog.h>
#include <librepcb/common/dialogs/filedialog.h>
#include <librepcb/common/dialogs/gridsettingsdialog.h>
#include <librepcb/common/graphics/graphicsscene.h>
#include <librepcb/common/graphics/graphicsview.h>
#include <librepcb/common/gridproperties.h>
#include <librepcb/common/undostack.h>
#include <librepcb/common/utils/exclusiveactiongroup.h>
#include <librepcb/common/utils/undostackactiongroup.h>
#include <librepcb/project/circuit/circuit.h>
#include <librepcb/project/circuit/componentinstance.h>
#include <librepcb/project/metadata/projectmetadata.h>
#include <librepcb/project/project.h>
#include <librepcb/project/schematics/cmd/cmdschematicadd.h>
#include <librepcb/project/schematics/cmd/cmdschematicedit.h>
#include <librepcb/project/schematics/cmd/cmdschematicremove.h>
#include <librepcb/project/schematics/items/si_symbol.h>
#include <librepcb/project/schematics/schematic.h>
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

SchematicEditor::SchematicEditor(ProjectEditor& projectEditor, Project& project)
  : QMainWindow(0),
    mProjectEditor(projectEditor),
    mProject(project),
    mUi(new Ui::SchematicEditor),
    mGraphicsView(nullptr),
    mActiveSchematicIndex(-1),
    mPagesDock(nullptr),
    mErcMsgDock(nullptr),
    mFsm(nullptr) {
  mUi->setupUi(this);
  mUi->actionSave_Project->setEnabled(mProject.getDirectory().isWritable());

  // set window title
  QString filenameStr = mProject.getFilepath().getFilename();
  if (!mProject.getDirectory().isWritable()) {
    filenameStr.append(QStringLiteral(" [Read-Only]"));
  }
  setWindowTitle(tr("%1 - LibrePCB Schematic Editor").arg(filenameStr));

  // Add Dock Widgets
  mPagesDock = new SchematicPagesDock(mProject, this);
  connect(this, &SchematicEditor::activeSchematicChanged, mPagesDock,
          &SchematicPagesDock::setSelectedSchematic);
  connect(mPagesDock, &SchematicPagesDock::selectedSchematicChanged, this,
          &SchematicEditor::setActiveSchematicIndex);
  connect(mPagesDock, &SchematicPagesDock::addSchematicTriggered, this,
          &SchematicEditor::addSchematic);
  connect(mPagesDock, &SchematicPagesDock::removeSchematicTriggered, this,
          &SchematicEditor::removeSchematic);
  connect(mPagesDock, &SchematicPagesDock::renameSchematicTriggered, this,
          &SchematicEditor::renameSchematic);
  addDockWidget(Qt::LeftDockWidgetArea, mPagesDock, Qt::Vertical);
  mErcMsgDock = new ErcMsgDock(mProject);
  addDockWidget(Qt::RightDockWidgetArea, mErcMsgDock, Qt::Vertical);

  // add graphics view as central widget
  mGraphicsView = new GraphicsView(nullptr, this);
  mGraphicsView->setUseOpenGl(
      mProjectEditor.getWorkspace().getSettings().useOpenGl.get());
  setCentralWidget(mGraphicsView);

  // Add actions to toggle visibility of dock widgets
  mUi->menuView->addSeparator();
  mUi->menuView->addAction(mPagesDock->toggleViewAction());
  mUi->menuView->addAction(mErcMsgDock->toggleViewAction());

  // connect some actions which are created with the Qt Designer
  connect(mUi->actionNew_Schematic_Page, &QAction::triggered, this,
          &SchematicEditor::addSchematic);
  connect(mUi->actionSave_Project, &QAction::triggered, &mProjectEditor,
          &ProjectEditor::saveProject);
  connect(mUi->actionQuit, &QAction::triggered, this, &SchematicEditor::close);
  connect(mUi->actionOpenWebsite, &QAction::triggered,
          []() { QDesktopServices::openUrl(QUrl("https://librepcb.org")); });
  connect(mUi->actionOnlineDocumentation, &QAction::triggered, []() {
    QDesktopServices::openUrl(QUrl("https://docs.librepcb.org"));
  });
  connect(mUi->actionAbout, &QAction::triggered, qApp, &Application::about);
  connect(mUi->actionAbout_Qt, &QAction::triggered, qApp,
          &QApplication::aboutQt);
  connect(mUi->actionZoom_In, &QAction::triggered, mGraphicsView,
          &GraphicsView::zoomIn);
  connect(mUi->actionZoom_Out, &QAction::triggered, mGraphicsView,
          &GraphicsView::zoomOut);
  connect(mUi->actionZoom_All, &QAction::triggered, mGraphicsView,
          &GraphicsView::zoomAll);
  connect(mUi->actionShow_Control_Panel, &QAction::triggered, &mProjectEditor,
          &ProjectEditor::showControlPanelClicked);
  connect(mUi->actionShow_Board_Editor, &QAction::triggered, &mProjectEditor,
          &ProjectEditor::showBoardEditor);
  connect(mUi->actionEditNetclasses, &QAction::triggered,
          [this]() { mProjectEditor.execNetClassesEditorDialog(this); });
  connect(mUi->actionProjectSettings, &QAction::triggered,
          [this]() { mProjectEditor.execProjectSettingsDialog(this); });
  connect(mUi->actionExportLppz, &QAction::triggered,
          [this]() { mProjectEditor.execLppzExportDialog(this); });

  // connect the undo/redo actions with the UndoStack of the project
  mUndoStackActionGroup.reset(
      new UndoStackActionGroup(*mUi->actionUndo, *mUi->actionRedo, nullptr,
                               &mProjectEditor.getUndoStack(), this));

  // build the whole schematic editor finite state machine with all its substate
  // objects
  SchematicEditorFsm::Context fsmContext{
      mProjectEditor.getWorkspace(), mProject, *this, *mUi, *mGraphicsView,
      mProjectEditor.getUndoStack()};
  mFsm = new SchematicEditorFsm(fsmContext);

  // connect the "tools" toolbar with the state machine
  mToolsActionGroup.reset(new ExclusiveActionGroup());
  mToolsActionGroup->addAction(SchematicEditorFsm::State::SELECT,
                               mUi->actionToolSelect);
  mToolsActionGroup->addAction(SchematicEditorFsm::State::DRAW_WIRE,
                               mUi->actionToolDrawWire);
  mToolsActionGroup->addAction(SchematicEditorFsm::State::ADD_NETLABEL,
                               mUi->actionToolAddNetLabel);
  mToolsActionGroup->addAction(SchematicEditorFsm::State::ADD_COMPONENT,
                               mUi->actionToolAddComponent);
  mToolsActionGroup->setCurrentAction(mFsm->getCurrentState());
  connect(mFsm, &SchematicEditorFsm::stateChanged, mToolsActionGroup.data(),
          &ExclusiveActionGroup::setCurrentAction);
  connect(mToolsActionGroup.data(),
          &ExclusiveActionGroup::changeRequestTriggered, this,
          &SchematicEditor::toolActionGroupChangeTriggered);

  // connect the "command" toolbar with the state machine
  connect(mUi->actionCommandAbort, &QAction::triggered, mFsm,
          &SchematicEditorFsm::processAbortCommand);

  // connect the "edit" toolbar with the state machine
  connect(mUi->actionSelectAll, &QAction::triggered, mFsm,
          &SchematicEditorFsm::processSelectAll);
  connect(mUi->actionCopy, &QAction::triggered, mFsm,
          &SchematicEditorFsm::processCopy);
  connect(mUi->actionCut, &QAction::triggered, mFsm,
          &SchematicEditorFsm::processCut);
  connect(mUi->actionPaste, &QAction::triggered, mFsm,
          &SchematicEditorFsm::processPaste);
  connect(mUi->actionRotate_CW, &QAction::triggered, mFsm,
          &SchematicEditorFsm::processRotateCw);
  connect(mUi->actionRotate_CCW, &QAction::triggered, mFsm,
          &SchematicEditorFsm::processRotateCcw);
  connect(mUi->actionMirror, &QAction::triggered, mFsm,
          &SchematicEditorFsm::processMirror);
  connect(mUi->actionRemove, &QAction::triggered, mFsm,
          &SchematicEditorFsm::processRemove);

  // setup "search" toolbar
  mUi->searchToolbar->setPlaceholderText(tr("Find symbol..."));
  mUi->searchToolbar->setCompleterListFunction(
      std::bind(&SchematicEditor::getSearchToolBarCompleterList, this));
  connect(mUi->searchToolbar, &SearchToolBar::goToTriggered, this,
          &SchematicEditor::goToSymbol);

  // setup status bar
  mUi->statusbar->setFields(StatusBar::AbsolutePosition |
                            StatusBar::ProgressBar);
  mUi->statusbar->setProgressBarTextFormat(tr("Scanning libraries (%p%)"));
  connect(&mProjectEditor.getWorkspace().getLibraryDb(),
          &workspace::WorkspaceLibraryDb::scanProgressUpdate, mUi->statusbar,
          &StatusBar::setProgressBarPercent, Qt::QueuedConnection);
  connect(mGraphicsView, &GraphicsView::cursorScenePositionChanged,
          mUi->statusbar, &StatusBar::setAbsoluteCursorPosition);

  // Make the icons in the components toolbar dependent from project settings
  updateComponentToolbarIcons();
  connect(&mProject.getSettings(), &ProjectSettings::settingsChanged, this,
          &SchematicEditor::updateComponentToolbarIcons);

  // Restore Window Geometry
  QSettings clientSettings;
  restoreGeometry(
      clientSettings.value("schematic_editor/window_geometry").toByteArray());
  restoreState(
      clientSettings.value("schematic_editor/window_state").toByteArray());

  // Load first schematic page
  if (mProject.getSchematics().count() > 0) setActiveSchematicIndex(0);

  // Set focus to graphics view (avoid having the focus in some arbitrary
  // widget).
  mGraphicsView->setFocus();

  // mGraphicsView->zoomAll(); does not work properly here, should be executed
  // later in the event loop (ugly, but seems to work...)
#if (QT_VERSION >= QT_VERSION_CHECK(5, 4, 0))
  QTimer::singleShot(200, mGraphicsView, &GraphicsView::zoomAll);
#else
  QTimer::singleShot(200, mGraphicsView, SLOT(zoomAll()));
#endif
}

SchematicEditor::~SchematicEditor() {
  // Save Window Geometry
  QSettings clientSettings;
  clientSettings.setValue("schematic_editor/window_geometry", saveGeometry());
  clientSettings.setValue("schematic_editor/window_state", saveState());

  delete mFsm;
  mFsm = nullptr;
  delete mErcMsgDock;
  mErcMsgDock = nullptr;
  delete mPagesDock;
  mPagesDock = nullptr;
  delete mGraphicsView;
  mGraphicsView = nullptr;
  delete mUi;
  mUi = nullptr;
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

Schematic* SchematicEditor::getActiveSchematic() const noexcept {
  return mProject.getSchematicByIndex(mActiveSchematicIndex);
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

bool SchematicEditor::setActiveSchematicIndex(int index) noexcept {
  if (index == mActiveSchematicIndex) return true;

  // "Ask" the FSM if changing the scene is allowed at the moment.
  // If the FSM accepts the event, we can switch to the specified schematic
  // page.
  if (!mFsm->processSwitchToSchematicPage(index)) {
    return false;  // changing the schematic page is not allowed!
  }

  // event accepted --> change the schematic page
  Schematic* schematic = getActiveSchematic();
  if (schematic) {
    // save current view scene rect
    schematic->saveViewSceneRect(mGraphicsView->getVisibleSceneRect());
  }
  schematic = mProject.getSchematicByIndex(index);
  if (schematic) {
    // show scene, restore view scene rect, set grid properties
    schematic->showInView(*mGraphicsView);
    mGraphicsView->setVisibleSceneRect(schematic->restoreViewSceneRect());
    mGraphicsView->setGridProperties(schematic->getGridProperties());
    mUi->statusbar->setLengthUnit(schematic->getGridProperties().getUnit());
  } else {
    mGraphicsView->setScene(nullptr);
  }

  // update toolbars
  mUi->actionGrid->setEnabled(schematic != nullptr);

  // schematic page has changed!
  mActiveSchematicIndex = index;
  emit activeSchematicChanged(mActiveSchematicIndex);
  return true;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SchematicEditor::abortAllCommands() noexcept {
  // ugly... ;-)
  mFsm->processAbortCommand();
  mFsm->processAbortCommand();
  mFsm->processAbortCommand();
}

/*******************************************************************************
 *  Inherited Methods
 ******************************************************************************/

void SchematicEditor::closeEvent(QCloseEvent* event) {
  if (!mProjectEditor.windowIsAboutToClose(*this))
    event->ignore();
  else
    QMainWindow::closeEvent(event);
}

/*******************************************************************************
 *  Actions
 ******************************************************************************/

void SchematicEditor::on_actionClose_Project_triggered() {
  mProjectEditor.closeAndDestroy(true, this);
}

void SchematicEditor::on_actionRenameSheet_triggered() {
  renameSchematic(mActiveSchematicIndex);
}

void SchematicEditor::on_actionGrid_triggered() {
  if (const Schematic* activeSchematic = getActiveSchematic()) {
    GridSettingsDialog dialog(activeSchematic->getGridProperties(), this);
    connect(&dialog, &GridSettingsDialog::gridPropertiesChanged,
            [this](const GridProperties& grid) {
              mGraphicsView->setGridProperties(grid);
              mUi->statusbar->setLengthUnit(grid.getUnit());
            });
    if (dialog.exec()) {
      foreach (Schematic* schematic, mProject.getSchematics()) {
        schematic->setGridProperties(dialog.getGrid());
      }
    }
  }
}

void SchematicEditor::on_actionPrint_triggered() {
  try {
    int pageCount = mProject.getSchematics().count();
    if (pageCount <= 0) {
      throw Exception(__FILE__, __LINE__, tr("No pages to print."));
    }
    QPrinter printer(QPrinter::HighResolution);
    printer.setPaperSize(QPrinter::A4);
    printer.setOrientation(QPrinter::Landscape);
    printer.setCreator(QString("LibrePCB %1").arg(qApp->applicationVersion()));
    printer.setDocName(*mProject.getMetadata().getName());
    QPrintDialog printDialog(&printer, this);
    printDialog.setOption(QAbstractPrintDialog::PrintSelection, false);
    printDialog.setMinMax(1, pageCount);
    if (printDialog.exec() == QDialog::Accepted) {
      int minPageIndex = 0;
      int maxPageIndex = 0;
      switch (printDialog.printRange()) {
        case QAbstractPrintDialog::PrintRange::PageRange: {
          minPageIndex = qMax(printDialog.fromPage() - 1, 0);
          maxPageIndex = qMax(printDialog.toPage() - 1, 0);
          break;
        }
        case QAbstractPrintDialog::PrintRange::CurrentPage: {
          minPageIndex = getActiveSchematicIndex();
          maxPageIndex = getActiveSchematicIndex();
          break;
        }
        default: {
          minPageIndex = 0;
          maxPageIndex = pageCount - 1;
          break;
        }
      }
      QList<int> pages;
      for (int i = minPageIndex; i <= maxPageIndex; ++i) pages.append(i);
      mProject.printSchematicPages(printer, pages);  // can throw
    }
  } catch (Exception& e) {
    QMessageBox::warning(this, tr("Error"), e.getMsg());
  }
}

void SchematicEditor::on_actionPDF_Export_triggered() {
  try {
    QString projectName =
        FilePath::cleanFileName(*mProject.getMetadata().getName(),
                                FilePath::ReplaceSpaces | FilePath::KeepCase);
    QString projectVersion =
        FilePath::cleanFileName(mProject.getMetadata().getVersion(),
                                FilePath::ReplaceSpaces | FilePath::KeepCase);
    QString relativePath =
        QString("output/%1/%2_Schematics.pdf").arg(projectVersion, projectName);
    FilePath defaultFilePath = mProject.getPath().getPathTo(relativePath);
    QDir().mkpath(defaultFilePath.getParentDir().toStr());
    QString filename = FileDialog::getSaveFileName(
        this, tr("PDF Export"), defaultFilePath.toNative(), "*.pdf");
    if (filename.isEmpty()) return;
    if (!filename.endsWith(".pdf")) filename.append(".pdf");
    FilePath filepath(filename);
    mProject.exportSchematicsAsPdf(
        filepath);  // this method can throw an exception
    QDesktopServices::openUrl(QUrl::fromLocalFile(filepath.toNative()));
  } catch (Exception& e) {
    QMessageBox::warning(this, tr("Error"), e.getMsg());
  }
}

void SchematicEditor::on_actionExportAsSvg_triggered() {
  try {
    Schematic* schematic = getActiveSchematic();
    if (!schematic) return;

    QString projectName =
        FilePath::cleanFileName(*mProject.getMetadata().getName(),
                                FilePath::ReplaceSpaces | FilePath::KeepCase);
    QString projectVersion =
        FilePath::cleanFileName(mProject.getMetadata().getVersion(),
                                FilePath::ReplaceSpaces | FilePath::KeepCase);
    QString schematicName = FilePath::cleanFileName(
        *schematic->getName(), FilePath::ReplaceSpaces | FilePath::KeepCase);
    QString relativePath = QString("output/%1/%2_%3.svg")
                               .arg(projectVersion, projectName, schematicName);
    FilePath defaultFilePath = mProject.getPath().getPathTo(relativePath);
    QDir().mkpath(defaultFilePath.getParentDir().toStr());
    QString filename = FileDialog::getSaveFileName(
        this, tr("SVG Export"), defaultFilePath.toNative(), "*.svg");
    if (filename.isEmpty()) return;
    if (!filename.endsWith(".svg")) filename.append(".svg");
    FilePath filepath(filename);

    // Export
    int dpi = 254;
    QRectF rectPx = schematic->getGraphicsScene().itemsBoundingRect();
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
    schematic->renderToQPainter(painter);
  } catch (Exception& e) {
    QMessageBox::warning(this, tr("Error"), e.getMsg());
  }
}

void SchematicEditor::on_actionGenerateBom_triggered() {
  const Board* board =
      mProject.getBoards().count() == 1 ? mProject.getBoardByIndex(0) : nullptr;
  BomGeneratorDialog dialog(mProject, board, this);
  dialog.exec();
}

void SchematicEditor::on_actionAddComp_Resistor_triggered() {
  Uuid componentUuid = Uuid::fromString("ef80cd5e-2689-47ee-8888-31d04fc99174");
  Uuid symbVarUuid = Uuid::fromString(
      useIeee315Symbols() ? "d16e1f44-16af-4773-a310-de370f744548"
                          : "a5995314-f535-45d4-8bd8-2d0b8a0dc42a");
  mFsm->processAddComponent(componentUuid, symbVarUuid);
}

void SchematicEditor::on_actionAddComp_BipolarCapacitor_triggered() {
  Uuid componentUuid = Uuid::fromString("d167e0e3-6a92-4b76-b013-77b9c230e5f1");
  Uuid symbVarUuid = Uuid::fromString(
      useIeee315Symbols() ? "6e639ff1-4e81-423b-9d0e-b28b35693a61"
                          : "8cd7b37f-e5fa-4af5-a8dd-d78830bba3af");
  mFsm->processAddComponent(componentUuid, symbVarUuid);
}

void SchematicEditor::on_actionAddComp_UnipolarCapacitor_triggered() {
  Uuid componentUuid = Uuid::fromString("c54375c5-7149-4ded-95c5-7462f7301ee7");
  Uuid symbVarUuid = Uuid::fromString(
      useIeee315Symbols() ? "20a01a81-506e-4fee-9dc0-8b50e6537cd4"
                          : "5412add2-af9c-44b8-876d-a0fb7c201897");
  mFsm->processAddComponent(componentUuid, symbVarUuid);
}

void SchematicEditor::on_actionAddComp_Inductor_triggered() {
  Uuid componentUuid = Uuid::fromString("506bd124-6062-400e-9078-b38bd7e1aaee");
  Uuid symbVarUuid = Uuid::fromString(
      useIeee315Symbols() ? "4245d515-6f6d-48cb-9958-a4ea23d0187f"
                          : "62a7598c-17fe-41cf-8fa1-4ed274c3adc2");
  mFsm->processAddComponent(componentUuid, symbVarUuid);
}

void SchematicEditor::on_actionAddComp_gnd_triggered() {
  Uuid componentUuid = Uuid::fromString("8076f6be-bfab-4fc1-9772-5d54465dd7e1");
  Uuid symbVarUuid = Uuid::fromString("f09ad258-595b-4ee9-a1fc-910804a203ae");
  mFsm->processAddComponent(componentUuid, symbVarUuid);
}

void SchematicEditor::on_actionAddComp_vcc_triggered() {
  Uuid componentUuid = Uuid::fromString("58c3c6cd-11eb-4557-aa3f-d3e05874afde");
  Uuid symbVarUuid = Uuid::fromString("afb86b45-68ec-47b6-8d96-153d73567228");
  mFsm->processAddComponent(componentUuid, symbVarUuid);
}

void SchematicEditor::on_actionProjectProperties_triggered() {
  ProjectPropertiesEditorDialog dialog(mProject.getMetadata(),
                                       mProjectEditor.getUndoStack(), this);
  dialog.exec();
}

void SchematicEditor::on_actionUpdateLibrary_triggered() {
  // ugly hack until we have a *real* project library updater...
  emit mProjectEditor.openProjectLibraryUpdaterClicked(mProject.getFilepath());
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool SchematicEditor::graphicsViewEventHandler(QEvent* event) {
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
    default: { break; }
  }

  // Always accept graphics scene events, even if we do not react on some of
  // the events! This will give us the full control over the graphics scene.
  // Otherwise, the graphics scene can react on some events and disturb our
  // state machine. Only the wheel event is ignored because otherwise the
  // view will not allow to zoom with the mouse wheel.
  return (event->type() != QEvent::GraphicsSceneWheel);
}

void SchematicEditor::toolActionGroupChangeTriggered(
    const QVariant& newTool) noexcept {
  // Note: Converting the QVariant to SchematicEditorFsm::State doesn't work
  // with some Qt versions, thus we convert to int instead. Fixed in:
  // https://codereview.qt-project.org/c/qt/qtbase/+/159277/
  switch (newTool.value<int>()) {
    case SchematicEditorFsm::State::SELECT:
      mFsm->processSelect();
      break;
    case SchematicEditorFsm::State::DRAW_WIRE:
      mFsm->processDrawWire();
      break;
    case SchematicEditorFsm::State::ADD_NETLABEL:
      mFsm->processAddNetLabel();
      break;
    case SchematicEditorFsm::State::ADD_COMPONENT:
      mFsm->processAddComponent();
      break;
    default:
      Q_ASSERT(false);
      qCritical() << "Unknown tool triggered!";
      break;
  }
}

void SchematicEditor::addSchematic() noexcept {
  bool ok = false;
  QString name = QInputDialog::getText(this, tr("Add schematic page"),
                                       tr("Choose a name:"), QLineEdit::Normal,
                                       tr("New Page"), &ok);
  if (!ok) return;

  try {
    CmdSchematicAdd* cmd =
        new CmdSchematicAdd(mProject, ElementName(name));  // can throw
    mProjectEditor.getUndoStack().execCmd(cmd);
    setActiveSchematicIndex(mProject.getSchematics().count() - 1);
  } catch (Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
  }
}

void SchematicEditor::removeSchematic(int index) noexcept {
  Schematic* schematic = mProject.getSchematicByIndex(index);
  if (!schematic) return;

  try {
    CmdSchematicRemove* cmd = new CmdSchematicRemove(mProject, *schematic);
    mProjectEditor.getUndoStack().execCmd(cmd);
  } catch (Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
  }
}

void SchematicEditor::renameSchematic(int index) noexcept {
  Schematic* schematic = mProject.getSchematicByIndex(index);
  if (!schematic) return;

  bool ok = false;
  QString name =
      QInputDialog::getText(this, tr("Rename sheet"), tr("Choose new name:"),
                            QLineEdit::Normal, *schematic->getName(), &ok);
  if (!ok) return;

  try {
    QScopedPointer<CmdSchematicEdit> cmd(new CmdSchematicEdit(*schematic));
    cmd->setName(ElementName(cleanElementName(name)));  // can throw
    mProjectEditor.getUndoStack().execCmd(cmd.take());
  } catch (Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
  }
}

QList<SI_Symbol*> SchematicEditor::getSearchCandidates() noexcept {
  QList<SI_Symbol*> candidates = {};
  foreach (const Schematic* schematic, mProject.getSchematics()) {
    Q_ASSERT(schematic);
    candidates += schematic->getSymbols();
  }
  std::sort(
      candidates.begin(), candidates.end(),
      [](SI_Symbol* a, SI_Symbol* b) { return a->getName() < b->getName(); });
  return candidates;
}

QStringList SchematicEditor::getSearchToolBarCompleterList() noexcept {
  QStringList list;
  foreach (SI_Symbol* symbol, getSearchCandidates()) {
    list.append(symbol->getName());
  }
  return list;
}

void SchematicEditor::goToSymbol(const QString& name,
                                 unsigned int index) noexcept {
  QList<SI_Symbol*> symbolCandidates = {};
  foreach (SI_Symbol* symbol, getSearchCandidates()) {
    if (symbol->getName().startsWith(name, Qt::CaseInsensitive)) {
      symbolCandidates.append(symbol);
    }
  }

  if (symbolCandidates.count()) {
    index %= symbolCandidates.count();
    SI_Symbol* symbol = symbolCandidates[index];
    Schematic& schematic = symbol->getSchematic();
    if (setActiveSchematicIndex(mProject.getSchematics().indexOf(&schematic))) {
      schematic.clearSelection();
      symbol->setSelected(true);
      QRectF rect = symbol->getBoundingRect();
      // Zoom to a rectangle relative to the maximum symbol dimension. The
      // symbol is 1/4th of the screen.
      qreal margin = 1.5f * std::max(rect.size().width(), rect.size().height());
      rect.adjust(-margin, -margin, margin, margin);
      mGraphicsView->zoomToRect(rect);
    }
  }
}

void SchematicEditor::updateComponentToolbarIcons() noexcept {
  QString suffix = useIeee315Symbols() ? "us" : "eu";
  mUi->actionAddComp_Resistor->setIcon(
      QIcon(":/img/library/resistor_" % suffix % ".png"));
  mUi->actionAddComp_Inductor->setIcon(
      QIcon(":/img/library/inductor_" % suffix % ".png"));
  mUi->actionAddComp_BipolarCapacitor->setIcon(
      QIcon(":/img/library/bipolar_capacitor_" % suffix % ".png"));
  mUi->actionAddComp_UnipolarCapacitor->setIcon(
      QIcon(":/img/library/unipolar_capacitor_" % suffix % ".png"));
}

bool SchematicEditor::useIeee315Symbols() const noexcept {
  foreach (const QString& norm, mProject.getSettings().getNormOrder()) {
    if (norm.toLower() == "ieee 315") {
      return true;
    } else if (norm.toLower() == "iec 60617") {
      return false;
    }
  }
  return false;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
