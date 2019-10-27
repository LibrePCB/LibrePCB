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

#include "libraryeditor.h"

#include "cmp/componenteditorwidget.h"
#include "cmpcat/componentcategoryeditorwidget.h"
#include "dev/deviceeditorwidget.h"
#include "lib/libraryoverviewwidget.h"
#include "pkg/packageeditorwidget.h"
#include "pkgcat/packagecategoryeditorwidget.h"
#include "sym/symboleditorwidget.h"
#include "ui_libraryeditor.h"

#include <librepcb/common/application.h>
#include <librepcb/common/dialogs/aboutdialog.h>
#include <librepcb/common/fileio/transactionalfilesystem.h>
#include <librepcb/common/utils/exclusiveactiongroup.h>
#include <librepcb/common/utils/undostackactiongroup.h>
#include <librepcb/library/library.h>
#include <librepcb/workspace/library/workspacelibrarydb.h>
#include <librepcb/workspace/settings/workspacesettings.h>
#include <librepcb/workspace/workspace.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

LibraryEditor::LibraryEditor(workspace::Workspace& ws, const FilePath& libFp,
                             bool readOnly)
  : QMainWindow(nullptr),
    mWorkspace(ws),
    mIsOpenedReadOnly(readOnly),
    mUi(new Ui::LibraryEditor),
    mCurrentEditorWidget(nullptr),
    mLibrary(nullptr) {
  mUi->setupUi(this);
  connect(mUi->actionClose, &QAction::triggered, this, &LibraryEditor::close);
  connect(mUi->actionNew, &QAction::triggered, this,
          &LibraryEditor::newElementTriggered);
  connect(mUi->actionSave, &QAction::triggered, this,
          &LibraryEditor::saveTriggered);
  connect(mUi->actionShowElementInFileManager, &QAction::triggered, this,
          &LibraryEditor::showElementInFileExplorerTriggered);
  connect(mUi->actionUpdateLibraryDb, &QAction::triggered,
          &mWorkspace.getLibraryDb(),
          &workspace::WorkspaceLibraryDb::startLibraryRescan);
  connect(mUi->actionCut, &QAction::triggered, this,
          &LibraryEditor::cutTriggered);
  connect(mUi->actionCopy, &QAction::triggered, this,
          &LibraryEditor::copyTriggered);
  connect(mUi->actionPaste, &QAction::triggered, this,
          &LibraryEditor::pasteTriggered);
  connect(mUi->actionRotateCw, &QAction::triggered, this,
          &LibraryEditor::rotateCwTriggered);
  connect(mUi->actionRotateCcw, &QAction::triggered, this,
          &LibraryEditor::rotateCcwTriggered);
  connect(mUi->actionRemove, &QAction::triggered, this,
          &LibraryEditor::removeTriggered);
  connect(mUi->actionAbortCommand, &QAction::triggered, this,
          &LibraryEditor::abortCommandTriggered);
  connect(mUi->actionZoomIn, &QAction::triggered, this,
          &LibraryEditor::zoomInTriggered);
  connect(mUi->actionZoomOut, &QAction::triggered, this,
          &LibraryEditor::zoomOutTriggered);
  connect(mUi->actionZoomAll, &QAction::triggered, this,
          &LibraryEditor::zoomAllTriggered);
  connect(mUi->actionGridProperties, &QAction::triggered, this,
          &LibraryEditor::editGridPropertiesTriggered);
  connect(mUi->tabWidget, &QTabWidget::currentChanged, this,
          &LibraryEditor::currentTabChanged);
  connect(mUi->tabWidget, &QTabWidget::tabCloseRequested, this,
          &LibraryEditor::tabCloseRequested);
  connect(mUi->actionOpenWebsite, &QAction::triggered,
          []() { QDesktopServices::openUrl(QUrl("https://librepcb.org")); });
  connect(mUi->actionOnlineDocumentation, &QAction::triggered, []() {
    QDesktopServices::openUrl(QUrl("https://docs.librepcb.org"));
  });
  connect(mUi->actionAbout, &QAction::triggered, qApp, &Application::about);
  connect(mUi->actionAbout_Qt, &QAction::triggered, qApp,
          &QApplication::aboutQt);

  // add overview tab
  EditorWidgetBase::Context context{mWorkspace, *this, false, readOnly};
  LibraryOverviewWidget*    overviewWidget =
      new LibraryOverviewWidget(context, libFp);
  mLibrary = &overviewWidget->getLibrary();
  connect(overviewWidget, &LibraryOverviewWidget::windowTitleChanged, this,
          &LibraryEditor::updateTabTitles);
  connect(overviewWidget, &LibraryOverviewWidget::dirtyChanged, this,
          &LibraryEditor::updateTabTitles);
  connect(overviewWidget, &EditorWidgetBase::elementEdited,
          &mWorkspace.getLibraryDb(),
          &workspace::WorkspaceLibraryDb::startLibraryRescan);

  // set window title and icon
  const QStringList localeOrder =
      mWorkspace.getSettings().getLibLocaleOrder().getLocaleOrder();
  QString libName = *mLibrary->getNames().value(localeOrder);
  if (readOnly) libName.append(tr(" [Read-Only]"));
  setWindowTitle(QString(tr("%1 - LibrePCB Library Editor")).arg(libName));
  setWindowIcon(mLibrary->getIconAsPixmap());

  // setup status bar
  mUi->statusBar->setFields(StatusBar::ProgressBar);
  mUi->statusBar->setProgressBarTextFormat(tr("Scanning libraries (%p%)"));
  connect(&mWorkspace.getLibraryDb(),
          &workspace::WorkspaceLibraryDb::scanProgressUpdate, mUi->statusBar,
          &StatusBar::setProgressBarPercent, Qt::QueuedConnection);

  // if the library was opened in read-only mode, we guess that it's a remote
  // library and thus show a warning that all modifications are lost after the
  // next update
  mUi->lblRemoteLibraryWarning->setVisible(readOnly);

  // create the undo stack action group
  mUndoStackActionGroup.reset(new UndoStackActionGroup(
      *mUi->actionUndo, *mUi->actionRedo, nullptr, nullptr, this));

  // create tools action group
  mToolsActionGroup.reset(new ExclusiveActionGroup());
  mToolsActionGroup->addAction(EditorWidgetBase::Tool::SELECT,
                               mUi->actionToolSelect);
  mToolsActionGroup->addAction(EditorWidgetBase::Tool::DRAW_LINE,
                               mUi->actionDrawLine);
  mToolsActionGroup->addAction(EditorWidgetBase::Tool::DRAW_RECT,
                               mUi->actionDrawRect);
  mToolsActionGroup->addAction(EditorWidgetBase::Tool::DRAW_POLYGON,
                               mUi->actionDrawPolygon);
  mToolsActionGroup->addAction(EditorWidgetBase::Tool::DRAW_CIRCLE,
                               mUi->actionDrawCircle);
  mToolsActionGroup->addAction(EditorWidgetBase::Tool::ADD_NAMES,
                               mUi->actionAddName);
  mToolsActionGroup->addAction(EditorWidgetBase::Tool::ADD_VALUES,
                               mUi->actionAddValue);
  mToolsActionGroup->addAction(EditorWidgetBase::Tool::DRAW_TEXT,
                               mUi->actionAddText);
  mToolsActionGroup->addAction(EditorWidgetBase::Tool::ADD_PINS,
                               mUi->actionAddSymbolPin);
  mToolsActionGroup->addAction(EditorWidgetBase::Tool::ADD_THT_PADS,
                               mUi->actionAddThtPad);
  mToolsActionGroup->addAction(EditorWidgetBase::Tool::ADD_SMT_PADS,
                               mUi->actionAddSmtPad);
  mToolsActionGroup->addAction(EditorWidgetBase::Tool::ADD_HOLES,
                               mUi->actionAddHole);
  mToolsActionGroup->setEnabled(false);

  // add all required schematic layers
  addLayer(GraphicsLayer::sSchematicReferences);
  addLayer(GraphicsLayer::sSchematicSheetFrames);
  addLayer(GraphicsLayer::sSymbolOutlines);
  addLayer(GraphicsLayer::sSymbolGrabAreas);
  addLayer(GraphicsLayer::sSymbolHiddenGrabAreas, true);
  addLayer(GraphicsLayer::sSymbolPinCirclesOpt);
  addLayer(GraphicsLayer::sSymbolPinCirclesReq);
  addLayer(GraphicsLayer::sSymbolPinNames);
  addLayer(GraphicsLayer::sSymbolPinNumbers);
  addLayer(GraphicsLayer::sSymbolNames);
  addLayer(GraphicsLayer::sSymbolValues);
  addLayer(GraphicsLayer::sSchematicNetLines);
  addLayer(GraphicsLayer::sSchematicNetLabels);
  addLayer(GraphicsLayer::sSchematicNetLabelAnchors);
  addLayer(GraphicsLayer::sSchematicDocumentation);
  addLayer(GraphicsLayer::sSchematicComments);
  addLayer(GraphicsLayer::sSchematicGuide);

  // add all required board layers
  addLayer(GraphicsLayer::sBoardSheetFrames);
  addLayer(GraphicsLayer::sBoardOutlines);
  addLayer(GraphicsLayer::sBoardMillingPth);
  addLayer(GraphicsLayer::sBoardDrillsNpth);
  addLayer(GraphicsLayer::sBoardViasTht);
  addLayer(GraphicsLayer::sBoardPadsTht);
  addLayer(GraphicsLayer::sBoardAirWires);
  addLayer(GraphicsLayer::sBoardMeasures);
  addLayer(GraphicsLayer::sBoardAlignment);
  addLayer(GraphicsLayer::sBoardDocumentation);
  addLayer(GraphicsLayer::sBoardComments);
  addLayer(GraphicsLayer::sBoardGuide);
  addLayer(GraphicsLayer::sTopCopper);
  for (int i = 1; i <= GraphicsLayer::getInnerLayerCount(); ++i) {
    addLayer(GraphicsLayer::getInnerLayerName(i));
  }
  addLayer(GraphicsLayer::sBotCopper);
  addLayer(GraphicsLayer::sTopReferences);
  addLayer(GraphicsLayer::sBotReferences);
  addLayer(GraphicsLayer::sTopGrabAreas);
  addLayer(GraphicsLayer::sBotGrabAreas);
  addLayer(GraphicsLayer::sTopHiddenGrabAreas, true);
  addLayer(GraphicsLayer::sBotHiddenGrabAreas, true);
  addLayer(GraphicsLayer::sTopPlacement);
  addLayer(GraphicsLayer::sBotPlacement);
  addLayer(GraphicsLayer::sTopDocumentation);
  addLayer(GraphicsLayer::sBotDocumentation);
  addLayer(GraphicsLayer::sTopNames);
  addLayer(GraphicsLayer::sBotNames);
  addLayer(GraphicsLayer::sTopValues);
  addLayer(GraphicsLayer::sBotValues);
  addLayer(GraphicsLayer::sTopCourtyard, true);
  addLayer(GraphicsLayer::sBotCourtyard, true);
  addLayer(GraphicsLayer::sTopStopMask, true);
  addLayer(GraphicsLayer::sBotStopMask, true);
  addLayer(GraphicsLayer::sTopSolderPaste, true);
  addLayer(GraphicsLayer::sBotSolderPaste, true);
  addLayer(GraphicsLayer::sTopGlue, true);
  addLayer(GraphicsLayer::sBotGlue, true);

  // add debug layers
#ifdef QT_DEBUG
  addLayer(GraphicsLayer::sDebugGraphicsItemsBoundingRects);
  addLayer(GraphicsLayer::sDebugGraphicsItemsTextsBoundingRects);
  addLayer(GraphicsLayer::sDebugSymbolPinNetSignalNames);
  addLayer(GraphicsLayer::sDebugNetLinesNetSignalNames);
  addLayer(GraphicsLayer::sDebugInvisibleNetPoints);
  addLayer(GraphicsLayer::sDebugComponentSymbolsCounts);
#endif

  // Edit element signals
  connect(overviewWidget, &LibraryOverviewWidget::newComponentCategoryTriggered,
          this, &LibraryEditor::newComponentCategoryTriggered);
  connect(overviewWidget, &LibraryOverviewWidget::newPackageCategoryTriggered,
          this, &LibraryEditor::newPackageCategoryTriggered);
  connect(overviewWidget, &LibraryOverviewWidget::newSymbolTriggered, this,
          &LibraryEditor::newSymbolTriggered);
  connect(overviewWidget, &LibraryOverviewWidget::newPackageTriggered, this,
          &LibraryEditor::newPackageTriggered);
  connect(overviewWidget, &LibraryOverviewWidget::newComponentTriggered, this,
          &LibraryEditor::newComponentTriggered);
  connect(overviewWidget, &LibraryOverviewWidget::newDeviceTriggered, this,
          &LibraryEditor::newDeviceTriggered);
  connect(overviewWidget,
          &LibraryOverviewWidget::editComponentCategoryTriggered, this,
          &LibraryEditor::editComponentCategoryTriggered);
  connect(overviewWidget, &LibraryOverviewWidget::editPackageCategoryTriggered,
          this, &LibraryEditor::editPackageCategoryTriggered);
  connect(overviewWidget, &LibraryOverviewWidget::editSymbolTriggered, this,
          &LibraryEditor::editSymbolTriggered);
  connect(overviewWidget, &LibraryOverviewWidget::editPackageTriggered, this,
          &LibraryEditor::editPackageTriggered);
  connect(overviewWidget, &LibraryOverviewWidget::editComponentTriggered, this,
          &LibraryEditor::editComponentTriggered);
  connect(overviewWidget, &LibraryOverviewWidget::editDeviceTriggered, this,
          &LibraryEditor::editDeviceTriggered);
  connect(overviewWidget,
          &LibraryOverviewWidget::duplicateComponentCategoryTriggered, this,
          &LibraryEditor::duplicateComponentCategoryTriggered);
  connect(overviewWidget,
          &LibraryOverviewWidget::duplicatePackageCategoryTriggered, this,
          &LibraryEditor::duplicatePackageCategoryTriggered);
  connect(overviewWidget, &LibraryOverviewWidget::duplicateSymbolTriggered,
          this, &LibraryEditor::duplicateSymbolTriggered);
  connect(overviewWidget, &LibraryOverviewWidget::duplicatePackageTriggered,
          this, &LibraryEditor::duplicatePackageTriggered);
  connect(overviewWidget, &LibraryOverviewWidget::duplicateComponentTriggered,
          this, &LibraryEditor::duplicateComponentTriggered);
  connect(overviewWidget, &LibraryOverviewWidget::duplicateDeviceTriggered,
          this, &LibraryEditor::duplicateDeviceTriggered);
  connect(overviewWidget, &LibraryOverviewWidget::removeElementTriggered, this,
          &LibraryEditor::closeTabIfOpen);

  mUi->tabWidget->addTab(overviewWidget, overviewWidget->windowIcon(),
                         overviewWidget->windowTitle());
  setActiveEditorWidget(overviewWidget);

  // remove close button on first tab (which is the library overview)
  QTabBar* tabBar = mUi->tabWidget->tabBar();
  Q_ASSERT(tabBar);
  tabBar->setTabButton(0, QTabBar::RightSide, nullptr);

  // Restore Window Geometry
  QSettings clientSettings;
  restoreGeometry(
      clientSettings.value("library_editor/window_geometry").toByteArray());
  restoreState(
      clientSettings.value("library_editor/window_state").toByteArray());
}

LibraryEditor::~LibraryEditor() noexcept {
  setActiveEditorWidget(nullptr);
  mLibrary = nullptr;
  for (int i = mUi->tabWidget->count() - 1; i >= 0; --i) {
    QWidget* widget = mUi->tabWidget->widget(i);
    mUi->tabWidget->removeTab(i);
    delete widget;
  }
  qDeleteAll(mLayers);
  mLayers.clear();
}

/*******************************************************************************
 *  Public Methods
 ******************************************************************************/

bool LibraryEditor::closeAndDestroy(bool askForSave) noexcept {
  // close tabs
  for (int i = mUi->tabWidget->count() - 1; i >= 0; --i) {
    if (askForSave) {
      if (!closeTab(i)) {
        return false;
      }
    } else {
      QWidget* widget = mUi->tabWidget->widget(i);
      mUi->tabWidget->removeTab(i);
      delete widget;
    }
  }

  // Save Window Geometry
  QSettings clientSettings;
  clientSettings.setValue("library_editor/window_geometry", saveGeometry());
  clientSettings.setValue("library_editor/window_state", saveState());

  deleteLater();
  return true;
}

/*******************************************************************************
 *  Public Slots
 ******************************************************************************/

void LibraryEditor::closeTabIfOpen(const FilePath& fp) noexcept {
  for (int i = 0; i < mUi->tabWidget->count(); i++) {
    EditorWidgetBase* widget =
        dynamic_cast<EditorWidgetBase*>(mUi->tabWidget->widget(i));
    if (widget && (widget->getFilePath() == fp)) {
      QWidget* widget = mUi->tabWidget->widget(i);
      mUi->tabWidget->removeTab(i);
      delete widget;
      return;
    }
  }
}

