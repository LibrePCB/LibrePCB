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

#include "../widgets/lengthedit.h"
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
namespace editor {

static std::shared_ptr<QGraphicsPathItem> createCrossGraphicsItem(bool cursor) {
  auto i = std::make_shared<QGraphicsPathItem>();
  i->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
  i->setPen(QPen(Qt::blue, 0));
  const qreal len = cursor ? 2000 : 30;
  QPainterPath p;
  p.moveTo(-len, 0);
  p.lineTo(len, 0);
  p.moveTo(0, -len);
  p.lineTo(0, len);
  if (!cursor) {
    p.addEllipse(QPointF(0, 0), 15, 15);
  }
  i->setPath(p);
  return i;
}

static std::shared_ptr<QGraphicsLineItem> createRefLineGraphicsItem() {
  auto i = std::make_shared<QGraphicsLineItem>();
  i->setPen(QPen(Qt::blue, 0));
  return i;
}

static std::shared_ptr<QWidget> createReferenceWidget(
    int index, QWidget* parent, QList<std::pair<QPointF, Point>>* refs,
    std::function<void()> cb) {
  auto w = std::make_shared<QWidget>(parent);
  w->setLayout(new QHBoxLayout());
  w->layout()->setContentsMargins(3, 3, 3, 3);
  w->layout()->setSpacing(3);
  LengthEdit* edtX = new LengthEdit(w.get());
  edtX->setValue(refs->value(index).second.getX());
  QObject::connect(edtX, &LengthEdit::valueChanged,
                   [index, refs, cb](const Length& v) {
                     if (index < refs->count()) {
                       std::get<1>((*refs)[index]).setX(v);
                       cb();
                     }
                   });
  w->layout()->addWidget(edtX);
  LengthEdit* edtY = new LengthEdit(w.get());
  edtY->setValue(refs->value(index).second.getY());
  QObject::connect(edtY, &LengthEdit::valueChanged,
                   [index, refs, cb](const Length& v) {
                     if (index < refs->count()) {
                       std::get<1>((*refs)[index]).setY(v);
                       cb();
                     }
                   });
  w->layout()->addWidget(edtY);
  w->setFocusProxy(edtX);
  w->adjustSize();
  w->move(0, index * w->height());
  w->show();
  return w;
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
    mImage(),
    mRotation(),
    mLoadedReferences(),
    mReferences(),
    mScreen(),
    mCountdownSecs(0),
    mRotateWidget(),
    mImageGraphicsItem(new QGraphicsPixmapItem()),
    mCursorGraphicsItem(createCrossGraphicsItem(true)),
    mCropGraphicsItem(new QGraphicsPathItem()) {
  mUi->setupUi(this);

  GraphicsScene* scene = new GraphicsScene(this);
  scene->setBackgroundColors(Qt::transparent, Qt::transparent);
  scene->setOriginCrossVisible(false);
  mUi->graphicsView->setSpinnerColor(Qt::transparent);
  mUi->graphicsView->setScene(scene);
  mUi->graphicsView->setEventHandlerObject(this);
  mUi->graphicsView->scene()->addItem(mImageGraphicsItem.get());
  mUi->graphicsView->scene()->addItem(mCursorGraphicsItem.get());
  mUi->graphicsView->scene()->addItem(mCropGraphicsItem.get());
  mImageGraphicsItem->setTransformationMode(Qt::SmoothTransformation);
  mCropGraphicsItem->setPen(QPen(Qt::blue, 0));

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
      mRotation += Angle::deg45();
      updateUi();
    });
    mRotateWidget->layout()->addWidget(btnRotateCcw);

    QToolButton* btnRotateCw = new QToolButton(mRotateWidget.get());
    btnRotateCw->setIcon(QIcon(":/img/actions/rotate_right.png"));
    connect(btnRotateCw, &QToolButton::clicked, this, [this]() {
      mRotation -= Angle::deg45();
      updateUi();
    });
    mRotateWidget->layout()->addWidget(btnRotateCw);

    QToolButton* btnMirror = new QToolButton(mRotateWidget.get());
    btnMirror->setIcon(QIcon(":/img/actions/mirror_horizontal.png"));
    btnMirror->setCheckable(true);
    connect(btnMirror, &QToolButton::toggled, this, [this]() {
      mImage = mImage.mirrored(true, false);
      updateUi();
    });
    mRotateWidget->layout()->addWidget(btnMirror);

