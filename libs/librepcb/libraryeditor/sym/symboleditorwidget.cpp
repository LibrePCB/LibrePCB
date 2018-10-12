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
#include "symboleditorwidget.h"

#include "fsm/symboleditorfsm.h"
#include "ui_symboleditorwidget.h"

#include <librepcb/common/dialogs/gridsettingsdialog.h>
#include <librepcb/common/graphics/circlegraphicsitem.h>
#include <librepcb/common/graphics/graphicslayer.h>
#include <librepcb/common/graphics/graphicsscene.h>
#include <librepcb/common/gridproperties.h>
#include <librepcb/common/utils/exclusiveactiongroup.h>
#include <librepcb/library/cmp/cmpsigpindisplaytype.h>
#include <librepcb/library/sym/symbol.h>
#include <librepcb/library/sym/symbolgraphicsitem.h>
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

SymbolEditorWidget::SymbolEditorWidget(const Context&  context,
                                       const FilePath& fp, QWidget* parent)
  : EditorWidgetBase(context, fp, parent),
    mUi(new Ui::SymbolEditorWidget),
    mGraphicsScene(new GraphicsScene()) {
  mUi->setupUi(this);
  mUi->graphicsView->setUseOpenGl(
      mContext.workspace.getSettings().getAppearance().getUseOpenGl());
  mUi->graphicsView->setScene(mGraphicsScene.data());
  connect(mUi->graphicsView, &GraphicsView::cursorScenePositionChanged, this,
          &SymbolEditorWidget::cursorPositionChanged);
  setWindowIcon(QIcon(":/img/library/symbol.png"));
  connect(mUi->edtName, &QLineEdit::textChanged, this,
          &QWidget::setWindowTitle);

  // insert category list editor widget
  mCategoriesEditorWidget.reset(
      new ComponentCategoryListEditorWidget(mContext.workspace, this));
  mCategoriesEditorWidget->setRequiresMinimumOneEntry(true);
  int                   row;
  QFormLayout::ItemRole role;
  mUi->formLayout->getWidgetPosition(mUi->lblCategories, &row, &role);
  mUi->formLayout->setWidget(row, QFormLayout::FieldRole,
                             mCategoriesEditorWidget.data());

  // load symbol
  mSymbol.reset(new Symbol(fp, false));  // can throw
  setWindowTitle(*mSymbol->getNames().value(getLibLocaleOrder()));
  mUi->edtName->setText(*mSymbol->getNames().value(getLibLocaleOrder()));
  mUi->edtDescription->setPlainText(
      mSymbol->getDescriptions().value(getLibLocaleOrder()));
  mUi->edtKeywords->setText(mSymbol->getKeywords().value(getLibLocaleOrder()));
  mUi->edtAuthor->setText(mSymbol->getAuthor());
  mUi->edtVersion->setText(mSymbol->getVersion().toStr());
  mCategoriesEditorWidget->setUuids(mSymbol->getCategories());
  mUi->cbxDeprecated->setChecked(mSymbol->isDeprecated());

  // show "interface broken" warning when related properties are modified
  mOriginalSymbolPinUuids = mSymbol->getPins().getUuidSet();
  setupInterfaceBrokenWarningWidget(*mUi->interfaceBrokenWarningWidget);

  // set dirty state when properties are modified
  connect(mUi->edtName, &QLineEdit::textEdited, this,
          &SymbolEditorWidget::setDirty);
  connect(mUi->edtDescription, &QPlainTextEdit::textChanged, this,
          &SymbolEditorWidget::setDirty);
  connect(mUi->edtKeywords, &QLineEdit::textEdited, this,
          &SymbolEditorWidget::setDirty);
  connect(mUi->edtAuthor, &QLineEdit::textEdited, this,
          &SymbolEditorWidget::setDirty);
  connect(mUi->edtVersion, &QLineEdit::textEdited, this,
          &SymbolEditorWidget::setDirty);
  connect(mUi->cbxDeprecated, &QCheckBox::clicked, this,
          &SymbolEditorWidget::setDirty);
  connect(mCategoriesEditorWidget.data(),
          &ComponentCategoryListEditorWidget::categoryAdded, this,
          &SymbolEditorWidget::setDirty);
  connect(mCategoriesEditorWidget.data(),
          &ComponentCategoryListEditorWidget::categoryRemoved, this,
          &SymbolEditorWidget::setDirty);

  // load graphics items recursively
  mGraphicsItem.reset(new SymbolGraphicsItem(*mSymbol, mContext.layerProvider));
  mGraphicsScene->addItem(*mGraphicsItem);
  mUi->graphicsView->zoomAll();

  // load finite state machine (FSM)
  SymbolEditorFsm::Context fsmContext{
      *this,           *mUndoStack,          mContext.layerProvider,
      *mGraphicsScene, *mUi->graphicsView,   *mSymbol,
      *mGraphicsItem,  *mCommandToolBarProxy};
  mFsm.reset(new SymbolEditorFsm(fsmContext));

  // last but not least, connect the graphics scene events with the FSM
  mUi->graphicsView->setEventHandlerObject(this);
}