/*******************************************************************************
 *  GUI Event Handlers
 ******************************************************************************/

void LibraryEditor::newElementTriggered() noexcept {
  NewElementWizard wizard(mWorkspace, *mLibrary, *this, this);
  if (wizard.exec() == QDialog::Accepted) {
    FilePath fp = wizard.getContext().getOutputDirectory();
    editNewLibraryElement(wizard.getContext().mElementType, fp);
    mWorkspace.getLibraryDb().startLibraryRescan();
  }
}

void LibraryEditor::saveTriggered() noexcept {
  if (mCurrentEditorWidget) mCurrentEditorWidget->save();
}

void LibraryEditor::showElementInFileExplorerTriggered() noexcept {
  if (!mCurrentEditorWidget) return;
  FilePath fp = mCurrentEditorWidget->getFilePath();
  QDesktopServices::openUrl(fp.toQUrl());
}

void LibraryEditor::cutTriggered() noexcept {
  if (mCurrentEditorWidget) mCurrentEditorWidget->cut();
}

void LibraryEditor::copyTriggered() noexcept {
  if (mCurrentEditorWidget) mCurrentEditorWidget->copy();
}

void LibraryEditor::pasteTriggered() noexcept {
  if (mCurrentEditorWidget) mCurrentEditorWidget->paste();
}

