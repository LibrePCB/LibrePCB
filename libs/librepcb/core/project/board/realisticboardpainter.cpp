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
#include "realisticboardpainter.h"

#include "../../3d/scenedata3d.h"
#include "../../types/pcbcolor.h"
#include "../../utils/clipperhelpers.h"
#include "../../utils/transform.h"
#include "../../workspace/theme.h"

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

RealisticBoardPainter::RealisticBoardPainter(std::shared_ptr<SceneData3D> data)
  : GraphicsPagePainter(),
    mMaxArcTolerance(5000),
    mData(data),
    mDataPreprocessed(false) {
  Q_ASSERT(data);
}

RealisticBoardPainter::~RealisticBoardPainter() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void RealisticBoardPainter::paint(
    QPainter& painter, const GraphicsExportSettings& settings) const noexcept {
  const QVector<std::pair<QColor, QPainterPath>> content = getContent(settings);

  painter.setPen(Qt::NoPen);
  for (const auto& pair : content) {
    painter.setBrush(pair.first);
    painter.drawPath(pair.second);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

QVector<std::pair<QColor, QPainterPath>> RealisticBoardPainter::getContent(
    const GraphicsExportSettings& settings) const noexcept {
  QMutexLocker lock(&mMutex);
  QVector<std::pair<QColor, QPainterPath>> content;

  try {
    if (!mDataPreprocessed) {
      mData->preprocess(false);
    }

    auto getPaths = [this](const QStringList layers) {
      ClipperLib::Paths paths;
      foreach (const auto& area, mData->getAreas()) {
        if (layers.contains(area.layer->getId())) {
          paths.push_back(
              ClipperHelpers::convert(area.outline, mMaxArcTolerance));
        }
      }
      return paths;
    };

    auto toPainterPath = [](const ClipperLib::Paths& paths) {
      QPainterPath p;
      p.setFillRule(Qt::OddEvenFill);
      for (const auto& path : paths) {
        p.addPath(ClipperHelpers::convert(path).toQPainterPathPx());
      }
      return p;
    };

    QColor color;
    QStringList layers;
    ClipperLib::Paths paths;

    // Holes/cutouts.
    if ((!mCachedHoles) || (!mCachedCopperHoles)) {
      layers = {Layer::boardCutouts().getId(),
                Layer::boardPlatedCutouts().getId()};
      mCachedHoles = getPaths(layers);
      mCachedCopperHoles = ClipperLib::Paths();
      for (auto& hole : mData->getHoles()) {
        paths = ClipperHelpers::convert(
            hole.path->toOutlineStrokes(hole.diameter), mMaxArcTolerance);
        if (hole.copperLayer &&
            (hole.copperLayer->isBottom() == settings.getMirror())) {
          mCachedCopperHoles->insert(mCachedCopperHoles->end(), paths.begin(),
                                     paths.end());
        } else {
          mCachedHoles->insert(mCachedHoles->end(), paths.begin(), paths.end());
        }
      }
      ClipperHelpers::unite(*mCachedHoles, {}, ClipperLib::pftNonZero,
                            ClipperLib::pftNonZero);
    }

    // Board outlines/area.
    if ((!mCachedBoardOutlines) || (!mCachedBoardArea)) {
      layers = QStringList{Layer::boardOutlines().getId()};
      mCachedBoardOutlines = getPaths(layers);
      mCachedBoardArea = *mCachedBoardOutlines;
      ClipperHelpers::subtract(*mCachedBoardArea, *mCachedHoles,
                               ClipperLib::pftNonZero, ClipperLib::pftNonZero);
    }

    // Solder resist.
    auto getSolderResistPaths = [this, &layers, getPaths](bool bottom) {
      auto& paths = bottom ? mCachedSolderResistBot : mCachedSolderResistTop;
      if (!paths) {
        layers = QStringList{bottom ? Layer::botStopMask().getId()
                                    : Layer::topStopMask().getId(),
                             Layer::boardCutouts().getId(),
                             Layer::boardPlatedCutouts().getId()};
        paths = *mCachedBoardOutlines;
        ClipperHelpers::subtract(*paths, getPaths(layers),
                                 ClipperLib::pftEvenOdd,
                                 ClipperLib::pftNonZero);
      }
      return *paths;
    };

    // Board body.
    color = settings.getColor(Theme::Color::sBoardOutlines);
    if (color.isValid() && (color.alpha() > 0)) {
      auto it = mCachedContentPerLayer.find(Theme::Color::sBoardOutlines);
      if (it == mCachedContentPerLayer.end()) {
        it = mCachedContentPerLayer.insert(Theme::Color::sBoardOutlines,
                                           toPainterPath(*mCachedBoardArea));
      }
      content.append(std::make_pair(color, *it));
    }

    // Copper.
    // Note: Theme::getCopperColorNames() returns the layers in
    // non-deterministic order, which is not good as it leads to
    // non-deterministic paint order. However, this is the most efficient way
    // and is most probably good enough as usually all copper layers have the
    // same color anyway and usually even only one layer is enabled.
    for (const QString& themeColor : Theme::getCopperColorNames()) {
      color = settings.getColor(themeColor);
      if (color.isValid() && (color.alpha() > 0)) {
        auto it = mCachedContentPerLayer.find(themeColor);
        if (it == mCachedContentPerLayer.end()) {
          layers = QStringList{(themeColor == Theme::Color::sBoardCopperTop)
                                   ? Layer::topCopper().getId()
                                   : Layer::botCopper().getId()};
          paths = *mCachedBoardArea;
          if (!mCachedCopperHoles->empty()) {
            ClipperHelpers::subtract(paths, *mCachedCopperHoles,
                                     ClipperLib::pftEvenOdd,
                                     ClipperLib::pftNonZero);
          }
          ClipperHelpers::intersect(paths, getPaths(layers),
                                    ClipperLib::pftEvenOdd,
                                    ClipperLib::pftNonZero);
          it = mCachedContentPerLayer.insert(themeColor, toPainterPath(paths));
        }
        content.append(std::make_pair(color, *it));
      }
    }

    // Solder resist.
    for (const char* themeColor :
         {Theme::Color::sBoardStopMaskTop, Theme::Color::sBoardStopMaskBot}) {
      color = settings.getColor(themeColor);
      if (color.isValid() && (color.alpha() == 0)) {
        if (auto boardColor = mData->getSolderResist()) {
          color = boardColor->toSolderResistColor();
        }
      }
      if (color.isValid() && (color.alpha() > 0)) {
        auto it = mCachedContentPerLayer.find(themeColor);
        if (it == mCachedContentPerLayer.end()) {
          it = mCachedContentPerLayer.insert(
              themeColor,
              toPainterPath(getSolderResistPaths(
                  themeColor == Theme::Color::sBoardStopMaskBot)));
        }
        content.append(std::make_pair(color, *it));
      }
    }

    // Silkscreen.
    for (const char* themeColor :
         {Theme::Color::sBoardLegendTop, Theme::Color::sBoardLegendBot}) {
      color = settings.getColor(themeColor);
      if (color.isValid() && (color.alpha() == 0)) {
        if (auto boardColor = mData->getSilkscreen()) {
          color = boardColor->toSilkscreenColor();
        }
      }
      if (color.isValid() && (color.alpha() > 0)) {
        auto it = mCachedContentPerLayer.find(themeColor);
        if (it == mCachedContentPerLayer.end()) {
          const bool isBottom = themeColor == Theme::Color::sBoardLegendBot;
          layers = QStringList();
          foreach (const Layer* layer,
                   isBottom ? mData->getSilkscreenLayersBot()
                            : mData->getSilkscreenLayersTop()) {
            layers.append(layer->getId());
          }
          paths = getPaths(layers);
          ClipperHelpers::intersect(paths, getSolderResistPaths(isBottom),
                                    ClipperLib::pftNonZero,
                                    ClipperLib::pftEvenOdd);
          it = mCachedContentPerLayer.insert(themeColor, toPainterPath(paths));
        }
        content.append(std::make_pair(color, *it));
      }
    }

    // Solder paste.
    for (const char* themeColor : {Theme::Color::sBoardSolderPasteTop,
                                   Theme::Color::sBoardSolderPasteBot}) {
      color = settings.getColor(themeColor);
      if (color.isValid() && (color.alpha() > 0)) {
        auto it = mCachedContentPerLayer.find(themeColor);
        if (it == mCachedContentPerLayer.end()) {
          layers =
              QStringList{(themeColor == Theme::Color::sBoardSolderPasteTop)
                              ? Layer::topSolderPaste().getId()
                              : Layer::botSolderPaste().getId()};
          paths = getPaths(layers);
          ClipperHelpers::intersect(paths, *mCachedBoardArea,
                                    ClipperLib::pftNonZero,
                                    ClipperLib::pftEvenOdd);
          it = mCachedContentPerLayer.insert(themeColor, toPainterPath(paths));
        }
        content.append(std::make_pair(settings.getColor(themeColor), *it));
      }
    }
  } catch (const Exception& e) {
    qCritical() << "Failed to export realistic board graphics:" << e.getMsg();
  }
  return content;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