SymbolEditorWidget::~SymbolEditorWidget() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void SymbolEditorWidget::setToolsActionGroup(
    ExclusiveActionGroup* group) noexcept {
  if (mToolsActionGroup) {
    disconnect(mFsm.data(), &SymbolEditorFsm::toolChanged, mToolsActionGroup,
               &ExclusiveActionGroup::setCurrentAction);
  }

  EditorWidgetBase::setToolsActionGroup(group);

  if (mToolsActionGroup) {
    mToolsActionGroup->setActionEnabled(Tool::SELECT, true);
    mToolsActionGroup->setActionEnabled(Tool::ADD_PINS, true);
    mToolsActionGroup->setActionEnabled(Tool::ADD_NAMES, true);
    mToolsActionGroup->setActionEnabled(Tool::ADD_VALUES, true);
    mToolsActionGroup->setActionEnabled(Tool::DRAW_LINE, true);
    mToolsActionGroup->setActionEnabled(Tool::DRAW_RECT, true);
    mToolsActionGroup->setActionEnabled(Tool::DRAW_POLYGON, true);
    mToolsActionGroup->setActionEnabled(Tool::DRAW_CIRCLE, true);
    mToolsActionGroup->setActionEnabled(Tool::DRAW_TEXT, true);
    mToolsActionGroup->setCurrentAction(mFsm->getCurrentTool());
    connect(mFsm.data(), &SymbolEditorFsm::toolChanged, mToolsActionGroup,
            &ExclusiveActionGroup::setCurrentAction);
  }
}

/*******************************************************************************
 *  Public Slots
 ******************************************************************************/

bool SymbolEditorWidget::save() noexcept {
  try {
    ElementName name(mUi->edtName->text().trimmed());  // can throw
    Version     version =
        Version::fromString(mUi->edtVersion->text().trimmed());  // can throw

    mSymbol->setName("", name);
    mSymbol->setDescription("", mUi->edtDescription->toPlainText().trimmed());
    mSymbol->setKeywords("", mUi->edtKeywords->text().trimmed());
    mSymbol->setAuthor(mUi->edtAuthor->text().trimmed());
    mSymbol->setVersion(version);
    mSymbol->setCategories(mCategoriesEditorWidget->getUuids());
    mSymbol->setDeprecated(mUi->cbxDeprecated->isChecked());
    mSymbol->save();
    mOriginalSymbolPinUuids = mSymbol->getPins().getUuidSet();
    return EditorWidgetBase::save();
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Save failed"), e.getMsg());
    return false;
  }
}

bool SymbolEditorWidget::rotateCw() noexcept {
  return mFsm->processRotateCw();
}

bool SymbolEditorWidget::rotateCcw() noexcept {
  return mFsm->processRotateCcw();
}

bool SymbolEditorWidget::remove() noexcept {
  return mFsm->processRemove();
}

bool SymbolEditorWidget::zoomIn() noexcept {
  mUi->graphicsView->zoomIn();
  return true;
}

bool SymbolEditorWidget::zoomOut() noexcept {
  mUi->graphicsView->zoomOut();
  return true;
}

bool SymbolEditorWidget::zoomAll() noexcept {
  mUi->graphicsView->zoomAll();
  return true;
}

bool SymbolEditorWidget::abortCommand() noexcept {
  return mFsm->processAbortCommand();
}

bool SymbolEditorWidget::editGridProperties() noexcept {
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

bool SymbolEditorWidget::graphicsViewEventHandler(QEvent* event) noexcept {
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

bool SymbolEditorWidget::toolChangeRequested(Tool newTool) noexcept {
  switch (newTool) {
    case Tool::SELECT:
      return mFsm->processStartSelecting();
    case Tool::ADD_PINS:
      return mFsm->processStartAddingSymbolPins();
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
    default:
      return false;
  }
}

bool SymbolEditorWidget::isInterfaceBroken() const noexcept {
  return mSymbol->getPins().getUuidSet() != mOriginalSymbolPinUuids;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb
