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
#include "packageeditorwidget.h"

#include "fsm/packageeditorfsm.h"
#include "ui_packageeditorwidget.h"

#include <librepcb/common/dialogs/gridsettingsdialog.h>
#include <librepcb/common/graphics/graphicsscene.h>
#include <librepcb/common/gridproperties.h>
#include <librepcb/common/utils/exclusiveactiongroup.h>
#include <librepcb/library/cmd/cmdlibraryelementedit.h>
#include <librepcb/library/pkg/package.h>
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

PackageEditorWidget::PackageEditorWidget(const Context&  context,
                                         const FilePath& fp, QWidget* parent)
  : EditorWidgetBase(context, fp, parent),
    mUi(new Ui::PackageEditorWidget),
    mGraphicsScene(new GraphicsScene()) {
  mUi->setupUi(this);
  mUi->graphicsView->setUseOpenGl(
      mContext.workspace.getSettings().getAppearance().getUseOpenGl());
  mUi->graphicsView->setScene(mGraphicsScene.data());
  mUi->graphicsView->setBackgroundBrush(Qt::black);
  mUi->graphicsView->setForegroundBrush(Qt::white);
  mUi->graphicsView->setEnabled(false);  // no footprint selected
  connect(mUi->graphicsView, &GraphicsView::cursorScenePositionChanged, this,
          &PackageEditorWidget::cursorPositionChanged);
  setWindowIcon(QIcon(":/img/library/package.png"));

  // Insert category list editor widget.
  mCategoriesEditorWidget.reset(
      new PackageCategoryListEditorWidget(mContext.workspace, this));
  mCategoriesEditorWidget->setRequiresMinimumOneEntry(true);
  int                   row;
  QFormLayout::ItemRole role;
  mUi->formLayout->getWidgetPosition(mUi->lblCategories, &row, &role);
  mUi->formLayout->setWidget(row, QFormLayout::FieldRole,
                             mCategoriesEditorWidget.data());

  // Load element.
  mPackage.reset(new Package(fp, false));  // can throw
  updateMetadata();

  // Setup footprint list editor widget.
  mUi->footprintEditorWidget->setReferences(mPackage->getFootprints(),
                                            *mUndoStack);
  connect(mUi->footprintEditorWidget,
          &FootprintListEditorWidget::currentFootprintChanged, this,
          &PackageEditorWidget::currentFootprintChanged);

  // Setup pad list editor widget.
  mUi->padListEditorWidget->setReferences(mPackage->getPads(),
                                          mUndoStack.data());

  // Show "interface broken" warning when related properties are modified.
  memorizePackageInterface();
  setupInterfaceBrokenWarningWidget(*mUi->interfaceBrokenWarningWidget);

  // Reload metadata on undo stack state changes.
  connect(mUndoStack.data(), &UndoStack::stateModified, this,
          &PackageEditorWidget::updateMetadata);

  // Handle changes of metadata.
  connect(mUi->edtName, &QLineEdit::editingFinished, this,
          &PackageEditorWidget::commitMetadata);
  connect(mUi->edtDescription, &PlainTextEdit::editingFinished, this,
          &PackageEditorWidget::commitMetadata);
  connect(mUi->edtKeywords, &QLineEdit::editingFinished, this,
          &PackageEditorWidget::commitMetadata);
  connect(mUi->edtAuthor, &QLineEdit::editingFinished, this,
          &PackageEditorWidget::commitMetadata);
  connect(mUi->edtVersion, &QLineEdit::editingFinished, this,
          &PackageEditorWidget::commitMetadata);
  connect(mUi->cbxDeprecated, &QCheckBox::clicked, this,
          &PackageEditorWidget::commitMetadata);
  connect(mCategoriesEditorWidget.data(),
          &ComponentCategoryListEditorWidget::edited, this,
          &PackageEditorWidget::commitMetadata);

  // Load finite state machine (FSM).
  PackageEditorFsm::Context fsmContext{*this,
                                       *mUndoStack,
                                       *mGraphicsScene,
                                       *mUi->graphicsView,
                                       mContext.layerProvider,
                                       *mPackage,
                                       nullptr,
                                       nullptr,
                                       *mCommandToolBarProxy};
  mFsm.reset(new PackageEditorFsm(fsmContext));
  currentFootprintChanged(0);  // small hack to select the first footprint...

  // Last but not least, connect the graphics scene events with the FSM.
  mUi->graphicsView->setEventHandlerObject(this);
}

