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

#include "../../3d/openglscenebuilder.h"
#include "../../cmd/cmdcircleedit.h"
#include "../../cmd/cmdholeedit.h"
#include "../../cmd/cmdpolygonedit.h"
#include "../../cmd/cmdstroketextedit.h"
#include "../../dialogs/backgroundimagesetupdialog.h"
#include "../../dialogs/gridsettingsdialog.h"
#include "../../editorcommandset.h"
#include "../../graphics/graphicsscene.h"
#include "../../utils/exclusiveactiongroup.h"
#include "../../utils/toolbarproxy.h"
#include "../../widgets/openglview.h"
#include "../../widgets/statusbar.h"
#include "../../widgets/unsignedlengthedit.h"
#include "../../workspace/desktopservices.h"
#include "../cmd/cmdfootprintedit.h"
#include "../cmd/cmdfootprintpadedit.h"
#include "../cmd/cmdpackageedit.h"
#include "fsm/packageeditorfsm.h"
#include "ui_packageeditorwidget.h"

#include <librepcb/core/application.h>
#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/font/stroketextpathbuilder.h>
#include <librepcb/core/library/librarybaseelementcheckmessages.h>
#include <librepcb/core/library/libraryelementcheckmessages.h>
#include <librepcb/core/library/pkg/footprintpainter.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/library/pkg/packagecheckmessages.h>
#include <librepcb/core/serialization/sexpression.h>
#include <librepcb/core/types/pcbcolor.h>
#include <librepcb/core/utils/clipperhelpers.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacesettings.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

template <>
std::unique_ptr<SExpression> serialize(const float& obj) {
  QString s = QString::number(obj, 'f', 6);
  while (s.endsWith("0") && (!s.endsWith(".0"))) {
    s.chop(1);
  }
  return SExpression::createToken(s);
}

template <>
std::unique_ptr<SExpression> serialize(const double& obj) {
  QString s = QString::number(obj, 'f', 6);
  while (s.endsWith("0") && (!s.endsWith(".0"))) {
    s.chop(1);
  }
  return SExpression::createToken(s);
}

template <>
float deserialize(const SExpression& node) {
  return node.getValue().toFloat();
}

template <>
double deserialize(const SExpression& node) {
  return node.getValue().toDouble();
}

