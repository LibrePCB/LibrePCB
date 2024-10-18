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
      offset = Point(root->getChild("offset"));
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
      offset.serialize(root->appendList("offset"));
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
      (offset == rhs.offset) && (rotation == rhs.rotation);
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
    mReferenceGraphicsItem(new QGraphicsPathItem()),
    mMeasure1GraphicsItem(new QGraphicsPathItem()),
    mMeasure2GraphicsItem(new QGraphicsPathItem()),
    mMeasureLineGraphicsItem(new QGraphicsLineItem()),
    mState(State::Idle) {
  mUi->setupUi(this);
  mUi->graphicsView->setOriginCrossVisible(false);
  mUi->graphicsView->setBackgroundColors(Qt::transparent, Qt::transparent);
  mUi->graphicsView->setScene(new GraphicsScene(this));
  mUi->graphicsView->setEventHandlerObject(this);
  mUi->graphicsView->scene()->addItem(mImageGraphicsItem.get());
  mUi->graphicsView->scene()->addItem(mReferenceGraphicsItem.get());
  mUi->graphicsView->scene()->addItem(mMeasure1GraphicsItem.get());
  mUi->graphicsView->scene()->addItem(mMeasure2GraphicsItem.get());
  mUi->graphicsView->scene()->addItem(mMeasureLineGraphicsItem.get());
  setupCrossGraphicsItem(mReferenceGraphicsItem.get(), Qt::red, true);
  setupCrossGraphicsItem(mMeasure1GraphicsItem.get(), Qt::blue, false);
  setupCrossGraphicsItem(mMeasure2GraphicsItem.get(), Qt::blue, false);
  setupGraphicsItem(mMeasureLineGraphicsItem.get(), Qt::blue, false);

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
  connect(mUi->btnSelectReference, &QToolButton::clicked, this,
          [this]() { setState(State::SelectReference); });
  connect(mUi->btnMeasureScale, &QToolButton::clicked, this,
          [this]() { setState(State::MeasureStep1); });
  connect(mUi->btnMeasureFinish, &QToolButton::clicked, this, [this]() {
    QPointF diff = mMeasure2GraphicsItem->pos() - mMeasure1GraphicsItem->pos();
    mUi->spbxDpiX->setValue(diff.x() / mUi->edtMeasureX->getValue().toInch());
    mUi->spbxDpiY->setValue(diff.y() / mUi->edtMeasureY->getValue().toInch());
    setState(State::Idle);
  });

  connect(mUi->spbxReferenceX, &QDoubleSpinBox::valueChanged, this,
          &BackgroundImageSetupDialog::settingsModified);
  connect(mUi->spbxReferenceX, &QDoubleSpinBox::valueChanged, this,
          &BackgroundImageSetupDialog::updateReferenceMarker);
  connect(mUi->spbxReferenceY, &QDoubleSpinBox::valueChanged, this,
          &BackgroundImageSetupDialog::settingsModified);
  connect(mUi->spbxReferenceY, &QDoubleSpinBox::valueChanged, this,
          &BackgroundImageSetupDialog::updateReferenceMarker);
  connect(mUi->spbxDpiX, &QDoubleSpinBox::valueChanged, this,
          &BackgroundImageSetupDialog::settingsModified);
  connect(mUi->spbxDpiY, &QDoubleSpinBox::valueChanged, this,
          &BackgroundImageSetupDialog::settingsModified);
  connect(mUi->edtOffsetX, &LengthEdit::valueChanged, this,
          &BackgroundImageSetupDialog::settingsModified);
  connect(mUi->edtOffsetY, &LengthEdit::valueChanged, this,
          &BackgroundImageSetupDialog::settingsModified);
  connect(mUi->edtRotation, &AngleEdit::valueChanged, this,
          &BackgroundImageSetupDialog::settingsModified);

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
  mUi->edtOffsetX->setValue(s.offset.getX());
  mUi->edtOffsetY->setValue(s.offset.getY());
  mUi->edtRotation->setValue(s.rotation);

  QTimer::singleShot(10, this, &BackgroundImageSetupDialog::updateImage);
}