void LibraryEditor::rotateCwTriggered() noexcept {
  if (mCurrentEditorWidget) mCurrentEditorWidget->rotateCw();
}

void LibraryEditor::rotateCcwTriggered() noexcept {
  if (mCurrentEditorWidget) mCurrentEditorWidget->rotateCcw();
}

void LibraryEditor::removeTriggered() noexcept {
  if (mCurrentEditorWidget) mCurrentEditorWidget->remove();
}

void LibraryEditor::abortCommandTriggered() noexcept {
  if (mCurrentEditorWidget) mCurrentEditorWidget->abortCommand();
}

void LibraryEditor::zoomInTriggered() noexcept {
  if (mCurrentEditorWidget) mCurrentEditorWidget->zoomIn();
}

void LibraryEditor::zoomOutTriggered() noexcept {
  if (mCurrentEditorWidget) mCurrentEditorWidget->zoomOut();
}

void LibraryEditor::zoomAllTriggered() noexcept {
  if (mCurrentEditorWidget) mCurrentEditorWidget->zoomAll();
}

void LibraryEditor::editGridPropertiesTriggered() noexcept {
  if (mCurrentEditorWidget) mCurrentEditorWidget->editGridProperties();
}

void LibraryEditor::newComponentCategoryTriggered() noexcept {
  newLibraryElement(NewElementWizardContext::ElementType::ComponentCategory);
}

