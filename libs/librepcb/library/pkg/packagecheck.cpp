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
#include "packagecheck.h"

#include "msg/msgdrillinsmdpad.h"
#include "msg/msgduplicatepadname.h"
#include "msg/msgmalformeddrill.h"
#include "msg/msgmissingdrill.h"
#include "msg/msgmissingfootprint.h"
#include "msg/msgmissingfootprintname.h"
#include "msg/msgmissingfootprintvalue.h"
#include "msg/msgpadoverlapswithplacement.h"
#include "msg/msgwrongfootprinttextlayer.h"
#include "package.h"

#include <librepcb/common/graphics/graphicslayer.h>
#include <librepcb/common/toolbox.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

PackageCheck::PackageCheck(const Package& package) noexcept
  : LibraryElementCheck(package), mPackage(package) {
}

PackageCheck::~PackageCheck() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

LibraryElementCheckMessageList PackageCheck::runChecks() const {
  LibraryElementCheckMessageList msgs = LibraryElementCheck::runChecks();
  checkDrillsInSmdPads(msgs);
  checkDuplicatePadNames(msgs);
  checkMalformedDrills(msgs);
  checkMissingDrills(msgs);
  checkMissingFootprint(msgs);
  checkMissingTexts(msgs);
  checkWrongTextLayers(msgs);
  checkPadsOverlapWithPlacement(msgs);
  return msgs;
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

void PackageCheck::checkDrillsInSmdPads(MsgList& msgs) const {
  for (const Footprint& footprint : mPackage.getFootprints()) {
    for (const FootprintPad& pad : footprint.getPads()) {
      if (pad.getDrillSize() &&
          pad.getBoardSide() != FootprintPad::BoardSide::THT) {
        msgs.append(std::make_shared<MsgDrillInSmdPad>(pad));
      }
    }
  }
}

void PackageCheck::checkDuplicatePadNames(MsgList& msgs) const {
  QSet<CircuitIdentifier> padNames;
  for (const PackagePad& pad : mPackage.getPads()) {
    if (padNames.contains(pad.getName())) {
      msgs.append(std::make_shared<MsgDuplicatePadName>(pad));
    } else {
      padNames.insert(pad.getName());
    }
  }
}

void PackageCheck::checkMalformedDrills(MsgList& msgs) const {
  for (const Footprint& footprint : mPackage.getFootprints()) {
    for (const FootprintPad& pad : footprint.getPads()) {
      if (pad.getDrillSize()) {
        if (pad.getDrillSize()->getWidth() >= pad.getWidth()) {
          msgs.append(std::make_shared<MsgMalformedDrill>(
              pad, MsgMalformedDrill::WIDER));
        }
        if (pad.getDrillSize()->getHeight() >= pad.getHeight()) {
          msgs.append(std::make_shared<MsgMalformedDrill>(
              pad, MsgMalformedDrill::TALLER));
        }
      }
    }
  }
}

void PackageCheck::checkMissingDrills(MsgList& msgs) const {
  for (const Footprint& footprint : mPackage.getFootprints()) {
    for (const FootprintPad& pad : footprint.getPads()) {
      if (!pad.getDrillSize() &&
          pad.getBoardSide() == FootprintPad::BoardSide::THT) {
        msgs.append(std::make_shared<MsgMissingDrill>(pad));
      }
    }
  }
}

void PackageCheck::checkMissingFootprint(MsgList& msgs) const {
  if (mPackage.getFootprints().isEmpty()) {
    msgs.append(std::make_shared<MsgMissingFootprint>());
  }
}

void PackageCheck::checkMissingTexts(MsgList& msgs) const {
  for (auto itFtp = mPackage.getFootprints().begin();
       itFtp != mPackage.getFootprints().end(); ++itFtp) {
    QHash<QString, QVector<std::shared_ptr<const StrokeText>>> texts;
    for (auto it = (*itFtp).getStrokeTexts().begin();
         it != (*itFtp).getStrokeTexts().end(); ++it) {
      texts[(*it).getText()].append(it.ptr());
    }
    if (texts.value("{{NAME}}").isEmpty()) {
      msgs.append(std::make_shared<MsgMissingFootprintName>(itFtp.ptr()));
    }
    if (texts.value("{{VALUE}}").isEmpty()) {
      msgs.append(std::make_shared<MsgMissingFootprintValue>(itFtp.ptr()));
    }
  }
}

void PackageCheck::checkWrongTextLayers(MsgList& msgs) const {
  QHash<QString, QString> textLayers = {
      std::make_pair("{{NAME}}", QString(GraphicsLayer::sTopNames)),
      std::make_pair("{{VALUE}}", QString(GraphicsLayer::sTopValues)),
  };
  for (auto itFtp = mPackage.getFootprints().begin();
       itFtp != mPackage.getFootprints().end(); ++itFtp) {
    for (auto it = (*itFtp).getStrokeTexts().begin();
         it != (*itFtp).getStrokeTexts().end(); ++it) {
      QString expectedLayer = textLayers.value((*it).getText());
      if ((!expectedLayer.isEmpty()) &&
          ((*it).getLayerName() != expectedLayer)) {
        msgs.append(std::make_shared<MsgWrongFootprintTextLayer>(
            itFtp.ptr(), it.ptr(), expectedLayer));
      }
    }
  }
}

void PackageCheck::checkPadsOverlapWithPlacement(MsgList& msgs) const {
  for (auto itFtp = mPackage.getFootprints().begin();
       itFtp != mPackage.getFootprints().end(); ++itFtp) {
    std::shared_ptr<const Footprint> footprint = itFtp.ptr();

    QPainterPath topPlacement;
    QPainterPath botPlacement;
    for (const Polygon& polygon : footprint->getPolygons()) {
      QPen pen(Qt::NoPen);
      if (polygon.getLineWidth() > 0) {
        pen.setStyle(Qt::SolidLine);
        pen.setWidthF(polygon.getLineWidth()->toPx());
      }
      QBrush brush(Qt::NoBrush);
      if (polygon.isFilled() && polygon.getPath().isClosed()) {
        brush.setStyle(Qt::SolidPattern);
      }
      QPainterPath area = Toolbox::shapeFromPath(
          polygon.getPath().toQPainterPathPx(), pen, brush);
      if (polygon.getLayerName() == GraphicsLayer::sTopPlacement) {
        topPlacement.addPath(area);
      } else if (polygon.getLayerName() == GraphicsLayer::sBotPlacement) {
        botPlacement.addPath(area);
      }
    }

    for (auto it = (*itFtp).getPads().begin(); it != (*itFtp).getPads().end();
         ++it) {
      std::shared_ptr<const FootprintPad> pad = it.ptr();
      std::shared_ptr<const PackagePad>   pkgPad =
          mPackage.getPads().find(pad->getUuid());
      Length clearance(150000);  // 150 µm
      Length tolerance(10);      // 0.01 µm, to avoid rounding issues
      Path   stopMaskPath = pad->getOutline(clearance - tolerance);
      stopMaskPath.rotate(pad->getRotation()).translate(pad->getPosition());
      QPainterPath stopMask = stopMaskPath.toQPainterPathPx();
      if (pad->isOnLayer(GraphicsLayer::sTopCopper) &&
          stopMask.intersects(topPlacement)) {
        msgs.append(std::make_shared<MsgPadOverlapsWithPlacement>(
            footprint, pad, pkgPad ? *pkgPad->getName() : QString(),
            clearance));
      } else if (pad->isOnLayer(GraphicsLayer::sBotCopper) &&
                 stopMask.intersects(botPlacement)) {
        msgs.append(std::make_shared<MsgPadOverlapsWithPlacement>(
            footprint, pad, pkgPad ? *pkgPad->getName() : QString(),
            clearance));
      }
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb
