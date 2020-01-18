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
#include "fsm/ses_fsm.h"
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
#include <librepcb/project/metadata/projectmetadata.h>
#include <librepcb/project/project.h>
#include <librepcb/project/schematics/cmd/cmdschematicadd.h>
#include <librepcb/project/schematics/cmd/cmdschematicedit.h>
#include <librepcb/project/schematics/cmd/cmdschematicremove.h>
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
    mGridProperties(nullptr),
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
  setWindowTitle(QString("%1 - LibrePCB Schematic Editor").arg(filenameStr));

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

  // create default grid properties
  mGridProperties = new GridProperties();

  // add graphics view as central widget
  mGraphicsView = new GraphicsView(nullptr, this);
  mGraphicsView->setUseOpenGl(
      mProjectEditor.getWorkspace().getSettings().useOpenGl.get());
  mGraphicsView->setGridProperties(*mGridProperties);
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
  mFsm =
      new SES_FSM(*this, *mUi, *mGraphicsView, mProjectEditor.getUndoStack());

  // connect the "tools" toolbar with the state machine
  mToolsActionGroup.reset(new ExclusiveActionGroup());
  mToolsActionGroup->addAction(SES_FSM::State::State_Select,
                               mUi->actionToolSelect);
  mToolsActionGroup->addAction(SES_FSM::State::State_DrawWire,
                               mUi->actionToolDrawWire);
  mToolsActionGroup->addAction(SES_FSM::State::State_AddNetLabel,
                               mUi->actionToolAddNetLabel);
  mToolsActionGroup->addAction(SES_FSM::State::State_AddComponent,
                               mUi->actionToolAddComponent);
  mToolsActionGroup->setCurrentAction(mFsm->getCurrentState());
  connect(mFsm, &SES_FSM::stateChanged, mToolsActionGroup.data(),
          &ExclusiveActionGroup::setCurrentAction);
  connect(mToolsActionGroup.data(),
          &ExclusiveActionGroup::changeRequestTriggered, this,
          &SchematicEditor::toolActionGroupChangeTriggered);

  // connect the "command" toolbar with the state machine
  connect(mUi->actionCommandAbort, &QAction::triggered, [this]() {
    mFsm->processEvent(new SEE_Base(SEE_Base::AbortCommand), true);
  });

  // connect the "edit" toolbar with the state machine
  connect(mUi->actionCopy, &QAction::triggered, [this]() {
    mFsm->processEvent(new SEE_Base(SEE_Base::Edit_Copy), true);
  });
  connect(mUi->actionCut, &QAction::triggered, [this]() {
    mFsm->processEvent(new SEE_Base(SEE_Base::Edit_Cut), true);
  });
  connect(mUi->actionPaste, &QAction::triggered, [this]() {
    mFsm->processEvent(new SEE_Base(SEE_Base::Edit_Paste), true);
  });
  connect(mUi->actionRotate_CW, &QAction::triggered, [this]() {
    mFsm->processEvent(new SEE_Base(SEE_Base::Edit_RotateCW), true);
  });
  connect(mUi->actionRotate_CCW, &QAction::triggered, [this]() {
    mFsm->processEvent(new SEE_Base(SEE_Base::Edit_RotateCCW), true);
  });
  connect(mUi->actionMirror, &QAction::triggered, [this]() {
    mFsm->processEvent(new SEE_Base(SEE_Base::Edit_Mirror), true);
  });
  connect(mUi->actionRemove, &QAction::triggered, [this]() {
    mFsm->processEvent(new SEE_Base(SEE_Base::Edit_Remove), true);
  });

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
      clientSettings.value("schematic_editor/window_geometry").toByteArray());
  restoreState(
      clientSettings.value("schematic_editor/window_state").toByteArray());

  // Load first schematic page
  if (mProject.getSchematics().count() > 0) setActiveSchematicIndex(0);

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
  delete mGridProperties;
  mGridProperties = nullptr;
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
  SEE_SwitchToSchematicPage* event = new SEE_SwitchToSchematicPage(index);
  mFsm->processEvent(event);
  bool accepted = event->isAccepted();
  delete event;
  if (!accepted) return false;  // changing the schematic page is not allowed!

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
  } else {
    mGraphicsView->setScene(nullptr);
  }

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
  mFsm->processEvent(new SEE_Base(SEE_Base::AbortCommand), true);
  mFsm->processEvent(new SEE_Base(SEE_Base::AbortCommand), true);
  mFsm->processEvent(new SEE_Base(SEE_Base::AbortCommand), true);
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
  GridSettingsDialog dialog(*mGridProperties, this);
  connect(&dialog, &GridSettingsDialog::gridPropertiesChanged,
          [this](const GridProperties& grid) {
            *mGridProperties = grid;
            mGraphicsView->setGridProperties(grid);
          });
  if (dialog.exec()) {
    foreach (Schematic* schematic, mProject.getSchematics())
      schematic->setGridProperties(*mGridProperties);
    // mProjectEditor.setModifiedFlag(); TODO
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
    QDesktopServices::openUrl(QUrl::fromLocalFile(filepath.toStr()));
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
    int    dpi    = 254;
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
  Uuid symbVarUuid   = Uuid::fromString("a5995314-f535-45d4-8bd8-2d0b8a0dc42a");
  SEE_StartAddComponent* addEvent =
      new SEE_StartAddComponent(componentUuid, symbVarUuid);
  mFsm->processEvent(addEvent, true);
}