    QToolButton* btnApply = new QToolButton(mRotateWidget.get());
    btnApply->setIcon(QIcon(":/img/actions/apply.png"));
    connect(btnApply, &QToolButton::clicked, this, [this]() {
      mState = State::SelectRef1;
      updateUi();
    });
    mRotateWidget->layout()->addWidget(btnApply);

    QToolButton* btnCancel = new QToolButton(mRotateWidget.get());
    btnCancel->setIcon(QIcon(":/img/actions/cancel.png"));
    connect(btnCancel, &QToolButton::clicked, this, [this]() {
      mState = State::Idle;
      updateUi();
    });
    mRotateWidget->layout()->addWidget(btnCancel);

    mRotateWidget->adjustSize();
  }

  // Set minimum side bar width to avoid dynamic resizing.
  if (auto w = createReferenceWidget(0, this, &mReferences, nullptr)) {
    mUi->hLine->setMinimumWidth(w->width());
  }

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
  connect(mUi->btnReset, &QToolButton::clicked, this, [this]() {
    mImage = QImage();
    mRotation = Angle::deg0();
    mReferences.clear();
    mCropGraphicsItem->setPath(QPainterPath());
    mState = State::Idle;
    updateUi();
  });

  // Load initial values and window geometry.
  QSettings cs;
  restoreGeometry(cs.value(mSettingsPrefix % "/window_geometry").toByteArray());

  // Try to load image from clipboard.
  pasteFromClipboard();
  updateUi();  // Clear error message if no image in clipboard.
}

