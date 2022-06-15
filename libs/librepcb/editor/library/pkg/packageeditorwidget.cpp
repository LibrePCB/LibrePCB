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

#include "../../cmd/cmdstroketextedit.h"
#include "../../dialogs/gridsettingsdialog.h"
#include "../../library/cmd/cmdlibraryelementedit.h"
#include "../../utils/exclusiveactiongroup.h"
#include "../../utils/toolbarproxy.h"
#include "../../widgets/statusbar.h"
#include "../../workspace/desktopservices.h"
#include "../cmd/cmdfootprintedit.h"
#include "fsm/packageeditorfsm.h"
#include "ui_packageeditorwidget.h"

#include <librepcb/core/graphics/graphicsscene.h>
#include <librepcb/core/library/msg/msgmissingauthor.h>
#include <librepcb/core/library/msg/msgmissingcategories.h>
#include <librepcb/core/library/msg/msgnamenottitlecase.h>
#include <librepcb/core/library/pkg/footprintpainter.h>
#include <librepcb/core/library/pkg/msg/msgmissingfootprint.h>
#include <librepcb/core/library/pkg/msg/msgmissingfootprintname.h>
#include <librepcb/core/library/pkg/msg/msgmissingfootprintvalue.h>
#include <librepcb/core/library/pkg/msg/msgwrongfootprinttextlayer.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/types/gridproperties.h>
#include <librepcb/core/workspace/workspace.h>
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