void LibraryEditor::newPackageCategoryTriggered() noexcept {
  newLibraryElement(NewElementWizardContext::ElementType::PackageCategory);
}

void LibraryEditor::newSymbolTriggered() noexcept {
  newLibraryElement(NewElementWizardContext::ElementType::Symbol);
}

void LibraryEditor::newPackageTriggered() noexcept {
  newLibraryElement(NewElementWizardContext::ElementType::Package);
}

void LibraryEditor::newComponentTriggered() noexcept {
  newLibraryElement(NewElementWizardContext::ElementType::Component);
}

void LibraryEditor::newDeviceTriggered() noexcept {
  newLibraryElement(NewElementWizardContext::ElementType::Device);
}

void LibraryEditor::editComponentCategoryTriggered(
    const FilePath& fp) noexcept {
  editLibraryElementTriggered<ComponentCategoryEditorWidget>(fp, false);
}

void LibraryEditor::editPackageCategoryTriggered(const FilePath& fp) noexcept {
  editLibraryElementTriggered<PackageCategoryEditorWidget>(fp, false);
}

void LibraryEditor::editSymbolTriggered(const FilePath& fp) noexcept {
  editLibraryElementTriggered<SymbolEditorWidget>(fp, false);
}

void LibraryEditor::editPackageTriggered(const FilePath& fp) noexcept {
  editLibraryElementTriggered<PackageEditorWidget>(fp, false);
}

