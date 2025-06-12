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
#include "symboltab.h"

#include "../../graphics/graphicsscene.h"
#include "../../graphics/slintgraphicsview.h"
#include "../../rulecheck/rulecheckmessagesmodel.h"
#include "../../undostack.h"
#include "../../utils/editortoolbox.h"
#include "../../utils/slinthelpers.h"
#include "../../utils/uihelpers.h"
#include "../../workspace/categorytreemodel2.h"
#include "../cmd/cmdlibraryelementedit.h"
#include "../libraryeditor2.h"
#include "../libraryelementcategoriesmodel.h"
#include "graphics/graphicslayerlist.h"
#include "symbolgraphicsitem.h"
#include "fsm/symboleditorfsm.h"
#include <librepcb/core/fileio/transactionaldirectory.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/library.h>
#include <librepcb/core/library/librarybaseelementcheckmessages.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>
#include <librepcb/core/workspace/workspacesettings.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SymbolTab::SymbolTab(LibraryEditor2& editor, std::unique_ptr<Symbol> sym,
                     bool wizardMode, QObject* parent) noexcept
  : LibraryEditorTab(editor, parent),
    onDerivedUiDataChanged(*this),
    mSymbol(std::move(sym)),
    mLayers(GraphicsLayerList::libraryLayers(
        &mEditor.getWorkspace().getSettings())),
    mView(new SlintGraphicsView(this)),
    mWizardMode(wizardMode),
    mCurrentPageIndex(wizardMode ? 0 : 2),
    mGridStyle(Theme::GridStyle::None),
    mGridInterval(2540000),
    mUnit(LengthUnit::millimeters()),
    mFrameIndex(0),
    mNameParsed(mSymbol->getNames().getDefaultValue()),
    mVersionParsed(mSymbol->getVersion()),
    mCategories(new LibraryElementCategoriesModel(
        editor.getWorkspace(),
        LibraryElementCategoriesModel::Type::ComponentCategory)),
    mCategoriesTree(
        new CategoryTreeModel2(editor.getWorkspace().getLibraryDb(),
                               editor.getWorkspace().getSettings(),
                               CategoryTreeModel2::Filter::CmpCat)) {
  // Setup graphics view.
  mView->setEventHandler(this);
  connect(mView.get(), &SlintGraphicsView::transformChanged, this,
          &SymbolTab::requestRepaint);
  connect(mView.get(), &SlintGraphicsView::stateChanged, this,
          &SymbolTab::notifyDerivedUiDataChanged);

  // Connect undo stack.
  connect(mUndoStack.get(), &UndoStack::stateModified, this,
          &SymbolTab::scheduleChecks);
  connect(mUndoStack.get(), &UndoStack::stateModified, this,
          &SymbolTab::refreshMetadata);

  // Connect models.
  connect(mCategories.get(), &LibraryElementCategoriesModel::modified, this,
          &SymbolTab::commitMetadata, Qt::QueuedConnection);

  // Load finite state machine (FSM).
  //SymbolEditorFsm::Context fsmContext{mContext,
  //                                    *this,
  //                                    *mUndoStack,
  //                                    *mGraphicsScene,
  //                                    *mUi->graphicsView,
  //                                    mLengthUnit,
  //                                    *mSymbol,
  //                                    *mGraphicsItem,
  //                                    *mCommandToolBarProxy};
  //mFsm.reset(new SymbolEditorFsm(fsmContext));
  //connect(mUndoStack.data(), &UndoStack::stateModified, mFsm.data(),
  //        &SymbolEditorFsm::updateAvailableFeatures);
  //connect(mFsm.data(), &SymbolEditorFsm::availableFeaturesChanged, this,
  //        [this]() { emit availableFeaturesChanged(getAvailableFeatures()); });
  //connect(mFsm.data(), &SymbolEditorFsm::statusBarMessageChanged, this,
  //        &SymbolEditorWidget::setStatusBarMessage);

  // Refresh content.
  refreshMetadata();
  scheduleChecks();

  // Clear name for new elements so the user can just start typing.
  if (isNewElement()) {
    mName = slint::SharedString();
    validateElementName(s2q(mName), mNameError);
  }
}

SymbolTab::~SymbolTab() noexcept {
  deactivate();

  // Clean up the state machine nicely to avoid unexpected behavior. Triggering
  // abort (Esc) two times is usually sufficient to leave any active tool, so
  // let's call it three times to be on the safe side. Unfortunately there's
  // no clean way to forcible and guaranteed leaving a tool.
  mFsm->processAbortCommand();
  mFsm->processAbortCommand();
  mFsm->processAbortCommand();
  mFsm.reset();

  // Delete all command objects in the undo stack. This mmust be done before
  // other important objects are deleted, as undo command objects can hold
  // pointers/references to them!
  mUndoStack->clear();
  mUndoStack.reset();

  mView->setEventHandler(nullptr);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

FilePath SymbolTab::getDirectoryPath() const noexcept {
  return mSymbol->getDirectory().getAbsPath();
}

ui::TabData SymbolTab::getUiData() const noexcept {
  const bool writable = isNewElement() || mSymbol->getDirectory().isWritable();

  ui::TabFeatures features = {};
  features.save = toFs(writable);
  features.undo = toFs(mUndoStack->canUndo());
  features.redo = toFs(mUndoStack->canRedo());

  return ui::TabData{
      ui::TabType::Symbol,  // Type
      q2s(*mSymbol->getNames().getDefaultValue()),  // Title
      features,  // Features
      !writable,  // Read-only
      hasUnsavedChanges(),  // Unsaved changes
      q2s(mUndoStack->getUndoCmdText()),  // Undo text
      q2s(mUndoStack->getRedoCmdText()),  // Redo text
      slint::SharedString(),  // Find term
      nullptr,  // Find suggestions
      nullptr,  // Layers
  };
}

ui::SymbolTabData SymbolTab::getDerivedUiData() const noexcept {
  const Theme& theme = mEditor.getWorkspace().getSettings().themes.getActive();
  const QColor bgColor =
      theme.getColor(Theme::Color::sSchematicBackground).getPrimaryColor();
  const QColor fgColor = (bgColor.lightnessF() >= 0.5) ? Qt::black : Qt::white;

  return ui::SymbolTabData{
      mEditor.getUiIndex(),  // Library index
      mWizardMode,  // Wizard mode
      mCurrentPageIndex,  // Page index
      q2s(mSymbol->getDirectory().getAbsPath().toStr()),  // Path
      mName,  // Name
      mNameError,  // Name error
      mDescription,  // Description
      mKeywords,  // Keywords
      mAuthor,  // Author
      mVersion,  // Version
      mVersionError,  // Version error
      mDeprecated,  // Deprecated
      mCategories,  // Categories
      mCategoriesTree,  // Categories tree
      ui::RuleCheckData{
          ui::RuleCheckType::SymbolCheck,  // Checks type
          ui::RuleCheckState::UpToDate,  // Checks state
          mCheckMessages,  // Checks messages
          mCheckMessages->getUnapprovedCount(),  // Checks unapproved count
          mCheckError,  // Checks execution error
      },
      q2s(bgColor),  // Background color
      q2s(fgColor),  // Foreground color
      q2s(theme.getColor(Theme::Color::sSchematicInfoBox)
              .getPrimaryColor()),  // Overlay color
      q2s(theme.getColor(Theme::Color::sSchematicInfoBox)
              .getSecondaryColor()),  // Overlay text color
      l2s(mGridStyle),  // Grid style
      l2s(*mGridInterval),  // Grid interval
      l2s(mUnit),  // Unit
      ui::EditorTool::Select,  // Tool
      q2s(Qt::ArrowCursor),  // Tool cursor
      slint::SharedString(),  // Tool overlay text
      ui::ComboBoxData{},  // Tool layer
      ui::LengthEditData{},  // Tool line width
      ui::LengthEditData{},  // Tool size
      false,  // Tool filled
      ui::LineEditData{},  // Tool value
      q2s(mSceneImagePos),  // Scene image position
      0,  // Frame index
      slint::SharedString(),  // New category
  };
}

void SymbolTab::setDerivedUiData(const ui::SymbolTabData& data) noexcept {
  mName = data.name;
  if (auto value = validateElementName(s2q(mName), mNameError)) {
    mNameParsed = *value;
  }
  mDescription = data.description;
  mKeywords = data.keywords;
  mAuthor = data.author;
  mVersion = data.version;
  if (auto value = validateVersion(s2q(mVersion), mVersionError)) {
    mVersionParsed = *value;
  }
  mDeprecated = data.deprecated;

  // Page index
  mCurrentPageIndex = data.page_index;

  mSceneImagePos = s2q(data.scene_image_pos);

  if (auto uuid = Uuid::tryFromString(s2q(data.new_category))) {
    mCategories->add(*uuid);
  }

  // Update UI on changes
  onDerivedUiDataChanged.notify();
}

void SymbolTab::activate() noexcept {
  mScene.reset(new GraphicsScene(this));
  // mScene->setGridInterval(mSchematic.getGridInterval());
  connect(mScene.get(), &GraphicsScene::changed, this,
          &SymbolTab::requestRepaint);

  mGraphicsItem.reset(new SymbolGraphicsItem(*mSymbol, *mLayers));
  mScene->addItem(*mGraphicsItem);

  applyTheme();
  requestRepaint();
}

void SymbolTab::deactivate() noexcept {
  mGraphicsItem.reset();
  mScene.reset();
}

void SymbolTab::trigger(ui::TabAction a) noexcept {
  switch (a) {
    case ui::TabAction::Back: {
      if (mWizardMode && (mCurrentPageIndex > 0)) {
        --mCurrentPageIndex;
      }
      onDerivedUiDataChanged.notify();
      break;
    }
    case ui::TabAction::Next: {
      if (mWizardMode && (mCurrentPageIndex == 1)) {
        mWizardMode = false;
        ++mCurrentPageIndex;
      }
      onDerivedUiDataChanged.notify();
      break;
    }
    case ui::TabAction::Apply: {
      commitMetadata();
      refreshMetadata();
      break;
    }
    case ui::TabAction::Save: {
      commitMetadata();
      save();
      break;
    }
    case ui::TabAction::Undo: {
      try {
        commitMetadata();
        mUndoStack->undo();
      } catch (const Exception& e) {
        QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
      }
      break;
    }
    case ui::TabAction::Redo: {
      try {
        commitMetadata();
        mUndoStack->redo();
      } catch (const Exception& e) {
        QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
      }
      break;
    }
    case ui::TabAction::Close: {
      if (requestClose()) {
        WindowTab::trigger(a);
      }
      break;
    }
    case ui::TabAction::Print: {
      // execGraphicsExportDialog(GraphicsExportDialog::Output::Print, "print");
      break;
    }
    case ui::TabAction::ExportImage: {
      // execGraphicsExportDialog(GraphicsExportDialog::Output::Image,
      //                          "image_export");
      break;
    }
    case ui::TabAction::ExportPdf: {
      // execGraphicsExportDialog(GraphicsExportDialog::Output::Pdf,
      // "pdf_export");
      break;
    }
    case ui::TabAction::SelectAll: {
       mFsm->processSelectAll();
      break;
    }
    case ui::TabAction::Abort: {
       mFsm->processAbortCommand();
      break;
    }
    case ui::TabAction::Cut: {
      mFsm->processCut();
      break;
    }
    case ui::TabAction::Copy: {
       mFsm->processCopy();
      break;
    }
    case ui::TabAction::Paste: {
       mFsm->processPaste();
      break;
    }
    case ui::TabAction::Delete: {
       mFsm->processRemove();
      break;
    }
    case ui::TabAction::RotateCcw: {
       mFsm->processRotate(Angle::deg90());
      break;
    }
    case ui::TabAction::RotateCw: {
       mFsm->processRotate(-Angle::deg90());
      break;
    }
    case ui::TabAction::MirrorHorizontally: {
       mFsm->processMirror(Qt::Horizontal);
      break;
    }
    case ui::TabAction::MirrorVertically: {
       mFsm->processMirror(Qt::Vertical);
      break;
    }
    case ui::TabAction::MoveLeft: {
      if (!mFsm->processMove(Point(-mGridInterval, 0))) {
        mView->scrollLeft();
      }
      break;
    }
    case ui::TabAction::MoveRight: {
      if (!mFsm->processMove(Point(*mGridInterval, 0))) {
        mView->scrollRight();
      }
      break;
    }
    case ui::TabAction::MoveUp: {
      if (!mFsm->processMove(Point(0, *mGridInterval))) {
        mView->scrollUp();
      }
      break;
    }
    case ui::TabAction::MoveDown: {
      if (!mFsm->processMove(Point(0, -mGridInterval))) {
        mView->scrollDown();
      }
      break;
    }
    case ui::TabAction::SnapToGrid: {
       mFsm->processSnapToGrid();
      break;
    }
    case ui::TabAction::EditProperties: {
       mFsm->processEditProperties();
      break;
    }
    /*case ui::TabAction::GridIntervalIncrease: {
      mSchematic.setGridInterval(
          PositiveLength(mSchematic.getGridInterval() * 2));
      if (mScene) {
        mScene->setGridInterval(mSchematic.getGridInterval());
        requestRepaint();
      }
      break;
    }
    case ui::TabAction::GridIntervalDecrease: {
      if ((*mSchematic.getGridInterval() % 2) == 0) {
        mSchematic.setGridInterval(
            PositiveLength(mSchematic.getGridInterval() / 2));
        if (mScene) {
          mScene->setGridInterval(mSchematic.getGridInterval());
          requestRepaint();
        }
      }
      break;
    }*/
    case ui::TabAction::ZoomIn: {
      mView->zoomIn();
      break;
    }
    case ui::TabAction::ZoomOut: {
      mView->zoomOut();
      break;
    }
    case ui::TabAction::ZoomFit: {
      if (mScene) mView->zoomToSceneRect(mScene->itemsBoundingRect());
      break;
    }
    case ui::TabAction::ToolSelect: {
      // mFsm->processSelect();
      break;
    }
    case ui::TabAction::ToolPolygon: {
      // mFsm->processDrawPolygon();
      break;
    }
    case ui::TabAction::ToolText: {
      // mFsm->processAddText();
      break;
    }
    case ui::TabAction::ToolMeasure: {
      // mFsm->processMeasure();
      break;
    }
    default: {
      WindowTab::trigger(a);
      break;
    }
  }
}

slint::Image SymbolTab::renderScene(float width, float height,
                                    int scene) noexcept {
  Q_UNUSED(scene);
  if (mScene) {
    return mView->render(*mScene, width, height);
  }
  return slint::Image();
}

bool SymbolTab::processScenePointerEvent(
    const QPointF& pos, slint::private_api::PointerEvent e) noexcept {
  return mView->pointerEvent(pos, e);
}

bool SymbolTab::processSceneScrolled(
    const QPointF& pos, slint::private_api::PointerScrollEvent e) noexcept {
  return mView->scrollEvent(pos, e);
}

bool SymbolTab::processSceneKeyEvent(
    const slint::private_api::KeyEvent& e) noexcept {
  return mView->keyEvent(e);
}

bool SymbolTab::requestClose() noexcept {
  commitMetadata();

  if ((!hasUnsavedChanges()) || (!mSymbol->getDirectory().isWritable())) {
    return true;  // Nothing to save.
  }

  const QMessageBox::StandardButton choice = QMessageBox::question(
      qApp->activeWindow(), tr("Save Changes?"),
      tr("The symbol '%1' contains unsaved changes.\n"
         "Do you want to save them before closing it?")
          .arg(*mSymbol->getNames().getDefaultValue()),
      QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
      QMessageBox::Yes);
  if (choice == QMessageBox::Yes) {
    return save();
  } else if (choice == QMessageBox::No) {
    return true;
  } else {
    return false;
  }
}

/*******************************************************************************
 *  IF_GraphicsViewEventHandler Methods
 ******************************************************************************/

bool SymbolTab::graphicsSceneKeyPressed(
    const GraphicsSceneKeyEvent& e) noexcept {
  return  mFsm->processKeyPressed(e);
}

bool SymbolTab::graphicsSceneKeyReleased(
    const GraphicsSceneKeyEvent& e) noexcept {
  return  mFsm->processKeyReleased(e);
}

bool SymbolTab::graphicsSceneMouseMoved(
    const GraphicsSceneMouseEvent& e) noexcept {
   emit cursorCoordinatesChanged(e.scenePos, mUnit);
  return  mFsm->processGraphicsSceneMouseMoved(e);
}

bool SymbolTab::graphicsSceneLeftMouseButtonPressed(
    const GraphicsSceneMouseEvent& e) noexcept {
  return  mFsm->processGraphicsSceneLeftMouseButtonPressed(e);
}

bool SymbolTab::graphicsSceneLeftMouseButtonReleased(
    const GraphicsSceneMouseEvent& e) noexcept {
  return  mFsm->processGraphicsSceneLeftMouseButtonReleased(e);
}

bool SymbolTab::graphicsSceneLeftMouseButtonDoubleClicked(
    const GraphicsSceneMouseEvent& e) noexcept {
  return  mFsm->processGraphicsSceneLeftMouseButtonDoubleClicked(e);
}

bool SymbolTab::graphicsSceneRightMouseButtonReleased(
    const GraphicsSceneMouseEvent& e) noexcept {
  return  mFsm->processGraphicsSceneRightMouseButtonReleased(e);
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

std::pair<RuleCheckMessageList, QSet<SExpression>> SymbolTab::runChecksImpl() {
  return std::make_pair(mSymbol->runChecks(), mSymbol->getMessageApprovals());
}

bool SymbolTab::autoFixImpl(const std::shared_ptr<const RuleCheckMessage>& msg,
                            bool checkOnly) {
  if (autoFixHelper<MsgNameNotTitleCase>(msg, checkOnly)) return true;
  if (autoFixHelper<MsgMissingAuthor>(msg, checkOnly)) return true;
  return false;
}

template <typename MessageType>
bool SymbolTab::autoFixHelper(
    const std::shared_ptr<const RuleCheckMessage>& msg, bool checkOnly) {
  if (msg) {
    if (auto m = msg->as<MessageType>()) {
      if (!checkOnly) autoFix(*m);  // can throw
      return true;
    }
  }
  return false;
}

template <>
void SymbolTab::autoFix(const MsgNameNotTitleCase& msg) {
  mNameParsed = msg.getFixedName();
  commitMetadata();
}

template <>
void SymbolTab::autoFix(const MsgMissingAuthor& msg) {
  Q_UNUSED(msg);
  mAuthor = q2s(getWorkspaceSettingsUserName());
  commitMetadata();
}

void SymbolTab::messageApprovalChanged(const SExpression& approval,
                                       bool approved) noexcept {
  if (mSymbol->setMessageApproved(approval, approved)) {
    if (!mManualModificationsMade) {
      mManualModificationsMade = true;
      onUiDataChanged.notify();
    }
  }
}

void SymbolTab::notifyDerivedUiDataChanged() noexcept {
  onDerivedUiDataChanged.notify();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void SymbolTab::refreshMetadata() noexcept {
  mName = q2s(*mSymbol->getNames().getDefaultValue());
  mNameError = slint::SharedString();
  mNameParsed = mSymbol->getNames().getDefaultValue();
  mDescription = q2s(mSymbol->getDescriptions().getDefaultValue());
  mKeywords = q2s(mSymbol->getKeywords().getDefaultValue());
  mAuthor = q2s(mSymbol->getAuthor());
  mVersion = q2s(mSymbol->getVersion().toStr());
  mVersionError = slint::SharedString();
  mVersionParsed = mSymbol->getVersion();
  mDeprecated = mSymbol->isDeprecated();
  mCategories->setCategories(mSymbol->getCategories());

  onUiDataChanged.notify();
  onDerivedUiDataChanged.notify();
}

void SymbolTab::commitMetadata() noexcept {
  try {
    std::unique_ptr<CmdLibraryElementEdit> cmd(
        new CmdLibraryElementEdit(*mSymbol, tr("Edit Symbol Metadata")));
    cmd->setName(QString(), mNameParsed);
    cmd->setDescription(QString(), s2q(mDescription).trimmed());
    const QString keywords = s2q(mKeywords);
    if (keywords != mSymbol->getKeywords().getDefaultValue()) {
      cmd->setKeywords(QString(), EditorToolbox::cleanKeywords(keywords));
    }
    cmd->setAuthor(s2q(mAuthor).trimmed());
    cmd->setVersion(mVersionParsed);
    cmd->setDeprecated(mDeprecated);
    cmd->setCategories(mCategories->getCategories());
    mUndoStack->execCmd(cmd.release());
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
  }
}

bool SymbolTab::save() noexcept {
  try {
    mSymbol->save();
    if (isNewElement()) {
      const QString dirName =
          mEditor.getLibrary().getElementsDirectoryName<Symbol>();
      const FilePath fp =
          mEditor.getLibrary().getDirectory().getAbsPath(dirName).getPathTo(
              mSymbol->getUuid().toStr());
      TransactionalDirectory dir(TransactionalFileSystem::open(
          fp, mEditor.isWritable(),
          &TransactionalFileSystem::RestoreMode::abort));
      mSymbol->saveTo(dir);
    }
    mSymbol->getDirectory().getFileSystem()->save();
    mUndoStack->setClean();
    mManualModificationsMade = false;

    if (mWizardMode && (mCurrentPageIndex == 0)) {
      ++mCurrentPageIndex;
    }
    refreshMetadata();

    mEditor.getWorkspace().getLibraryDb().startLibraryRescan();
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
    refreshMetadata();
    return false;
  }
}

void SymbolTab::requestRepaint() noexcept {
  ++mFrameIndex;
  onDerivedUiDataChanged.notify();
}

void SymbolTab::applyTheme() noexcept {
  const Theme& theme = mEditor.getWorkspace().getSettings().themes.getActive();
  mGridStyle = theme.getSchematicGridStyle();

  if (mScene) {
    mScene->setBackgroundColors(
        theme.getColor(Theme::Color::sSchematicBackground).getPrimaryColor(),
        theme.getColor(Theme::Color::sSchematicBackground).getSecondaryColor());
    mScene->setOverlayColors(
        theme.getColor(Theme::Color::sSchematicOverlays).getPrimaryColor(),
        theme.getColor(Theme::Color::sSchematicOverlays).getSecondaryColor());
    mScene->setSelectionRectColors(
        theme.getColor(Theme::Color::sSchematicSelection).getPrimaryColor(),
        theme.getColor(Theme::Color::sSchematicSelection).getSecondaryColor());
    mScene->setGridStyle(mGridStyle);
  }

  onDerivedUiDataChanged.notify();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
