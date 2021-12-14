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

#include "msg/msgduplicatepadname.h"
#include "msg/msgmissingfootprint.h"
#include "msg/msgmissingfootprintname.h"
#include "msg/msgmissingfootprintvalue.h"
#include "msg/msgpadclearanceviolation.h"
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
  checkDuplicatePadNames(msgs);
  checkMissingFootprint(msgs);
  checkMissingTexts(msgs);
  checkWrongTextLayers(msgs);
  checkPadsClearanceToPads(msgs);
  checkPadsClearanceToPlacement(msgs);
  return msgs;
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

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

void PackageCheck::checkPadsClearanceToPads(MsgList& msgs) const {
  Length clearance(200000);  // 200 µm
  Length tolerance(10);  // 0.01 µm, to avoid rounding issues

  // Check all footprints.
  for (auto itFtp = mPackage.getFootprints().begin();
       itFtp != mPackage.getFootprints().end(); ++itFtp) {
    std::shared_ptr<const Footprint> footprint = itFtp.ptr();

    // Check all pads.
    for (auto itPad1 = (*itFtp).getPads().begin();
         itPad1 != (*itFtp).getPads().end(); ++itPad1) {
      std::shared_ptr<const FootprintPad> pad1 = itPad1.ptr();
      std::shared_ptr<const PackagePad> pkgPad1 = pad1->getPackagePadUuid()
          ? mPackage.getPads().find(*pad1->getPackagePadUuid())
          : nullptr;
      Path pad1Path = pad1->getOutline(clearance - tolerance);
      pad1Path.rotate(pad1->getRotation()).translate(pad1->getPosition());
      const QPainterPath pad1PathPx = pad1Path.toQPainterPathPx();

      // Compare with all pads *after* pad1 to avoid duplicate messages!
      // So, don't initialize the iterator with begin() but with pad1 + 1.
      auto itPad2 = itPad1;
      for (++itPad2; itPad2 != (*itFtp).getPads().end(); ++itPad2) {
        std::shared_ptr<const FootprintPad> pad2 = itPad2.ptr();
        std::shared_ptr<const PackagePad> pkgPad2 = pad2->getPackagePadUuid()
            ? mPackage.getPads().find(*pad2->getPackagePadUuid())
            : nullptr;
        Path pad2Path = pad2->getOutline();
        pad2Path.rotate(pad2->getRotation()).translate(pad2->getPosition());
        const QPainterPath pad2PathPx = pad2Path.toQPainterPathPx();

        // Only warn if both pads have copper on the same board side.
        if ((pad1->getBoardSide() == pad2->getBoardSide()) ||
            (pad1->getBoardSide() == FootprintPad::BoardSide::THT) ||
            (pad2->getBoardSide() == FootprintPad::BoardSide::THT)) {
          // Only warn if both pads have different net signal, or one of them
          // is unconnected (an unconnected pad is considered as a different
          // net signal).
          if ((pad1->getPackagePadUuid() != pad2->getPackagePadUuid()) ||
              (!pad1->getPackagePadUuid()) || (!pad2->getPackagePadUuid())) {
            // Now check if the clearance is really too small.
            if (pad1PathPx.intersects(pad2PathPx)) {
              msgs.append(std::make_shared<MsgPadClearanceViolation>(
                  footprint, pad1, pkgPad1 ? *pkgPad1->getName() : QString(),
                  pad2, pkgPad2 ? *pkgPad2->getName() : QString(), clearance));
            }
          }
        }
      }
    }
  }
}

void PackageCheck::checkPadsClearanceToPlacement(MsgList& msgs) const {
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
      std::shared_ptr<const PackagePad> pkgPad = pad->getPackagePadUuid()
          ? mPackage.getPads().find(*pad->getPackagePadUuid())
          : nullptr;
      Length clearance(150000);  // 150 µm
      Length tolerance(10);  // 0.01 µm, to avoid rounding issues
      Path stopMaskPath = pad->getOutline(clearance - tolerance);
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