void LibraryEditor::editComponentTriggered(const FilePath& fp) noexcept {
  editLibraryElementTriggered<ComponentEditorWidget>(fp, false);
}

void LibraryEditor::editDeviceTriggered(const FilePath& fp) noexcept {
  editLibraryElementTriggered<DeviceEditorWidget>(fp, false);
}

void LibraryEditor::duplicateComponentCategoryTriggered(
    const FilePath& fp) noexcept {
  duplicateLibraryElement(
      NewElementWizardContext::ElementType::ComponentCategory, fp);
}

void LibraryEditor::duplicatePackageCategoryTriggered(
    const FilePath& fp) noexcept {
  duplicateLibraryElement(NewElementWizardContext::ElementType::PackageCategory,
                          fp);
}

void LibraryEditor::duplicateSymbolTriggered(const FilePath& fp) noexcept {
  duplicateLibraryElement(NewElementWizardContext::ElementType::Symbol, fp);
}

void LibraryEditor::duplicatePackageTriggered(const FilePath& fp) noexcept {
  duplicateLibraryElement(NewElementWizardContext::ElementType::Package, fp);
}

void LibraryEditor::duplicateComponentTriggered(const FilePath& fp) noexcept {
  duplicateLibraryElement(NewElementWizardContext::ElementType::Component, fp);
}

void LibraryEditor::duplicateDeviceTriggered(const FilePath& fp) noexcept {
  duplicateLibraryElement(NewElementWizardContext::ElementType::Device, fp);
}

template <typename EditWidgetType>
void LibraryEditor::editLibraryElementTriggered(const FilePath& fp,
                                                bool isNewElement) noexcept {
  try {
    for (int i = 0; i < mUi->tabWidget->count(); i++) {
      EditorWidgetBase* widget =
          dynamic_cast<EditorWidgetBase*>(mUi->tabWidget->widget(i));
      if (widget && (widget->getFilePath() == fp)) {
        mUi->tabWidget->setCurrentIndex(i);
        return;
      }
    }

    EditorWidgetBase::Context context{mWorkspace, *this, isNewElement,
                                      mIsOpenedReadOnly};
    EditWidgetType*           widget = new EditWidgetType(context, fp);
    connect(widget, &QWidget::windowTitleChanged, this,
            &LibraryEditor::updateTabTitles);
    connect(widget, &EditorWidgetBase::cursorPositionChanged, mUi->statusBar,
            &StatusBar::setAbsoluteCursorPosition);
    connect(widget, &EditorWidgetBase::dirtyChanged, this,
            &LibraryEditor::updateTabTitles);
    connect(widget, &EditorWidgetBase::elementEdited,
            &mWorkspace.getLibraryDb(),
            &workspace::WorkspaceLibraryDb::startLibraryRescan);
    int index = mUi->tabWidget->addTab(widget, widget->windowIcon(),
                                       widget->windowTitle());
    mUi->tabWidget->setCurrentIndex(index);
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Failed to open category"), e.getMsg());
  }
}

