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

#ifndef LIBREPCB_CORE_BOARDDESIGNRULECHECKDATA_H
#define LIBREPCB_CORE_BOARDDESIGNRULECHECKDATA_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../../geometry/padgeometry.h"
#include "../../../geometry/zone.h"
#include "../../../types/uuid.h"
#include "boarddesignrulechecksettings.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Board;
class Layer;

/*******************************************************************************
 *  Class BoardDesignRuleCheckData
 ******************************************************************************/

/**
 * @brief Input data structure for ::librepcb::BoardDesignRuleCheck
 */
struct BoardDesignRuleCheckData final {
  struct Junction {
    Uuid uuid;
    Point position;
    qsizetype traces;
  };
  struct Trace {
    Uuid uuid;
    Point startPosition;
    Point endPosition;
    PositiveLength width;
    const Layer* layer;
  };
  struct Via {
    Uuid uuid;
    Point position;
    PositiveLength size;
    PositiveLength drillDiameter;
    const Layer* startLayer;
    const Layer* endLayer;
    std::optional<std::pair<const Layer*, const Layer*>> drillLayerSpan;
    bool isBuried;
    bool isBlind;
    std::optional<PositiveLength> stopMaskDiameterTop;
    std::optional<PositiveLength> stopMaskDiameterBot;
  };
  struct Segment {
    Uuid uuid;
    std::optional<Uuid> net;
    QString netName;  // Empty if no net.
    QHash<Uuid, Junction> junctions;
    QList<Trace> traces;
    QHash<Uuid, Via> vias;
  };
  struct AirWireAnchor {
    Point position;
    std::optional<Uuid> device;  // If it's a pad.
    std::optional<Uuid> pad;  // If it's a pad.
    std::optional<Uuid> segment;  // If it's a junction or via.
    std::optional<Uuid> junction;  // If it's a junction.
    std::optional<Uuid> via;  // If it's a via.
  };
  struct AirWire {
    AirWireAnchor p1;
    AirWireAnchor p2;
    QString netName;  // Always valid.
  };
  struct Plane {
    Uuid uuid;
    std::optional<Uuid> net;
    QString netName;  // Empty if no net.
    const Layer* layer;
    UnsignedLength minWidth;
    Path outline;
    QVector<Path> fragments;
  };
  struct Polygon {
    Uuid uuid;
    const Layer* layer;
    UnsignedLength lineWidth;
    bool filled;
    Path path;
  };
  struct Circle {
    Uuid uuid;
    Point center;
    PositiveLength diameter;
    const Layer* layer;
    UnsignedLength lineWidth;
    bool filled;
  };
  struct StrokeText {
    Uuid uuid;
    Point position;
    Angle rotation;
    bool mirror;
    const Layer* layer;
    UnsignedLength strokeWidth;
    PositiveLength height;
    QVector<Path> paths;
  };
  struct Hole {
    Uuid uuid;
    PositiveLength diameter;
    NonEmptyPath path;
    std::optional<Length> stopMaskOffset;
  };
  struct Zone {
    Uuid uuid;
    QSet<const Layer*> boardLayers;  // Only set for board zones!
    librepcb::Zone::Layers footprintLayers;  // Only set for device zones!
    librepcb::Zone::Rules rules;
    Path outline;
  };
  struct Pad {
    Uuid uuid;
    QString libPkgPadName;  // Empty if not connected to a package pad.
    Point position;  // Absolute transform.
    Angle rotation;  // Absolute transform.
    bool mirror;  // Absolute transform.
    QList<Hole> holes;
    QHash<const Layer*, QList<PadGeometry>> geometries;
    QSet<const Layer*> layersWithTraces;  // Layers where traces are connected.
    UnsignedLength copperClearance;
    std::optional<Uuid> net;
    QString netName;  // Empty if no net.
  };
  struct Device {
    Uuid uuid;
    QString cmpInstanceName;
    Point position;
    Angle rotation;
    bool mirror;
    QHash<Uuid, Pad> pads;  // With absolute transform.
    QList<Polygon> polygons;  // From library footprint.
    QList<Circle> circles;  // From library footprint.
    QList<StrokeText> strokeTexts;  // With absolute transform.
    QList<Hole> holes;  // From library footprint.
    QList<Zone> zones;  // From library footprint.
  };

  // NOTE: We create a `const` copy of this structure for each thread to
  // ensure thread-safety. For the implicitly shared Qt containers this is
  // a lightweight operation.
  BoardDesignRuleCheckSettings settings;
  bool quick = false;
  QSet<const Layer*> copperLayers;  // All board copper layers.
  QVector<const Layer*> silkscreenLayersTop;
  QVector<const Layer*> silkscreenLayersBot;
  QHash<Uuid, Segment> segments;
  QList<Plane> planes;
  QList<Polygon> polygons;
  QList<StrokeText> strokeTexts;
  QList<Hole> holes;
  QList<Zone> zones;
  QHash<Uuid, Device> devices;
  QList<AirWire> airWires;
  QMap<Uuid, QString> unplacedComponents;  // UUID and name.

  // Constructors / Destructor
  BoardDesignRuleCheckData(const Board& board,
                           const BoardDesignRuleCheckSettings& drcSettings,
                           bool quickCheck) noexcept;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
