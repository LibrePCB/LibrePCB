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

#include "../../graphics/graphicslayer.h"
#include "../../utils/toolbox.h"
#include "../../utils/transform.h"
#include "package.h"
#include "packagecheckmessages.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

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

RuleCheckMessageList PackageCheck::runChecks() const {
  RuleCheckMessageList msgs = LibraryElementCheck::runChecks();
  checkAssemblyType(msgs);
  checkDuplicatePadNames(msgs);
  checkMissingFootprint(msgs);
  checkMissingTexts(msgs);
  checkWrongTextLayers(msgs);
  checkPadsClearanceToPads(msgs);
  checkPadsClearanceToPlacement(msgs);
  checkPadsAnnularRing(msgs);
  checkPadsConnectionPoint(msgs);
  checkCustomPadOutline(msgs);
  checkHolesStopMask(msgs);
  return msgs;
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

void PackageCheck::checkAssemblyType(MsgList& msgs) const {
  // Check for deprecated assembly type.
  if (mPackage.getAssemblyType(false) == Package::AssemblyType::Auto) {
    msgs.append(std::make_shared<MsgDeprecatedAssemblyType>());
  }

  // Check if the assembly type looks reasonable (only possible if there is at
  // least one footprint).
  if ((!mPackage.getFootprints().isEmpty()) &&
      (mPackage.getAssemblyType(false) != Package::AssemblyType::Auto) &&
      (mPackage.getAssemblyType(false) != mPackage.guessAssemblyType())) {
    msgs.append(std::make_shared<MsgSuspiciousAssemblyType>());
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
      const Transform pad1Transform(pad1->getPosition(), pad1->getRotation());
      const QPainterPath pad1PathPxWithoutClearance =
          pad1Transform.mapPx(pad1->getGeometry().toFilledQPainterPathPx());
      const QPainterPath pad1PathPxWithClearance =
          pad1Transform.mapPx(pad1->getGeometry()
                                  .withOffset(clearance - tolerance)
                                  .toFilledQPainterPathPx());

      // Compare with all pads *after* pad1 to avoid duplicate messages!
      // So, don't initialize the iterator with begin() but with pad1 + 1.
      auto itPad2 = itPad1;
      for (++itPad2; itPad2 != (*itFtp).getPads().end(); ++itPad2) {
        std::shared_ptr<const FootprintPad> pad2 = itPad2.ptr();
        std::shared_ptr<const PackagePad> pkgPad2 = pad2->getPackagePadUuid()
            ? mPackage.getPads().find(*pad2->getPackagePadUuid())
            : nullptr;
        const Transform pad2Transform(pad2->getPosition(), pad2->getRotation());
        const QPainterPath pad2PathPx =
            pad2Transform.mapPx(pad2->getGeometry().toFilledQPainterPathPx());

        // Only warn if both pads have copper on the same board side.
        if ((pad1->getComponentSide() == pad2->getComponentSide()) ||
            (pad1->isTht()) || (pad2->isTht())) {
          // Only warn if both pads have different net signal, or one of them
          // is unconnected (an unconnected pad is considered as a different
          // net signal).
          if ((pad1->getPackagePadUuid() != pad2->getPackagePadUuid()) ||
              (!pad1->getPackagePadUuid()) || (!pad2->getPackagePadUuid())) {
            // Now check if the clearance is really too small.
            if (pad1PathPxWithoutClearance.intersects(pad2PathPx)) {
              msgs.append(std::make_shared<MsgOverlappingPads>(
                  footprint, pad1, pkgPad1 ? *pkgPad1->getName() : QString(),
                  pad2, pkgPad2 ? *pkgPad2->getName() : QString()));
            } else if (pad1PathPxWithClearance.intersects(pad2PathPx)) {
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
      const Transform transform(pad->getPosition(), pad->getRotation());
      const QPainterPath stopMask =
          transform.mapPx(pad->getGeometry()
                              .withOffset(clearance - tolerance)
                              .toFilledQPainterPathPx());
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

void PackageCheck::checkPadsAnnularRing(MsgList& msgs) const {
  const Length annularRing(150000);  // 150 µm
  const Length tolerance(10);  // 0.01 µm, to avoid rounding issues

  // Check all footprints.
  for (auto itFtp = mPackage.getFootprints().begin();
       itFtp != mPackage.getFootprints().end(); ++itFtp) {
    std::shared_ptr<const Footprint> footprint = itFtp.ptr();

    // Check all pads.
    for (auto itPad = (*itFtp).getPads().begin();
         itPad != (*itFtp).getPads().end(); ++itPad) {
      std::shared_ptr<const FootprintPad> pad = itPad.ptr();
      std::shared_ptr<const PackagePad> pkgPad = pad->getPackagePadUuid()
          ? mPackage.getPads().find(*pad->getPackagePadUuid())
          : nullptr;
      const QPainterPath padPathPx =
          pad->getGeometry().toFilledQPainterPathPx();

      // Check all holes.
      bool emitError = false;
      bool emitWarning = false;
      for (auto itHole1 = (*itPad).getHoles().begin();
           itHole1 != (*itPad).getHoles().end(); ++itHole1) {
        std::shared_ptr<const PadHole> hole1 = itHole1.ptr();
        const QVector<Path> hole1Paths =
            hole1->getPath()->toOutlineStrokes(hole1->getDiameter());
        const QVector<Path> hole1PathsWithAnnular =
            hole1->getPath()->toOutlineStrokes(
                hole1->getDiameter() +
                PositiveLength((annularRing * 2) - tolerance));
        const QPainterPath hole1PathPx =
            Path::toQPainterPathPx(hole1Paths, true);
        const QPainterPath hole1PathPxWithAnnular =
            Path::toQPainterPathPx(hole1PathsWithAnnular, true);

        // Check annular rings.
        if (!padPathPx.contains(hole1PathPx)) {
          emitError = true;
        } else if (!padPathPx.contains(hole1PathPxWithAnnular)) {
          emitWarning = true;
        } else {
          // Compare with all holes *after* hole1 to avoid redundant checks.
          // So, don't initialize the iterator with begin() but with hole1 + 1.
          auto itHole2 = itHole1;
          for (++itHole2; itHole2 != (*itPad).getHoles().end(); ++itHole2) {
            std::shared_ptr<const PadHole> hole2 = itHole2.ptr();
            const QVector<Path> hole2Paths =
                hole2->getPath()->toOutlineStrokes(hole2->getDiameter());
            const QPainterPath hole2PathPx =
                Path::toQPainterPathPx(hole2Paths, true);

            // Now check if the annular ring is really too small.
            if (hole1PathPx.intersects(hole2PathPx)) {
              emitError = true;
            } else if (hole1PathPxWithAnnular.intersects(hole2PathPx)) {
              emitWarning = true;
            }
          }
        }
      }

      // Only show one message even if there are multiple violations.
      if (emitError) {
        msgs.append(std::make_shared<MsgPadHoleOutsideCopper>(
            footprint, pad, pkgPad ? *pkgPad->getName() : QString()));
      } else if (emitWarning) {
        msgs.append(std::make_shared<MsgPadAnnularRingViolation>(
            footprint, pad, pkgPad ? *pkgPad->getName() : QString(),
            annularRing));
      }
    }
  }
}

void PackageCheck::checkPadsConnectionPoint(MsgList& msgs) const {
  for (auto itFtp = mPackage.getFootprints().begin();
       itFtp != mPackage.getFootprints().end(); ++itFtp) {
    std::shared_ptr<const Footprint> footprint = itFtp.ptr();
    for (auto itPad = (*itFtp).getPads().begin();
         itPad != (*itFtp).getPads().end(); ++itPad) {
      std::shared_ptr<const FootprintPad> pad = itPad.ptr();
      std::shared_ptr<const PackagePad> pkgPad = pad->getPackagePadUuid()
          ? mPackage.getPads().find(*pad->getPackagePadUuid())
          : nullptr;
      const QPainterPath allowedArea = pad->isTht()
          ? pad->getGeometry().toHolesQPainterPathPx()
          : pad->getGeometry().toFilledQPainterPathPx();
      if (!allowedArea.contains(QPointF(0, 0))) {
        msgs.append(std::make_shared<MsgPadOriginOutsideCopper>(
            footprint, pad, pkgPad ? *pkgPad->getName() : QString()));
      }
    }
  }
}

void PackageCheck::checkCustomPadOutline(MsgList& msgs) const {
  for (auto itFtp = mPackage.getFootprints().begin();
       itFtp != mPackage.getFootprints().end(); ++itFtp) {
    std::shared_ptr<const Footprint> footprint = itFtp.ptr();
    for (auto itPad = (*itFtp).getPads().begin();
         itPad != (*itFtp).getPads().end(); ++itPad) {
      std::shared_ptr<const FootprintPad> pad = itPad.ptr();
      std::shared_ptr<const PackagePad> pkgPad = pad->getPackagePadUuid()
          ? mPackage.getPads().find(*pad->getPackagePadUuid())
          : nullptr;
      if ((pad->getShape() == FootprintPad::Shape::Custom) &&
          (!PadGeometry::isValidCustomOutline(pad->getCustomShapeOutline()))) {
        msgs.append(std::make_shared<MsgInvalidCustomPadOutline>(
            footprint, pad, pkgPad ? *pkgPad->getName() : QString()));
      } else if ((pad->getShape() != FootprintPad::Shape::Custom) &&
                 (!pad->getCustomShapeOutline().getVertices().isEmpty())) {
        msgs.append(std::make_shared<MsgUnusedCustomPadOutline>(
            footprint, pad, pkgPad ? *pkgPad->getName() : QString()));
      }
    }
  }
}

void PackageCheck::checkHolesStopMask(MsgList& msgs) const {
  for (auto itFtp = mPackage.getFootprints().begin();
       itFtp != mPackage.getFootprints().end(); ++itFtp) {
    std::shared_ptr<const Footprint> footprint = itFtp.ptr();
    for (auto itHole = (*itFtp).getHoles().begin();
         itHole != (*itFtp).getHoles().end(); ++itHole) {
      std::shared_ptr<const Hole> hole = itHole.ptr();
      if (!hole->getStopMaskConfig().isEnabled()) {
        msgs.append(std::make_shared<MsgHoleWithoutStopMask>(footprint, hole));
      }
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
