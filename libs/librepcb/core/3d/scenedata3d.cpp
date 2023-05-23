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
#include "scenedata3d.h"

#include "../types/layer.h"
#include "../types/pcbcolor.h"
#include "../utils/toolbox.h"

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SceneData3D::SceneData3D(std::shared_ptr<FileSystem> fs,
                         bool autoBoardOutline) noexcept
  : mFileSystem(fs),
    mThickness(1600000),
    mSolderResist(&PcbColor::green()),
    mSilkscreen(&PcbColor::white()),
    mSilkscreenLayersTop({&Layer::topPlacement(), &Layer::topNames()}),
    mSilkscreenLayersBot({&Layer::botPlacement(), &Layer::botNames()}),
    mAutoBoardOutline(autoBoardOutline),
    mStepAlphaValue(1),
    mProjectName("LibrePCB Project") {
}

SceneData3D::~SceneData3D() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SceneData3D::addDevice(const Uuid& uuid, const Transform& transform,
                            const QString& stepFile,
                            const Point3D& stepPosition,
                            const Angle3D& stepRotation,
                            const QString& name) noexcept {
  mDevices.append(
      DeviceData{uuid, transform, stepFile, stepPosition, stepRotation, name});
}

void SceneData3D::addPolygon(const Polygon& polygon,
                             const Transform& transform) noexcept {
  mPolygons.append(PolygonData{polygon, transform});
}

void SceneData3D::addCircle(const Circle& circle,
                            const Transform& transform) noexcept {
  mCircles.append(CircleData{circle, transform});
}

void SceneData3D::addStroke(const Layer& layer, const QVector<Path>& paths,
                            const Length& width,
                            const Transform& transform) noexcept {
  mStrokes.append(StrokeData{&layer, paths, width, transform});
}

void SceneData3D::addVia(
    const Point& position, const PositiveLength& size,
    const PositiveLength& drillDiameter, const Layer& startLayer,
    const Layer& endLayer,
    const tl::optional<PositiveLength>& stopMaskDiameterTop,
    const tl::optional<PositiveLength>& stopMaskDiameterBottom) noexcept {
  mVias.append(ViaData{position, size, drillDiameter, &startLayer, &endLayer,
                       stopMaskDiameterTop, stopMaskDiameterBottom});
}

void SceneData3D::addHole(const NonEmptyPath& path,
                          const PositiveLength& diameter, bool plated, bool via,
                          const Transform& transform) noexcept {
  mHoles.append(HoleData{path, diameter, plated, via, nullptr, transform});
}

void SceneData3D::addArea(const Layer& layer, const Path& outline,
                          const Transform& transform) noexcept {
  mAreas.append(AreaData{&layer, outline, transform});
}