PackageEditorWidget::~PackageEditorWidget() noexcept {
  mFsm.reset();
  mPackage.take()
      ->deleteLater();  // avoid dangling pointer! todo: make this less ugly ;)
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void PackageEditorWidget::setToolsActionGroup(
    ExclusiveActionGroup* group) noexcept {
  if (mToolsActionGroup) {
    disconnect(mFsm.data(), &PackageEditorFsm::toolChanged, mToolsActionGroup,
               &ExclusiveActionGroup::setCurrentAction);
  }

  EditorWidgetBase::setToolsActionGroup(group);

  if (mToolsActionGroup) {
    mToolsActionGroup->setActionEnabled(Tool::SELECT, true);
    mToolsActionGroup->setActionEnabled(Tool::ADD_THT_PADS, true);
    mToolsActionGroup->setActionEnabled(Tool::ADD_SMT_PADS, true);
    mToolsActionGroup->setActionEnabled(Tool::ADD_NAMES, true);
    mToolsActionGroup->setActionEnabled(Tool::ADD_VALUES, true);
    mToolsActionGroup->setActionEnabled(Tool::DRAW_LINE, true);
    mToolsActionGroup->setActionEnabled(Tool::DRAW_RECT, true);
    mToolsActionGroup->setActionEnabled(Tool::DRAW_POLYGON, true);
    mToolsActionGroup->setActionEnabled(Tool::DRAW_CIRCLE, true);
    mToolsActionGroup->setActionEnabled(Tool::DRAW_TEXT, true);
    mToolsActionGroup->setActionEnabled(Tool::ADD_HOLES, true);
    mToolsActionGroup->setCurrentAction(mFsm->getCurrentTool());
    connect(mFsm.data(), &PackageEditorFsm::toolChanged, mToolsActionGroup,
            &ExclusiveActionGroup::setCurrentAction);
  }
}

/*******************************************************************************
 *  Public Slots
 ******************************************************************************/

bool PackageEditorWidget::save() noexcept {
  // Commit metadata.
  QString errorMsg = commitMetadata();
  if (!errorMsg.isEmpty()) {
    QMessageBox::critical(this, tr("Invalid metadata"), errorMsg);
    return false;
  }

  // Save element.
  try {
    mPackage->save();  // can throw
    memorizePackageInterface();
    return EditorWidgetBase::save();
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Save failed"), e.getMsg());
    return false;
  }
}

bool PackageEditorWidget::rotateCw() noexcept {
  return mFsm->processRotateCw();
}

bool PackageEditorWidget::rotateCcw() noexcept {
  return mFsm->processRotateCcw();
}

bool PackageEditorWidget::remove() noexcept {
  return mFsm->processRemove();
}

bool PackageEditorWidget::zoomIn() noexcept {
  mUi->graphicsView->zoomIn();
  return true;
}

bool PackageEditorWidget::zoomOut() noexcept {
  mUi->graphicsView->zoomOut();
  return true;
}

bool PackageEditorWidget::zoomAll() noexcept {
  mUi->graphicsView->zoomAll();
  return true;
}

bool PackageEditorWidget::abortCommand() noexcept {
  return mFsm->processAbortCommand();
}

bool PackageEditorWidget::editGridProperties() noexcept {
  GridSettingsDialog dialog(mUi->graphicsView->getGridProperties(), this);
  connect(&dialog, &GridSettingsDialog::gridPropertiesChanged,
          mUi->graphicsView, &GraphicsView::setGridProperties);
  if (dialog.exec()) {
    mUi->graphicsView->setGridProperties(dialog.getGrid());
  }
  return true;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void PackageEditorWidget::updateMetadata() noexcept {
  setWindowTitle(*mPackage->getNames().getDefaultValue());
  mUi->edtName->setText(*mPackage->getNames().getDefaultValue());
  mUi->edtDescription->setPlainText(
      mPackage->getDescriptions().getDefaultValue());
  mUi->edtKeywords->setText(mPackage->getKeywords().getDefaultValue());
  mUi->edtAuthor->setText(mPackage->getAuthor());
  mUi->edtVersion->setText(mPackage->getVersion().toStr());
  mUi->cbxDeprecated->setChecked(mPackage->isDeprecated());
  mCategoriesEditorWidget->setUuids(mPackage->getCategories());
}

QString PackageEditorWidget::commitMetadata() noexcept {
  try {
    QScopedPointer<CmdLibraryElementEdit> cmd(
        new CmdLibraryElementEdit(*mPackage, tr("Edit package metadata")));
    try {
      // throws on invalid name
      cmd->setName("", ElementName(mUi->edtName->text().trimmed()));
    } catch (const Exception& e) {
    }
    cmd->setDescription("", mUi->edtDescription->toPlainText().trimmed());
    cmd->setKeywords("", mUi->edtKeywords->text().trimmed());
    try {
      // throws on invalid version
      cmd->setVersion(Version::fromString(mUi->edtVersion->text().trimmed()));
    } catch (const Exception& e) {
    }
    cmd->setAuthor(mUi->edtAuthor->text().trimmed());
    cmd->setDeprecated(mUi->cbxDeprecated->isChecked());
    cmd->setCategories(mCategoriesEditorWidget->getUuids());

    // Commit all changes.
    mUndoStack->execCmd(cmd.take());  // can throw

    // Reload metadata into widgets to discard invalid input.
    updateMetadata();
  } catch (const Exception& e) {
    return e.getMsg();
  }
  return QString();
}

bool PackageEditorWidget::graphicsViewEventHandler(QEvent* event) noexcept {
  Q_ASSERT(event);
  switch (event->type()) {
    case QEvent::GraphicsSceneMouseMove: {
      auto* e = dynamic_cast<QGraphicsSceneMouseEvent*>(event);
      Q_ASSERT(e);
      return mFsm->processGraphicsSceneMouseMoved(*e);
    }
    case QEvent::GraphicsSceneMousePress: {
      auto* e = dynamic_cast<QGraphicsSceneMouseEvent*>(event);
      Q_ASSERT(e);
      switch (e->button()) {
        case Qt::LeftButton:
          return mFsm->processGraphicsSceneLeftMouseButtonPressed(*e);
        default:
          return false;
      }
    }
    case QEvent::GraphicsSceneMouseRelease: {
      auto* e = dynamic_cast<QGraphicsSceneMouseEvent*>(event);
      Q_ASSERT(e);
      switch (e->button()) {
        case Qt::LeftButton:
          return mFsm->processGraphicsSceneLeftMouseButtonReleased(*e);
        case Qt::RightButton:
          return mFsm->processGraphicsSceneRightMouseButtonReleased(*e);
        default:
          return false;
      }
    }
    case QEvent::GraphicsSceneMouseDoubleClick: {
      auto* e = dynamic_cast<QGraphicsSceneMouseEvent*>(event);
      Q_ASSERT(e);
      switch (e->button()) {
        case Qt::LeftButton:
          return mFsm->processGraphicsSceneLeftMouseButtonDoubleClicked(*e);
        default:
          return false;
      }
    }
    default: { return false; }
  }
}

bool PackageEditorWidget::toolChangeRequested(Tool newTool) noexcept {
  switch (newTool) {
    case Tool::SELECT:
      return mFsm->processStartSelecting();
    case Tool::ADD_THT_PADS:
      return mFsm->processStartAddingFootprintThtPads();
    case Tool::ADD_SMT_PADS:
      return mFsm->processStartAddingFootprintSmtPads();
    case Tool::ADD_NAMES:
      return mFsm->processStartAddingNames();
    case Tool::ADD_VALUES:
      return mFsm->processStartAddingValues();
    case Tool::DRAW_LINE:
      return mFsm->processStartDrawLines();
    case Tool::DRAW_RECT:
      return mFsm->processStartDrawRects();
    case Tool::DRAW_POLYGON:
      return mFsm->processStartDrawPolygons();
    case Tool::DRAW_CIRCLE:
      return mFsm->processStartDrawCircles();
    case Tool::DRAW_TEXT:
      return mFsm->processStartDrawTexts();
    case Tool::ADD_HOLES:
      return mFsm->processStartAddingHoles();
    default:
      return false;
  }
}

void PackageEditorWidget::currentFootprintChanged(int index) noexcept {
  mFsm->processChangeCurrentFootprint(mPackage->getFootprints().value(index));
}

void PackageEditorWidget::memorizePackageInterface() noexcept {
  mOriginalPadUuids   = mPackage->getPads().getUuidSet();
  mOriginalFootprints = mPackage->getFootprints();
}

bool PackageEditorWidget::isInterfaceBroken() const noexcept {
  if (mPackage->getPads().getUuidSet() != mOriginalPadUuids) return true;
  for (const Footprint& original : mOriginalFootprints) {
    const Footprint* current =
        mPackage->getFootprints().find(original.getUuid()).get();
    if (!current) return true;
    if (current->getPads().getUuidSet() != original.getPads().getUuidSet())
      return true;
  }
  return false;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb
