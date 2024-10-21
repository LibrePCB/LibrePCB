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
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BackgroundImageSetupDialog::BackgroundImageSetupDialog(
    const QString& settingsPrefix, QWidget* parent) noexcept
  : QDialog(parent),
    mUi(new Ui::BackgroundImageSetupDialog),
    mSettingsPrefix(settingsPrefix % "/background_image_dialog"),
    mImageGraphicsItem(new QGraphicsPixmapItem()),
    mCropGraphicsItem(new QGraphicsPathItem()) {
  mUi->setupUi(this);
  mUi->graphicsView->setOriginCrossVisible(false);
  mUi->graphicsView->setBackgroundColors(Qt::transparent, Qt::transparent);
  mUi->graphicsView->setScene(new GraphicsScene(this));
  mUi->graphicsView->setEventHandlerObject(this);
  mUi->graphicsView->scene()->addItem(mImageGraphicsItem.get());
  mUi->graphicsView->scene()->addItem(mCropGraphicsItem.get());
  mImageGraphicsItem->setTransformationMode(Qt::SmoothTransformation);
  mCropGraphicsItem->setPen(QPen(Qt::blue, 0));

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
    mCropGraphicsItem->setPath(QPainterPath());
    updateUi();
  });

  // Load initial values and window geometry.
  QSettings cs;
  restoreGeometry(cs.value(mSettingsPrefix % "/window_geometry").toByteArray());

  updateUi();
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

void BackgroundImageSetupDialog::setImage(const QImage& image) noexcept {
  mImage = image;
  updateUi();
  QTimer::singleShot(10, this, &BackgroundImageSetupDialog::fitImageInView);
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
    QPainterPath p;
    p.moveTo(e->scenePos());
    mCropGraphicsItem->setPath(p);
  } else if (event->type() == QEvent::GraphicsSceneMouseRelease) {
    QGraphicsSceneMouseEvent* e = static_cast<QGraphicsSceneMouseEvent*>(event);
    if (e->button() != Qt::LeftButton) return false;
    QPainterPath path = mCropGraphicsItem->path();
    mCropGraphicsItem->setPath(QPainterPath());
    path.closeSubpath();
    path.translate(-mImageGraphicsItem->pos());
    if (path.elementCount() > 10) {
      mImage = cropImage(mImage, path);
      updateUi();
      fitImageInView();
    }
  } else if (event->type() == QEvent::GraphicsSceneMouseMove) {
    QGraphicsSceneMouseEvent* e = static_cast<QGraphicsSceneMouseEvent*>(event);
    if (mCropGraphicsItem->path().elementCount() > 0) {
      QPainterPath p = mCropGraphicsItem->path();
      p.lineTo(e->scenePos());
      mCropGraphicsItem->setPath(p);
    }
  }
  return false;
}

void BackgroundImageSetupDialog::startScreenshot() noexcept {
  mImage = QImage();

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

  mImage = mScreen->grabWindow(0).toImage();
  mScreen = nullptr;
  if (mImage.isNull()) {
    updateUi(
        tr("Could not take a screenshot. Note that this feature does not "
           "work on some systems due to security mechanisms."));
  } else {
    updateUi();
  }
  fitImageInView();
}

void BackgroundImageSetupDialog::pasteFromClipboard() noexcept {
  mImage = qApp->clipboard()->image();
  updateUi(mImage.isNull() ? tr("No image found in the clipboard.")
                           : QString());
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
  fp = FileDialog::getOpenFileName(parentWidget(), tr("Choose image"), fp,
                                   extensionsStr);
  if (fp.isEmpty()) {
    return;  // Aborted.
  }
  cs.setValue(key, fp);
  mImage = QImage();
  if (!mImage.load(fp)) {
    updateUi(tr("Failed to open the selected image file."));
  } else {
    updateUi();
  }
  fitImageInView();
}

void BackgroundImageSetupDialog::updateUi(QString msg) noexcept {
  const bool valid = (!mImage.isNull()) && mImage.width() && mImage.height();
  if (valid) {
    mUi->graphicsView->setCursor(Qt::CrossCursor);
  } else {
    mUi->graphicsView->unsetCursor();
  }

  // Show message if no image available to display.
  if (msg.isEmpty() && (!valid)) {
    msg = "<p>" %
        tr("This tool allows you to set a background image in the footprint "
           "editor to easily verify the size &amp; position of footprint pads "
           "etc. Typically a screenshot of the package drawing from the part's "
           "datasheet may be used as background.") %
        "</p>";
    msg += "<ol>";
    QStringList lines;
    lines.append(tr("Load an image with one of the buttons above."));
    lines.append(
        tr("Draw a line around the footprint to cut out the relevant area."));
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
    mImageGraphicsItem->setPos(-mImage.rect().center());
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

  // Update status bar text.
  if (valid) {
    mUi->lblStatusBar->setText(
        tr("Crop the image by drawing a line with the cursor around the "
           "footprint"));
  } else {
    mUi->lblStatusBar->clear();
  }
}

void BackgroundImageSetupDialog::fitImageInView() noexcept {
  mUi->graphicsView->setVisibleSceneRect(
      mUi->graphicsView->scene()->itemsBoundingRect());
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
