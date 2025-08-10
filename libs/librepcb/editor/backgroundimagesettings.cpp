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
#include "backgroundimagesettings.h"

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

namespace editor {

/*******************************************************************************
 *  General Methods
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

  // Get the images background color. This could be improved a lot :-/
  QColor imgBgColor = img.pixelColor(0, 0);

  // Detect if the images background is either white or black.
  const int blackThreshold = 30;
  const int whiteThreshold = 255 - 30;
  const bool imgBgIsBlack = (imgBgColor.red() <= blackThreshold) &&
      (imgBgColor.green() <= blackThreshold) &&
      (imgBgColor.blue() <= blackThreshold);
  const bool imgBgIsWhite = (imgBgColor.red() >= whiteThreshold) &&
      (imgBgColor.green() >= whiteThreshold) &&
      (imgBgColor.blue() >= whiteThreshold);

  // If the images background is either white or black, make it transparent.
  // This is important for datasheet drawings to get only the drawing lines,
  // not the PDF background. However, for images neither white or black, we
  // don't do this since it is probably a photo of a PCB where we must not
  // remove the background as it might also remove the traces.
  if (imgBgIsBlack || imgBgIsWhite) {
    auto colorDiff = [](const QColor& a, const QColor& b) {
      return std::abs(a.lightnessF() - b.lightnessF());
    };

    // If the image background color is the inverse of the graphics view
    // background, invert the image to get good contrast for lines in the image.
    if (colorDiff(imgBgColor, bgColor) > 0.5) {
      img.invertPixels();
    }

    // Make the image background transparent.
    imgBgColor = img.pixelColor(0, 0);  // Might have been inverted!
    for (int i = 0; i < img.width(); ++i) {
      for (int k = 0; k < img.height(); ++k) {
        if (colorDiff(img.pixelColor(i, k), imgBgColor) < 0.3) {
          img.setPixelColor(i, k, Qt::transparent);
        }
      }
    }
  }

  return QPixmap::fromImage(img);
}

QTransform BackgroundImageSettings::calcTransform() const noexcept {
  QTransform t;
  t.rotate(-rotation.toDeg());
  if (references.count() >= 2) {
    const Point deltaPx =
        Point::fromPx(references.at(1).first - references.at(0).first)
            .rotated(-rotation);
    const Point deltaMm = (references.at(1).second - references.at(0).second);

    const qreal scaleFactorX =
        std::abs(deltaMm.toMmQPointF().x() / deltaPx.toMmQPointF().x());
    const qreal scaleFactorY =
        std::abs(deltaMm.toMmQPointF().y() / deltaPx.toMmQPointF().y());

    t.scale(scaleFactorX, scaleFactorY);
    t.translate(-references.first().first.x(), -references.first().first.y());
  }
  return t;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