void LibraryEditor::currentTabChanged(int index) noexcept {
  setActiveEditorWidget(
      dynamic_cast<EditorWidgetBase*>(mUi->tabWidget->widget(index)));
}

void LibraryEditor::tabCloseRequested(int index) noexcept {
  // Don't allow closing the overview widget
  LibraryOverviewWidget* widget =
      dynamic_cast<LibraryOverviewWidget*>(mUi->tabWidget->widget(index));
  if (widget != nullptr) {
    return;
  }

  closeTab(index);
}

bool LibraryEditor::closeTab(int index) noexcept {
  // Get editor widget reference
  EditorWidgetBase* widget =
      dynamic_cast<EditorWidgetBase*>(mUi->tabWidget->widget(index));
  if (widget == nullptr) {
    qCritical()
        << "Cannot close tab, widget is not an EditorWidgetBase subclass";
    return false;
  }

  // Move focus out of the editor widget to enforce updating the "dirty" state
  // of the editor before closing it. This is needed to make sure the
  // "save changes?" message box appears if the user just edited some property
  // of the library element and the focus is still in the property editor
  // widget. See https://github.com/LibrePCB/LibrePCB/issues/492.
  if (QWidget* focus = focusWidget()) {
    focus->clearFocus();
  }

  // Handle closing
  if (widget == mCurrentEditorWidget) {
    setActiveEditorWidget(nullptr);
  }
  if (widget->isDirty()) {
    QString msg =
        tr("You have unsaved changes in the library element.\n"
           "Do you want to save them before closing it?");
    QMessageBox::StandardButton choice = QMessageBox::question(
        this, tr("Unsaved changes"), msg,
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
        QMessageBox::Yes);
    switch (choice) {
      case QMessageBox::Yes:
        if (widget->save()) {
          delete widget;
        } else {
          return false;
        }
        break;
      case QMessageBox::No:
        delete widget;
        break;
      default:
        return false;
    }
  } else {
    delete widget;
  }
  return true;
}

