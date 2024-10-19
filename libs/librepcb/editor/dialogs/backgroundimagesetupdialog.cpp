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
#include "backgroundimagesetupdialog.h"

#include "filedialog.h"
#include "ui_backgroundimagesetupdialog.h"

#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/serialization/sexpression.h>
#include <librepcb/core/utils/toolbox.h>
#include <librepcb/editor/graphics/graphicsscene.h>

#include <QtCore>

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
  try {
    const FilePath fp = dir.getPathTo("settings.lp");
    if (fp.isExistingFile()) {
      image.load(dir.getPathTo("image.png").toStr(), "png");
      std::unique_ptr<SExpression> root =
          SExpression::parse(FileUtils::readFile(fp), fp);
      enabled = deserialize<bool>(root->getChild("enabled/@0"));
      referencePos =
          QPointF(deserialize<qreal>(root->getChild("reference/@0")),
                  deserialize<qreal>(root->getChild("reference/@1")));
      dpi = std::make_pair(deserialize<qreal>(root->getChild("dpi/@0")),
                           deserialize<qreal>(root->getChild("dpi/@1")));
      position = Point(root->getChild("position"));
      rotation = deserialize<Angle>(root->getChild("rotation/@0"));
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
      SExpression& refNode = root->appendList("reference");
      refNode.appendChild(referencePos.x());
      refNode.appendChild(referencePos.y());
      root->ensureLineBreak();
      SExpression& dpiNode = root->appendList("dpi");
      dpiNode.appendChild(dpi.first);
      dpiNode.appendChild(dpi.second);
      root->ensureLineBreak();
      position.serialize(root->appendList("position"));
      root->ensureLineBreak();
      root->appendChild("rotation", rotation);
      root->ensureLineBreak();
      FileUtils::writeFile(dir.getPathTo("settings.lp"), root->toByteArray());
    } else if (dir.isExistingDir()) {
      FileUtils::removeDirRecursively(dir);
    }
  } catch (const Exception& e) {
    qWarning() << "Failed to save background image data:" << e.getMsg();
  }
}

bool BackgroundImageSettings::operator==(
    const BackgroundImageSettings& rhs) const noexcept {
  return (enabled == rhs.enabled) && (image == rhs.image) &&
      (referencePos == rhs.referencePos) && (dpi == rhs.dpi) &&
      (position == rhs.position) && (rotation == rhs.rotation);
}

