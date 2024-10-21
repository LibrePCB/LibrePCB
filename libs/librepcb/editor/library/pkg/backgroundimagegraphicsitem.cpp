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
#include "backgroundimagegraphicsitem.h"

#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/serialization/sexpression.h>

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

BackgroundImageGraphicsItem::BackgroundImageGraphicsItem() noexcept
  : QGraphicsItem(nullptr), mSettings() {
  setZValue(-1000);
  setOpacity(0.8);
  setVisible(false);
}

BackgroundImageGraphicsItem::~BackgroundImageGraphicsItem() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BackgroundImageGraphicsItem::setSettings(
    const BackgroundImageSettings& settings) noexcept {
  mSettings = settings;
  mImage= settings.image;
  const bool enable = mSettings.enabled && (!mSettings.image.isNull());
  setVisible(enable);
  if (enable) {
    mImage = mImage.convertToFormat(QImage::Format_ARGB32);

    // If the image background color is the inverse of the graphics view
    // background, invert the image to get good contrast for lines in the image.
    if (std::abs(mImage.pixelColor(0, 0).lightnessF() -
                 mBackgroundColor.lightnessF()) > 0.5) {
      mImage.invertPixels();
    }

    // Make the image background transparent.
    const QColor imgBgColor = mImage.pixelColor(0, 0);
    for (int i = 0; i < mImage.width(); ++i) {
      for (int k = 0; k < mImage.height(); ++k) {
        if (mImage.pixelColor(i, k) == imgBgColor) {
          mImage.setPixelColor(i, k, Qt::transparent);
        }
      }
    }

    // Apply the transform.
    const qreal scaleFactorX = Length(25400000).toPx() / mSettings.dpi.first;
    const qreal scaleFactorY = Length(25400000).toPx() / mSettings.dpi.second;
    QTransform t;
    t.rotate(-mSettings.rotation.toDeg());
    t.translate(-mSettings.referencePos.x() * scaleFactorX,
                -mSettings.referencePos.y() * scaleFactorY);
    t.scale(scaleFactorX, scaleFactorY);
    setTransform(t);
    setPos(mSettings.position.toPxQPointF());
  }
  update();
}

QRectF BackgroundImageGraphicsItem::boundingRect() const noexcept {
  return mImage.rect();
}

QPainterPath BackgroundImageGraphicsItem::shape() const noexcept {
  return QPainterPath();
}

void BackgroundImageGraphicsItem::paint(QPainter* painter,
                                        const QStyleOptionGraphicsItem* option,
                                        QWidget* widget) noexcept {
  Q_UNUSED(option);
  Q_UNUSED(widget);
  painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
  painter->drawImage(0, 0, mImage);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