BackgroundImageSetupDialog::~BackgroundImageSetupDialog() noexcept {
  mUi->graphicsView->setEventHandlerObject(nullptr);

  // Save the values and window geometry.
  QSettings cs;
  cs.setValue(mSettingsPrefix % "/window_geometry", saveGeometry());
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BackgroundImageSetupDialog::setData(
    const QImage& image, const Angle& rotation,
    const QList<std::pair<QPointF, Point>>& references) noexcept {
  mImage = image;
  mRotation = rotation;
  mLoadedReferences = references;
  mReferences = references;
  updateUi();
  fitImageInView();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BackgroundImageSetupDialog::keyPressEvent(QKeyEvent* event) noexcept {
  if ((event->key() == Qt::Key_Escape) &&
      (mCropGraphicsItem->path().elementCount())) {
    mCropGraphicsItem->setPath(QPainterPath());
    updateUi();
    event->accept();
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
  if (event->type() == QEvent::GraphicsSceneMousePress) {
    QGraphicsSceneMouseEvent* e = static_cast<QGraphicsSceneMouseEvent*>(event);
    if (e->button() != Qt::LeftButton) return false;
    if (mState == State::Crop) {
      QPainterPath p;
      p.moveTo(e->scenePos());
      mCropGraphicsItem->setPath(p);
    } else if (mState == State::Rotate) {
      mState = State::SelectRef1;
      updateUi();
    } else if (mState == State::SelectRef1) {
      auto ref = mLoadedReferences.value(0);
      ref.first = mImageGraphicsItem->mapFromScene(e->scenePos());
      mReferences.append(ref);
      mState = State::SelectRef2;
      updateUi();
    } else if (mState == State::SelectRef2) {
      auto ref = mLoadedReferences.value(1);
      ref.first = mImageGraphicsItem->mapFromScene(e->scenePos());
      mReferences.append(ref);
      mState = State::Idle;
      updateUi();
      if (auto w = mReferenceWidgets.value(0)) {
        w->setFocus(Qt::TabFocusReason);
      }
    }
  } else if (event->type() == QEvent::GraphicsSceneMouseRelease) {
    QGraphicsSceneMouseEvent* e = static_cast<QGraphicsSceneMouseEvent*>(event);
    if (e->button() != Qt::LeftButton) return false;
    if (mState == State::Crop) {
      QPainterPath path = mCropGraphicsItem->path();
      mCropGraphicsItem->setPath(QPainterPath());
      path.closeSubpath();
      path.translate(-mImageGraphicsItem->pos());
      if (path.elementCount() > 10) {
        mImage = cropImage(mImage, path);
      }
      mState = State::Rotate;
      updateUi();
      fitImageInView();
    }
  } else if (event->type() == QEvent::GraphicsSceneMouseMove) {
    QGraphicsSceneMouseEvent* e = static_cast<QGraphicsSceneMouseEvent*>(event);
    mCursorGraphicsItem->setPos(e->scenePos());
    if ((mState == State::Crop) &&
        (mCropGraphicsItem->path().elementCount() > 0)) {
      QPainterPath p = mCropGraphicsItem->path();
      p.lineTo(e->scenePos());
      mCropGraphicsItem->setPath(p);
    }
    updateAnchors();
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
      QString name = tr("Screen %1").arg(screenMap.count() + 1);
      QString type = (screen->manufacturer() + " " + screen->model()).trimmed();
      if (!type.isEmpty()) {
        name += QString(" (%1)").arg(type);
      }
      screenMap.insert(menu.addAction(name), screen);
    }
    mScreen = screenMap.value(menu.exec(QCursor::pos()));
    if (!mScreen) return;
  }

  mUi->btnReset->click();
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

  mImage = mScreen->grabWindow(0).toImage();
  mScreen = nullptr;
  if (mImage.isNull()) {
    mState = State::Idle;
    updateUi(
        tr("Could not take a screenshot. Note that this feature does not "
           "work on some systems due to security mechanisms."));
  } else {
    mState = State::Crop;
    updateUi();
  }
  fitImageInView();
  raise();
  activateWindow();
}

void BackgroundImageSetupDialog::pasteFromClipboard() noexcept {
  mUi->btnReset->click();

  mImage = qApp->clipboard()->image();
  if (mImage.isNull()) {
    updateUi(tr("No image found in the clipboard."));
  } else {
    mState = State::Crop;
    updateUi();
  }
  fitImageInView();
}

void BackgroundImageSetupDialog::loadFromFile() noexcept {
  QString extensionsStr;
  foreach (const QString& ext, QImageReader::supportedImageFormats()) {
    extensionsStr += "*." % ext % " ";
  }

  QSettings cs;
  QString key = mSettingsPrefix % "/file";
  QString fp = cs.value(key, QDir::homePath()).toString();
  fp = FileDialog::getOpenFileName(this, tr("Choose image"), fp, extensionsStr);
  if (fp.isEmpty()) {
    return;  // Aborted.
  }
  cs.setValue(key, fp);
  mUi->btnReset->click();
  if (!mImage.load(fp)) {
    updateUi(tr("Failed to open the selected image file."));
  } else {
    mState = State::Crop;
    updateUi();
  }
  fitImageInView();
}

void BackgroundImageSetupDialog::updateUi(QString msg) noexcept {
  const bool valid = (!mImage.isNull()) && mImage.width() && mImage.height();
  if (valid &&
      ((mState == State::SelectRef1) || (mState == State::SelectRef2))) {
    mUi->graphicsView->setCursor(Qt::BlankCursor);
  } else if (valid && (mState == State::Crop)) {
    mUi->graphicsView->setCursor(Qt::CrossCursor);
  } else {
    mUi->graphicsView->unsetCursor();
  }

  // Update widgets & graphics items.
  mRotateWidget->setVisible(mState == State::Rotate);
  if (mRotateWidget->isVisible()) {
    mRotateWidget->move(mUi->graphicsView->rect().center() -
                        mRotateWidget->rect().center());
  }
  mImageGraphicsItem->setTransformOriginPoint(mImage.rect().center());
  mImageGraphicsItem->setRotation(-mRotation.toDeg());
  mImageGraphicsItem->setPos(-mImage.rect().center());
  mCursorGraphicsItem->setVisible((mState == State::SelectRef1) ||
                                  (mState == State::SelectRef2));
  while (mReferenceGraphicsItems.count() > mReferences.count()) {
    mReferenceGraphicsItems.takeLast();
    mReferenceLineGraphicsItems.takeLast();
    mReferenceWidgets.takeLast();
  }
  while (mReferenceGraphicsItems.count() < mReferences.count()) {
    auto c = createCrossGraphicsItem(false);
    mUi->graphicsView->scene()->addItem(c.get());
    mReferenceGraphicsItems.append(c);
    auto l = createRefLineGraphicsItem();
    mUi->graphicsView->scene()->addItem(l.get());
    mReferenceLineGraphicsItems.append(l);
    auto w = createReferenceWidget(
        mReferenceWidgets.count(), this, &mReferences,
        std::bind(&BackgroundImageSetupDialog::updateStatusMsg, this));
    mUi->widgetsLayout->addWidget(w.get());
    mReferenceWidgets.append(w);
  }
  for (int i = 0; i < mReferenceGraphicsItems.count(); ++i) {
    mReferenceGraphicsItems.at(i)->setPos(
        mImageGraphicsItem->mapToScene(mReferences.value(i).first));
  }
  mUi->lblCoordinates->setVisible(mReferences.count() > 0);
  QTimer::singleShot(10, this, &BackgroundImageSetupDialog::updateAnchors);

  // Show message if no image available to display.
  if (msg.isEmpty() && (!valid)) {
    msg = "<p>" %
        tr("This tool allows you to set a background image (typically a "
           "datasheet drawing) in the footprint editor to easily verify the "
           "size &amp; position of footprint pads etc. Note that the image "
           "won't appear on the board, it's only visible in the footprint "
           "editor.") %
        "</p>";
    msg += "<ol>";
    QStringList lines;
    lines.append(tr("Load an image with one of the buttons on the left side."));
    lines.append(
        tr("Draw a line around the footprint to cut out the relevant area."));
    lines.append(tr("Rotate/mirror the image."));
    lines.append(
        tr("Specify two reference points to calculate X/Y scale & offset."));
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

  // Update status text.
  updateStatusMsg();
}

void BackgroundImageSetupDialog::fitImageInView() noexcept {
  QTimer::singleShot(10, this, [this]() {
    mUi->graphicsView->setVisibleSceneRect(
        mImageGraphicsItem->mapToScene(mImageGraphicsItem->boundingRect())
            .boundingRect());
    updateAnchors();
  });
}

void BackgroundImageSetupDialog::updateAnchors() noexcept {
  for (int i = 0; i < mReferenceGraphicsItems.count(); ++i) {
    const QPoint widget = mReferenceWidgets.at(i)->geometry().center();
    const QPoint anchor(0, mUi->graphicsView->mapFrom(this, widget).y());
    const QPointF anchorScene = mUi->graphicsView->mapToScene(anchor);
    mReferenceLineGraphicsItems.at(i)->setLine(
        QLineF(anchorScene,
               mImageGraphicsItem->mapToScene(mReferences.value(i).first)));
  }
}

void BackgroundImageSetupDialog::updateStatusMsg() noexcept {
  QStringList lines;
  auto addStep = [&lines](int step) { lines.append(tr("Step %1:").arg(step)); };
  const QString note =
      tr("Note that the two points must be located diagonally to get a large "
         "distance in both X- and Y-direction.");

  if (mState == State::Crop) {
    addStep(1);
    lines.append(
        tr("Crop the image by drawing a line with the cursor around the "
           "footprint (single click to skip)."));
  } else if (mState == State::Rotate) {
    addStep(2);
    lines.append(tr(
        "Rotate/mirror the image to match the orientation of the footprint."));
  } else if (mState == State::SelectRef1) {
    addStep(3);
    lines.append(
        tr("Click into the image to select the first reference point with "
           "known X/Y coordinates."));
    lines.append(note);
  } else if (mState == State::SelectRef2) {
    addStep(4);
    lines.append(
        tr("Click into the image to select the second reference point with "
           "known X/Y coordinates."));
    lines.append(note);
  } else if (!mImage.isNull()) {
    QString err;
    if (mReferences.count() < 2) {
      err = tr("Too few reference points (2 required).");
    } else if (mReferences.last().second.isOrigin()) {
      addStep(5);
      lines.append(tr(
          "Specify the target coordinates for the chosen reference points."));
    } else {
      const int minPixels = std::min(mImage.width(), mImage.height()) / 5;
      const Length minLength(100000);
      const QPointF dPx = mReferences[1].first - mReferences[0].first;
      const Point dMm = mReferences[1].second - mReferences[0].second;
      const qreal scaleX = std::abs(dPx.x() / dMm.toMmQPointF().x());
      const qreal scaleY = std::abs(dPx.y() / dMm.toMmQPointF().y());
      if ((std::abs(dPx.x()) < minPixels) || (std::abs(dPx.y()) < minPixels) ||
          (dMm.getX().abs() < minLength) || (dMm.getY().abs() < minLength)) {
        err = tr(
            "There's not enough distance in either X- or Y direction. Choose "
            "reference points with a large distance in both directions.");
      } else if ((std::abs(scaleX - scaleY) / std::min(scaleX, scaleY)) > 0.5) {
        err = tr(
            "There is a high deviation between X- and Y scale factor. Please "
            "check the reference points.");
      }
    }
    if (!err.isEmpty()) {
      lines.append("<span style=\"color:red\">" % err % "</span>");
    }
  }

  QString s;
  for (const QString& l : lines) {
    s += "<p>" % l % "</p>";
  }
  mUi->lblStatus->setText(s);
}

QImage BackgroundImageSetupDialog::cropImage(const QImage& img,
                                             const QPainterPath& p) noexcept {
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
  return pixmap.copy(rect).toImage();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