void LibraryEditor::cursorPositionChanged(const Point& pos) noexcept {
  mUi->statusBar->showMessage(QString("(%1mm | %2mm)")
                                  .arg(pos.toMmQPointF().x())
                                  .arg(pos.toMmQPointF().y()));
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void LibraryEditor::setActiveEditorWidget(EditorWidgetBase* widget) {
  bool hasGraphicalEditor = false;
  bool isOverviewTab = dynamic_cast<LibraryOverviewWidget*>(widget) != nullptr;
  if (mCurrentEditorWidget) {
    mCurrentEditorWidget->setUndoStackActionGroup(nullptr);
    mCurrentEditorWidget->setToolsActionGroup(nullptr);
    mCurrentEditorWidget->setCommandToolBar(nullptr);
  }
  mCurrentEditorWidget = widget;
  if (mCurrentEditorWidget) {
    mCurrentEditorWidget->setUndoStackActionGroup(mUndoStackActionGroup.data());
    mCurrentEditorWidget->setToolsActionGroup(mToolsActionGroup.data());
    mCurrentEditorWidget->setCommandToolBar(mUi->commandToolbar);
    hasGraphicalEditor = mCurrentEditorWidget->hasGraphicalEditor();
  }
  foreach (QAction* action, mUi->editToolbar->actions()) {
    action->setEnabled(hasGraphicalEditor);
  }
  if (isOverviewTab) {
    mUi->actionRemove->setEnabled(true);
  }
  foreach (QAction* action, mUi->viewToolbar->actions()) {
    action->setEnabled(hasGraphicalEditor);
  }
  mUi->commandToolbar->setEnabled(hasGraphicalEditor);
  mUi->statusBar->setField(StatusBar::AbsolutePosition, hasGraphicalEditor);
  updateTabTitles();  // force updating the "Save" action title
}

void LibraryEditor::newLibraryElement(
    NewElementWizardContext::ElementType type) {
  NewElementWizard wizard(mWorkspace, *mLibrary, *this, this);
  wizard.setNewElementType(type);
  if (wizard.exec() == QDialog::Accepted) {
    FilePath fp = wizard.getContext().getOutputDirectory();
    editNewLibraryElement(wizard.getContext().mElementType, fp);
    mWorkspace.getLibraryDb().startLibraryRescan();
  }
}

void LibraryEditor::duplicateLibraryElement(
    NewElementWizardContext::ElementType type, const FilePath& fp) {
  NewElementWizard wizard(mWorkspace, *mLibrary, *this, this);
  wizard.setElementToCopy(type, fp);
  if (wizard.exec() == QDialog::Accepted) {
    FilePath fp = wizard.getContext().getOutputDirectory();
    editNewLibraryElement(wizard.getContext().mElementType, fp);
    mWorkspace.getLibraryDb().startLibraryRescan();
  }
}

void LibraryEditor::editNewLibraryElement(
    NewElementWizardContext::ElementType type, const FilePath& fp) {
  switch (type) {
    case NewElementWizardContext::ElementType::ComponentCategory:
      editLibraryElementTriggered<ComponentCategoryEditorWidget>(fp, true);
      break;
    case NewElementWizardContext::ElementType::PackageCategory:
      editLibraryElementTriggered<PackageCategoryEditorWidget>(fp, true);
      break;
    case NewElementWizardContext::ElementType::Symbol:
      editLibraryElementTriggered<SymbolEditorWidget>(fp, true);
      break;
    case NewElementWizardContext::ElementType::Package:
      editLibraryElementTriggered<PackageEditorWidget>(fp, true);
      break;
    case NewElementWizardContext::ElementType::Component:
      editLibraryElementTriggered<ComponentEditorWidget>(fp, true);
      break;
    case NewElementWizardContext::ElementType::Device:
      editLibraryElementTriggered<DeviceEditorWidget>(fp, true);
      break;
    default:
      break;
  }
}

void LibraryEditor::updateTabTitles() noexcept {
  for (int i = 0; i < mUi->tabWidget->count(); ++i) {
    QWidget*                widget = mUi->tabWidget->widget(i);
    const EditorWidgetBase* editorWidget =
        dynamic_cast<EditorWidgetBase*>(widget);
    if (editorWidget) {
      if (editorWidget->isDirty()) {
        mUi->tabWidget->setTabText(i, '*' % editorWidget->windowTitle());
      } else {
        mUi->tabWidget->setTabText(i, editorWidget->windowTitle());
      }
    } else {
      qWarning() << "Tab widget is not a subclass of EditorWidgetBase!";
    }
  }

  if (mCurrentEditorWidget) {
    mUi->actionSave->setEnabled(true);
    mUi->actionSave->setText(
        QString(tr("&Save '%1'")).arg(mCurrentEditorWidget->windowTitle()));
  } else {
    mUi->actionSave->setEnabled(false);
  }
}

void LibraryEditor::closeEvent(QCloseEvent* event) noexcept {
  if (closeAndDestroy(true)) {
    QMainWindow::closeEvent(event);
  } else {
    event->ignore();
  }
}

void LibraryEditor::addLayer(const QString& name, bool forceVisible) noexcept {
  QScopedPointer<GraphicsLayer> layer(new GraphicsLayer(name));
  if (forceVisible) layer->setVisible(true);
  mLayers.append(layer.take());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb
