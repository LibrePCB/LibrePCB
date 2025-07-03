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

#include "libraryeditorlegacy.h"

#include "../editorcommandset.h"
#include "../graphics/graphicslayerlist.h"
#include "../utils/exclusiveactiongroup.h"
#include "../utils/menubuilder.h"
#include "../utils/standardeditorcommandhandler.h"
#include "../utils/undostackactiongroup.h"
#include "../widgets/openglview.h"
#include "../widgets/searchtoolbar.h"
#include "cat/componentcategoryeditorwidget.h"
#include "cat/packagecategoryeditorwidget.h"
#include "cmp/componenteditorwidget.h"
#include "dev/deviceeditorwidget.h"
#include "eaglelibraryimportwizard/eaglelibraryimportwizard.h"
#include "kicadlibraryimportwizard/kicadlibraryimportwizard.h"
#include "lib/libraryoverviewwidget.h"
#include "pkg/packageeditorwidget.h"
#include "sym/symboleditorwidget.h"
#include "ui_libraryeditorlegacy.h"

#include <librepcb/core/application.h>
#include <librepcb/core/exceptions.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/library.h>
#include <librepcb/core/types/layer.h>
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

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

LibraryEditorLegacy::LibraryEditorLegacy(Workspace& ws, const FilePath& libFp,
                                         bool readOnly)
  : QMainWindow(nullptr),
    mWorkspace(ws),
    mIsOpenedReadOnly(readOnly),
    mUi(new Ui::LibraryEditorLegacy),
    mStandardCommandHandler(
        new StandardEditorCommandHandler(mWorkspace.getSettings(), this)),
    mLayers(GraphicsLayerList::libraryLayers(&mWorkspace.getSettings())),
    mCurrentEditorWidget(nullptr),
    mLibrary(nullptr) {
  mUi->setupUi(this);

  // Workaround for automatically closing window when opening 3D viewer,
  // see https://github.com/LibrePCB/LibrePCB/issues/1363.
  {
    QOpenGLWidget* w = new QOpenGLWidget(this);
    w->hide();
  }

  // Create all actions, window menus, toolbars and dock widgets.
  createActions();
  createToolBars();
  createMenus();

  // If the library was opened in read-only mode, we guess that it's a remote
  // library and thus show a warning that all modifications are lost after the
  // next update.
  mUi->lblRemoteLibraryWarning->setVisible(readOnly);

  // Setup status bar.
  mUi->statusBar->setFields(StatusBar::ProgressBar);
  mUi->statusBar->setProgressBarTextFormat(tr("Scanning libraries (%p%)"));
  connect(&mWorkspace.getLibraryDb(), &WorkspaceLibraryDb::scanProgressUpdate,
          mUi->statusBar, &StatusBar::setProgressBarPercent,
          Qt::QueuedConnection);
  mUi->statusBar->setProgressBarPercent(
      mWorkspace.getLibraryDb().getScanProgressPercent());

  // Add overview tab.
  LibraryOverviewWidget* overviewWidget =
      new LibraryOverviewWidget(createContext(false), libFp);
  mLibrary = &overviewWidget->getLibrary();
  mUi->tabWidget->addTab(overviewWidget, overviewWidget->windowIcon(),
                         overviewWidget->windowTitle());
  tabCountChanged();
  connect(overviewWidget, &LibraryOverviewWidget::windowTitleChanged, this,
          &LibraryEditorLegacy::updateTabTitles);
  connect(overviewWidget, &LibraryOverviewWidget::dirtyChanged, this,
          &LibraryEditorLegacy::updateTabTitles);
  connect(overviewWidget, &EditorWidgetBase::elementEdited,
          &mWorkspace.getLibraryDb(), &WorkspaceLibraryDb::startLibraryRescan);
  connect(overviewWidget, &LibraryOverviewWidget::newComponentCategoryTriggered,
          this, &LibraryEditorLegacy::newComponentCategoryTriggered);
  connect(overviewWidget, &LibraryOverviewWidget::newPackageCategoryTriggered,
          this, &LibraryEditorLegacy::newPackageCategoryTriggered);
  connect(overviewWidget, &LibraryOverviewWidget::newSymbolTriggered, this,
          &LibraryEditorLegacy::newSymbolTriggered);
  connect(overviewWidget, &LibraryOverviewWidget::newPackageTriggered, this,
          &LibraryEditorLegacy::newPackageTriggered);
  connect(overviewWidget, &LibraryOverviewWidget::newComponentTriggered, this,
          &LibraryEditorLegacy::newComponentTriggered);
  connect(overviewWidget, &LibraryOverviewWidget::newDeviceTriggered, this,
          &LibraryEditorLegacy::newDeviceTriggered);
  connect(overviewWidget,
          &LibraryOverviewWidget::editComponentCategoryTriggered, this,
          &LibraryEditorLegacy::editComponentCategoryTriggered);
  connect(overviewWidget, &LibraryOverviewWidget::editPackageCategoryTriggered,
          this, &LibraryEditorLegacy::editPackageCategoryTriggered);
  connect(overviewWidget, &LibraryOverviewWidget::editSymbolTriggered, this,
          &LibraryEditorLegacy::editSymbolTriggered);
  connect(overviewWidget, &LibraryOverviewWidget::editPackageTriggered, this,
          &LibraryEditorLegacy::editPackageTriggered);
  connect(overviewWidget, &LibraryOverviewWidget::editComponentTriggered, this,
          &LibraryEditorLegacy::editComponentTriggered);
  connect(overviewWidget, &LibraryOverviewWidget::editDeviceTriggered, this,
          &LibraryEditorLegacy::editDeviceTriggered);
  connect(overviewWidget,
          &LibraryOverviewWidget::duplicateComponentCategoryTriggered, this,
          &LibraryEditorLegacy::duplicateComponentCategoryTriggered);
  connect(overviewWidget,
          &LibraryOverviewWidget::duplicatePackageCategoryTriggered, this,
          &LibraryEditorLegacy::duplicatePackageCategoryTriggered);
  connect(overviewWidget, &LibraryOverviewWidget::duplicateSymbolTriggered,
          this, &LibraryEditorLegacy::duplicateSymbolTriggered);
  connect(overviewWidget, &LibraryOverviewWidget::duplicatePackageTriggered,
          this, &LibraryEditorLegacy::duplicatePackageTriggered);
  connect(overviewWidget, &LibraryOverviewWidget::duplicateComponentTriggered,
          this, &LibraryEditorLegacy::duplicateComponentTriggered);
  connect(overviewWidget, &LibraryOverviewWidget::duplicateDeviceTriggered,
          this, &LibraryEditorLegacy::duplicateDeviceTriggered);
  connect(overviewWidget, &LibraryOverviewWidget::removeElementTriggered, this,
          &LibraryEditorLegacy::closeTabIfOpen);

  // Remove close button on first tab (which is the library overview).
  QTabBar* tabBar = mUi->tabWidget->tabBar();
  Q_ASSERT(tabBar);
  tabBar->setTabButton(0, QTabBar::RightSide, nullptr);

  // Set window title and icon.
  const QStringList localeOrder =
      mWorkspace.getSettings().libraryLocaleOrder.get();
  QString libName = *mLibrary->getNames().value(localeOrder);
  if (readOnly) libName.append(tr(" [Read-Only]"));
  setWindowTitle(tr("%1 - LibrePCB Library Editor").arg(libName));
  setWindowIcon(mLibrary->getIconAsPixmap());

  // Open the overview tab.
  setActiveEditorWidget(overviewWidget);
  connect(mUi->tabWidget, &QTabWidget::currentChanged, this,
          &LibraryEditorLegacy::currentTabChanged);
  connect(mUi->tabWidget, &QTabWidget::tabCloseRequested, this,
          &LibraryEditorLegacy::tabCloseRequested);

  // Restore window geometry.
  QSettings clientSettings;
  restoreGeometry(
      clientSettings.value("library_editor/window_geometry").toByteArray());
  restoreState(
      clientSettings.value("library_editor/window_state_v2").toByteArray());
}

LibraryEditorLegacy::~LibraryEditorLegacy() noexcept {
  setActiveEditorWidget(nullptr);
  mLibrary = nullptr;
  closeAllTabs(true, false);
}

/*******************************************************************************
 *  Public Methods
 ******************************************************************************/

bool LibraryEditorLegacy::closeAndDestroy(bool askForSave) noexcept {
  // Close tabs.
  if (!closeAllTabs(true, askForSave)) {
    return false;
  }

  // Save Window Geometry
  QSettings clientSettings;
  clientSettings.setValue("library_editor/window_geometry", saveGeometry());
  clientSettings.setValue("library_editor/window_state_v2", saveState());

  deleteLater();
  return true;
}

/*******************************************************************************
 *  Public Slots
 ******************************************************************************/

void LibraryEditorLegacy::closeTabIfOpen(const FilePath& fp) noexcept {
  for (int i = 0; i < mUi->tabWidget->count(); i++) {
    EditorWidgetBase* widget =
        dynamic_cast<EditorWidgetBase*>(mUi->tabWidget->widget(i));
    if (widget && (widget->getFilePath() == fp)) {
      QWidget* widget = mUi->tabWidget->widget(i);
      mUi->tabWidget->removeTab(i);
      delete widget;
      tabCountChanged();
      return;
    }
  }
}

/*******************************************************************************
 *  GUI Event Handlers
 ******************************************************************************/

void LibraryEditorLegacy::newComponentCategoryTriggered() noexcept {
  newLibraryElement(NewElementWizardContext::ElementType::ComponentCategory);
}

void LibraryEditorLegacy::newPackageCategoryTriggered() noexcept {
  newLibraryElement(NewElementWizardContext::ElementType::PackageCategory);
}

void LibraryEditorLegacy::newSymbolTriggered() noexcept {
  newLibraryElement(NewElementWizardContext::ElementType::Symbol);
}

void LibraryEditorLegacy::newPackageTriggered() noexcept {
  newLibraryElement(NewElementWizardContext::ElementType::Package);
}

void LibraryEditorLegacy::newComponentTriggered() noexcept {
  newLibraryElement(NewElementWizardContext::ElementType::Component);
}

void LibraryEditorLegacy::newDeviceTriggered() noexcept {
  newLibraryElement(NewElementWizardContext::ElementType::Device);
}

void LibraryEditorLegacy::editComponentCategoryTriggered(
    const FilePath& fp) noexcept {
  editLibraryElementTriggered<ComponentCategoryEditorWidget>(fp, false);
}

void LibraryEditorLegacy::editPackageCategoryTriggered(
    const FilePath& fp) noexcept {
  editLibraryElementTriggered<PackageCategoryEditorWidget>(fp, false);
}

void LibraryEditorLegacy::editSymbolTriggered(const FilePath& fp) noexcept {
  editLibraryElementTriggered<SymbolEditorWidget>(fp, false);
}

void LibraryEditorLegacy::editPackageTriggered(const FilePath& fp) noexcept {
  editLibraryElementTriggered<PackageEditorWidget>(fp, false);
}

void LibraryEditorLegacy::editComponentTriggered(const FilePath& fp) noexcept {
  editLibraryElementTriggered<ComponentEditorWidget>(fp, false);
}

void LibraryEditorLegacy::editDeviceTriggered(const FilePath& fp) noexcept {
  editLibraryElementTriggered<DeviceEditorWidget>(fp, false);
}

void LibraryEditorLegacy::duplicateComponentCategoryTriggered(
    const FilePath& fp) noexcept {
  duplicateLibraryElement(
      NewElementWizardContext::ElementType::ComponentCategory, fp);
}

void LibraryEditorLegacy::duplicatePackageCategoryTriggered(
    const FilePath& fp) noexcept {
  duplicateLibraryElement(NewElementWizardContext::ElementType::PackageCategory,
                          fp);
}

void LibraryEditorLegacy::duplicateSymbolTriggered(
    const FilePath& fp) noexcept {
  duplicateLibraryElement(NewElementWizardContext::ElementType::Symbol, fp);
}

void LibraryEditorLegacy::duplicatePackageTriggered(
    const FilePath& fp) noexcept {
  duplicateLibraryElement(NewElementWizardContext::ElementType::Package, fp);
}

