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
    mDrawSolderPaste(false),
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
  const Content& content = getContent(settings.getMirror());

  painter.setPen(Qt::NoPen);

  // Body.
  painter.setBrush(QColor(70, 80, 70));
  painter.drawPath(content.body);

  // Copper.
  painter.setBrush(QColor(188, 156, 105));
  painter.drawPath(content.copper);

  // Solder resist.
  if (auto color = mData->getSolderResist()) {
    painter.setBrush(color->toSolderResistColor());
    painter.drawPath(content.solderResist);
  }

  // Silkscreen.
  if (auto color = mData->getSilkscreen()) {
    painter.setBrush(color->toSilkscreenColor());
    painter.drawPath(content.silkscreen);
  }

  // Solder paste.
  if (mDrawSolderPaste) {
    painter.setBrush(Qt::darkGray);
    painter.drawPath(content.solderPaste);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

const RealisticBoardPainter::Content& RealisticBoardPainter::getContent(
    bool mirrored) const noexcept {
  QMutexLocker lock(&mMutex);
  RealisticBoardPainter::Content& content =
      mirrored ? mContentBot : mContentTop;

  try {
    if (!mDataPreprocessed) {
      mData->preprocess(false);
    }

    if (!content.initialized) {
      const Transform transform(Point(), Angle(), mirrored);

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
        foreach (const auto& path, paths) {
          p.addPath(ClipperHelpers::convert(path).toQPainterPathPx());
        }
        return p;
      };

      // Holes/cutouts.
      QStringList layers = {Layer::boardCutouts().getId(),
                            Layer::boardPlatedCutouts().getId()};
      ClipperLib::Paths holes = getPaths(layers);
      ClipperLib::Paths copperHoles;
      for (auto& hole : mData->getHoles()) {
        const auto paths = ClipperHelpers::convert(
            hole.path->toOutlineStrokes(hole.diameter), mMaxArcTolerance);
        if (hole.copperLayer && (hole.copperLayer->isBottom() == mirrored)) {
          copperHoles.insert(copperHoles.end(), paths.begin(), paths.end());
        } else {
          holes.insert(holes.end(), paths.begin(), paths.end());
        }
      }
      ClipperHelpers::unite(holes, {}, ClipperLib::pftNonZero,
                            ClipperLib::pftNonZero);

      // Board body.
      layers = QStringList{Layer::boardOutlines().getId()};
      ClipperLib::Paths boardOutlines = getPaths(layers);
      ClipperLib::Paths boardArea = boardOutlines;
      ClipperHelpers::subtract(boardArea, holes, ClipperLib::pftNonZero,
                               ClipperLib::pftNonZero);
      content.body = toPainterPath(boardArea);

      // Copper.
      layers = QStringList{transform.map(Layer::topCopper()).getId()};
      ClipperLib::Paths paths = boardArea;
      if (!copperHoles.empty()) {
        ClipperHelpers::subtract(paths, copperHoles, ClipperLib::pftEvenOdd,
                                 ClipperLib::pftNonZero);
      }
      ClipperHelpers::intersect(paths, getPaths(layers), ClipperLib::pftEvenOdd,
                                ClipperLib::pftNonZero);
      content.copper = toPainterPath(paths);

      // Solder resist.
      ClipperLib::Paths solderResist;
      if (mData->getSolderResist()) {
        layers = QStringList{transform.map(Layer::topStopMask()).getId(),
                             Layer::boardCutouts().getId(),
                             Layer::boardPlatedCutouts().getId()};
        solderResist = boardOutlines;
        ClipperHelpers::subtract(solderResist, getPaths(layers),
                                 ClipperLib::pftEvenOdd,
                                 ClipperLib::pftNonZero);
        content.solderResist = toPainterPath(solderResist);
      }

      // Silkscreen.
      if (mData->getSilkscreen()) {
        layers = QStringList();
        foreach (const Layer* layer,
                 mirrored ? mData->getSilkscreenLayersBot()
                          : mData->getSilkscreenLayersTop()) {
          layers.append(layer->getId());
        }
        paths = getPaths(layers);
        ClipperHelpers::intersect(paths, solderResist, ClipperLib::pftNonZero,
                                  ClipperLib::pftEvenOdd);
        content.silkscreen = toPainterPath(paths);
      }

      // Solder paste.
      if (mDrawSolderPaste) {
        layers = QStringList{transform.map(Layer::topSolderPaste()).getId()};
        paths = getPaths(layers);
        ClipperHelpers::intersect(paths, boardArea, ClipperLib::pftNonZero,
                                  ClipperLib::pftEvenOdd);
        content.solderPaste = toPainterPath(paths);
      }

      content.initialized = true;
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
