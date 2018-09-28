/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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
#include "newelementwizard/newelementwizard.h"
#include "pkg/packageeditorwidget.h"
#include "pkgcat/packagecategoryeditorwidget.h"
#include "sym/symboleditorwidget.h"
#include "ui_libraryeditor.h"

#include <librepcb/common/application.h>
#include <librepcb/common/dialogs/aboutdialog.h>
#include <librepcb/common/fileio/fileutils.h>
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

LibraryEditor::LibraryEditor(workspace::Workspace&   ws,
                             QSharedPointer<Library> lib)
  : QMainWindow(nullptr),
    mWorkspace(ws),
    mLibrary(lib),
    mUi(new Ui::LibraryEditor),
    mCurrentEditorWidget(nullptr),
    mLock(lib->getFilePath()) {
  mUi->setupUi(this);
  connect(mUi->actionClose, &QAction::triggered, this, &LibraryEditor::close);
  connect(mUi->actionNew, &QAction::triggered, this,
          &LibraryEditor::newElementTriggered);
  connect(mUi->actionSave, &QAction::triggered, this,
          &LibraryEditor::saveTriggered);
  connect(mUi->actionRemoveElement, &QAction::triggered, this,
          &LibraryEditor::removeElementTriggered);
  connect(mUi->actionUpdateLibraryDb, &QAction::triggered,
          &mWorkspace.getLibraryDb(),
          &workspace::WorkspaceLibraryDb::startLibraryRescan);
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
  connect(mUi->actionAbout, &QAction::triggered, qApp, &Application::about);
  connect(mUi->actionAbout_Qt, &QAction::triggered, qApp,
          &QApplication::aboutQt);

  // lock the library directory
  mLock.tryLock();  // can throw

  // set window title
  const QStringList localeOrder =
      mWorkspace.getSettings().getLibLocaleOrder().getLocaleOrder();
  QString libName = *mLibrary->getNames().value(localeOrder);
  if (mLibrary->isOpenedReadOnly()) libName.append(tr(" [Read-Only]"));
  setWindowTitle(QString(tr("%1 - LibrePCB Library Editor")).arg(libName));

  // setup status bar
  mUi->statusBar->setFields(StatusBar::ProgressBar);
  mUi->statusBar->setProgressBarTextFormat(tr("Scanning libraries (%p%)"));
  connect(&mWorkspace.getLibraryDb(),
          &workspace::WorkspaceLibraryDb::scanStarted, mUi->statusBar,
          &StatusBar::showProgressBar, Qt::QueuedConnection);
  connect(&mWorkspace.getLibraryDb(),
          &workspace::WorkspaceLibraryDb::scanSucceeded, mUi->statusBar,
          &StatusBar::hideProgressBar, Qt::QueuedConnection);
  connect(&mWorkspace.getLibraryDb(),
          &workspace::WorkspaceLibraryDb::scanProgressUpdate, mUi->statusBar,
          &StatusBar::setProgressBarPercent, Qt::QueuedConnection);

  // if the library was opened in read-only mode, we guess that it's a remote
  // library and thus show a warning that all modifications are lost after the
  // next update
  mUi->lblRemoteLibraryWarning->setVisible(mLibrary->isOpenedReadOnly());

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
  addLayer(GraphicsLayer::sSymbolHiddenGrabAreas,
           true);  // force it to be visible!
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
  addLayer(GraphicsLayer::sTopHiddenGrabAreas,
           true);  // force it to be visible!
  addLayer(GraphicsLayer::sBotHiddenGrabAreas,
           true);  // force it to be visible!
  addLayer(GraphicsLayer::sTopPlacement);
  addLayer(GraphicsLayer::sBotPlacement);
  addLayer(GraphicsLayer::sTopDocumentation);
  addLayer(GraphicsLayer::sBotDocumentation);
  addLayer(GraphicsLayer::sTopNames);
  addLayer(GraphicsLayer::sBotNames);
  addLayer(GraphicsLayer::sTopValues);
  addLayer(GraphicsLayer::sBotValues);
  addLayer(GraphicsLayer::sTopCourtyard);
  addLayer(GraphicsLayer::sBotCourtyard);
  addLayer(GraphicsLayer::sTopStopMask);
  addLayer(GraphicsLayer::sBotStopMask);
  addLayer(GraphicsLayer::sTopSolderPaste);
  addLayer(GraphicsLayer::sBotSolderPaste);
  addLayer(GraphicsLayer::sTopGlue);
  addLayer(GraphicsLayer::sBotGlue);

  // add debug layers
#ifdef QT_DEBUG
  addLayer(GraphicsLayer::sDebugGraphicsItemsBoundingRects);
  addLayer(GraphicsLayer::sDebugGraphicsItemsTextsBoundingRects);
  addLayer(GraphicsLayer::sDebugSymbolPinNetSignalNames);
  addLayer(GraphicsLayer::sDebugNetLinesNetSignalNames);
  addLayer(GraphicsLayer::sDebugInvisibleNetPoints);
  addLayer(GraphicsLayer::sDebugComponentSymbolsCounts);
#endif

  // add overview tab
  EditorWidgetBase::Context context{mWorkspace, *this, false};
  LibraryOverviewWidget*    overviewWidget =
      new LibraryOverviewWidget(context, lib);
  connect(overviewWidget, &LibraryOverviewWidget::windowTitleChanged, this,
          &LibraryEditor::updateTabTitles);
  connect(overviewWidget, &LibraryOverviewWidget::dirtyChanged, this,
          &LibraryEditor::updateTabTitles);
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
      if (!tabCloseRequested(i)) {
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
 *  GUI Event Handlers
 ******************************************************************************/

void LibraryEditor::newElementTriggered() noexcept {
  NewElementWizard wizard(mWorkspace, *mLibrary, *this, this);
  if (wizard.exec() == QDialog::Accepted) {
    FilePath fp = wizard.getContext().getOutputDirectory();
    switch (wizard.getContext().mElementType) {
      case NewElementWizardContext::ElementType::ComponentCategory:
        editLibraryElementTriggered<ComponentCategory,
                                    ComponentCategoryEditorWidget>(fp, true);
        break;
      case NewElementWizardContext::ElementType::PackageCategory:
        editLibraryElementTriggered<PackageCategory,
                                    PackageCategoryEditorWidget>(fp, true);
        break;
      case NewElementWizardContext::ElementType::Symbol:
        editLibraryElementTriggered<Symbol, SymbolEditorWidget>(fp, true);
        break;
      case NewElementWizardContext::ElementType::Package:
        editLibraryElementTriggered<Package, PackageEditorWidget>(fp, true);
        break;
      case NewElementWizardContext::ElementType::Component:
        editLibraryElementTriggered<Component, ComponentEditorWidget>(fp, true);
        break;
      case NewElementWizardContext::ElementType::Device:
        editLibraryElementTriggered<Device, DeviceEditorWidget>(fp, true);
        break;
      default:
        break;
    }
    mWorkspace.getLibraryDb().startLibraryRescan();
  }
}

void LibraryEditor::saveTriggered() noexcept {
  if (mCurrentEditorWidget) mCurrentEditorWidget->save();
}

void LibraryEditor::removeElementTriggered() noexcept {
  if (!mCurrentEditorWidget) return;
  if (dynamic_cast<LibraryOverviewWidget*>(mCurrentEditorWidget)) return;
  int ret = QMessageBox::warning(
      this, tr("Remove library element"),
      QString(tr("WARNING: Library elements must normally NOT be removed "
                 "because this will break "
                 "other elements which depend on this one! They should be just "
                 "marked as "
                 "deprecated instead.\n\nAre you still sure to delete the "
                 "whole library element "
                 "\"%1\"?\n\nThis cannot be undone!"))
          .arg(mCurrentEditorWidget->windowTitle()),
      QMessageBox::Yes, QMessageBox::Cancel);
  if (ret == QMessageBox::Yes) {
    FilePath elementDir = mCurrentEditorWidget->getFilePath();
    mUi->tabWidget->removeTab(mUi->tabWidget->currentIndex());
    setActiveEditorWidget(nullptr);
    delete mCurrentEditorWidget;
    try {
      FileUtils::removeDirRecursively(elementDir);
    } catch (const Exception& e) {
      QMessageBox::critical(this, tr("Error"), e.getMsg());
    }
    mWorkspace.getLibraryDb().startLibraryRescan();
  }
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

void LibraryEditor::editComponentCategoryTriggered(
    const FilePath& fp) noexcept {
  editLibraryElementTriggered<ComponentCategory, ComponentCategoryEditorWidget>(
      fp, false);
}

void LibraryEditor::editPackageCategoryTriggered(const FilePath& fp) noexcept {
  editLibraryElementTriggered<PackageCategory, PackageCategoryEditorWidget>(
      fp, false);
}

void LibraryEditor::editSymbolTriggered(const FilePath& fp) noexcept {
  editLibraryElementTriggered<Symbol, SymbolEditorWidget>(fp, false);
}

void LibraryEditor::editPackageTriggered(const FilePath& fp) noexcept {
  editLibraryElementTriggered<Package, PackageEditorWidget>(fp, false);
}

void LibraryEditor::editComponentTriggered(const FilePath& fp) noexcept {
  editLibraryElementTriggered<Component, ComponentEditorWidget>(fp, false);
}

void LibraryEditor::editDeviceTriggered(const FilePath& fp) noexcept {
  editLibraryElementTriggered<Device, DeviceEditorWidget>(fp, false);
}

template <typename ElementType, typename EditWidgetType>
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

    EditorWidgetBase::Context context{mWorkspace, *this, isNewElement};
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

bool LibraryEditor::tabCloseRequested(int index) noexcept {
  EditorWidgetBase* widget =
      dynamic_cast<EditorWidgetBase*>(mUi->tabWidget->widget(index));
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
  mUi->editToolbar->setEnabled(hasGraphicalEditor);
  mUi->viewToolbar->setEnabled(hasGraphicalEditor);
  mUi->commandToolbar->setEnabled(hasGraphicalEditor);
  mUi->statusBar->setField(StatusBar::AbsolutePosition, hasGraphicalEditor);
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