void LibraryEditorLegacy::duplicateComponentTriggered(
    const FilePath& fp) noexcept {
  duplicateLibraryElement(NewElementWizardContext::ElementType::Component, fp);
}

void LibraryEditorLegacy::duplicateDeviceTriggered(
    const FilePath& fp) noexcept {
  duplicateLibraryElement(NewElementWizardContext::ElementType::Device, fp);
}

template <typename EditWidgetType>
void LibraryEditorLegacy::editLibraryElementTriggered(
    const FilePath& fp, bool isNewElement) noexcept {
  try {
    for (int i = 0; i < mUi->tabWidget->count(); i++) {
      EditorWidgetBase* widget =
          dynamic_cast<EditorWidgetBase*>(mUi->tabWidget->widget(i));
      if (widget && (widget->getFilePath() == fp)) {
        mUi->tabWidget->setCurrentIndex(i);
        return;
      }
    }

    EditWidgetType* widget =
        new EditWidgetType(createContext(isNewElement), fp);
    connect(widget, &QWidget::windowTitleChanged, this,
            &LibraryEditorLegacy::updateTabTitles);
    connect(widget, &EditorWidgetBase::dirtyChanged, this,
            &LibraryEditorLegacy::updateTabTitles);
    connect(widget, &EditorWidgetBase::elementEdited,
            &mWorkspace.getLibraryDb(),
            &WorkspaceLibraryDb::startLibraryRescan);
    int index = mUi->tabWidget->addTab(widget, widget->windowIcon(),
                                       widget->windowTitle());
    mUi->tabWidget->setCurrentIndex(index);
    tabCountChanged();
  } catch (const UserCanceled& e) {
    // User requested to abort -> do nothing.
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Failed to open category"), e.getMsg());
  }
}

void LibraryEditorLegacy::currentTabChanged(int index) noexcept {
  setActiveEditorWidget(
      dynamic_cast<EditorWidgetBase*>(mUi->tabWidget->widget(index)));
}

void LibraryEditorLegacy::tabCloseRequested(int index) noexcept {
  const EditorWidgetBase* widget =
      dynamic_cast<const EditorWidgetBase*>(mUi->tabWidget->widget(index));
  if (widget &&
      widget->getAvailableFeatures().contains(
          EditorWidgetBase::Feature::Close)) {
    closeTab(index);
  }
}

bool LibraryEditorLegacy::closeTab(int index) noexcept {
  // Get editor widget reference
  EditorWidgetBase* widget =
      dynamic_cast<EditorWidgetBase*>(mUi->tabWidget->widget(index));
  if (widget == nullptr) {
    qCritical()
        << "Cannot close tab, widget is not an EditorWidgetBase subclass.";
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
        if (!widget->save()) {
          return false;
        }
        break;
      case QMessageBox::No:
        break;
      default:
        return false;
    }
  }
  if (widget == mCurrentEditorWidget) {
    setActiveEditorWidget(nullptr);
  }
  delete widget;
  tabCountChanged();
  return true;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void LibraryEditorLegacy::createActions() noexcept {
  const EditorCommandSet& cmd = EditorCommandSet::instance();

  mActionAboutLibrePcb.reset(cmd.aboutLibrePcb.createAction(
      this, this, &LibraryEditorLegacy::aboutLibrePcbRequested));
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
  mActionSave.reset(cmd.save.createAction(this, this, [this]() {
    if (mCurrentEditorWidget) mCurrentEditorWidget->save();
  }));
  mActionSave->setEnabled(!mIsOpenedReadOnly);
  mActionSaveAll.reset(cmd.saveAll.createAction(this, this, [this]() {
    for (int i = 0; i < mUi->tabWidget->count(); ++i) {
      if (auto widget =
              dynamic_cast<EditorWidgetBase*>(mUi->tabWidget->widget(i))) {
        widget->save();
      }
    }
  }));
  mActionSaveAll->setEnabled(!mIsOpenedReadOnly);
  mActionCloseTab.reset(cmd.tabClose.createAction(this, this, [this]() {
    tabCloseRequested(mUi->tabWidget->currentIndex());
  }));
  mActionCloseAllTabs.reset(cmd.tabCloseAll.createAction(
      this, this, [this]() { closeAllTabs(false, true); }));
  mActionCloseWindow.reset(
      cmd.windowClose.createAction(this, this, &LibraryEditorLegacy::close));
  mActionQuit.reset(cmd.applicationQuit.createAction(
      this, qApp, &QApplication::closeAllWindows,
      EditorCommand::ActionFlag::QueuedConnection));
  mActionFileManager.reset(cmd.fileManager.createAction(this, this, [this]() {
    if (mCurrentEditorWidget) {
      mStandardCommandHandler->fileManager(mCurrentEditorWidget->getFilePath());
    }
  }));
  mActionRescanLibraries.reset(cmd.workspaceLibrariesRescan.createAction(
      this, &mWorkspace.getLibraryDb(),
      &WorkspaceLibraryDb::startLibraryRescan));
  mActionImportDxf.reset(cmd.importDxf.createAction(this, this, [this]() {
    if (mCurrentEditorWidget) mCurrentEditorWidget->importDxf();
  }));
  mActionImportEagleLibrary.reset(
      cmd.importEagleLibrary.createAction(this, this, [this]() {
        EagleLibraryImportWizard wizard(
            mWorkspace, mLibrary->getDirectory().getAbsPath(), this);
        wizard.exec();
      }));
  mActionImportEagleLibrary->setEnabled(!mIsOpenedReadOnly);
  mActionImportKiCadLibrary.reset(
      cmd.importKiCadLibrary.createAction(this, this, [this]() {
        KiCadLibraryImportWizard wizard(
            mWorkspace, mLibrary->getDirectory().getAbsPath(), this);
        wizard.exec();
      }));
  mActionImportKiCadLibrary->setEnabled(!mIsOpenedReadOnly);
  mActionExportImage.reset(cmd.exportImage.createAction(this, this, [this]() {
    if (mCurrentEditorWidget) mCurrentEditorWidget->exportImage();
  }));
  mActionExportPdf.reset(cmd.exportPdf.createAction(this, this, [this]() {
    if (mCurrentEditorWidget) mCurrentEditorWidget->exportPdf();
  }));
  mActionPrint.reset(cmd.print.createAction(this, this, [this]() {
    if (mCurrentEditorWidget) mCurrentEditorWidget->print();
  }));
  mActionNewElement.reset(
      cmd.libraryElementNew.createAction(this, this, [this]() {
        NewElementWizard wizard(mWorkspace, *mLibrary, *mLayers, this);
        if (wizard.exec() == QDialog::Accepted) {
          FilePath fp = wizard.getContext().getOutputDirectory();
          editNewLibraryElement(wizard.getContext().mElementType, fp);
          mWorkspace.getLibraryDb().startLibraryRescan();
        }
      }));
  mActionNewElement->setEnabled(!mIsOpenedReadOnly);
  mActionNextPage.reset(cmd.pageNext.createAction(this, this, [this]() {
    const int newIndex = mUi->tabWidget->currentIndex() + 1;
    if (newIndex < mUi->tabWidget->count()) {
      mUi->tabWidget->setCurrentIndex(newIndex);
    }
  }));
  addAction(mActionNextPage.data());
  mActionPreviousPage.reset(cmd.pagePrevious.createAction(this, this, [this]() {
    const int newIndex = mUi->tabWidget->currentIndex() - 1;
    if (newIndex >= 0) {
      mUi->tabWidget->setCurrentIndex(newIndex);
    }
  }));
  addAction(mActionPreviousPage.data());
  mActionFind.reset(cmd.find.createAction(this));
  mActionSelectAll.reset(cmd.selectAll.createAction(this, this, [this]() {
    if (mCurrentEditorWidget) mCurrentEditorWidget->selectAll();
  }));
  mActionGridProperties.reset(
      cmd.gridProperties.createAction(this, this, [this]() {
        if (mCurrentEditorWidget) mCurrentEditorWidget->editGridProperties();
      }));
  mActionGridIncrease.reset(cmd.gridIncrease.createAction(this, this, [this]() {
    if (mCurrentEditorWidget) mCurrentEditorWidget->increaseGridInterval();
  }));
  mActionGridDecrease.reset(cmd.gridDecrease.createAction(this, this, [this]() {
    if (mCurrentEditorWidget) mCurrentEditorWidget->decreaseGridInterval();
  }));
  mActionToggleBgImage.reset(
      cmd.toggleBackgroundImage.createAction(this, this, [this]() {
        if (mCurrentEditorWidget) {
          const bool enabled = mCurrentEditorWidget->toggleBackgroundImage();
          mActionToggleBgImage->setCheckable(enabled);
          mActionToggleBgImage->setChecked(enabled);
        }
      }));
  mActionZoomFit.reset(cmd.zoomFitContent.createAction(this, this, [this]() {
    if (mCurrentEditorWidget) mCurrentEditorWidget->zoomAll();
  }));
  mActionZoomIn.reset(cmd.zoomIn.createAction(this, this, [this]() {
    if (mCurrentEditorWidget) mCurrentEditorWidget->zoomIn();
  }));
  mActionZoomOut.reset(cmd.zoomOut.createAction(this, this, [this]() {
    if (mCurrentEditorWidget) mCurrentEditorWidget->zoomOut();
  }));
  mActionToggle3D.reset(cmd.toggle3d.createAction(this, this, [this]() {
    if (mCurrentEditorWidget) mCurrentEditorWidget->toggle3D();
  }));
  mActionUndo.reset(cmd.undo.createAction(this));
  mActionRedo.reset(cmd.redo.createAction(this));
  mActionCut.reset(cmd.clipboardCut.createAction(this, this, [this]() {
    if (mCurrentEditorWidget) mCurrentEditorWidget->cut();
  }));
  mActionCopy.reset(cmd.clipboardCopy.createAction(this, this, [this]() {
    if (mCurrentEditorWidget) mCurrentEditorWidget->copy();
  }));
  mActionPaste.reset(cmd.clipboardPaste.createAction(this, this, [this]() {
    if (mCurrentEditorWidget) mCurrentEditorWidget->paste();
  }));
  mActionMoveLeft.reset(cmd.moveLeft.createAction(this, this, [this]() {
    if (mCurrentEditorWidget) mCurrentEditorWidget->move(Qt::LeftArrow);
  }));
  addAction(mActionMoveLeft.data());
  mActionMoveRight.reset(cmd.moveRight.createAction(this, this, [this]() {
    if (mCurrentEditorWidget) mCurrentEditorWidget->move(Qt::RightArrow);
  }));
  addAction(mActionMoveRight.data());
  mActionMoveUp.reset(cmd.moveUp.createAction(this, this, [this]() {
    if (mCurrentEditorWidget) mCurrentEditorWidget->move(Qt::UpArrow);
  }));
  addAction(mActionMoveUp.data());
  mActionMoveDown.reset(cmd.moveDown.createAction(this, this, [this]() {
    if (mCurrentEditorWidget) mCurrentEditorWidget->move(Qt::DownArrow);
  }));
  addAction(mActionMoveDown.data());
  mActionRotateCcw.reset(cmd.rotateCcw.createAction(this, this, [this]() {
    if (mCurrentEditorWidget) mCurrentEditorWidget->rotate(Angle::deg90());
  }));
  mActionRotateCw.reset(cmd.rotateCw.createAction(this, this, [this]() {
    if (mCurrentEditorWidget) mCurrentEditorWidget->rotate(-Angle::deg90());
  }));
  mActionMirrorHorizontal.reset(
      cmd.mirrorHorizontal.createAction(this, this, [this]() {
        if (mCurrentEditorWidget) mCurrentEditorWidget->mirror(Qt::Horizontal);
      }));
  mActionMirrorVertical.reset(
      cmd.mirrorVertical.createAction(this, this, [this]() {
        if (mCurrentEditorWidget) mCurrentEditorWidget->mirror(Qt::Vertical);
      }));
  mActionFlipHorizontal.reset(
      cmd.flipHorizontal.createAction(this, this, [this]() {
        if (mCurrentEditorWidget) mCurrentEditorWidget->flip(Qt::Horizontal);
      }));
  mActionFlipVertical.reset(cmd.flipVertical.createAction(this, this, [this]() {
    if (mCurrentEditorWidget) mCurrentEditorWidget->flip(Qt::Vertical);
  }));
  mActionMoveAlign.reset(cmd.moveAlign.createAction(this, this, [this]() {
    if (mCurrentEditorWidget) mCurrentEditorWidget->moveAlign();
  }));
  mActionSnapToGrid.reset(cmd.snapToGrid.createAction(this, this, [this]() {
    if (mCurrentEditorWidget) mCurrentEditorWidget->snapToGrid();
  }));
  mActionProperties.reset(cmd.properties.createAction(this, this, [this]() {
    if (mCurrentEditorWidget) mCurrentEditorWidget->editProperties();
  }));
  mActionRemove.reset(cmd.remove.createAction(this, this, [this]() {
    if (mCurrentEditorWidget) mCurrentEditorWidget->remove();
  }));
  mActionHelperTools.reset(cmd.helperTools.createAction(this));
  mActionGenerateOutline.reset(
      cmd.toolGenerateOutline.createAction(this, this, [this]() {
        if (mCurrentEditorWidget)
          mCurrentEditorWidget->processGenerateOutline();
      }));
  mActionGenerateCourtyard.reset(
      cmd.toolGenerateCourtyard.createAction(this, this, [this]() {
        if (mCurrentEditorWidget)
          mCurrentEditorWidget->processGenerateCourtyard();
      }));
  mActionAbort.reset(cmd.abort.createAction(this, this, [this]() {
    if (mCurrentEditorWidget) mCurrentEditorWidget->abortCommand();
  }));
  mActionToolSelect.reset(cmd.toolSelect.createAction(this));
  mActionToolLine.reset(cmd.toolLine.createAction(this));
  mActionToolRect.reset(cmd.toolRect.createAction(this));
  mActionToolPolygon.reset(cmd.toolPolygon.createAction(this));
  mActionToolCircle.reset(cmd.toolCircle.createAction(this));
  mActionToolArc.reset(cmd.toolArc.createAction(this));
  mActionToolText.reset(cmd.toolText.createAction(this));
  mActionToolName.reset(cmd.toolName.createAction(this));
  mActionToolValue.reset(cmd.toolValue.createAction(this));
  mActionToolPin.reset(cmd.toolPin.createAction(this));
  mActionToolSmtPadStandard.reset(cmd.toolPadSmt.createAction(this));
  mActionToolThtPad.reset(cmd.toolPadTht.createAction(this));
  mActionToolSpecialPadThermal.reset(cmd.toolPadThermal.createAction(this));
  mActionToolSpecialPadBga.reset(cmd.toolPadBga.createAction(this));
  mActionToolSpecialPadEdgeConnector.reset(
      cmd.toolPadEdgeConnector.createAction(this));
  mActionToolSpecialPadTest.reset(cmd.toolPadTest.createAction(this));
  mActionToolSpecialPadLocalFiducial.reset(
      cmd.toolPadLocalFiducial.createAction(this));
  mActionToolSpecialPadGlobalFiducial.reset(
      cmd.toolPadGlobalFiducial.createAction(this));
  mActionToolZone.reset(cmd.toolZone.createAction(this));
  mActionToolHole.reset(cmd.toolHole.createAction(this));
  mActionToolMeasure.reset(cmd.toolMeasure.createAction(this));
  mActionReNumberPads.reset(cmd.toolReNumberPads.createAction(this));

  // Undo stack action group.
  mUndoStackActionGroup.reset(new UndoStackActionGroup(
      *mActionUndo, *mActionRedo, nullptr, nullptr, this));

  // Tools action group.
  mToolsActionGroup.reset(new ExclusiveActionGroup());
  mToolsActionGroup->addAction(mActionToolSelect.data(),
                               EditorWidgetBase::Tool::SELECT);
  mToolsActionGroup->addAction(mActionToolLine.data(),
                               EditorWidgetBase::Tool::DRAW_LINE);
  mToolsActionGroup->addAction(mActionToolRect.data(),
                               EditorWidgetBase::Tool::DRAW_RECT);
  mToolsActionGroup->addAction(mActionToolPolygon.data(),
                               EditorWidgetBase::Tool::DRAW_POLYGON);
  mToolsActionGroup->addAction(mActionToolCircle.data(),
                               EditorWidgetBase::Tool::DRAW_CIRCLE);
  mToolsActionGroup->addAction(mActionToolArc.data(),
                               EditorWidgetBase::Tool::DRAW_ARC);
  mToolsActionGroup->addAction(mActionToolName.data(),
                               EditorWidgetBase::Tool::ADD_NAMES);
  mToolsActionGroup->addAction(mActionToolValue.data(),
                               EditorWidgetBase::Tool::ADD_VALUES);
  mToolsActionGroup->addAction(mActionToolText.data(),
                               EditorWidgetBase::Tool::DRAW_TEXT);
  mToolsActionGroup->addAction(mActionToolPin.data(),
                               EditorWidgetBase::Tool::ADD_PINS);
  mToolsActionGroup->addAction(mActionToolThtPad.data(),
                               EditorWidgetBase::Tool::ADD_THT_PADS);
  mToolsActionGroup->addAction(
      mActionToolSmtPadStandard.data(), EditorWidgetBase::Tool::ADD_SMT_PADS,
      QVariant::fromValue(FootprintPad::Function::StandardPad));
  mToolsActionGroup->addAction(
      mActionToolSpecialPadThermal.data(), EditorWidgetBase::Tool::ADD_SMT_PADS,
      QVariant::fromValue(FootprintPad::Function::ThermalPad));
  mToolsActionGroup->addAction(
      mActionToolSpecialPadBga.data(), EditorWidgetBase::Tool::ADD_SMT_PADS,
      QVariant::fromValue(FootprintPad::Function::BgaPad));
  mToolsActionGroup->addAction(
      mActionToolSpecialPadEdgeConnector.data(),
      EditorWidgetBase::Tool::ADD_SMT_PADS,
      QVariant::fromValue(FootprintPad::Function::EdgeConnectorPad));
  mToolsActionGroup->addAction(
      mActionToolSpecialPadTest.data(), EditorWidgetBase::Tool::ADD_SMT_PADS,
      QVariant::fromValue(FootprintPad::Function::TestPad));
  mToolsActionGroup->addAction(
      mActionToolSpecialPadLocalFiducial.data(),
      EditorWidgetBase::Tool::ADD_SMT_PADS,
      QVariant::fromValue(FootprintPad::Function::LocalFiducial));
  mToolsActionGroup->addAction(
      mActionToolSpecialPadGlobalFiducial.data(),
      EditorWidgetBase::Tool::ADD_SMT_PADS,
      QVariant::fromValue(FootprintPad::Function::GlobalFiducial));
  mToolsActionGroup->addAction(mActionToolZone.data(),
                               EditorWidgetBase::Tool::DRAW_ZONE);
  mToolsActionGroup->addAction(mActionToolHole.data(),
                               EditorWidgetBase::Tool::ADD_HOLES);
  mToolsActionGroup->addAction(mActionToolMeasure.data(),
                               EditorWidgetBase::Tool::MEASURE);
  mToolsActionGroup->addAction(mActionReNumberPads.data(),
                               EditorWidgetBase::Tool::RENUMBER_PADS);
  mToolsActionGroup->setEnabled(false);
}