bool BackgroundImageSettings::operator!=(
    const BackgroundImageSettings& rhs) const noexcept {
  return !(*this == rhs);
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

template <typename T>
static void setupGraphicsItem(T* item, QColor color, bool show) {
  item->setPen(QPen(color, 0));
  item->setVisible(show);
}

static void setupCrossGraphicsItem(QGraphicsPathItem* item, QColor color,
                                   bool show) {
  const qreal r = 30;
  QPainterPath crossPath;
  crossPath.moveTo(-r, 0);
  crossPath.lineTo(r, 0);
  crossPath.moveTo(0, -r);
  crossPath.lineTo(0, r);
  crossPath.addEllipse(-r / 4, -r / 4, r / 2, r / 2);
  item->setPath(crossPath);
  item->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
  setupGraphicsItem(item, color, show);
}

BackgroundImageSetupDialog::BackgroundImageSetupDialog(
    const QString& settingsPrefix, QWidget* parent) noexcept
  : QDialog(parent),
    mUi(new Ui::BackgroundImageSetupDialog),
    mSettingsPrefix(settingsPrefix % "/background_image_dialog"),
    mImageGraphicsItem(new QGraphicsPixmapItem()),
    mCursorGraphicsItem(new QGraphicsPathItem()),
    mCropGraphicsItem(new QGraphicsPathItem()),
    mReferenceGraphicsItem(new QGraphicsPathItem()),
    mMeasure1GraphicsItem(new QGraphicsPathItem()),
    mMeasure2GraphicsItem(new QGraphicsPathItem()),
    mMeasureLineGraphicsItem(new QGraphicsLineItem()),
    mRotateWidget(),
    mMeasuredLengthWidget(),
    mState(State::Idle),
    mAutoNextState(false) {
  mUi->setupUi(this);
  mUi->graphicsView->setOriginCrossVisible(false);
  mUi->graphicsView->setBackgroundColors(Qt::transparent, Qt::transparent);
  mUi->graphicsView->setScene(new GraphicsScene(this));
  mUi->graphicsView->setEventHandlerObject(this);
  mUi->graphicsView->scene()->addItem(mImageGraphicsItem.get());
  mUi->graphicsView->scene()->addItem(mCursorGraphicsItem.get());
  mUi->graphicsView->scene()->addItem(mCropGraphicsItem.get());
  mUi->graphicsView->scene()->addItem(mReferenceGraphicsItem.get());
  mUi->graphicsView->scene()->addItem(mMeasure1GraphicsItem.get());
  mUi->graphicsView->scene()->addItem(mMeasure2GraphicsItem.get());
  mUi->graphicsView->scene()->addItem(mMeasureLineGraphicsItem.get());
  setupCrossGraphicsItem(mCursorGraphicsItem.get(), Qt::blue, false);
  setupGraphicsItem(mCropGraphicsItem.get(), Qt::blue, false);
  setupCrossGraphicsItem(mReferenceGraphicsItem.get(), Qt::red, true);
  setupCrossGraphicsItem(mMeasure1GraphicsItem.get(), Qt::blue, false);
  setupCrossGraphicsItem(mMeasure2GraphicsItem.get(), Qt::blue, false);
  setupGraphicsItem(mMeasureLineGraphicsItem.get(), Qt::blue, false);

  // UI Handlers.
  connect(mUi->buttonBox, &QDialogButtonBox::accepted, this,
          &BackgroundImageSetupDialog::accept);
  connect(mUi->buttonBox, &QDialogButtonBox::rejected, this,
          &BackgroundImageSetupDialog::reject);
  connect(mUi->btnScreenshot, &QPushButton::clicked, this,
          &BackgroundImageSetupDialog::startScreenshot);
  connect(mUi->btnPaste, &QPushButton::clicked, this,
          &BackgroundImageSetupDialog::pasteFromClipboard);
  connect(mUi->btnOpen, &QPushButton::clicked, this,
          &BackgroundImageSetupDialog::loadFromFile);
  connect(mUi->btnReset, &QToolButton::clicked, this,
          [this]() { setSettings(BackgroundImageSettings()); });
  connect(mUi->btnCrop, &QToolButton::clicked, this,
          [this]() { setState(State::Crop); });
  connect(mUi->btnSelectReference, &QToolButton::clicked, this,
          [this]() { setState(State::SelectReference); });
  connect(mUi->btnMeasureScaleX, &QToolButton::clicked, this, [this]() {
    mMeasureDirection = Qt::Horizontal;
    setState(State::MeasureStep1);
  });
  connect(mUi->btnMeasureScaleY, &QToolButton::clicked, this, [this]() {
    mMeasureDirection = Qt::Vertical;
    setState(State::MeasureStep1);
  });

  // Modified signals.
  connect(mUi->spbxReferenceX,
          static_cast<void (QDoubleSpinBox::*)(double)>(
              &QDoubleSpinBox::valueChanged),
          this, [this]() {
            updateUi();
            emit settingsModified();
          });
  connect(mUi->spbxReferenceY,
          static_cast<void (QDoubleSpinBox::*)(double)>(
              &QDoubleSpinBox::valueChanged),
          this, [this]() {
            updateUi();
            emit settingsModified();
          });
  connect(mUi->spbxDpiX,
          static_cast<void (QDoubleSpinBox::*)(double)>(
              &QDoubleSpinBox::valueChanged),
          this, &BackgroundImageSetupDialog::settingsModified);
  connect(mUi->spbxDpiY,
          static_cast<void (QDoubleSpinBox::*)(double)>(
              &QDoubleSpinBox::valueChanged),
          this, &BackgroundImageSetupDialog::settingsModified);
  connect(mUi->edtPositionX, &LengthEdit::valueChanged, this,
          &BackgroundImageSetupDialog::settingsModified);
  connect(mUi->edtPositionY, &LengthEdit::valueChanged, this,
          &BackgroundImageSetupDialog::settingsModified);
  connect(mUi->edtRotation, &AngleEdit::valueChanged, this, [this]() {
    updateUi();
    emit settingsModified();
  });

  // Create widget for rotating.
  {
    mRotateWidget.reset(new QWidget(mUi->graphicsView));
    mRotateWidget->setAutoFillBackground(true);
    mRotateWidget->setLayout(new QHBoxLayout());
    mRotateWidget->layout()->setContentsMargins(3, 3, 3, 3);
    mRotateWidget->layout()->setSpacing(3);
    QToolButton* btnRotateCcw = new QToolButton(mRotateWidget.get());
    btnRotateCcw->setIcon(QIcon(":/img/actions/rotate_left.png"));
    connect(btnRotateCcw, &QToolButton::clicked, this, [this]() {
      mUi->edtRotation->setValue(mUi->edtRotation->getValue() + Angle::deg90());
    });
    mRotateWidget->layout()->addWidget(btnRotateCcw);
    QToolButton* btnRotateCw = new QToolButton(mRotateWidget.get());
    btnRotateCw->setIcon(QIcon(":/img/actions/rotate_right.png"));
    connect(btnRotateCw, &QToolButton::clicked, this, [this]() {
      mUi->edtRotation->setValue(mUi->edtRotation->getValue() - Angle::deg90());
    });
    mRotateWidget->layout()->addWidget(btnRotateCw);
    QToolButton* btnApply = new QToolButton(mRotateWidget.get());
    btnApply->setIcon(QIcon(":/img/actions/apply.png"));
    connect(btnApply, &QToolButton::clicked, this,
            &BackgroundImageSetupDialog::commitOperation);
    mRotateWidget->layout()->addWidget(btnApply);
    QToolButton* btnCancel = new QToolButton(mRotateWidget.get());
    btnCancel->setIcon(QIcon(":/img/actions/cancel.png"));
    connect(btnCancel, &QToolButton::clicked, this,
            &BackgroundImageSetupDialog::cancelOperation);
    mRotateWidget->layout()->addWidget(btnCancel);
    mRotateWidget->adjustSize();
  }

  // Create widget for measurements.
  {
    mMeasuredLengthWidget.reset(new QWidget(mUi->graphicsView));
    mMeasuredLengthWidget->setAutoFillBackground(true);
    mMeasuredLengthWidget->setLayout(new QHBoxLayout());
    mMeasuredLengthWidget->layout()->setContentsMargins(3, 3, 3, 3);
    mMeasuredLengthWidget->layout()->setSpacing(3);
    mMeasuredLengthWidget->layout()->addWidget(
        new QLabel((mMeasureDirection == Qt::Vertical) ? "ΔY:" : "ΔX:",
                   mMeasuredLengthWidget.get()));
    mMeasuredLengthEdit = new LengthEdit(mMeasuredLengthWidget.get());
    mMeasuredLengthWidget->layout()->addWidget(mMeasuredLengthEdit);
    QToolButton* btnApply = new QToolButton(mMeasuredLengthWidget.get());
    btnApply->setIcon(QIcon(":/img/actions/apply.png"));
    connect(btnApply, &QToolButton::clicked, this,
            &BackgroundImageSetupDialog::commitOperation);
    mMeasuredLengthWidget->layout()->addWidget(btnApply);
    QToolButton* btnCancel = new QToolButton(mMeasuredLengthWidget.get());
    btnCancel->setIcon(QIcon(":/img/actions/cancel.png"));
    connect(btnCancel, &QToolButton::clicked, this,
            &BackgroundImageSetupDialog::cancelOperation);
    mMeasuredLengthWidget->layout()->addWidget(btnCancel);
    mMeasuredLengthWidget->adjustSize();
  }

  // Reset state.
  setState(State::Idle);

  // Load initial values and window geometry.
  QSettings cs;
  restoreGeometry(cs.value(mSettingsPrefix % "/window_geometry").toByteArray());
}

BackgroundImageSetupDialog::~BackgroundImageSetupDialog() noexcept {
  mUi->graphicsView->setEventHandlerObject(nullptr);

  // Save the values and window geometry.
  QSettings cs;
  cs.setValue(mSettingsPrefix % "/window_geometry", saveGeometry());
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

void BackgroundImageSetupDialog::setSettings(
    const BackgroundImageSettings& s) noexcept {
  QImage image = s.image;
  // If no image was loaded but is available in clipboard, use it.
  if (image.isNull()) {
    image = qApp->clipboard()->image();
  }
  setImage(image);

  mUi->spbxReferenceX->setValue(s.referencePos.x());
  mUi->spbxReferenceY->setValue(s.referencePos.y());
  mUi->spbxDpiX->setValue(s.dpi.first);
  mUi->spbxDpiY->setValue(s.dpi.second);
  mUi->edtPositionX->setValue(s.position.getX());
  mUi->edtPositionY->setValue(s.position.getY());
  mUi->edtRotation->setValue(s.rotation);

  QTimer::singleShot(10, this, &BackgroundImageSetupDialog::fitImageInView);
}

BackgroundImageSettings BackgroundImageSetupDialog::getSettings()
    const noexcept {
  return BackgroundImageSettings{
      true,
      mImage,
      QPointF(mUi->spbxReferenceX->value(), mUi->spbxReferenceY->value()),
      std::make_pair(mUi->spbxDpiX->value(), mUi->spbxDpiY->value()),
      Point(mUi->edtPositionX->getValue(), mUi->edtPositionY->getValue()),
      mUi->edtRotation->getValue(),
  };
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BackgroundImageSetupDialog::keyPressEvent(QKeyEvent* event) noexcept {
  if ((mState != State::Idle) && (event->key() == Qt::Key_Escape)) {
    cancelOperation();
    event->accept();
    return;
  } else if ((mState == State::MeasureStep3) &&
             (event->key() == Qt::Key_Return)) {
    commitOperation();
    return;
  } else if (mScreen) {
    mScreen = nullptr;
    screenshotCountdownTick();
    return;
  }

  QDialog::keyPressEvent(event);
}

bool BackgroundImageSetupDialog::graphicsViewEventHandler(
    QEvent* event) noexcept {
  if (event->type() == QEvent::GraphicsSceneMouseMove) {
    QGraphicsSceneMouseEvent* e = static_cast<QGraphicsSceneMouseEvent*>(event);
    mCursorGraphicsItem->setPos(e->scenePos());
    if ((mState == State::Crop) && (mCropGraphicsItem->isVisible())) {
      QPainterPath p = mCropGraphicsItem->path();
      p.lineTo(e->scenePos());
      mCropGraphicsItem->setPath(p);
    } else if (mState == State::SelectReference) {
      mReferenceGraphicsItem->setPos(e->scenePos());
    } else if (mState == State::MeasureStep1) {
      mMeasure1GraphicsItem->setPos(e->scenePos());
      mMeasure2GraphicsItem->setPos(e->scenePos());
      mMeasureLineGraphicsItem->setLine(
          QLineF(mMeasure1GraphicsItem->pos(), mMeasure2GraphicsItem->pos()));
    } else if (mState == State::MeasureStep2) {
      mMeasure2GraphicsItem->setPos(e->scenePos());
      mMeasureLineGraphicsItem->setLine(
          QLineF(mMeasure1GraphicsItem->pos(), mMeasure2GraphicsItem->pos()));
    }
  } else if (event->type() == QEvent::GraphicsSceneMousePress) {
    QGraphicsSceneMouseEvent* e = static_cast<QGraphicsSceneMouseEvent*>(event);
    if (e->button() != Qt::LeftButton) return false;
    if (mState == State::Crop) {
      QPainterPath p;
      p.moveTo(e->scenePos());
      mCropGraphicsItem->setPath(p);
      mCropGraphicsItem->show();
    } else if (mState == State::SelectReference) {
      QPointF p = mImageGraphicsItem->mapFromScene(e->scenePos());
      mUi->spbxReferenceX->setValue(p.x());
      mUi->spbxReferenceY->setValue(p.y());
      if (mAutoNextState) {
        mMeasureDirection = Qt::Horizontal;
        setState(State::MeasureStep1);
      } else {
        setState(State::Idle);
      }
    } else if (mState == State::MeasureStep1) {
      setState(State::MeasureStep2);
    } else if (mState == State::MeasureStep2) {
      mMeasuredLengthEdit->setFocus(Qt::TabFocusReason);
      setState(State::MeasureStep3);
    }
  } else if (event->type() == QEvent::GraphicsSceneMouseRelease) {
    if (mState == State::Crop) {
      QPainterPath path = mCropGraphicsItem->path();
      path.closeSubpath();
      path = mImageGraphicsItem->transform().inverted().map(path);
      mCropGraphicsItem->hide();
      if (path.elementCount() > 10) {
        QPointF delta;
        mImage = cropImage(mImage, path, delta);
        mUi->spbxReferenceX->setValue(mUi->spbxReferenceX->value() - delta.x());
        mUi->spbxReferenceY->setValue(mUi->spbxReferenceY->value() - delta.y());
        updateUi();
        fitImageInView();
      }
      setState(mAutoNextState ? State::Rotate : State::Idle);
    }
  }
  return false;
}

void BackgroundImageSetupDialog::startScreenshot() noexcept {
  QList<QScreen*> screens = QGuiApplication::screens();
  mScreen = screens.value(0);
  if (screens.count() > 1) {
    QMenu menu;
    QHash<QAction*, QScreen*> screenMap;
    for (QScreen* screen : screens) {
      screenMap.insert(menu.addAction(tr("Screen %1: %2")
                                          .arg(screenMap.count() + 1)
                                          .arg(screen->model())),
                       screen);
    }
    mScreen = screenMap.value(menu.exec(QCursor::pos()));
    if (!mScreen) return;
  }

  mCountdownSecs = 4;
  screenshotCountdownTick();
}

void BackgroundImageSetupDialog::screenshotCountdownTick() noexcept {
  --mCountdownSecs;
  if (!mScreen) {
    // Screen disappeared or screenshot aborted.
    updateUi();
  } else if (mCountdownSecs <= 0) {
    takeScreenshot();
  } else {
    updateUi(QString::number(mCountdownSecs));
    QTimer::singleShot(1000, this,
                       &BackgroundImageSetupDialog::screenshotCountdownTick);
  }
}

void BackgroundImageSetupDialog::takeScreenshot() noexcept {
  if (!mScreen) return;

  QPixmap pixmap = mScreen->grabWindow(0);
  setImage(pixmap.toImage());
  mScreen = nullptr;
  if (pixmap.isNull()) {
    updateUi(
        tr("Could not take a screenshot. Note that this feature does not "
           "work on some systems due to security mechanisms."));
  } else {
    mAutoNextState = true;
    setState(State::Crop);
  }
}

void BackgroundImageSetupDialog::pasteFromClipboard() noexcept {
  QImage image = qApp->clipboard()->image();
  setImage(image);
  if (image.isNull()) {
    updateUi(tr("Please make sure to copy an image into the clipboard."));
  } else {
    mAutoNextState = true;
    setState(State::Crop);
  }
}

void BackgroundImageSetupDialog::loadFromFile() noexcept {
  QString extensionsStr;
  foreach (const QString& ext, QImageReader::supportedImageFormats()) {
    extensionsStr += "*." % ext % " ";
  }

  QSettings cs;
  QString key = mSettingsPrefix % "/file";
  QString fp = cs.value(key, QDir::homePath()).toString();
  fp = FileDialog::getOpenFileName(parentWidget(), tr("Choose image"), fp,
                                   extensionsStr);
  if (fp.isEmpty()) {
    return;  // Aborted.
  }
  cs.setValue(key, fp);
  QImage image;
  if (!image.load(fp)) {
    QMessageBox::warning(this, tr("Error"),
                         tr("Failed to open the selected image file."));
  }
  setImage(image);
  if (!image.isNull()) {
    mAutoNextState = true;
    setState(State::Crop);
  }
}

void BackgroundImageSetupDialog::cancelOperation() noexcept {
  mAutoNextState = false;
  setState(State::Idle);
  updateUi();
}

void BackgroundImageSetupDialog::commitOperation() noexcept {
  if (mState == State::Rotate) {
    setState(mAutoNextState ? State::SelectReference : State::Idle);
  } else if (mState == State::MeasureStep3) {
    if (!mMeasuredLengthEdit) return;
    QPointF diff = mMeasure2GraphicsItem->pos() - mMeasure1GraphicsItem->pos();
    const qreal len = (mMeasureDirection == Qt::Vertical) ? diff.y() : diff.x();
    if (len < 50) {
      QMessageBox::warning(
          this, tr("Inaccurate measurement"),
          tr("The measured distance is very short or the resolution is too "
             "low, thus the calculated scale factor will be inaccurate. "
             "Make sure to use a high-resolution image and measure distances "
             "as long as possible."));
    }
    if (mMeasureDirection == Qt::Vertical) {
      mUi->spbxDpiY->setValue(len / mMeasuredLengthEdit->getValue().toInch());
    } else {
      mUi->spbxDpiX->setValue(len / mMeasuredLengthEdit->getValue().toInch());
    }
    if (mAutoNextState && (mMeasureDirection == Qt::Horizontal)) {
      mMeasureDirection = Qt::Vertical;
      setState(State::MeasureStep1);
    } else {
      mAutoNextState = false;
      setState(State::Idle);
    }
  }
}

void BackgroundImageSetupDialog::setImage(const QImage& image) noexcept {
  mImage = image;
  mUi->spbxReferenceX->setValue(image.width() / 2);
  mUi->spbxReferenceY->setValue(image.height() / 2);
  mUi->spbxDpiX->setValue(image.width());
  mUi->spbxDpiY->setValue(image.width());
  updateUi();
  fitImageInView();
  emit settingsModified();
}

void BackgroundImageSetupDialog::setState(State state) noexcept {
  mState = state;
  updateUi();
}

void BackgroundImageSetupDialog::updateUi(QString msg) noexcept {
  const bool valid = !mImage.isNull();
  const bool idle = (mState == State::Idle);
  mUi->btnScreenshot->setEnabled(idle);
  mUi->btnPaste->setEnabled(idle);
  mUi->btnOpen->setEnabled(idle);
  mUi->btnReset->setEnabled(idle);
  mUi->btnCrop->setEnabled(valid && idle);
  mUi->spbxReferenceX->setEnabled(valid && idle);
  mUi->spbxReferenceY->setEnabled(valid && idle);
  mUi->btnSelectReference->setEnabled(valid && idle);
  mUi->btnSelectReference->setCheckable(valid &&
                                        (mState == State::SelectReference));
  mUi->btnSelectReference->setChecked(valid &&
                                      (mState == State::SelectReference));
  mUi->spbxDpiX->setEnabled(valid && idle);
  mUi->spbxDpiY->setEnabled(valid && idle);
  mUi->btnMeasureScaleX->setEnabled(valid && idle);
  mUi->btnMeasureScaleX->setCheckable((mState == State::MeasureStep1) ||
                                      (mState == State::MeasureStep2));
  mUi->btnMeasureScaleX->setChecked((mState == State::MeasureStep1) ||
                                    (mState == State::MeasureStep2));
  mUi->btnMeasureScaleY->setEnabled(valid && idle);
  mUi->btnMeasureScaleY->setCheckable((mState == State::MeasureStep1) ||
                                      (mState == State::MeasureStep2));
  mUi->btnMeasureScaleY->setChecked((mState == State::MeasureStep1) ||
                                    (mState == State::MeasureStep2));
  mUi->edtPositionX->setEnabled(valid && idle);
  mUi->edtPositionY->setEnabled(valid && idle);
  mUi->edtRotation->setEnabled(valid && idle);
  mCursorGraphicsItem->setVisible(
      (mState == State::Crop) ||
      ((mState >= State::MeasureStep1) && (mState < State::MeasureStep3)));
  mMeasure1GraphicsItem->setVisible(mState >= State::MeasureStep1);
  mMeasure2GraphicsItem->setVisible(mState >= State::MeasureStep2);
  mMeasureLineGraphicsItem->setVisible(mState >= State::MeasureStep2);
  mRotateWidget->setVisible(mState == State::Rotate);
  if (mRotateWidget->isVisible()) {
    mRotateWidget->move(mUi->graphicsView->rect().center() -
                        mRotateWidget->rect().center());
  }
  mMeasuredLengthWidget->setVisible(mState == State::MeasureStep3);
  if (mMeasuredLengthWidget->isVisible()) {
    mMeasuredLengthWidget->move(mUi->graphicsView->rect().center() -
                                mMeasuredLengthWidget->rect().center());
  }
  if ((mState == State::Crop) ||
      ((mState >= State::SelectReference) && (mState < State::MeasureStep3))) {
    mUi->graphicsView->setCursor(Qt::BlankCursor);
  } else {
    mUi->graphicsView->unsetCursor();
  }

  // Show message if no image available to display.
  if (msg.isEmpty() &&
      (mImage.isNull() || (!mImage.width()) || (!mImage.height()))) {
    QStringList lines;
    lines.append(tr("Load an image with one of the buttons on the left side."));
    lines.append(tr("Draw a line around the footprint to cut it out."));
    lines.append(tr("Rotate the image as desired."));
    lines.append(tr("Select a reference point (e.g. [0, 0]) in the image."));
    lines.append(tr(
        "Measure a distance in X-direction to calibrate the X scale factor."));
    lines.append(tr(
        "Measure a distance in Y-direction to calibrate the Y scale factor."));
    lines.append(
        tr("Specify the position of the reference point in the footprint "
           "editor."));
    msg = "<p>" %
        tr("This tool allows you to set a background image in the footprint "
           "editor to easily verify the size &amp; position of footprint pads "
           "etc. Typically a screenshot of the package drawing from the part's "
           "datasheet may be used as background.") %
        "</p>";
    msg += "<ol>";
    for (const QString& line : lines) {
      msg += QString("<li>%1</li>").arg(line);
    }
    msg += "</ol>";
    msg += "<p><b>" %
        tr("Important: Make sure to zoom in as much as possible when taking "
           "the screenshot, to get a reasonably high resolution!") %
        "</b></p>";
  }

  if (msg.isEmpty()) {
    // Show image.
    mUi->lblMessage->hide();
    QTransform t;
    t.rotate(-mUi->edtRotation->getValue().toDeg());
    t.translate(-mImage.width() / 2, -mImage.height() / 2);
    mImageGraphicsItem->setTransform(t);
    mImageGraphicsItem->setPixmap(QPixmap::fromImage(mImage));
    mUi->graphicsView->show();
  } else {
    // Show text.
    const bool multiline = msg.contains("<p>");
    QFont f = mUi->lblMessage->font();
    f.setPointSize((msg.length() == 1) ? 40 : (multiline ? 12 : 20));
    mUi->lblMessage->setFont(f);

    mUi->graphicsView->hide();
    mUi->lblMessage->setAlignment(multiline ? (Qt::AlignLeft | Qt::AlignVCenter)
                                            : Qt::AlignCenter);
    mUi->lblMessage->setText(msg);
    mUi->lblMessage->show();
  }

  // Update reference marker.
  const QPointF p = mImageGraphicsItem->mapToScene(
      mUi->spbxReferenceX->value(), mUi->spbxReferenceY->value());
  mReferenceGraphicsItem->setPos(p);

  // Update status bar text.
  switch (mState) {
    case State::Crop: {
      mUi->lblStatusBar->setText(
          tr("Hold down the left mouse button while drawing a line around "
             "the footprint"));
      break;
    }
    case State::SelectReference: {
      mUi->lblStatusBar->setText(
          tr("Click into the image to specify the reference coordinates"));
      break;
    }
    case State::MeasureStep1: {
      mUi->lblStatusBar->setText(
          tr("Click into the image to specify the first coordinate"));
      break;
    }
    case State::MeasureStep2: {
      mUi->lblStatusBar->setText(
          tr("Click into the image to specify the second coordinate"));
      break;
    }
    case State::MeasureStep3: {
      mUi->lblStatusBar->setText(
          tr("Specify the real X- or Y-distance of the measurement"));
      break;
    }
    default: {
      mUi->lblStatusBar->clear();
      break;
    }
  }
}

void BackgroundImageSetupDialog::fitImageInView() noexcept {
  QRectF br = mImageGraphicsItem->boundingRect();
  br = mImageGraphicsItem->mapToScene(br).boundingRect();
  mUi->graphicsView->setVisibleSceneRect(br);
}

QImage BackgroundImageSetupDialog::cropImage(const QImage& img,
                                             const QPainterPath& p,
                                             QPointF& delta) noexcept {
  // Determine background color.
  QHash<QRgb, int> histogram;
  for (int i = 0; i <= 100; ++i) {
    QPoint pos = p.pointAtPercent(i / qreal(100)).toPoint();
    if (pos.x() < 0) pos.setX(0);
    if (pos.y() < 0) pos.setY(0);
    if (pos.x() >= img.width()) pos.setX(img.width() - 1);
    if (pos.x() >= img.height()) pos.setY(img.height() - 1);
    histogram[img.pixel(pos)]++;
  }
  const int maxHistogramCount = Toolbox::sorted(histogram.values()).last();
  const QRgb bgColor = histogram.key(maxHistogramCount);

  // Create new empty pixmap.
  QPixmap pixmap(img.width(), img.height());
  pixmap.fill(bgColor);

  // Paste cropped image content.
  {
    QPainter painter(&pixmap);
    painter.setClipPath(p);
    painter.drawImage(0, 0, img);
  }

  // Auto-crop to content.
  QRect rect = p.boundingRect().toRect();
  const int m = std::min(rect.width(), rect.height()) / 20;
  rect = rect.adjusted(-m, -m, m, m);
  delta = rect.topLeft();
  return pixmap.copy(rect).toImage();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