void SchematicEditor::on_actionAddComp_BipolarCapacitor_triggered() {
  Uuid componentUuid = Uuid::fromString("d167e0e3-6a92-4b76-b013-77b9c230e5f1");
  Uuid symbVarUuid   = Uuid::fromString("8cd7b37f-e5fa-4af5-a8dd-d78830bba3af");
  SEE_StartAddComponent* addEvent =
      new SEE_StartAddComponent(componentUuid, symbVarUuid);
  mFsm->processEvent(addEvent, true);
}

void SchematicEditor::on_actionAddComp_UnipolarCapacitor_triggered() {
  Uuid componentUuid = Uuid::fromString("c54375c5-7149-4ded-95c5-7462f7301ee7");
  Uuid symbVarUuid   = Uuid::fromString("5412add2-af9c-44b8-876d-a0fb7c201897");
  SEE_StartAddComponent* addEvent =
      new SEE_StartAddComponent(componentUuid, symbVarUuid);
  mFsm->processEvent(addEvent, true);
}

void SchematicEditor::on_actionAddComp_Inductor_triggered() {
  Uuid componentUuid = Uuid::fromString("506bd124-6062-400e-9078-b38bd7e1aaee");
  Uuid symbVarUuid   = Uuid::fromString("62a7598c-17fe-41cf-8fa1-4ed274c3adc2");
  SEE_StartAddComponent* addEvent =
      new SEE_StartAddComponent(componentUuid, symbVarUuid);
  mFsm->processEvent(addEvent, true);
}

void SchematicEditor::on_actionAddComp_gnd_triggered() {
  Uuid componentUuid = Uuid::fromString("8076f6be-bfab-4fc1-9772-5d54465dd7e1");
  Uuid symbVarUuid   = Uuid::fromString("f09ad258-595b-4ee9-a1fc-910804a203ae");
  SEE_StartAddComponent* addEvent =
      new SEE_StartAddComponent(componentUuid, symbVarUuid);
  mFsm->processEvent(addEvent, true);
}

void SchematicEditor::on_actionAddComp_vcc_triggered() {
  Uuid componentUuid = Uuid::fromString("58c3c6cd-11eb-4557-aa3f-d3e05874afde");
  Uuid symbVarUuid   = Uuid::fromString("afb86b45-68ec-47b6-8d96-153d73567228");
  SEE_StartAddComponent* addEvent =
      new SEE_StartAddComponent(componentUuid, symbVarUuid);
  mFsm->processEvent(addEvent, true);
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
  SEE_RedirectedQEvent* e =
      new SEE_RedirectedQEvent(SEE_Base::GraphicsViewEvent, event);
  return mFsm->processEvent(e, true);
}

void SchematicEditor::toolActionGroupChangeTriggered(
    const QVariant& newTool) noexcept {
  switch (newTool.toInt()) {
    case SES_FSM::State::State_Select:
      mFsm->processEvent(new SEE_Base(SEE_Base::StartSelect), true);
      break;
    case SES_FSM::State::State_DrawWire:
      mFsm->processEvent(new SEE_Base(SEE_Base::StartDrawWire), true);
      break;
    case SES_FSM::State::State_AddNetLabel:
      mFsm->processEvent(new SEE_Base(SEE_Base::StartAddNetLabel), true);
      break;
    case SES_FSM::State::State_AddComponent:
      mFsm->processEvent(new SEE_StartAddComponent(), true);
      break;
    default:
      Q_ASSERT(false);
      qCritical() << "Unknown tool triggered!";
      break;
  }
}

void SchematicEditor::addSchematic() noexcept {
  bool    ok   = false;
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

  bool    ok = false;
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

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