void LibraryEditorLegacy::createToolBars() noexcept {
  // File.
  mToolBarFile.reset(new QToolBar(tr("File"), this));
  mToolBarFile->setObjectName("toolBarFile");
  mToolBarFile->addAction(mActionNewElement.data());
  mToolBarFile->addAction(mActionSave.data());
  mToolBarFile->addAction(mActionPrint.data());
  mToolBarFile->addAction(mActionExportPdf.data());
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
  mToolBarEdit->addAction(mActionMirrorHorizontal.data());
  mToolBarEdit->addAction(mActionFlipHorizontal.data());
  addToolBar(Qt::TopToolBarArea, mToolBarEdit.data());

  // View.
  mToolBarView.reset(new QToolBar(tr("View"), this));
  mToolBarView->setObjectName("toolBarView");
  mToolBarView->addAction(mActionGridProperties.data());
  mToolBarView->addAction(mActionToggleBgImage.data());
  mToolBarView->addAction(mActionZoomIn.data());
  mToolBarView->addAction(mActionZoomOut.data());
  mToolBarView->addAction(mActionZoomFit.data());
  mToolBarView->addAction(mActionToggle3D.data());
  addToolBar(Qt::TopToolBarArea, mToolBarView.data());

  // Search.
  mToolBarSearch.reset(new SearchToolBar(this));
  mToolBarSearch->setObjectName("toolBarSearch");
  mToolBarSearch->setPlaceholderText(tr("Filter elements..."));
  connect(mActionFind.data(), &QAction::triggered, mToolBarSearch.data(),
          &SearchToolBar::selectAllAndSetFocus);
  addToolBar(Qt::TopToolBarArea, mToolBarSearch.data());
  connect(mToolBarSearch.data(), &SearchToolBar::textChanged, this,
          [this](const QString& text) {
            if (auto w = dynamic_cast<LibraryOverviewWidget*>(
                    mUi->tabWidget->widget(0))) {
              w->setFilter(text);
            } else {
              qCritical() << "Could not get overview widget in library editor.";
            }
          });

  // Command.
  mToolBarCommand.reset(new QToolBar(tr("Command"), this));
  mToolBarCommand->setObjectName("toolBarCommand");
  mToolBarCommand->addAction(mActionAbort.data());
  mToolBarCommand->addSeparator();
  addToolBarBreak(Qt::TopToolBarArea);
  addToolBar(Qt::TopToolBarArea, mToolBarCommand.data());

  // Tools.
  mToolBarTools.reset(new QToolBar(tr("Tools"), this));
  mToolBarTools->setObjectName("toolBarTools");
  mToolBarTools->addAction(mActionToolSelect.data());
  mToolBarTools->addAction(mActionToolLine.data());
  mToolBarTools->addAction(mActionToolRect.data());
  mToolBarTools->addAction(mActionToolPolygon.data());
  mToolBarTools->addAction(mActionToolCircle.data());
  mToolBarTools->addAction(mActionToolArc.data());
  mToolBarTools->addAction(mActionToolName.data());
  mToolBarTools->addAction(mActionToolValue.data());
  mToolBarTools->addAction(mActionToolText.data());
  mToolBarTools->addSeparator();
  mToolBarTools->addAction(mActionToolPin.data());
  mToolBarTools->addSeparator();
  mToolBarTools->addAction(mActionToolThtPad.data());
  mToolBarTools->addAction(mActionToolSmtPadStandard.data());
  if (auto btn = qobject_cast<QToolButton*>(
          mToolBarTools->widgetForAction(mActionToolSmtPadStandard.data()))) {
    QMenu* menu = new QMenu(mToolBarTools.data());
    menu->addAction(mActionToolSpecialPadThermal.data());
    menu->addAction(mActionToolSpecialPadBga.data());
    menu->addAction(mActionToolSpecialPadEdgeConnector.data());
    menu->addAction(mActionToolSpecialPadTest.data());
    menu->addAction(mActionToolSpecialPadLocalFiducial.data());
    menu->addAction(mActionToolSpecialPadGlobalFiducial.data());
    btn->setMenu(menu);
    btn->setPopupMode(QToolButton::DelayedPopup);
  }
  mToolBarTools->addAction(mActionToolZone.data());
  mToolBarTools->addAction(mActionToolHole.data());
  mToolBarTools->addSeparator();
  mToolBarTools->addAction(mActionHelperTools.data());
  if (auto btn = qobject_cast<QToolButton*>(
          mToolBarTools->widgetForAction(mActionHelperTools.data()))) {
    QMenu* menu = new QMenu(mToolBarTools.data());
    menu->addAction(mActionGenerateOutline.data());
    menu->addAction(mActionGenerateCourtyard.data());
    menu->addAction(mActionReNumberPads.data());
    btn->setMenu(menu);
    btn->setPopupMode(QToolButton::InstantPopup);
  }
  mToolBarTools->addSeparator();
  mToolBarTools->addAction(mActionToolMeasure.data());
  addToolBar(Qt::LeftToolBarArea, mToolBarTools.data());
}