namespace editor {

/*******************************************************************************
 *  Class BackgroundImageSettings
 ******************************************************************************/

bool BackgroundImageSettings::tryLoadFromDir(const FilePath& dir) noexcept {
  *this = BackgroundImageSettings();  // Reset.

  try {
    const FilePath fp = dir.getPathTo("settings.lp");
    if (fp.isExistingFile()) {
      image.load(dir.getPathTo("image.png").toStr(), "png");
      std::unique_ptr<SExpression> root =
          SExpression::parse(FileUtils::readFile(fp), fp);
      enabled = deserialize<bool>(root->getChild("enabled/@0"));
      rotation = deserialize<Angle>(root->getChild("rotation/@0"));
      foreach (const SExpression* node, root->getChildren("reference")) {
        const QPointF source(deserialize<qreal>(node->getChild("source/@0")),
                             deserialize<qreal>(node->getChild("source/@1")));
        const Point target(node->getChild("target"));
        references.append(std::make_pair(source, target));
      }
      return true;
    }
  } catch (const Exception& e) {
    qWarning() << "Failed to load background image data:" << e.getMsg();
  }
  return false;
}

void BackgroundImageSettings::saveToDir(const FilePath& dir) noexcept {
  try {
    if (!image.isNull()) {
      FileUtils::makePath(dir);
      image.save(dir.getPathTo("image.png").toStr(), "png");
      std::unique_ptr<SExpression> root =
          SExpression::createList("librepcb_background_image");
      root->ensureLineBreak();
      root->appendChild("enabled", enabled);
      root->ensureLineBreak();
      root->appendChild("rotation", rotation);
      for (const auto& ref : references) {
        root->ensureLineBreak();
        SExpression& refNode = root->appendList("reference");
        SExpression& sourceNode = refNode.appendList("source");
        sourceNode.appendChild(ref.first.x());
        sourceNode.appendChild(ref.first.y());
        ref.second.serialize(refNode.appendList("target"));
      }
      root->ensureLineBreak();
      FileUtils::writeFile(dir.getPathTo("settings.lp"), root->toByteArray());
    } else if (dir.isExistingDir()) {
      FileUtils::removeDirRecursively(dir);
    }
  } catch (const Exception& e) {
    qWarning() << "Failed to save background image data:" << e.getMsg();
  }
}

QPixmap BackgroundImageSettings::buildPixmap(
    const QColor& bgColor) const noexcept {
  QImage img = image.convertToFormat(QImage::Format_ARGB32);

  auto colorDiff = [](const QColor& a, const QColor& b) {
    return std::abs(a.lightnessF() - b.lightnessF());
  };

  // If the image background color is the inverse of the graphics view
  // background, invert the image to get good contrast for lines in the image.
  if (colorDiff(img.pixelColor(0, 0), bgColor) > 0.5) {
    img.invertPixels();
  }

  // Make the image background transparent.
  const QColor imgBgColor = img.pixelColor(0, 0);
  for (int i = 0; i < img.width(); ++i) {
    for (int k = 0; k < img.height(); ++k) {
      if (colorDiff(img.pixelColor(i, k), imgBgColor) < 0.3) {
        img.setPixelColor(i, k, Qt::transparent);
      }
    }
  }

  return QPixmap::fromImage(img);
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

PackageEditorWidget::PackageEditorWidget(const Context& context,
                                         const FilePath& fp, QWidget* parent)
  : EditorWidgetBase(context, fp, parent),
    mUi(new Ui::PackageEditorWidget),
    mGraphicsScene(new GraphicsScene()),
    mOpenGlSceneBuildScheduled(false),
    mBackgroundImageGraphicsItem(new QGraphicsPixmapItem()) {
  mUi->setupUi(this);
  mUi->lstMessages->setHandler(this);
  mUi->lstMessages->setReadOnly(mContext.readOnly);
  mUi->edtName->setReadOnly(mContext.readOnly);
  mUi->edtDescription->setReadOnly(mContext.readOnly);
  mUi->edtKeywords->setReadOnly(mContext.readOnly);
  mUi->edtAuthor->setReadOnly(mContext.readOnly);
  mUi->edtVersion->setReadOnly(mContext.readOnly);
  mUi->cbxDeprecated->setCheckable(!mContext.readOnly);
  mUi->cbxAssemblyType->setEnabled(!mContext.readOnly);
  mUi->padListEditorWidget->setReadOnly(mContext.readOnly);
  mUi->padListEditorWidget->setFrameStyle(QFrame::NoFrame);
  mUi->footprintEditorWidget->setReadOnly(mContext.readOnly);
  mUi->footprintEditorWidget->setFrameStyle(QFrame::NoFrame);
  mUi->modelListEditorWidget->setReadOnly(mContext.readOnly);
  mUi->modelListEditorWidget->setFrameStyle(QFrame::NoFrame);
  setupErrorNotificationWidget(*mUi->errorNotificationWidget);
  const Theme& theme = mContext.workspace.getSettings().themes.getActive();
  mUi->graphicsView->setBackgroundColors(
      theme.getColor(Theme::Color::sBoardBackground).getPrimaryColor(),
      theme.getColor(Theme::Color::sBoardBackground).getSecondaryColor());
  mUi->graphicsView->setOverlayColors(
      theme.getColor(Theme::Color::sBoardOverlays).getPrimaryColor(),
      theme.getColor(Theme::Color::sBoardOverlays).getSecondaryColor());
  mUi->graphicsView->setInfoBoxColors(
      theme.getColor(Theme::Color::sBoardInfoBox).getPrimaryColor(),
      theme.getColor(Theme::Color::sBoardInfoBox).getSecondaryColor());
  mGraphicsScene->setSelectionRectColors(
      theme.getColor(Theme::Color::sBoardSelection).getPrimaryColor(),
      theme.getColor(Theme::Color::sBoardSelection).getSecondaryColor());
  mUi->graphicsView->setGridStyle(theme.getSchematicGridStyle());
  mUi->graphicsView->setUseOpenGl(
      mContext.workspace.getSettings().useOpenGl.get());
  mUi->graphicsView->setScene(mGraphicsScene.data());
  mUi->graphicsView->setEnabled(false);  // no footprint selected
  mUi->graphicsView->addAction(
      EditorCommandSet::instance().commandToolBarFocus.createAction(
          this, this,
          [this]() {
            mCommandToolBarProxy->startTabFocusCycle(*mUi->graphicsView);
          },
          EditorCommand::ActionFlag::WidgetShortcut));
  setWindowIcon(QIcon(":/img/library/package.png"));

  // Apply grid properties unit from workspace settings
  setGridProperties(PositiveLength(2540000),
                    mContext.workspace.getSettings().defaultLengthUnit.get(),
                    theme.getBoardGridStyle());

  // Setup 2D/3D mode switcher.
  connect(mUi->btnToggle3d, &QToolButton::clicked, this,
          &PackageEditorWidget::toggle3D);
  mUi->modelListEditorWidget->hide();
  connect(mUndoStack.data(), &UndoStack::stateModified, this,
          &PackageEditorWidget::scheduleOpenGlSceneUpdate);
  QTimer* openGlBuilderTimer = new QTimer(this);
  connect(openGlBuilderTimer, &QTimer::timeout, this,
          &PackageEditorWidget::updateOpenGlScene);
  openGlBuilderTimer->start(100);

  // List mount types.
  mUi->cbxAssemblyType->addItem(
      tr("THT (all leads)"), QVariant::fromValue(Package::AssemblyType::Tht));
  mUi->cbxAssemblyType->addItem(
      tr("SMT (all leads)"), QVariant::fromValue(Package::AssemblyType::Smt));
  mUi->cbxAssemblyType->addItem(
      tr("THT+SMT (mixed leads)"),
      QVariant::fromValue(Package::AssemblyType::Mixed));
  mUi->cbxAssemblyType->addItem(
      tr("Other (included in BOM/PnP)"),
      QVariant::fromValue(Package::AssemblyType::Other));
  mUi->cbxAssemblyType->addItem(
      tr("None (excluded from BOM/PnP)"),
      QVariant::fromValue(Package::AssemblyType::None));
  mUi->cbxAssemblyType->addItem(
      tr("Auto-detect (not recommended)"),
      QVariant::fromValue(Package::AssemblyType::Auto));

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
  mPackage = Package::open(std::unique_ptr<TransactionalDirectory>(
      new TransactionalDirectory(mFileSystem)));  // can throw
  updateMetadata();

  // Setup pad list editor widget.
  mUi->padListEditorWidget->setReferences(&mPackage->getPads(),
                                          mUndoStack.data());

  // Setup footprint list editor widget.
  mUi->footprintEditorWidget->setReferences(mPackage.get(), mUndoStack.data());
  mUi->footprintEditorWidget->setLengthUnit(mLengthUnit);
  connect(mUi->footprintEditorWidget,
          &FootprintListEditorWidget::currentFootprintChanged, this,
          &PackageEditorWidget::currentFootprintChanged);

  // Setup 3D model list editor widget.
  mUi->modelListEditorWidget->setReferences(mPackage.get(), mUndoStack.data());
  mUi->modelListEditorWidget->setCurrentFootprint(mCurrentFootprint);
  connect(mUi->modelListEditorWidget,
          &PackageModelListEditorWidget::currentIndexChanged, this,
          &PackageEditorWidget::currentModelChanged);

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
  connect(
      mUi->cbxAssemblyType,
      static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
      this, &PackageEditorWidget::commitMetadata);
  connect(mCategoriesEditorWidget.data(), &CategoryListEditorWidget::edited,
          this, &PackageEditorWidget::commitMetadata);

  // Load finite state machine (FSM).
  PackageEditorFsm::Context fsmContext{mContext,
                                       *this,
                                       *mUndoStack,
                                       *mGraphicsScene,
                                       *mUi->graphicsView,
                                       mLengthUnit,
                                       *mPackage,
                                       nullptr,
                                       nullptr,
                                       *mCommandToolBarProxy};
  mFsm.reset(new PackageEditorFsm(fsmContext));
  connect(mUndoStack.data(), &UndoStack::stateModified, mFsm.data(),
          &PackageEditorFsm::updateAvailableFeatures);
  connect(mFsm.data(), &PackageEditorFsm::availableFeaturesChanged, this,
          [this]() { emit availableFeaturesChanged(getAvailableFeatures()); });
  connect(mFsm.data(), &PackageEditorFsm::statusBarMessageChanged, this,
          &PackageEditorWidget::setStatusBarMessage);
  currentFootprintChanged(0);  // small hack to select the first footprint...

  // Setup background image.
  mBackgroundImageGraphicsItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
  mBackgroundImageGraphicsItem->setTransformationMode(Qt::SmoothTransformation);
  mBackgroundImageGraphicsItem->setZValue(-1000);
  mBackgroundImageGraphicsItem->setOpacity(0.8);
  mBackgroundImageGraphicsItem->setVisible(false);
  mGraphicsScene->addItem(*mBackgroundImageGraphicsItem);
  mBackgroundImageSettings.tryLoadFromDir(getBackgroundImageCacheDir());
  applyBackgroundImageSettings();

  // Last but not least, connect the graphics scene events with the FSM.
  mUi->graphicsView->setEventHandlerObject(this);
}

PackageEditorWidget::~PackageEditorWidget() noexcept {
  // Clean up the state machine nicely to avoid unexpected behavior. Triggering
  // abort (Esc) two times is usually sufficient to leave any active tool, so
  // let's call it three times to be on the safe side. Unfortunately there's
  // no clean way to forcible and guaranteed leaving a tool.
  mFsm->processAbortCommand();
  mFsm->processAbortCommand();
  mFsm->processAbortCommand();
  mFsm.reset();

  // Disconnect UI from package to avoid dangling pointers.
  mUi->modelListEditorWidget->setReferences(nullptr, nullptr);
  mUi->footprintEditorWidget->setReferences(nullptr, nullptr);
  mUi->padListEditorWidget->setReferences(nullptr, nullptr);
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool PackageEditorWidget::isBackgroundImageSet() const noexcept {
  return mBackgroundImageGraphicsItem->isVisible();
}

QSet<EditorWidgetBase::Feature> PackageEditorWidget::getAvailableFeatures()
    const noexcept {
  QSet<EditorWidgetBase::Feature> features = {
      EditorWidgetBase::Feature::Close,
      EditorWidgetBase::Feature::GraphicsView,
      EditorWidgetBase::Feature::OpenGlView,
      EditorWidgetBase::Feature::BackgroundImage,
      EditorWidgetBase::Feature::ExportGraphics,
      EditorWidgetBase::Feature::GenerateOutline,
      EditorWidgetBase::Feature::GenerateCourtyard,
      EditorWidgetBase::Feature::ReNumberPads,
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
  mToolsActionGroup->setActionEnabled(Tool::DRAW_ARC, enabled);
  mToolsActionGroup->setActionEnabled(Tool::DRAW_TEXT, enabled);
  mToolsActionGroup->setActionEnabled(Tool::DRAW_ZONE, enabled);
  mToolsActionGroup->setActionEnabled(Tool::ADD_HOLES, enabled);
  mToolsActionGroup->setActionEnabled(Tool::MEASURE, true);
  mToolsActionGroup->setActionEnabled(Tool::RENUMBER_PADS, enabled);
  mToolsActionGroup->setCurrentAction(mFsm->getCurrentTool());
  connect(mFsm.data(), &PackageEditorFsm::toolChanged, mToolsActionGroup,
          &ExclusiveActionGroup::setCurrentAction);

  mStatusBar->setField(StatusBar::AbsolutePosition, true);
  mStatusBar->setLengthUnit(mLengthUnit);
  connect(mUi->graphicsView, &GraphicsView::cursorScenePositionChanged,
          mStatusBar, &StatusBar::setAbsoluteCursorPosition);
}

void PackageEditorWidget::disconnectEditor() noexcept {
  disconnect(mFsm.data(), &PackageEditorFsm::toolChanged, mToolsActionGroup,
             &ExclusiveActionGroup::setCurrentAction);

  mStatusBar->setField(StatusBar::AbsolutePosition, false);
  disconnect(mUi->graphicsView, &GraphicsView::cursorScenePositionChanged,
             mStatusBar, &StatusBar::setAbsoluteCursorPosition);

  EditorWidgetBase::disconnectEditor();
}

/*******************************************************************************
 *  Public Slots
 ******************************************************************************/

bool PackageEditorWidget::save() noexcept {
  // Remove obsolete message approvals (bypassing the undo stack).
  mPackage->setMessageApprovals(mPackage->getMessageApprovals() -
                                mDisappearedApprovals);

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
  Point delta;
  switch (direction) {
    case Qt::LeftArrow: {
      delta.setX(-mUi->graphicsView->getGridInterval());
      break;
    }
    case Qt::RightArrow: {
      delta.setX(*mUi->graphicsView->getGridInterval());
      break;
    }
    case Qt::UpArrow: {
      delta.setY(*mUi->graphicsView->getGridInterval());
      break;
    }
    case Qt::DownArrow: {
      delta.setY(-mUi->graphicsView->getGridInterval());
      break;
    }
    default: {
      qWarning() << "Unhandled switch-case in PackageEditorWidget::move():"
                 << direction;
      break;
    }
  }
  return mFsm->processMove(delta);
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

bool PackageEditorWidget::moveAlign() noexcept {
  return mFsm->processMoveAlign();
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
  if (mOpenGlView && is3DModeEnabled()) {
    mOpenGlView->zoomIn();
  } else {
    mUi->graphicsView->zoomIn();
  }
  return true;
}

bool PackageEditorWidget::zoomOut() noexcept {
  if (mOpenGlView && is3DModeEnabled()) {
    mOpenGlView->zoomOut();
  } else {
    mUi->graphicsView->zoomOut();
  }
  return true;
}

bool PackageEditorWidget::zoomAll() noexcept {
  if (mOpenGlView && is3DModeEnabled()) {
    mOpenGlView->zoomAll();
  } else {
    mUi->graphicsView->zoomAll();
  }
  return true;
}

bool PackageEditorWidget::toggle3D() noexcept {
  toggle3DMode(!is3DModeEnabled());
  return true;
}

bool PackageEditorWidget::abortCommand() noexcept {
  return mFsm->processAbortCommand();
}

bool PackageEditorWidget::processGenerateOutline() noexcept {
  return mFsm->processGenerateOutline();
}

bool PackageEditorWidget::processGenerateCourtyard() noexcept {
  return mFsm->processGenerateCourtyard();
}

bool PackageEditorWidget::importDxf() noexcept {
  return mFsm->processStartDxfImport();
}

bool PackageEditorWidget::editGridProperties() noexcept {
  GridSettingsDialog dialog(mUi->graphicsView->getGridInterval(), mLengthUnit,
                            mUi->graphicsView->getGridStyle(), this);
  connect(&dialog, &GridSettingsDialog::gridPropertiesChanged, this,
          &PackageEditorWidget::setGridProperties);
  dialog.exec();
  return true;
}

bool PackageEditorWidget::increaseGridInterval() noexcept {
  const Length interval = mUi->graphicsView->getGridInterval() * 2;
  setGridProperties(PositiveLength(interval), mLengthUnit,
                    mUi->graphicsView->getGridStyle());
  return true;
}

bool PackageEditorWidget::decreaseGridInterval() noexcept {
  const Length interval = *mUi->graphicsView->getGridInterval();
  if ((interval % 2) == 0) {
    setGridProperties(PositiveLength(interval / 2), mLengthUnit,
                      mUi->graphicsView->getGridStyle());
  }
  return true;
}

bool PackageEditorWidget::toggleBackgroundImage() noexcept {
  if (mBackgroundImageGraphicsItem->isVisible()) {
    mBackgroundImageSettings.enabled = false;
  } else {
    // Show dialog.
    BackgroundImageSetupDialog dlg("package_editor", this);
    if (!mBackgroundImageSettings.image.isNull()) {
      dlg.setData(mBackgroundImageSettings.image,
                  mBackgroundImageSettings.rotation,
                  mBackgroundImageSettings.references);
    }
    if (dlg.exec() != QDialog::Accepted) {
      return true;  // Aborted.
    }

    mBackgroundImageSettings.image = dlg.getImage();
    mBackgroundImageSettings.rotation = dlg.getRotation();
    mBackgroundImageSettings.references = dlg.getReferences();
    mBackgroundImageSettings.enabled =
        (!mBackgroundImageSettings.image.isNull()) &&
        (mBackgroundImageSettings.references.count() >= 2);
    toggle3DMode(false);
  }

  // Store & apply new settings.
  mBackgroundImageSettings.saveToDir(getBackgroundImageCacheDir());
  applyBackgroundImageSettings();
  return mBackgroundImageGraphicsItem->isVisible();
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
  mUi->cbxAssemblyType->setCurrentIndex(mUi->cbxAssemblyType->findData(
      QVariant::fromValue(mPackage->getAssemblyType(false))));
  mUi->lstMessages->setApprovals(mPackage->getMessageApprovals());
  mCategoriesEditorWidget->setUuids(mPackage->getCategories());
}

QString PackageEditorWidget::commitMetadata() noexcept {
  try {
    QScopedPointer<CmdPackageEdit> cmd(new CmdPackageEdit(*mPackage));
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
    const QVariant asblyType = mUi->cbxAssemblyType->currentData();
    if (asblyType.isValid() && asblyType.canConvert<Package::AssemblyType>()) {
      cmd->setAssemblyType(asblyType.value<Package::AssemblyType>());
    }
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
    case QEvent::KeyPress: {
      auto* e = dynamic_cast<QKeyEvent*>(event);
      Q_ASSERT(e);
      return mFsm->processKeyPressed(*e);
    }
    case QEvent::KeyRelease: {
      auto* e = dynamic_cast<QKeyEvent*>(event);
      Q_ASSERT(e);
      return mFsm->processKeyReleased(*e);
    }
    default: {
      return false;
    }
  }
}

bool PackageEditorWidget::toolChangeRequested(Tool newTool,
                                              const QVariant& mode) noexcept {
  switch (newTool) {
    case Tool::SELECT:
      return mFsm->processStartSelecting();
    case Tool::ADD_THT_PADS:
      return mFsm->processStartAddingFootprintThtPads();
    case Tool::ADD_SMT_PADS: {
      FootprintPad::Function function = FootprintPad::Function::StandardPad;
      if (mode.isValid() && mode.canConvert<FootprintPad::Function>()) {
        function = mode.value<FootprintPad::Function>();
      }
      return mFsm->processStartAddingFootprintSmtPads(function);
    }
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
    case Tool::DRAW_ARC:
      return mFsm->processStartDrawArcs();
    case Tool::DRAW_TEXT:
      return mFsm->processStartDrawTexts();
    case Tool::DRAW_ZONE:
      return mFsm->processStartDrawZones();
    case Tool::ADD_HOLES:
      return mFsm->processStartAddingHoles();
    case Tool::MEASURE:
      return mFsm->processStartMeasure();
    case Tool::RENUMBER_PADS:
      return mFsm->processStartReNumberPads();
    default:
      return false;
  }
}

void PackageEditorWidget::currentFootprintChanged(int index) noexcept {
  mCurrentFootprint = mPackage->getFootprints().value(index);
  mFsm->processChangeCurrentFootprint(mCurrentFootprint);
  mUi->modelListEditorWidget->setCurrentFootprint(mCurrentFootprint);
  scheduleOpenGlSceneUpdate();
}

void PackageEditorWidget::currentModelChanged(int index) noexcept {
  mCurrentModel = mPackage->getModels().value(index);
  scheduleOpenGlSceneUpdate();
}

void PackageEditorWidget::scheduleOpenGlSceneUpdate() noexcept {
  mOpenGlSceneBuildScheduled = true;
}

void PackageEditorWidget::updateOpenGlScene() noexcept {
  if ((!mOpenGlSceneBuildScheduled) || (!mOpenGlSceneBuilder) ||
      mOpenGlSceneBuilder->isBusy()) {
    return;
  }

  std::shared_ptr<SceneData3D> data = std::make_shared<SceneData3D>(
      std::make_shared<TransactionalDirectory>(mPackage->getDirectory()), true);
  data->setSolderResist(&PcbColor::green());
  data->setSilkscreen(&PcbColor::white());
  data->setSilkscreenLayersTop(
      {&Layer::topLegend(), &Layer::topNames(), &Layer::topValues()});
  data->setSilkscreenLayersBot(
      {&Layer::botLegend(), &Layer::botNames(), &Layer::botValues()});
  data->setStepAlphaValue(0.7);
  if (mCurrentFootprint) {
    for (const FootprintPad& pad : mCurrentFootprint->getPads()) {
      const Transform transform(pad.getPosition(), pad.getRotation(), false);
      auto geometries = pad.buildPreviewGeometries();
      for (auto it = geometries.begin(); it != geometries.end(); it++) {
        foreach (const PadGeometry& geometry, it.value()) {
          foreach (const Path& outline, geometry.toOutlines()) {
            data->addArea(*it.key(), outline, transform);
          }
          for (const PadHole& hole : geometry.getHoles()) {
            data->addHole(hole.getPath(), hole.getDiameter(), true, false,
                          transform);
          }
        }
      }
    }
    for (const Polygon& polygon : mCurrentFootprint->getPolygons()) {
      data->addPolygon(polygon, Transform());
    }
    for (const Circle& circle : mCurrentFootprint->getCircles()) {
      data->addCircle(circle, Transform());
    }
    for (const StrokeText& text : mCurrentFootprint->getStrokeTexts()) {
      data->addStroke(text.getLayer(),
                      text.generatePaths(Application::getDefaultStrokeFont()),
                      *text.getStrokeWidth(), Transform(text));
    }
    for (const Hole& hole : mCurrentFootprint->getHoles()) {
      data->addHole(hole.getPath(), hole.getDiameter(), false, false,
                    Transform());
      if (auto offset = hole.getPreviewStopMaskOffset()) {
        const Length width = hole.getDiameter() + (*offset) + (*offset);
        for (const Layer* layer :
             {&Layer::topStopMask(), &Layer::botStopMask()}) {
          data->addStroke(*layer, {*hole.getPath()}, width, Transform());
        }
      }
    }
    if (mCurrentModel) {
      data->addDevice(mPackage->getUuid(), Transform(),
                      mCurrentModel->getFileName(),
                      mCurrentFootprint->getModelPosition(),
                      mCurrentFootprint->getModelRotation(), QString());
    }
  } else {
    const QVector<Path> paths = StrokeTextPathBuilder::build(
        Application::getDefaultStrokeFont(), StrokeTextSpacing(),
        StrokeTextSpacing(), PositiveLength(10000000), UnsignedLength(1000000),
        Alignment(HAlign::center(), VAlign::center()), Angle::deg0(), true,
        tr("Please select a footprint."));
    data->addStroke(Layer::topLegend(), paths, Length(1000000), Transform());
  }

  mOpenGlSceneBuildScheduled = false;
  mOpenGlSceneBuilder->start(data);
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

bool PackageEditorWidget::runChecks(RuleCheckMessageList& msgs) const {
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
void PackageEditorWidget::fixMsg(const MsgDeprecatedAssemblyType& msg) {
  Q_UNUSED(msg);
  QScopedPointer<CmdPackageEdit> cmd(new CmdPackageEdit(*mPackage));
  cmd->setAssemblyType(mPackage->guessAssemblyType());
  mUndoStack->execCmd(cmd.take());
}

template <>
void PackageEditorWidget::fixMsg(const MsgSuspiciousAssemblyType& msg) {
  Q_UNUSED(msg);
  QScopedPointer<CmdPackageEdit> cmd(new CmdPackageEdit(*mPackage));
  cmd->setAssemblyType(mPackage->guessAssemblyType());
  mUndoStack->execCmd(cmd.take());
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
void PackageEditorWidget::fixMsg(const MsgMissingPackageOutline& msg) {
  mUi->footprintEditorWidget->setCurrentIndex(
      mPackage->getFootprints().indexOf(msg.getFootprint().get()));
  mFsm->processGenerateOutline();
}

template <>
void PackageEditorWidget::fixMsg(const MsgMinimumWidthViolation& msg) {
  if (!mCurrentFootprint) return;

  QDialog dlg(this);
  dlg.setWindowTitle(tr("New Line Width"));
  QVBoxLayout* vLayout = new QVBoxLayout(&dlg);
  UnsignedLengthEdit* edtWidth = new UnsignedLengthEdit(&dlg);
  edtWidth->configure(mLengthUnit, LengthEditBase::Steps::generic(),
                      "package_editor/fix_minimum_width_dialog");
  edtWidth->setValue(UnsignedLength(200000));
  edtWidth->setFocus();
  vLayout->addWidget(edtWidth);
  QDialogButtonBox* btnBox = new QDialogButtonBox(&dlg);
  btnBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
  connect(btnBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
  vLayout->addWidget(btnBox);
  if (dlg.exec() != QDialog::Accepted) {
    return;
  }

  if (auto p = mCurrentFootprint->getPolygons().find(msg.getPolygon().get())) {
    QScopedPointer<CmdPolygonEdit> cmd(new CmdPolygonEdit(*p));
    cmd->setLineWidth(edtWidth->getValue(), false);
    mUndoStack->execCmd(cmd.take());
  } else if (auto c =
                 mCurrentFootprint->getCircles().find(msg.getCircle().get())) {
    QScopedPointer<CmdCircleEdit> cmd(new CmdCircleEdit(*c));
    cmd->setLineWidth(edtWidth->getValue(), false);
    mUndoStack->execCmd(cmd.take());
  } else if (auto t = mCurrentFootprint->getStrokeTexts().find(
                 msg.getStrokeText().get())) {
    QScopedPointer<CmdStrokeTextEdit> cmd(new CmdStrokeTextEdit(*t));
    cmd->setStrokeWidth(edtWidth->getValue(), false);
    mUndoStack->execCmd(cmd.take());
  } else {
    throw LogicError(__FILE__, __LINE__,
                     "Whoops, not implemented! Please open a bug report.");
  }
}

template <>
void PackageEditorWidget::fixMsg(const MsgMissingCourtyard& msg) {
  mUi->footprintEditorWidget->setCurrentIndex(
      mPackage->getFootprints().indexOf(msg.getFootprint().get()));
  mFsm->processGenerateCourtyard();
}

template <>
void PackageEditorWidget::fixMsg(const MsgMissingFootprint& msg) {
  Q_UNUSED(msg);
  std::shared_ptr<Footprint> fpt = std::make_shared<Footprint>(
      Uuid::createRandom(), ElementName("default"), "");
  mUndoStack->execCmd(new CmdFootprintInsert(mPackage->getFootprints(), fpt));
}

template <>
void PackageEditorWidget::fixMsg(const MsgMissingFootprintModel& msg) {
  Q_UNUSED(msg);
  toggle3DMode(true);
}

template <>
void PackageEditorWidget::fixMsg(const MsgMissingFootprintName& msg) {
  Q_UNUSED(msg);
  mUi->footprintEditorWidget->setCurrentIndex(
      mPackage->getFootprints().indexOf(msg.getFootprint().get()));
  mFsm->processStartAddingNames();
}

template <>
void PackageEditorWidget::fixMsg(const MsgMissingFootprintValue& msg) {
  Q_UNUSED(msg);
  mUi->footprintEditorWidget->setCurrentIndex(
      mPackage->getFootprints().indexOf(msg.getFootprint().get()));
  mFsm->processStartAddingValues();
}

template <>
void PackageEditorWidget::fixMsg(const MsgFootprintOriginNotInCenter& msg) {
  mFsm->processAbortCommand();
  mFsm->processAbortCommand();
  currentFootprintChanged(
      mPackage->getFootprints().indexOf(msg.getFootprint().get()));
  mFsm->processSelectAll();
  mFsm->processMove(-msg.getCenter());
  mFsm->processAbortCommand();  // Clear selection.
}

template <>
void PackageEditorWidget::fixMsg(const MsgWrongFootprintTextLayer& msg) {
  std::shared_ptr<Footprint> footprint =
      mPackage->getFootprints().get(msg.getFootprint().get());
  std::shared_ptr<StrokeText> text =
      footprint->getStrokeTexts().get(msg.getText().get());
  QScopedPointer<CmdStrokeTextEdit> cmd(new CmdStrokeTextEdit(*text));
  cmd->setLayer(msg.getExpectedLayer(), false);
  mUndoStack->execCmd(cmd.take());
}

template <>
void PackageEditorWidget::fixMsg(const MsgUnusedCustomPadOutline& msg) {
  std::shared_ptr<Footprint> footprint =
      mPackage->getFootprints().get(msg.getFootprint().get());
  std::shared_ptr<FootprintPad> pad =
      footprint->getPads().get(msg.getPad().get());
  QScopedPointer<CmdFootprintPadEdit> cmd(new CmdFootprintPadEdit(*pad));
  cmd->setCustomShapeOutline(Path());
  mUndoStack->execCmd(cmd.take());
}

template <>
void PackageEditorWidget::fixMsg(const MsgInvalidCustomPadOutline& msg) {
  std::shared_ptr<Footprint> footprint =
      mPackage->getFootprints().get(msg.getFootprint().get());
  std::shared_ptr<FootprintPad> pad =
      footprint->getPads().get(msg.getPad().get());
  QScopedPointer<CmdFootprintPadEdit> cmd(new CmdFootprintPadEdit(*pad));
  cmd->setShape(FootprintPad::Shape::RoundedRect, false);
  mUndoStack->execCmd(cmd.take());
}

template <>
void PackageEditorWidget::fixMsg(const MsgPadStopMaskOff& msg) {
  std::shared_ptr<Footprint> footprint =
      mPackage->getFootprints().get(msg.getFootprint().get());
  std::shared_ptr<FootprintPad> pad =
      footprint->getPads().get(msg.getPad().get());
  QScopedPointer<CmdFootprintPadEdit> cmd(new CmdFootprintPadEdit(*pad));
  cmd->setStopMaskConfig(MaskConfig::automatic(), false);
  mUndoStack->execCmd(cmd.take());
}

template <>
void PackageEditorWidget::fixMsg(const MsgSmtPadWithSolderPaste& msg) {
  std::shared_ptr<Footprint> footprint =
      mPackage->getFootprints().get(msg.getFootprint().get());
  std::shared_ptr<FootprintPad> pad =
      footprint->getPads().get(msg.getPad().get());
  QScopedPointer<CmdFootprintPadEdit> cmd(new CmdFootprintPadEdit(*pad));
  cmd->setSolderPasteConfig(MaskConfig::off());
  mUndoStack->execCmd(cmd.take());
}

template <>
void PackageEditorWidget::fixMsg(const MsgThtPadWithSolderPaste& msg) {
  std::shared_ptr<Footprint> footprint =
      mPackage->getFootprints().get(msg.getFootprint().get());
  std::shared_ptr<FootprintPad> pad =
      footprint->getPads().get(msg.getPad().get());
  QScopedPointer<CmdFootprintPadEdit> cmd(new CmdFootprintPadEdit(*pad));
  cmd->setSolderPasteConfig(MaskConfig::off());
  mUndoStack->execCmd(cmd.take());
}

template <>
void PackageEditorWidget::fixMsg(const MsgPadWithCopperClearance& msg) {
  std::shared_ptr<Footprint> footprint =
      mPackage->getFootprints().get(msg.getFootprint().get());
  std::shared_ptr<FootprintPad> pad =
      footprint->getPads().get(msg.getPad().get());
  QScopedPointer<CmdFootprintPadEdit> cmd(new CmdFootprintPadEdit(*pad));
  cmd->setCopperClearance(UnsignedLength(0));
  mUndoStack->execCmd(cmd.take());
}

template <>
void PackageEditorWidget::fixMsg(
    const MsgFiducialClearanceLessThanStopMask& msg) {
  std::shared_ptr<Footprint> footprint =
      mPackage->getFootprints().get(msg.getFootprint().get());
  std::shared_ptr<FootprintPad> pad =
      footprint->getPads().get(msg.getPad().get());
  const tl::optional<Length> offset = pad->getStopMaskConfig().getOffset();
  if (offset && (*offset > 0)) {
    QScopedPointer<CmdFootprintPadEdit> cmd(new CmdFootprintPadEdit(*pad));
    cmd->setCopperClearance(UnsignedLength(*offset));
    mUndoStack->execCmd(cmd.take());
  }
}

template <>
void PackageEditorWidget::fixMsg(const MsgHoleWithoutStopMask& msg) {
  std::shared_ptr<Footprint> footprint =
      mPackage->getFootprints().get(msg.getFootprint().get());
  std::shared_ptr<Hole> hole = footprint->getHoles().get(msg.getHole().get());
  QScopedPointer<CmdHoleEdit> cmd(new CmdHoleEdit(*hole));
  cmd->setStopMaskConfig(MaskConfig::automatic());
  mUndoStack->execCmd(cmd.take());
}

template <>
void PackageEditorWidget::fixMsg(const MsgUnspecifiedPadFunction& msg) {
  fixPadFunction(msg);
}

template <>
void PackageEditorWidget::fixMsg(const MsgSuspiciousPadFunction& msg) {
  fixPadFunction(msg);
}

template <typename MessageType>
void PackageEditorWidget::fixPadFunction(const MessageType& msg) {
  QMenu menu(this);
  QAction* aAll = menu.addAction(tr("Apply to all unspecified pads"));
  aAll->setCheckable(true);
  menu.addSeparator();
  for (int i = 0; i < static_cast<int>(FootprintPad::Function::_COUNT); ++i) {
    const FootprintPad::Function value = static_cast<FootprintPad::Function>(i);
    if (value != FootprintPad::Function::Unspecified) {
      QAction* action =
          menu.addAction(FootprintPad::getFunctionDescriptionTr(value));
      action->setData(QVariant::fromValue(value));
    }
  }

  QAction* action = nullptr;
  const QPoint pos = QCursor::pos();
  do {
    action = menu.exec(pos);
  } while (action == aAll);

  if (action && action->data().isValid() &&
      action->data().canConvert<FootprintPad::Function>()) {
    if (aAll->isChecked()) {
      UndoStackTransaction transaction(*mUndoStack,
                                       tr("Fix Unspecified Pad Functions"));
      for (auto& footprint : mPackage->getFootprints()) {
        for (auto& pad : footprint.getPads()) {
          if (pad.getFunction() == FootprintPad::Function::Unspecified) {
            QScopedPointer<CmdFootprintPadEdit> cmd(
                new CmdFootprintPadEdit(pad));
            cmd->setFunction(action->data().value<FootprintPad::Function>(),
                             false);
            transaction.append(cmd.take());
          }
        }
      }
      transaction.commit();
    } else {
      std::shared_ptr<Footprint> footprint =
          mPackage->getFootprints().get(msg.getFootprint().get());
      std::shared_ptr<FootprintPad> pad =
          footprint->getPads().get(msg.getPad().get());
      QScopedPointer<CmdFootprintPadEdit> cmd(new CmdFootprintPadEdit(*pad));
      cmd->setFunction(action->data().value<FootprintPad::Function>(), false);
      mUndoStack->execCmd(cmd.take());
    }
  }
}

template <typename MessageType>
bool PackageEditorWidget::fixMsgHelper(
    std::shared_ptr<const RuleCheckMessage> msg, bool applyFix) {
  if (msg) {
    if (auto m = msg->as<MessageType>()) {
      if (applyFix) fixMsg(*m);  // can throw
      return true;
    }
  }
  return false;
}

bool PackageEditorWidget::processRuleCheckMessage(
    std::shared_ptr<const RuleCheckMessage> msg, bool applyFix) {
  if (fixMsgHelper<MsgDeprecatedAssemblyType>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgSuspiciousAssemblyType>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgNameNotTitleCase>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgMissingAuthor>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgMissingCategories>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgMissingPackageOutline>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgMinimumWidthViolation>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgMissingCourtyard>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgMissingFootprint>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgMissingFootprintModel>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgMissingFootprintName>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgMissingFootprintValue>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgFootprintOriginNotInCenter>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgWrongFootprintTextLayer>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgUnusedCustomPadOutline>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgInvalidCustomPadOutline>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgPadStopMaskOff>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgSmtPadWithSolderPaste>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgThtPadWithSolderPaste>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgPadWithCopperClearance>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgFiducialClearanceLessThanStopMask>(msg, applyFix))
    return true;
  if (fixMsgHelper<MsgHoleWithoutStopMask>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgUnspecifiedPadFunction>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgSuspiciousPadFunction>(msg, applyFix)) return true;
  return false;
}

void PackageEditorWidget::ruleCheckApproveRequested(
    std::shared_ptr<const RuleCheckMessage> msg, bool approve) noexcept {
  setMessageApproved(*mPackage, msg, approve);
  updateMetadata();
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
        mContext.workspace.getSettings().themes.getActive(),
        "package_editor/" % settingsKey, this);
    connect(&dialog, &GraphicsExportDialog::requestOpenFile, this,
            [this](const FilePath& fp) {
              DesktopServices services(mContext.workspace.getSettings(), this);
              services.openLocalPath(fp);
            });
    dialog.exec();
  } catch (const Exception& e) {
    QMessageBox::warning(this, tr("Error"), e.getMsg());
  }
  return true;
}

void PackageEditorWidget::setGridProperties(const PositiveLength& interval,
                                            const LengthUnit& unit,
                                            Theme::GridStyle style) noexcept {
  mUi->graphicsView->setGridInterval(interval);
  mUi->graphicsView->setGridStyle(style);
  mLengthUnit = unit;
  if (mStatusBar) {
    mStatusBar->setLengthUnit(unit);
  }
  if (mFsm) {
    mFsm->updateAvailableFeatures();  // Re-calculate "snap to grid" feature!
  }
}

void PackageEditorWidget::applyBackgroundImageSettings() noexcept {
  BackgroundImageSettings& s = mBackgroundImageSettings;

  const bool enable = s.enabled && (!s.image.isNull());
  mBackgroundImageGraphicsItem->setVisible(enable);

  if (enable) {
    // Make the image background transparent.
    const Theme& theme = mContext.workspace.getSettings().themes.getActive();
    mBackgroundImageGraphicsItem->setPixmap(s.buildPixmap(
        theme.getColor(Theme::Color::sBoardBackground).getPrimaryColor()));

    // Apply the transform.
    QTransform t;
    t.rotate(-s.rotation.toDeg());
    if (s.references.count() >= 2) {
      const Point deltaPx =
          Point::fromPx(s.references.at(1).first - s.references.at(0).first)
              .rotated(-s.rotation);
      const Point deltaMm =
          (s.references.at(1).second - s.references.at(0).second);

      const qreal scaleFactorX =
          std::abs(deltaMm.toMmQPointF().x() / deltaPx.toMmQPointF().x());
      const qreal scaleFactorY =
          std::abs(deltaMm.toMmQPointF().y() / deltaPx.toMmQPointF().y());

      t.scale(scaleFactorX, scaleFactorY);
      t.translate(-s.references.first().first.x(),
                  -s.references.first().first.y());
    }
    mBackgroundImageGraphicsItem->setTransform(t);
    if (s.references.count() >= 1) {
      mBackgroundImageGraphicsItem->setPos(
          s.references.first().second.toPxQPointF());
    }
  }
}

FilePath PackageEditorWidget::getBackgroundImageCacheDir() const noexcept {
  return Application::getCacheDir()
      .getPathTo("backgrounds")
      .getPathTo(mPackage->getUuid().toStr());
}

void PackageEditorWidget::toggle3DMode(bool enable) noexcept {
  if (enable) {
    mUi->graphicsView->hide();
    mUi->modelListEditorWidget->show();
    mUi->btnToggle3d->setArrowType(Qt::RightArrow);
    mOpenGlView.reset(new OpenGlView(this));
    mUi->mainLayout->insertWidget(0, mOpenGlView.data(), 2);
    mOpenGlSceneBuilder.reset(new OpenGlSceneBuilder());
    connect(mOpenGlSceneBuilder.data(), &OpenGlSceneBuilder::started,
            mOpenGlView.data(), &OpenGlView::startSpinning);
    connect(mOpenGlSceneBuilder.data(), &OpenGlSceneBuilder::finished,
            mOpenGlView.data(), &OpenGlView::stopSpinning);
    connect(mOpenGlSceneBuilder.data(), &OpenGlSceneBuilder::objectAdded,
            mOpenGlView.data(), &OpenGlView::addObject);
    connect(mOpenGlSceneBuilder.data(), &OpenGlSceneBuilder::objectRemoved,
            mOpenGlView.data(), &OpenGlView::removeObject);
    connect(mOpenGlSceneBuilder.data(), &OpenGlSceneBuilder::objectUpdated,
            mOpenGlView.data(),
            static_cast<void (OpenGlView::*)()>(&OpenGlView::update));
    scheduleOpenGlSceneUpdate();
  } else {
    mOpenGlView.reset();
    mUi->modelListEditorWidget->hide();
    mUi->graphicsView->show();
    mUi->btnToggle3d->setArrowType(Qt::LeftArrow);
  }
}

bool PackageEditorWidget::is3DModeEnabled() const noexcept {
  return mUi->modelListEditorWidget->isVisible();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
