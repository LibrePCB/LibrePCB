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

#ifndef LIBREPCB_CORE_SCENEDATA3D_H
#define LIBREPCB_CORE_SCENEDATA3D_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../geometry/circle.h"
#include "../geometry/path.h"
#include "../geometry/polygon.h"
#include "../utils/transform.h"

#include <QtCore>

#include <memory>
#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class FileSystem;
class Layer;
class PcbColor;

/*******************************************************************************
 *  Class SceneData3D
 ******************************************************************************/

/**
 * @brief 3D scene data representing a board with package models
 */
class SceneData3D final {
  Q_DECLARE_TR_FUNCTIONS(SceneData3D)

public:
  // Types
  struct DeviceData {
    Uuid uuid;
    Transform transform;
    QString stepFile;
    Point3D stepPosition;
    Angle3D stepRotation;
    QString name;
  };

  struct PolygonData {
    Polygon polygon;
    Transform transform;
  };

  struct CircleData {
    Circle circle;
    Transform transform;
  };

  struct StrokeData {
    const Layer* layer;
    QVector<Path> paths;
    Length width;
    Transform transform;
  };

  struct ViaData {
    Point position;
    PositiveLength drillDiameter;
    PositiveLength size;
    const Layer* startLayer;
    const Layer* endLayer;
    std::optional<PositiveLength> stopMaskDiameterTop;
    std::optional<PositiveLength> stopMaskDiameterBottom;
  };

  struct HoleData {
    NonEmptyPath path;
    PositiveLength diameter;
    bool plated;
    bool via;
    const Layer* copperLayer;  ///< `nullptr` for through-hole.
    Transform transform;  ///< Reset by #preprocess().
  };

  struct AreaData {
    const Layer* layer;
    Path outline;
    Transform transform;  ///< Reset by #preprocess().
  };

  // Constructors / Destructor
  explicit SceneData3D(std::shared_ptr<FileSystem> fs = nullptr,
                       bool autoBoardOutline = false) noexcept;
  SceneData3D(const SceneData3D& other) = delete;
  ~SceneData3D() noexcept;

  // Getters
  const std::shared_ptr<FileSystem>& getFileSystem() const noexcept {
    return mFileSystem;
  }
  const PositiveLength& getThickness() const noexcept { return mThickness; }
  const PcbColor* getSolderResist() const noexcept { return mSolderResist; }
  const PcbColor* getSilkscreen() const noexcept { return mSilkscreen; }
  const QSet<const Layer*>& getSilkscreenLayersTop() const noexcept {
    return mSilkscreenLayersTop;
  }
  const QSet<const Layer*>& getSilkscreenLayersBot() const noexcept {
    return mSilkscreenLayersBot;
  }
  bool getAutoBoardOutline() const noexcept { return mAutoBoardOutline; }
  const QString& getProjectName() const noexcept { return mProjectName; }
  const QList<DeviceData>& getDevices() const noexcept { return mDevices; }
  const QList<PolygonData>& getPolygons() const noexcept { return mPolygons; }
  const QList<CircleData>& getCircles() const noexcept { return mCircles; }
  const QList<StrokeData>& getStrokes() const noexcept { return mStrokes; }
  const QList<ViaData>& getVias() const noexcept { return mVias; }
  const QList<HoleData>& getHoles() const noexcept { return mHoles; }
  const QList<AreaData>& getAreas() const noexcept { return mAreas; }

  // Setters
  void setThickness(const PositiveLength& value) noexcept {
    mThickness = value;
  }
  void setSolderResist(const PcbColor* value) noexcept {
    mSolderResist = value;
  }
  void setSilkscreen(const PcbColor* value) noexcept { mSilkscreen = value; }
  void setSilkscreenLayersTop(const QSet<const Layer*>& value) noexcept {
    mSilkscreenLayersTop = value;
  }
  void setSilkscreenLayersBot(const QSet<const Layer*>& value) noexcept {
    mSilkscreenLayersBot = value;
  }
  void setAutoBoardOutline(bool value) noexcept { mAutoBoardOutline = value; }
  void setProjectName(const QString& value) noexcept { mProjectName = value; }

  // General Methods
  void addDevice(const Uuid& uuid, const Transform& transform,
                 const QString& stepFile, const Point3D& stepPosition,
                 const Angle3D& stepRotation, const QString& name) noexcept;
  void addPolygon(const Polygon& polygon, const Transform& transform) noexcept;
  void addCircle(const Circle& circle, const Transform& transform) noexcept;
  void addStroke(const Layer& layer, const QVector<Path>& paths,
                 const Length& width, const Transform& transform) noexcept;
  void addVia(
      const Point& position, const PositiveLength& drillDiameter,
      const PositiveLength& size, const Layer& startLayer,
      const Layer& endLayer,
      const std::optional<PositiveLength>& stopMaskDiameterTop,
      const std::optional<PositiveLength>& stopMaskDiameterBottom) noexcept;
  void addHole(const NonEmptyPath& path, const PositiveLength& diameter,
               bool plated, bool via, const Transform& transform) noexcept;
  void addArea(const Layer& layer, const Path& outline,
               const Transform& transform) noexcept;
  void preprocess(bool center, bool sortDevices = false,
                  Length* width = nullptr, Length* height = nullptr);

  // Operator Overloadings
  SceneData3D& operator=(const SceneData3D& rhs) = delete;

private:  // Data
  std::shared_ptr<FileSystem> mFileSystem;
  PositiveLength mThickness;
  const PcbColor* mSolderResist;
  const PcbColor* mSilkscreen;
  QSet<const Layer*> mSilkscreenLayersTop;
  QSet<const Layer*> mSilkscreenLayersBot;
  bool mAutoBoardOutline;
  QString mProjectName;

  QList<DeviceData> mDevices;
  QList<PolygonData> mPolygons;  ///< Cleared by #preprocess().
  QList<CircleData> mCircles;  ///< Cleared by #preprocess().
  QList<StrokeData> mStrokes;  ///< Cleared by #preprocess().
  QList<ViaData> mVias;  /// Cleared by #preprocess().
  QList<HoleData> mHoles;
  QList<AreaData> mAreas;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
