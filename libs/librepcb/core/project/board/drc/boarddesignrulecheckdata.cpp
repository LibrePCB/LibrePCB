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
#include "boarddesignrulecheckdata.h"

#include "../../../geometry/hole.h"
#include "../../../geometry/stroketext.h"
#include "../../../geometry/via.h"
#include "../../../library/cmp/component.h"
#include "../../../library/pkg/footprint.h"
#include "../../../library/pkg/footprintpad.h"
#include "../../../library/pkg/packagepad.h"
#include "../../../utils/clipperhelpers.h"
#include "../../circuit/circuit.h"
#include "../../circuit/componentinstance.h"
#include "../../circuit/netsignal.h"
#include "../../project.h"
#include "../board.h"
#include "../items/bi_airwire.h"
#include "../items/bi_device.h"
#include "../items/bi_hole.h"
#include "../items/bi_netline.h"
#include "../items/bi_netpoint.h"
#include "../items/bi_netsegment.h"
#include "../items/bi_pad.h"
#include "../items/bi_plane.h"
#include "../items/bi_polygon.h"
#include "../items/bi_stroketext.h"
#include "../items/bi_via.h"
#include "../items/bi_zone.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardDesignRuleCheckData::BoardDesignRuleCheckData(
    const Board& board, const BoardDesignRuleCheckSettings& drcSettings,
    bool quickCheck) noexcept {
  settings = drcSettings;
  quick = quickCheck;
  copperLayers = board.getCopperLayers();
  silkscreenLayersTop = board.getSilkscreenLayersTop();
  silkscreenLayersBot = board.getSilkscreenLayersBot();
  foreach (const BI_NetSegment* ns, board.getNetSegments()) {
    const NetSignal* net = ns->getNetSignal();
    Segment nsd{
        ns->getUuid(),
        net ? net->getUuid() : std::optional<Uuid>(),
        net ? *net->getName() : QString(),
        {},
        {},
        {},
    };
    foreach (const BI_NetPoint* np, ns->getNetPoints()) {
      nsd.junctions.insert(np->getUuid(),
                           Junction{np->getUuid(), np->getPosition(),
                                    np->getNetLines().count()});
    }
    foreach (const BI_NetLine* nl, ns->getNetLines()) {
      nsd.traces.append(Trace{nl->getUuid(), nl->getP1().getPosition(),
                              nl->getP2().getPosition(), nl->getWidth(),
                              &nl->getLayer()});
    }
    foreach (const BI_Via* biVia, ns->getVias()) {
      QSet<const Layer*> connectedLayers;

      // Add all the layers of traces directly connected to the current via to
      // the connectedLayers set. The via may be connected to more layers
      // through other mechanisms like planes, but identifying those connections
      // can be expensive, so it's not done here.
      foreach (const BI_NetLine* nl, biVia->getNetLines()) {
        connectedLayers.insert(&nl->getLayer());
      }

      const librepcb::Via& via = biVia->getVia();
      nsd.vias.insert(
          biVia->getUuid(),
          Via{biVia->getUuid(), biVia->getPosition(), biVia->getSize(),
              biVia->getDrillDiameter(), connectedLayers, &via.getStartLayer(),
              &via.getEndLayer(), biVia->getDrillLayerSpan(), via.isBuried(),
              via.isBlind(), biVia->getStopMaskDiameterTop(),
              biVia->getStopMaskDiameterBottom()});
    }
    segments.insert(ns->getUuid(), nsd);
  }
  foreach (const BI_Plane* plane, board.getPlanes()) {
    const NetSignal* net = plane->getNetSignal();
    planes.append(Plane{
        plane->getUuid(),
        net ? std::make_optional(net->getUuid()) : std::optional<Uuid>(),
        net ? *net->getName() : QString(), &plane->getLayer(),
        plane->getMinWidth(), plane->getOutline(), plane->getFragments()});
  }
  foreach (const BI_Polygon* polygon, board.getPolygons()) {
    polygons.append(
        Polygon{polygon->getData().getUuid(), &polygon->getData().getLayer(),
                polygon->getData().getLineWidth(),
                polygon->getData().isFilled(), polygon->getData().getPath()});
  }
  foreach (const BI_StrokeText* strokeText, board.getStrokeTexts()) {
    strokeTexts.append(StrokeText{
        strokeText->getData().getUuid(), strokeText->getData().getPosition(),
        strokeText->getData().getRotation(),
        strokeText->getData().getMirrored(), &strokeText->getData().getLayer(),
        strokeText->getData().getStrokeWidth(),
        strokeText->getData().getHeight(), strokeText->getPaths()});
  }
  foreach (const BI_Hole* hole, board.getHoles()) {
    holes.append(Hole{hole->getData().getUuid(), hole->getData().getDiameter(),
                      hole->getData().getPath(), hole->getStopMaskOffset()});
  }
  foreach (const BI_Zone* zone, board.getZones()) {
    zones.append(Zone{zone->getData().getUuid(), zone->getData().getLayers(),
                      librepcb::Zone::Layers(), zone->getData().getRules(),
                      zone->getData().getOutline()});
  }
  foreach (const BI_Device* dev, board.getDeviceInstances()) {
    Device dd{
        dev->getComponentInstanceUuid(),
        *dev->getComponentInstance().getName(),
        dev->getPosition(),
        dev->getRotation(),
        dev->getMirrored(),
        {},
        {},
        {},
        {},
        {},
        {},
    };
    foreach (const BI_Pad* pad, dev->getPads()) {
      QSet<const Layer*> layersWithTraces;
      foreach (const BI_NetLine* netLine, pad->getNetLines()) {
        layersWithTraces.insert(&netLine->getLayer());
      }
      const NetSignal* net = pad->getCompSigInstNetSignal();
      Pad pd{
          pad->getLibPadUuid(),
          pad->getLibPackagePad() ? *pad->getLibPackagePad()->getName()
                                  : QString(),
          pad->getPosition(),
          pad->getRotation(),
          pad->getMirrored(),
          {},
          pad->getGeometries(),
          layersWithTraces,
          pad->getLibPad().getCopperClearance(),
          net ? std::make_optional(net->getUuid()) : std::optional<Uuid>(),
          net ? *net->getName() : QString(),
      };
      for (const PadHole& hole : pad->getLibPad().getHoles()) {
        pd.holes.append(Hole{hole.getUuid(), hole.getDiameter(), hole.getPath(),
                             std::optional<Length>()});
      }
      dd.pads.insert(pad->getLibPadUuid(), pd);
    }
    for (const librepcb::Polygon& polygon :
         dev->getLibFootprint().getPolygons()) {
      dd.polygons.append(Polygon{polygon.getUuid(), &polygon.getLayer(),
                                 polygon.getLineWidth(), polygon.isFilled(),
                                 polygon.getPath()});
    }
    for (const librepcb::Circle& circle : dev->getLibFootprint().getCircles()) {
      dd.circles.append(Circle{circle.getUuid(), circle.getCenter(),
                               circle.getDiameter(), &circle.getLayer(),
                               circle.getLineWidth(), circle.isFilled()});
    }
    foreach (const BI_StrokeText* strokeText, dev->getStrokeTexts()) {
      dd.strokeTexts.append(StrokeText{
          strokeText->getData().getUuid(), strokeText->getData().getPosition(),
          strokeText->getData().getRotation(),
          strokeText->getData().getMirrored(),
          &strokeText->getData().getLayer(),
          strokeText->getData().getStrokeWidth(),
          strokeText->getData().getHeight(), strokeText->getPaths()});
    }
    for (const librepcb::Hole& hole : dev->getLibFootprint().getHoles()) {
      dd.holes.append(Hole{hole.getUuid(), hole.getDiameter(), hole.getPath(),
                           dev->getHoleStopMasks().value(hole.getUuid())});
    }
    for (const librepcb::Zone& zone : dev->getLibFootprint().getZones()) {
      dd.zones.append(Zone{zone.getUuid(),
                           {},
                           zone.getLayers(),
                           zone.getRules(),
                           zone.getOutline()});
    }
    devices.insert(dev->getComponentInstanceUuid(), dd);
  }
  auto convertAnchor = [](const BI_NetLineAnchor& a) {
    AirWireAnchor ret;
    ret.position = a.getPosition();
    if (const BI_Pad* pad = dynamic_cast<const BI_Pad*>(&a)) {
      ret.device = pad->getDevice().getComponentInstanceUuid();
      ret.pad = pad->getLibPadUuid();
    } else if (const BI_NetPoint* np = dynamic_cast<const BI_NetPoint*>(&a)) {
      ret.segment = np->getNetSegment().getUuid();
      ret.junction = np->getUuid();
    } else if (const BI_Via* via = dynamic_cast<const BI_Via*>(&a)) {
      ret.segment = via->getNetSegment().getUuid();
      ret.via = via->getUuid();
    } else {
      qCritical() << "Unknown anchor type, DRC will fail later.";
    }
    return ret;
  };
  foreach (const BI_AirWire* aw, board.getAirWires()) {
    airWires.append(AirWire{convertAnchor(aw->getP1()),
                            convertAnchor(aw->getP2()),
                            *aw->getNetSignal().getName()});
  }
  foreach (const ComponentInstance* cmp,
           board.getProject().getCircuit().getComponentInstances()) {
    // A bit unusual, but the actual check is already done here to avoid
    // copying lots of data for such a lightweight check.
    const BI_Device* dev =
        board.getDeviceInstanceByComponentUuid(cmp->getUuid());
    if ((!dev) && (!cmp->getLibComponent().isSchematicOnly())) {
      unplacedComponents.insert(cmp->getUuid(), *cmp->getName());
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