BackgroundImageSettings BackgroundImageSetupDialog::getSettings()
    const noexcept {
  return BackgroundImageSettings{
      true,
      mImage,
      QPointF(mUi->spbxReferenceX->value(), mUi->spbxReferenceY->value()),
      std::make_pair(mUi->spbxDpiX->value(), mUi->spbxDpiY->value()),
      Point(mUi->edtOffsetX->getValue(), mUi->edtOffsetY->getValue()),
      mUi->edtRotation->getValue(),
  };
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BackgroundImageSetupDialog::keyPressEvent(QKeyEvent* event) noexcept {
  if ((mState != State::Idle) && (event->key() == Qt::Key_Escape)) {
    setState(State::Idle);
    updateImage();
    updateReferenceMarker();
    event->accept();
    return;
  } else if ((mState == State::MeasureStep3) &&
             (event->key() == Qt::Key_Return)) {
    mUi->btnMeasureFinish->click();
    return;
  }

  QDialog::keyPressEvent(event);
}

bool BackgroundImageSetupDialog::graphicsViewEventHandler(
    QEvent* event) noexcept {
  if (event->type() == QEvent::GraphicsSceneMouseMove) {
    QGraphicsSceneMouseEvent* e = static_cast<QGraphicsSceneMouseEvent*>(event);
    if (mState == State::SelectReference) {
      mReferenceGraphicsItem->setPos(e->scenePos());
    } else if (mState == State::MeasureStep1) {
      mMeasure1GraphicsItem->setPos(e->scenePos());
    } else if (mState == State::MeasureStep2) {
      mMeasure2GraphicsItem->setPos(e->scenePos());
      mMeasureLineGraphicsItem->setLine(
          QLineF(mMeasure1GraphicsItem->pos(), mMeasure2GraphicsItem->pos()));
    }
  } else if (event->type() == QEvent::GraphicsSceneMousePress) {
    QGraphicsSceneMouseEvent* e = static_cast<QGraphicsSceneMouseEvent*>(event);
    if (mState == State::SelectReference) {
      mUi->spbxReferenceX->setValue(e->scenePos().x());
      mUi->spbxReferenceY->setValue(e->scenePos().y());
      setState(State::Idle);
    } else if (mState == State::MeasureStep1) {
      setState(State::MeasureStep2);
    } else if (mState == State::MeasureStep2) {
      setState(State::MeasureStep3);
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
  if (mCountdownSecs <= 0) {
    takeScreenshot();
  } else {
    setMessage(QString::number(mCountdownSecs));
    QTimer::singleShot(1000, this,
                       &BackgroundImageSetupDialog::screenshotCountdownTick);
  }
}

void BackgroundImageSetupDialog::takeScreenshot() noexcept {
  if (mScreen) {
    QPixmap pixmap = mScreen->grabWindow(0);
    if (pixmap.isNull()) {
      QMessageBox::critical(this, tr("Error"),
                            tr("Could not take a screenshot. Note that this "
                               "feature does not work "
                               "on some systems due to security mechanisms."));
    } else {
      setImage(pixmap.toImage());
      return;
    }
  }
  updateImage();
}

void BackgroundImageSetupDialog::pasteFromClipboard() noexcept {
  QImage image = qApp->clipboard()->image();
  if (image.isNull()) {
    QMessageBox::warning(
        this, tr("No image in clipboard"),
        tr("Please make sure to copy an image into the clipboard."));
  }
  setImage(image);
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
  if (!fp.isEmpty()) {
    cs.setValue(key, fp);
  }
  QImage image;
  if (!image.load(fp)) {
    QMessageBox::warning(this, tr("Error"),
                         tr("Failed to open the selected image file."));
  }
  setImage(image);
}

void BackgroundImageSetupDialog::setImage(const QImage& image) noexcept {
  mImage = image;
  mUi->spbxReferenceX->setValue(image.width() / 2);
  mUi->spbxReferenceY->setValue(image.height() / 2);
  mUi->spbxDpiX->setValue(image.width());
  mUi->spbxDpiY->setValue(image.width());
  updateImage();
  mUi->graphicsView->zoomAll();
  emit settingsModified();
}

void BackgroundImageSetupDialog::updateImage() noexcept {
  if ((mImage.isNull()) || (!mImage.width()) || (!mImage.height())) {
    setMessage(tr("Load an image with one of the buttons on the left side."));
    return;
  }

  mImageGraphicsItem->setPixmap(QPixmap::fromImage(mImage));
}

void BackgroundImageSetupDialog::updateReferenceMarker() noexcept {
  mReferenceGraphicsItem->setPos(mUi->spbxReferenceX->value(),
                                 mUi->spbxReferenceY->value());
}

void BackgroundImageSetupDialog::setMessage(const QString& msg) noexcept {
  /*mUi->lblImage->setPixmap(QPixmap());
  mUi->lblImage->setText(msg);
  mUi->lblImage->setAlignment(Qt::AlignCenter);
  mUi->lblImage->setMargin(50);*/
}

void BackgroundImageSetupDialog::setState(State state) noexcept {
  mUi->btnScreenshot->setEnabled(state == State::Idle);
  mUi->btnPaste->setEnabled(state == State::Idle);
  mUi->btnOpen->setEnabled(state == State::Idle);
  mUi->spbxReferenceX->setEnabled(state == State::Idle);
  mUi->spbxReferenceY->setEnabled(state == State::Idle);
  mUi->btnSelectReference->setEnabled(state == State::Idle);
  mUi->btnSelectReference->setCheckable(state == State::SelectReference);
  mUi->btnSelectReference->setChecked(state == State::SelectReference);
  mUi->spbxDpiX->setEnabled(state == State::Idle);
  mUi->spbxDpiY->setEnabled(state == State::Idle);
  mUi->btnMeasureScale->setEnabled(state == State::Idle);
  mUi->btnMeasureScale->setCheckable((state == State::MeasureStep1) ||
                                     (state == State::MeasureStep2));
  mUi->btnMeasureScale->setChecked((state == State::MeasureStep1) ||
                                   (state == State::MeasureStep2));
  mUi->edtMeasureX->setEnabled(state == State::MeasureStep3);
  mUi->edtMeasureY->setEnabled(state == State::MeasureStep3);
  mUi->btnMeasureFinish->setEnabled(state == State::MeasureStep3);
  mUi->edtOffsetX->setEnabled(state == State::Idle);
  mUi->edtOffsetY->setEnabled(state == State::Idle);
  mUi->edtRotation->setEnabled(state == State::Idle);
  mMeasure1GraphicsItem->setVisible(state >= State::MeasureStep1);
  mMeasure2GraphicsItem->setVisible(state >= State::MeasureStep2);
  mMeasureLineGraphicsItem->setVisible(state >= State::MeasureStep2);
  if ((state != State::Idle) && (state != State::MeasureStep3)) {
    mUi->graphicsView->setCursor(Qt::BlankCursor);
  } else {
    mUi->graphicsView->unsetCursor();
  }
  mState = state;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