PackageEditorWidget::PackageEditorWidget(const Context& context,
                                         const FilePath& fp, QWidget* parent)
  : EditorWidgetBase(context, fp, parent),
    mUi(new Ui::PackageEditorWidget),
    mGraphicsScene(new GraphicsScene()) {
  mUi->setupUi(this);
  mUi->lstMessages->setHandler(this);
  mUi->lstMessages->setProvideFixes(!mContext.readOnly);
  mUi->edtName->setReadOnly(mContext.readOnly);
  mUi->edtDescription->setReadOnly(mContext.readOnly);
  mUi->edtKeywords->setReadOnly(mContext.readOnly);
  mUi->edtAuthor->setReadOnly(mContext.readOnly);
  mUi->edtVersion->setReadOnly(mContext.readOnly);
  mUi->cbxDeprecated->setCheckable(!mContext.readOnly);
  mUi->footprintEditorWidget->setReadOnly(mContext.readOnly);
  mUi->padListEditorWidget->setReadOnly(mContext.readOnly);
  setupErrorNotificationWidget(*mUi->errorNotificationWidget);
  mUi->graphicsView->setUseOpenGl(
      mContext.workspace.getSettings().useOpenGl.get());
  mUi->graphicsView->setScene(mGraphicsScene.data());
  mUi->graphicsView->setBackgroundBrush(Qt::black);
  mUi->graphicsView->setForegroundBrush(Qt::white);
  mUi->graphicsView->setEnabled(false);  // no footprint selected
  setWindowIcon(QIcon(":/img/library/package.png"));

  // Apply grid properties unit from workspace settings
  {
    GridProperties p = mUi->graphicsView->getGridProperties();
    p.setUnit(mContext.workspace.getSettings().defaultLengthUnit.get());
    mUi->graphicsView->setGridProperties(p);
  }

  // Insert category list editor widget.
  mCategoriesEditorWidget.reset(new CategoryListEditorWidget(
      mContext.workspace, CategoryListEditorWidget::Categories::Package, this));
  mCategoriesEditorWidget->setReadOnly(mContext.readOnly);
  mCategoriesEditorWidget->setRequiresMinimumOneEntry(true);
  int row;
  QFormLayout::ItemRole role;
  mUi->formLayout->getWidgetPosition(mUi->lblCategories, &row, &role);
  mUi->formLayout->setWidget(row, QFormLayout::FieldRole,
                             mCategoriesEditorWidget.data());

  // Load element.
  mPackage.reset(new Package(std::unique_ptr<TransactionalDirectory>(
      new TransactionalDirectory(mFileSystem))));  // can throw
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
  connect(mCategoriesEditorWidget.data(), &CategoryListEditorWidget::edited,
          this, &PackageEditorWidget::commitMetadata);

  // Load finite state machine (FSM).
  PackageEditorFsm::Context fsmContext{
      mContext,  *this,   *mUndoStack, *mGraphicsScene,      *mUi->graphicsView,
      *mPackage, nullptr, nullptr,     *mCommandToolBarProxy};
  mFsm.reset(new PackageEditorFsm(fsmContext));
  connect(mUndoStack.data(), &UndoStack::stateModified, mFsm.data(),
          &PackageEditorFsm::updateAvailableFeatures);
  connect(mFsm.data(), &PackageEditorFsm::availableFeaturesChanged, this,
          [this]() { emit availableFeaturesChanged(getAvailableFeatures()); });
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
 *  Getters
 ******************************************************************************/

QSet<EditorWidgetBase::Feature> PackageEditorWidget::getAvailableFeatures()
    const noexcept {
  QSet<EditorWidgetBase::Feature> features = {
      EditorWidgetBase::Feature::Close,
      EditorWidgetBase::Feature::GraphicsView,
      EditorWidgetBase::Feature::ExportGraphics,
  };
  return features + mFsm->getAvailableFeatures();
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void PackageEditorWidget::connectEditor(
    UndoStackActionGroup& undoStackActionGroup,
    ExclusiveActionGroup& toolsActionGroup, QToolBar& commandToolBar,
    StatusBar& statusBar) noexcept {
  EditorWidgetBase::connectEditor(undoStackActionGroup, toolsActionGroup,
                                  commandToolBar, statusBar);

  bool enabled = !mContext.readOnly;
  mToolsActionGroup->setActionEnabled(Tool::SELECT, true);
  mToolsActionGroup->setActionEnabled(Tool::ADD_THT_PADS, enabled);
  mToolsActionGroup->setActionEnabled(Tool::ADD_SMT_PADS, enabled);
  mToolsActionGroup->setActionEnabled(Tool::ADD_NAMES, enabled);
  mToolsActionGroup->setActionEnabled(Tool::ADD_VALUES, enabled);
  mToolsActionGroup->setActionEnabled(Tool::DRAW_LINE, enabled);
  mToolsActionGroup->setActionEnabled(Tool::DRAW_RECT, enabled);
  mToolsActionGroup->setActionEnabled(Tool::DRAW_POLYGON, enabled);
  mToolsActionGroup->setActionEnabled(Tool::DRAW_CIRCLE, enabled);
  mToolsActionGroup->setActionEnabled(Tool::DRAW_TEXT, enabled);
  mToolsActionGroup->setActionEnabled(Tool::ADD_HOLES, enabled);
  mToolsActionGroup->setCurrentAction(mFsm->getCurrentTool());
  connect(mFsm.data(), &PackageEditorFsm::toolChanged, mToolsActionGroup,
          &ExclusiveActionGroup::setCurrentAction);

  mStatusBar->setField(StatusBar::AbsolutePosition, true);
  mStatusBar->setLengthUnit(mUi->graphicsView->getGridProperties().getUnit());
  connect(mUi->graphicsView, &GraphicsView::cursorScenePositionChanged,
          mStatusBar, &StatusBar::setAbsoluteCursorPosition);
}

void PackageEditorWidget::disconnectEditor() noexcept {
  EditorWidgetBase::disconnectEditor();

  disconnect(mFsm.data(), &PackageEditorFsm::toolChanged, mToolsActionGroup,
             &ExclusiveActionGroup::setCurrentAction);

  mStatusBar->setField(StatusBar::AbsolutePosition, false);
  disconnect(mUi->graphicsView, &GraphicsView::cursorScenePositionChanged,
             mStatusBar, &StatusBar::setAbsoluteCursorPosition);
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
    mFileSystem->save();  // can throw
    memorizePackageInterface();
    return EditorWidgetBase::save();
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Save failed"), e.getMsg());
    return false;
  }
}

bool PackageEditorWidget::selectAll() noexcept {
  return mFsm->processSelectAll();
}

bool PackageEditorWidget::cut() noexcept {
  return mFsm->processCut();
}

bool PackageEditorWidget::copy() noexcept {
  return mFsm->processCopy();
}

bool PackageEditorWidget::paste() noexcept {
  return mFsm->processPaste();
}

bool PackageEditorWidget::move(Qt::ArrowType direction) noexcept {
  return mFsm->processMove(direction);
}

bool PackageEditorWidget::rotate(const Angle& rotation) noexcept {
  return mFsm->processRotate(rotation);
}

bool PackageEditorWidget::mirror(Qt::Orientation orientation) noexcept {
  return mFsm->processMirror(orientation);
}

bool PackageEditorWidget::flip(Qt::Orientation orientation) noexcept {
  return mFsm->processFlip(orientation);
}

bool PackageEditorWidget::snapToGrid() noexcept {
  return mFsm->processSnapToGrid();
}

bool PackageEditorWidget::remove() noexcept {
  return mFsm->processRemove();
}

bool PackageEditorWidget::editProperties() noexcept {
  return mFsm->processEditProperties();
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

bool PackageEditorWidget::importDxf() noexcept {
  return mFsm->processStartDxfImport();
}

bool PackageEditorWidget::editGridProperties() noexcept {
  GridSettingsDialog dialog(mUi->graphicsView->getGridProperties(), this);
  connect(&dialog, &GridSettingsDialog::gridPropertiesChanged, this,
          &PackageEditorWidget::setGridProperties);
  dialog.exec();
  return true;
}

bool PackageEditorWidget::increaseGridInterval() noexcept {
  GridProperties grid = mUi->graphicsView->getGridProperties();
  grid.setInterval(PositiveLength(grid.getInterval() * 2));
  setGridProperties(grid);
  return true;
}

bool PackageEditorWidget::decreaseGridInterval() noexcept {
  GridProperties grid = mUi->graphicsView->getGridProperties();
  if ((*grid.getInterval()) % 2 == 0) {
    grid.setInterval(PositiveLength(grid.getInterval() / 2));
    setGridProperties(grid);
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
  mOriginalPadUuids = mPackage->getPads().getUuidSet();
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

bool PackageEditorWidget::runChecks(
    LibraryElementCheckMessageList& msgs) const {
  if ((mFsm->getCurrentTool() != NONE) && (mFsm->getCurrentTool() != SELECT)) {
    // Do not run checks if a tool is active because it could lead to annoying,
    // flickering messages. For example when placing pads, they always overlap
    // right after placing them, so we have to wait until the user has moved the
    // cursor to place the pad at a different position.
    return false;
  }
  msgs = mPackage->runChecks();  // can throw
  mUi->lstMessages->setMessages(msgs);
  return true;
}

template <>
void PackageEditorWidget::fixMsg(const MsgNameNotTitleCase& msg) {
  mUi->edtName->setText(*msg.getFixedName());
  commitMetadata();
}

template <>
void PackageEditorWidget::fixMsg(const MsgMissingAuthor& msg) {
  Q_UNUSED(msg);
  mUi->edtAuthor->setText(getWorkspaceSettingsUserName());
  commitMetadata();
}

template <>
void PackageEditorWidget::fixMsg(const MsgMissingCategories& msg) {
  Q_UNUSED(msg);
  mCategoriesEditorWidget->openAddCategoryDialog();
}

template <>
void PackageEditorWidget::fixMsg(const MsgMissingFootprint& msg) {
  Q_UNUSED(msg);
  std::shared_ptr<Footprint> fpt = std::make_shared<Footprint>(
      Uuid::createRandom(), ElementName("default"), "");
  mUndoStack->execCmd(new CmdFootprintInsert(mPackage->getFootprints(), fpt));
}

template <>
void PackageEditorWidget::fixMsg(const MsgMissingFootprintName& msg) {
  Q_UNUSED(msg);
  mFsm->processStartAddingNames();
}

template <>
void PackageEditorWidget::fixMsg(const MsgMissingFootprintValue& msg) {
  Q_UNUSED(msg);
  mFsm->processStartAddingValues();
}

template <>
void PackageEditorWidget::fixMsg(const MsgWrongFootprintTextLayer& msg) {
  std::shared_ptr<Footprint> footprint =
      mPackage->getFootprints().get(msg.getFootprint().get());
  std::shared_ptr<StrokeText> text =
      footprint->getStrokeTexts().get(msg.getText().get());
  QScopedPointer<CmdStrokeTextEdit> cmd(new CmdStrokeTextEdit(*text));
  cmd->setLayerName(GraphicsLayerName(msg.getExpectedLayerName()), false);
  mUndoStack->execCmd(cmd.take());
}

template <typename MessageType>
bool PackageEditorWidget::fixMsgHelper(
    std::shared_ptr<const LibraryElementCheckMessage> msg, bool applyFix) {
  if (msg) {
    if (auto m = msg->as<MessageType>()) {
      if (applyFix) fixMsg(*m);  // can throw
      return true;
    }
  }
  return false;
}

bool PackageEditorWidget::processCheckMessage(
    std::shared_ptr<const LibraryElementCheckMessage> msg, bool applyFix) {
  if (fixMsgHelper<MsgNameNotTitleCase>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgMissingAuthor>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgMissingCategories>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgMissingFootprint>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgMissingFootprintName>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgMissingFootprintValue>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgWrongFootprintTextLayer>(msg, applyFix)) return true;
  return false;
}

bool PackageEditorWidget::execGraphicsExportDialog(
    GraphicsExportDialog::Output output, const QString& settingsKey) noexcept {
  try {
    // Get current footprint.
    std::shared_ptr<const Footprint> footprint = mFsm->getCurrentFootprint();

    // Determine default file path.
    QString packageName =
        FilePath::cleanFileName(*mPackage->getNames().getDefaultValue(),
                                FilePath::ReplaceSpaces | FilePath::KeepCase);
    if ((mPackage->getFootprints().count() > 1) && (footprint)) {
      packageName += "_" % footprint->getNames().getDefaultValue();
    }
    FilePath defaultFilePath(QDir::homePath() % "/" % packageName %
                             "_Footprint");

    // Copy package items to allow processing them in worker threads.
    QList<std::shared_ptr<GraphicsPagePainter>> pages;
    if (footprint) {
      pages.append(std::make_shared<FootprintPainter>(*footprint));
    }

    // Show dialog, which will do all the work.
    GraphicsExportDialog dialog(
        GraphicsExportDialog::Mode::Board, output, pages, 0,
        *mPackage->getNames().getDefaultValue(), 0, defaultFilePath,
        mContext.workspace.getSettings().defaultLengthUnit.get(),
        "package_editor/" % settingsKey, this);
    connect(&dialog, &GraphicsExportDialog::requestOpenFile, this,
            [this](const FilePath& fp) {
              DesktopServices services(mContext.workspace.getSettings(), true);
              services.openFile(fp);
            });
    dialog.exec();
  } catch (const Exception& e) {
    QMessageBox::warning(this, tr("Error"), e.getMsg());
  }
  return true;
}

void PackageEditorWidget::setGridProperties(
    const GridProperties& grid) noexcept {
  mUi->graphicsView->setGridProperties(grid);
  if (mStatusBar) {
    mStatusBar->setLengthUnit(grid.getUnit());
  }
  if (mFsm) {
    mFsm->updateAvailableFeatures();  // Re-calculate "snap to grid" feature!
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
