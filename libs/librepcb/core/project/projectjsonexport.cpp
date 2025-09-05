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
#include "projectjsonexport.h"

#include "../library/pkg/footprint.h"
#include "../library/pkg/footprintpad.h"
#include "../types/layer.h"
#include "../types/pcbcolor.h"
#include "board/board.h"
#include "board/items/bi_device.h"
#include "board/items/bi_hole.h"
#include "board/items/bi_netsegment.h"
#include "board/items/bi_pad.h"
#include "board/items/bi_plane.h"
#include "board/items/bi_via.h"
#include "circuit/assemblyvariant.h"
#include "circuit/circuit.h"
#include "project.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ProjectJsonExport::ProjectJsonExport() noexcept {
}

ProjectJsonExport::~ProjectJsonExport() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

QJsonArray ProjectJsonExport::toJson(const QStringList& obj) const {
  return QJsonArray::fromStringList(obj);
}

QJsonValue ProjectJsonExport::toJson(const Length& obj) const {
  return QJsonValue(obj.toMm());
}

QJsonValue ProjectJsonExport::toJson(const std::optional<Length>& obj) const {
  return obj ? QJsonValue(obj->toMm()) : QJsonValue(QJsonValue::Null);
}

QJsonArray ProjectJsonExport::toJson(const QSet<Length>& obj) const {
  QJsonArray json;
  foreach (const Length& value, Toolbox::sortedQSet(obj)) {
    json.append(toJson(value));
  }
  return json;
}

QJsonValue ProjectJsonExport::toJson(const PcbColor* obj) const {
  // Not returning Null to distinguish from unknown resp. parsing failure.
  return obj ? QJsonValue(obj->getId()) : QJsonValue("none");
}

QJsonObject ProjectJsonExport::toJson(const AssemblyVariant& obj) const {
  QJsonObject json;
  json["uuid"] = obj.getUuid().toStr();
  json["name"] = *obj.getName();
  json["description"] = obj.getDescription();
  return json;
}

QJsonValue ProjectJsonExport::toJson(const BoundingBox& obj) const {
  if (const auto& points = obj.points) {
    QJsonObject json;
    json["x"] = toJson(std::min(points->first.getX(), points->second.getX()));
    json["y"] = toJson(std::min(points->first.getY(), points->second.getY()));
    const Point size = points->second - points->first;
    json["width"] = toJson(size.getX().abs());
    json["height"] = toJson(size.getY().abs());
    return json;
  } else {
    return QJsonValue();
  }
}

QJsonObject ProjectJsonExport::toJson(const ToolList& obj) const {
  return {
      {"count", obj.diameters.count()},
      {"diameters", toJson(Toolbox::toSet(obj.diameters))},
  };
}

QJsonObject ProjectJsonExport::toJson(const Board& obj) const {
  ToolList thtVias;
  ToolList blindVias;
  ToolList buriedVias;
  ToolList pthDrills;
  ToolList pthSlots;
  ToolList npthDrills;
  ToolList npthSlots;
  QSet<Length> copperWidths;
  foreach (const BI_NetSegment* netSegment, obj.getNetSegments()) {
    foreach (const BI_Pad* pad, netSegment->getPads()) {
      for (const PadHole& hole : pad->getProperties().getHoles()) {
        if (hole.isSlot()) {
          pthSlots.diameters.append(*hole.getDiameter());
        } else {
          pthDrills.diameters.append(*hole.getDiameter());
        }
      }
    }
    foreach (const BI_Via* via, netSegment->getVias()) {
      if (auto span = via->getDrillLayerSpan()) {
        if (span->first->isTop() && span->second->isBottom()) {
          thtVias.diameters.append(*via->getDrillDiameter());
        } else if (span->first->isTop() || span->second->isBottom()) {
          blindVias.diameters.append(*via->getDrillDiameter());
        } else {
          buriedVias.diameters.append(*via->getDrillDiameter());
        }
      }
    }
    foreach (const BI_NetLine* netLine, netSegment->getNetLines()) {
      copperWidths.insert(*netLine->getWidth());
    }
  }
  foreach (const BI_Device* device, obj.getDeviceInstances()) {
    foreach (const BI_Pad* pad, device->getPads()) {
      for (const PadHole& hole : pad->getProperties().getHoles()) {
        if (hole.isSlot()) {
          pthSlots.diameters.append(*hole.getDiameter());
        } else {
          pthDrills.diameters.append(*hole.getDiameter());
        }
      }
    }
    for (const Hole& hole : device->getLibFootprint().getHoles()) {
      if (hole.isSlot()) {
        npthSlots.diameters.append(*hole.getDiameter());
      } else {
        npthDrills.diameters.append(*hole.getDiameter());
      }
    }
  }
  foreach (const BI_Hole* hole, obj.getHoles()) {
    if (hole->getData().isSlot()) {
      npthSlots.diameters.append(*hole->getData().getDiameter());
    } else {
      npthDrills.diameters.append(*hole->getData().getDiameter());
    }
  }
  foreach (const BI_Plane* plane, obj.getPlanes()) {
    copperWidths.insert(*plane->getMinWidth());
  }
  const std::optional<Length> minCopperWidth = copperWidths.isEmpty()
      ? std::nullopt
      : std::make_optional(
            *std::min_element(copperWidths.begin(), copperWidths.end()));

  QJsonObject json;
  json["uuid"] = obj.getUuid().toStr();
  json["name"] = *obj.getName();
  json["directory"] = obj.getDirectoryName();
  json["inner_layers"] = obj.getInnerLayerCount();
  json["pcb_thickness"] = obj.getPcbThickness()->toMm();
  json["solder_resist"] = toJson(obj.getSolderResist());
  json["silkscreen_top"] = toJson(obj.getSilkscreenColorTop());
  json["silkscreen_bottom"] = toJson(obj.getSilkscreenColorBot());
  json["bounding_box"] = toJson(BoundingBox{obj.calculateBoundingRect()});
  json["vias_tht"] = toJson(thtVias);
  json["vias_blind"] = toJson(blindVias);
  json["vias_buried"] = toJson(buriedVias);
  json["pth_drills"] = toJson(pthDrills);
  json["pth_slots"] = toJson(pthSlots);
  json["npth_drills"] = toJson(npthDrills);
  json["npth_slots"] = toJson(npthSlots);
  json["min_copper_width"] = toJson(minCopperWidth);
  return json;
}

QJsonObject ProjectJsonExport::toJson(const Project& obj) const {
  QJsonObject json;
  json["filename"] = obj.getFileName();
  json["uuid"] = obj.getUuid().toStr();
  json["name"] = *obj.getName();
  json["author"] = obj.getAuthor();
  json["version"] = *obj.getVersion();
  json["created"] = obj.getCreated().toUTC().toString(Qt::ISODate);
  json["locales"] = toJson(obj.getLocaleOrder());
  json["norms"] = toJson(obj.getNormOrder());
  {
    QJsonArray jsonChild;
    for (const AssemblyVariant& av : obj.getCircuit().getAssemblyVariants()) {
      jsonChild.append(toJson(av));
    }
    json["variants"] = jsonChild;
  }
  {
    QJsonArray jsonChild;
    foreach (const Board* board, obj.getBoards()) {
      jsonChild.append(toJson(*board));
    }
    json["boards"] = jsonChild;
  }
  return json;
}

QByteArray ProjectJsonExport::toUtf8(const Project& obj) const {
  QJsonObject json;
  json["format"] = QJsonObject{
      {"major", 1},  // Only increment (when needed) for new major releases!!!
      {"minor", 0},  // Increment on every backwards-compatible format addition.
      {"type", "librepcb-project"},
  };
  json["project"] = toJson(obj);
  return QJsonDocument(json).toJson(QJsonDocument::Indented);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
