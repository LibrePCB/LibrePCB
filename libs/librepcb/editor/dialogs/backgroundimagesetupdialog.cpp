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

void BackgroundImageSettings::tryLoadFromDir(const FilePath& dir) noexcept {
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
    }
  } catch (const Exception& e) {
    qWarning() << "Failed to load background image data:" << e.getMsg();
  }
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

BackgroundImageSetupDialog::BackgroundImageSetupDialog(
    const QString& settingsPrefix, QWidget* parent) noexcept
  : QDialog(parent),
    mUi(new Ui::BackgroundImageSetupDialog),
    mSettingsPrefix(settingsPrefix % "/background_image_dialog"),
    mState(State::Idle),
    mViewScaleFactor(1) {
  mUi->setupUi(this);
  mUi->lblImage->installEventFilter(this);
  connect(mUi->buttonBox, &QDialogButtonBox::accepted, this,
          &BackgroundImageSetupDialog::accept);
  connect(mUi->buttonBox, &QDialogButtonBox::rejected, this,
          &BackgroundImageSetupDialog::reject);
  connect(mUi->btnScreenshot, &QPushButton::clicked, this,
          &BackgroundImageSetupDialog::takeScreenshot);
  connect(mUi->btnPaste, &QPushButton::clicked, this,
          &BackgroundImageSetupDialog::pasteFromClipboard);
  connect(mUi->btnOpen, &QPushButton::clicked, this,
          &BackgroundImageSetupDialog::loadFromFile);
  connect(mUi->btnSelectReference, &QToolButton::clicked, this,
          [this]() { setState(State::SelectReference); });
  connect(mUi->btnMeasureScale, &QToolButton::clicked, this,
          [this]() { setState(State::MeasureStep1); });
  connect(mUi->btnMeasureFinish, &QToolButton::clicked, this, [this]() {
    mUi->spbxDpiX->setValue(mMeasuredDistance.x() /
                            mUi->edtMeasureX->getValue().toInch());
    mUi->spbxDpiY->setValue(mMeasuredDistance.y() /
                            mUi->edtMeasureY->getValue().toInch());
    setState(State::Idle);
  });

  connect(mUi->spbxReferenceX, &QDoubleSpinBox::valueChanged, this,
          &BackgroundImageSetupDialog::settingsModified);
  connect(mUi->spbxReferenceY, &QDoubleSpinBox::valueChanged, this,
          &BackgroundImageSetupDialog::settingsModified);
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
  connect(mUi->cbxEnable, &QCheckBox::toggled, this,
          &BackgroundImageSetupDialog::settingsModified);

  // Reset state.
  setState(State::Idle);

  // Load initial values and window geometry.
  QSettings cs;
  restoreGeometry(cs.value(mSettingsPrefix % "/window_geometry").toByteArray());
}

BackgroundImageSetupDialog::~BackgroundImageSetupDialog() noexcept {
  // Save the values and window geometry.
  QSettings cs;
  cs.setValue(mSettingsPrefix % "/window_geometry", saveGeometry());
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

void BackgroundImageSetupDialog::setSettings(
    const BackgroundImageSettings& s) noexcept {
  mUi->cbxEnable->setChecked(s.enabled);
  mImage = s.image;
  mUi->spbxReferenceX->setValue(s.referencePos.x());
  mUi->spbxReferenceY->setValue(s.referencePos.y());
  mUi->spbxDpiX->setValue(s.dpi.first);
  mUi->spbxDpiY->setValue(s.dpi.second);
  mUi->edtOffsetX->setValue(s.offset.getX());
  mUi->edtOffsetY->setValue(s.offset.getY());
  mUi->edtRotation->setValue(s.rotation);
  QTimer::singleShot(10, this, &BackgroundImageSetupDialog::updateImageLabel);
}

BackgroundImageSettings BackgroundImageSetupDialog::getSettings()
    const noexcept {
  return BackgroundImageSettings{
      mUi->cbxEnable->isChecked(),
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
    event->accept();
    return;
  }

  QDialog::keyPressEvent(event);
}

bool BackgroundImageSetupDialog::eventFilter(QObject* obj, QEvent* e) noexcept {
  if (e->type() == QEvent::MouseMove) {
    QMouseEvent* me = static_cast<QMouseEvent*>(e);
    const QPointF pos = me->position() / mViewScaleFactor;
    if (mState == State::SelectReference) {
      mCursorPos = pos;
      updateImageLabel();
    }
  } else
  if (e->type() == QEvent::MouseButtonPress) {
    QMouseEvent* me = static_cast<QMouseEvent*>(e);
    const QPointF pos = me->position() / mViewScaleFactor;
    if (mState == State::SelectReference) {
      mUi->spbxReferenceX->setValue(mCursorPos.x());
      mUi->spbxReferenceY->setValue(mCursorPos.y());
      setState(State::Idle);
    } else if (mState == State::MeasureStep1) {
      mCursorPos = pos;
      setState(State::MeasureStep2);
    } else if (mState == State::MeasureStep2) {
      mMeasuredDistance = pos - mCursorPos;
      setState(State::MeasureStep3);
    }
  } else if (e->type() == QEvent::Resize) {
    updateImageLabel();
  }

  return QDialog::eventFilter(obj, e);
}

void BackgroundImageSetupDialog::takeScreenshot() noexcept {
  emit settingsModified();
}

void BackgroundImageSetupDialog::pasteFromClipboard() noexcept {
  mImage = qApp->clipboard()->image();
  updateImageLabel();
  emit settingsModified();
}

void BackgroundImageSetupDialog::loadFromFile() noexcept {
  emit settingsModified();
}

void BackgroundImageSetupDialog::updateImageLabel() noexcept {
  if ((mImage.isNull()) || (!mImage.width()) || (!mImage.height())) {
    mUi->lblImage->setPixmap(QPixmap());
    return;
  }

  // Scale image.
  const qreal w = mUi->lblImage->width();
  const qreal h = mUi->lblImage->height();
  mViewScaleFactor = std::min(w / mImage.width(), h / mImage.height());
  QPixmap pixmap = QPixmap::fromImage(mImage).scaled(
      mImage.size() * mViewScaleFactor, Qt::KeepAspectRatio,
      Qt::SmoothTransformation);

  // Draw origin cross.
  {
   const QPointF pos = ((mState == State::SelectReference) ? mCursorPos : QPointF(mUi->spbxReferenceX->value(), mUi->spbxReferenceY->value())) * mViewScaleFactor;
    const qreal r = 30;
   QPainter painter(&pixmap);
   painter.setPen(QPen(Qt::red, 0));
  painter.drawLine(pos.x() - r, pos.y(), pos.x()+r, pos.y());
  painter.drawLine(pos.x(), pos.y() - r, pos.x(), pos.y() + r);
  }

  // Show image.
  mUi->lblImage->setPixmap(pixmap);
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
  mUi->cbxEnable->setEnabled(state == State::Idle);
  if (state != State::Idle) {
    mUi->lblImage->setCursor(Qt::BlankCursor);
  } else {
    mUi->lblImage->unsetCursor();
  }
  mState = state;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