void SceneData3D::preprocess(bool center, bool sortDevices, Length* width,
                             Length* height) {
  // Sort devices by name for cleaner structure in the MCAD.
  if (sortDevices) {
    Toolbox::sortNumeric(
        mDevices,
        [](const QCollator& cmp, const DeviceData& a, const DeviceData& b) {
          return cmp(a.name, b.name);
        });
  }

  // Perform hole transformations.
  for (auto& hole : mHoles) {
    hole.path = hole.transform.map(hole.path);
    hole.transform = Transform();
  }

  // Perform area transformations.
  for (auto& area : mAreas) {
    area.outline = area.transform.map(area.outline);
    area.transform = Transform();
  }

  // Convert polygons to areas.
  foreach (const auto& obj, mPolygons) {
    const Layer& layer = obj.transform.map(obj.polygon.getLayer());
    const Path path = obj.transform.map(obj.polygon.getPath());
    const bool isOutline = (layer.getId() == Layer::boardOutlines().getId());
    if ((!isOutline) && (obj.polygon.getLineWidth() > 0)) {
      foreach (
          const Path& outline,
          path.toOutlineStrokes(PositiveLength(*obj.polygon.getLineWidth()))) {
        mAreas.append(AreaData{&layer, outline, Transform()});
      }
    }
    if ((isOutline || obj.polygon.isFilled()) &&
        obj.polygon.getPath().isClosed()) {
      mAreas.append(AreaData{&layer, path, Transform()});
    }
  }
  mPolygons.clear();

  // Convert circles to areas.
  foreach (const auto& obj, mCircles) {
    const Layer& layer = obj.transform.map(obj.circle.getLayer());
    const Point center = obj.transform.map(obj.circle.getCenter());
    const Path path = Path::circle(obj.circle.getDiameter()).translated(center);
    const bool isOutline = (layer.getId() == Layer::boardOutlines().getId());
    if ((!isOutline) && (obj.circle.getLineWidth() > 0)) {
      foreach (
          const Path& outline,
          path.toOutlineStrokes(PositiveLength(*obj.circle.getLineWidth()))) {
        mAreas.append(AreaData{&layer, outline, Transform()});
      }
    }
    if (isOutline || obj.circle.isFilled()) {
      mAreas.append(AreaData{&layer, path, Transform()});
    }
  }
  mCircles.clear();

  // Convert strokes to areas.
  foreach (const auto& obj, mStrokes) {
    Q_ASSERT(obj.layer);
    if (obj.width > 0) {
      foreach (const Path& stroke, obj.paths) {
        foreach (const Path& outline,
                 obj.transform.map(stroke).toOutlineStrokes(
                     PositiveLength(obj.width))) {
          mAreas.append(AreaData{obj.layer, outline, Transform()});
        }
      }
    }
  }
  mStrokes.clear();

  // Convert vias to holes & areas.
  foreach (const auto& obj, mVias) {
    Q_ASSERT(obj.startLayer && obj.endLayer);
    const bool onTop = (obj.startLayer == &Layer::topCopper());
    const bool onBottom = (obj.endLayer == &Layer::botCopper());
    // Copper area.
    const Path outline = Path::circle(obj.size).translated(obj.position);
    if (onTop) {
      mAreas.append(AreaData{&Layer::topCopper(), outline, Transform()});
    }
    if (onBottom) {
      mAreas.append(AreaData{&Layer::botCopper(), outline, Transform()});
    }
    // Stop mask area.
    auto stopMasks = {
        std::make_pair(&Layer::topStopMask(), obj.stopMaskDiameterTop),
        std::make_pair(&Layer::botStopMask(), obj.stopMaskDiameterBottom),
    };
    for (const auto& cfg : stopMasks) {
      if (const auto diameter = cfg.second) {
        const Path stopMaskOutline =
            Path::circle(*diameter).translated(obj.position);
        mAreas.append(AreaData{cfg.first, stopMaskOutline, Transform()});
      }
    }
    // Hole.
    if (onTop && onBottom) {
      mHoles.append(HoleData{makeNonEmptyPath(obj.position), obj.drillDiameter,
                             true, true, nullptr, Transform()});
    } else if (onTop) {
      mHoles.append(HoleData{makeNonEmptyPath(obj.position), obj.drillDiameter,
                             true, true, &Layer::topCopper(), Transform()});
    } else if (onBottom) {
      mHoles.append(HoleData{makeNonEmptyPath(obj.position), obj.drillDiameter,
                             true, true, &Layer::botCopper(), Transform()});
    }
  }
  mVias.clear();

  // Determine bounding rect of board.
  QPainterPath outlinesPx;
  for (auto& area : mAreas) {
    if (area.layer->getId() == Layer::boardOutlines().getId()) {
      outlinesPx |= area.outline.toQPainterPathPx();
    }
  }
  QRectF boundingRectPx = outlinesPx.boundingRect();

  // Auto-add board outline if there is none.
  if (outlinesPx.isEmpty() && mAutoBoardOutline) {
    for (auto& area : mAreas) {
      outlinesPx |= area.outline.toQPainterPathPx();
    }
    for (auto& hole : mHoles) {
      outlinesPx |= Path::toQPainterPathPx(
          hole.path->toOutlineStrokes(hole.diameter), true);
    }
    qreal ext = 0.1 *
        std::max(outlinesPx.boundingRect().width(),
                 outlinesPx.boundingRect().height());
    ext = qBound(Length(3000000).toPx(), ext, Length(20000000).toPx());
    boundingRectPx = outlinesPx.boundingRect().adjusted(-ext, -ext, ext, ext);
    mAreas.append(
        AreaData{&Layer::boardOutlines(),
                 Path::rect(Point::fromPx(boundingRectPx.topLeft()),
                            Point::fromPx(boundingRectPx.bottomRight())),
                 Transform()});
  }

  // Export board size, if needed.
  if (width) {
    *width = Length::fromPx(boundingRectPx.width());
  }
  if (height) {
    *height = Length::fromPx(boundingRectPx.height());
  }

  // Move all objects to new center.
  if (center) {
    const Point centerPos = Point::fromPx(boundingRectPx.center());
    for (auto& device : mDevices) {
      device.transform.setPosition(device.transform.getPosition() - centerPos);
    }
    for (auto& hole : mHoles) {
      hole.path = NonEmptyPath(hole.path->translated(-centerPos));
    }
    for (auto& area : mAreas) {
      area.outline.translate(-centerPos);
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