void LibraryEditorLegacy::createMenus() noexcept {
  MenuBuilder mb(mUi->menuBar);

  // File.
  mb.newMenu(&MenuBuilder::createFileMenu);
  mb.addAction(mActionNewElement);
  mb.addAction(mActionSave);
  mb.addAction(mActionSaveAll);
  mb.addAction(mActionFileManager);
  mb.addAction(mActionRescanLibraries);
  mb.addSeparator();
  {
    MenuBuilder smb(mb.addSubMenu(&MenuBuilder::createImportMenu));
    smb.addAction(mActionImportDxf);
    smb.addAction(mActionImportEagleLibrary);
    smb.addAction(mActionImportKiCadLibrary);
  }
  {
    MenuBuilder smb(mb.addSubMenu(&MenuBuilder::createExportMenu));
    smb.addAction(mActionExportPdf);
    smb.addAction(mActionExportImage);
  }
  mb.addSeparator();
  mb.addAction(mActionPrint);
  mb.addSeparator();
  mb.addAction(mActionCloseTab);
  mb.addAction(mActionCloseAllTabs);
  mb.addAction(mActionCloseWindow);
  mb.addSeparator();
  mb.addAction(mActionQuit);

  // Edit.
  mb.newMenu(&MenuBuilder::createEditMenu);
  mb.addAction(mActionUndo);
  mb.addAction(mActionRedo);
  mb.addSeparator();
  mb.addAction(mActionSelectAll);
  mb.addSeparator();
  mb.addAction(mActionRotateCcw);
  mb.addAction(mActionRotateCw);
  mb.addAction(mActionMirrorHorizontal);
  mb.addAction(mActionMirrorVertical);
  mb.addAction(mActionFlipHorizontal);
  mb.addAction(mActionFlipVertical);
  mb.addAction(mActionMoveAlign);
  mb.addAction(mActionSnapToGrid);
  mb.addSeparator();
  mb.addAction(mActionCopy);
  mb.addAction(mActionCut);
  mb.addAction(mActionPaste);
  mb.addAction(mActionRemove);
  mb.addSeparator();
  mb.addAction(mActionFind);
  mb.addSeparator();
  mb.addAction(mActionProperties);

  // View.
  mb.newMenu(&MenuBuilder::createViewMenu);
  mb.addAction(mActionGridProperties);
  mb.addAction(mActionGridIncrease.data());
  mb.addAction(mActionGridDecrease.data());
  mb.addAction(mActionToggleBgImage);
  mb.addSeparator();
  mb.addAction(mActionZoomIn);
  mb.addAction(mActionZoomOut);
  mb.addAction(mActionZoomFit);
  mb.addSeparator();
  mb.addAction(mActionToggle3D);

  // Tools.
  mb.newMenu(&MenuBuilder::createToolsMenu);
  mb.addAction(mActionToolSelect);
  mb.addAction(mActionToolLine);
  mb.addAction(mActionToolRect);
  mb.addAction(mActionToolPolygon);
  mb.addAction(mActionToolCircle);
  mb.addAction(mActionToolArc);
  mb.addAction(mActionToolName);
  mb.addAction(mActionToolValue);
  mb.addAction(mActionToolText);
  mb.addSeparator();
  mb.addAction(mActionToolPin);
  mb.addSeparator();
  mb.addAction(mActionToolThtPad);
  mb.addAction(mActionToolSmtPadStandard);
  mb.addAction(mActionToolSpecialPadThermal);
  mb.addAction(mActionToolSpecialPadBga);
  mb.addAction(mActionToolSpecialPadEdgeConnector);
  mb.addAction(mActionToolSpecialPadTest);
  mb.addAction(mActionToolSpecialPadLocalFiducial);
  mb.addAction(mActionToolSpecialPadGlobalFiducial);
  mb.addAction(mActionToolZone);
  mb.addAction(mActionToolHole);
  mb.addSeparator();
  mb.addAction(mActionGenerateOutline);
  mb.addAction(mActionGenerateCourtyard);
  mb.addAction(mActionReNumberPads);
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

EditorWidgetBase::Context LibraryEditorLegacy::createContext(
    bool isNewElement) noexcept {
  return {
      mWorkspace, *mLayers, isNewElement, mIsOpenedReadOnly, mLibrary,
  };
}

void LibraryEditorLegacy::setAvailableFeatures(
    const QSet<EditorWidgetBase::Feature>& features) noexcept {
  using Feature = EditorWidgetBase::Feature;

  mActionAbort->setEnabled(features.contains(Feature::Abort));
  mActionFind->setEnabled(features.contains(Feature::Filter));
  mActionCopy->setEnabled(features.contains(Feature::Copy));
  mActionCut->setEnabled(features.contains(Feature::Cut));
  mActionExportImage->setEnabled(features.contains(Feature::ExportGraphics));
  mActionExportPdf->setEnabled(features.contains(Feature::ExportGraphics));
  mActionGridProperties->setEnabled(features.contains(Feature::GraphicsView));
  mActionGridIncrease->setEnabled(features.contains(Feature::GraphicsView));
  mActionGridDecrease->setEnabled(features.contains(Feature::GraphicsView));
  mActionPaste->setEnabled(features.contains(Feature::Paste));
  mActionPrint->setEnabled(features.contains(Feature::ExportGraphics));
  mActionRemove->setEnabled(features.contains(Feature::Remove));
  mActionMoveLeft->setEnabled(features.contains(Feature::Move));
  mActionMoveRight->setEnabled(features.contains(Feature::Move));
  mActionMoveUp->setEnabled(features.contains(Feature::Move));
  mActionMoveDown->setEnabled(features.contains(Feature::Move));
  mActionRotateCcw->setEnabled(features.contains(Feature::Rotate));
  mActionRotateCw->setEnabled(features.contains(Feature::Rotate));
  mActionSelectAll->setEnabled(features.contains(Feature::SelectGraphics));
  mActionToggleBgImage->setEnabled(features.contains(Feature::BackgroundImage));
  mActionZoomFit->setEnabled(features.contains(Feature::GraphicsView));
  mActionZoomIn->setEnabled(features.contains(Feature::GraphicsView));
  mActionZoomOut->setEnabled(features.contains(Feature::GraphicsView));
  mActionToggle3D->setEnabled(features.contains(Feature::OpenGlView));
  mActionMirrorHorizontal->setEnabled(features.contains(Feature::Mirror));
  mActionMirrorVertical->setEnabled(features.contains(Feature::Mirror));
  mActionFlipHorizontal->setEnabled(features.contains(Feature::Flip));
  mActionFlipVertical->setEnabled(features.contains(Feature::Flip));
  mActionMoveAlign->setEnabled(features.contains(Feature::MoveAlign));
  mActionHelperTools->setEnabled(features.contains(Feature::GenerateOutline) ||
                                 features.contains(Feature::GenerateCourtyard));
  mActionGenerateOutline->setEnabled(
      features.contains(Feature::GenerateOutline));
  mActionGenerateCourtyard->setEnabled(
      features.contains(Feature::GenerateCourtyard));
  mActionReNumberPads->setEnabled(features.contains(Feature::ReNumberPads));
  mActionImportDxf->setEnabled(features.contains(Feature::ImportGraphics));
  mActionSnapToGrid->setEnabled(features.contains(Feature::SnapToGrid));
  mActionProperties->setEnabled(features.contains(Feature::Properties));
  mActionCloseTab->setEnabled(features.contains(Feature::Close));

  mToolBarSearch->setEnabled(features.contains(Feature::Filter));
}

void LibraryEditorLegacy::setActiveEditorWidget(EditorWidgetBase* widget) {
  if (mCurrentEditorWidget) {
    mCurrentEditorWidget->disconnectEditor();
    disconnect(mCurrentEditorWidget,
               &EditorWidgetBase::availableFeaturesChanged, this,
               &LibraryEditorLegacy::setAvailableFeatures);
  }
  mCurrentEditorWidget = widget;
  if (mCurrentEditorWidget) {
    mCurrentEditorWidget->connectEditor(*mUndoStackActionGroup,
                                        *mToolsActionGroup, *mToolBarCommand,
                                        *mUi->statusBar);
    const bool bgImageSet = mCurrentEditorWidget->isBackgroundImageSet();
    mActionToggleBgImage->setCheckable(bgImageSet);
    mActionToggleBgImage->setChecked(bgImageSet);
    setAvailableFeatures(mCurrentEditorWidget->getAvailableFeatures());
    connect(mCurrentEditorWidget, &EditorWidgetBase::availableFeaturesChanged,
            this, &LibraryEditorLegacy::setAvailableFeatures);
  } else {
    mActionToggleBgImage->setChecked(false);
    setAvailableFeatures({});
  }
  updateTabTitles();  // force updating the "Save" action title
}

void LibraryEditorLegacy::newLibraryElement(
    NewElementWizardContext::ElementType type) {
  NewElementWizard wizard(mWorkspace, *mLibrary, *mLayers, this);
  wizard.setNewElementType(type);
  if (wizard.exec() == QDialog::Accepted) {
    FilePath fp = wizard.getContext().getOutputDirectory();
    editNewLibraryElement(wizard.getContext().mElementType, fp);
    mWorkspace.getLibraryDb().startLibraryRescan();
  }
}

void LibraryEditorLegacy::duplicateLibraryElement(
    NewElementWizardContext::ElementType type, const FilePath& fp) {
  NewElementWizard wizard(mWorkspace, *mLibrary, *mLayers, this);
  wizard.setElementToCopy(type, fp);
  if (wizard.exec() == QDialog::Accepted) {
    FilePath fp = wizard.getContext().getOutputDirectory();
    editNewLibraryElement(wizard.getContext().mElementType, fp);
    mWorkspace.getLibraryDb().startLibraryRescan();
  }
}

void LibraryEditorLegacy::editNewLibraryElement(
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

void LibraryEditorLegacy::updateTabTitles() noexcept {
  for (int i = 0; i < mUi->tabWidget->count(); ++i) {
    QWidget* widget = mUi->tabWidget->widget(i);
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

  if (mCurrentEditorWidget && (!mIsOpenedReadOnly)) {
    mActionSave->setEnabled(true);
    mActionSave->setText(EditorCommandSet::instance().save.getText() % " '" %
                         mCurrentEditorWidget->windowTitle() % "'");
    mActionSave->setToolTip(mActionSave->text());
  } else {
    mActionSave->setEnabled(false);
  }
}

void LibraryEditorLegacy::tabCountChanged() noexcept {
  mActionCloseAllTabs->setEnabled(mUi->tabWidget->count() > 1);
}

void LibraryEditorLegacy::keyPressEvent(QKeyEvent* event) noexcept {
  // If the overview tab is opened and a filter is active, discard the filter
  // with the escape key.
  if ((event->key() == Qt::Key_Escape) && (mToolBarSearch) &&
      (!mToolBarSearch->getText().isEmpty()) && (mCurrentEditorWidget) &&
      (mCurrentEditorWidget->getAvailableFeatures().contains(
          EditorWidgetBase::Feature::Filter))) {
    mToolBarSearch->clear();
    return;
  }
  QMainWindow::keyPressEvent(event);
}

void LibraryEditorLegacy::closeEvent(QCloseEvent* event) noexcept {
  if (closeAndDestroy(true)) {
    QMainWindow::closeEvent(event);
  } else {
    event->ignore();
  }
}

bool LibraryEditorLegacy::closeAllTabs(bool withNonClosable,
                                       bool askForSave) noexcept {
  for (int i = mUi->tabWidget->count() - 1; i >= 0; --i) {
    auto widget = dynamic_cast<EditorWidgetBase*>(mUi->tabWidget->widget(i));
    if (withNonClosable ||
        (widget &&
         widget->getAvailableFeatures().contains(
             EditorWidgetBase::Feature::Close))) {
      if (askForSave) {
        if (!closeTab(i)) {
          return false;
        }
      } else {
        mUi->tabWidget->removeTab(i);
        delete widget;
        tabCountChanged();
      }
    }
  }
  return true;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
