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
#include "board.h"

#include "../../export/d356netlistgenerator.h"
#include "../../library/pkg/footprintpad.h"
#include "../../library/pkg/packagepad.h"
#include "../circuit/componentinstance.h"
#include "../circuit/netsignal.h"
#include "../project.h"
#include "boardd356netlistexport.h"
#include "items/bi_device.h"
#include "items/bi_footprintpad.h"
#include "items/bi_netsegment.h"
#include "items/bi_via.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardD356NetlistExport::BoardD356NetlistExport(const Board& board) noexcept
  : mBoard(board), mCreationDateTime(QDateTime::currentDateTime()) {
}

BoardD356NetlistExport::~BoardD356NetlistExport() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

QByteArray BoardD356NetlistExport::generate() const {
  D356NetlistGenerator gen(*mBoard.getProject().getName(),
                           mBoard.getProject().getVersion(), *mBoard.getName(),
                           mCreationDateTime);

  // Vias.
  foreach (const BI_NetSegment* segment, mBoard.getNetSegments()) {
    QString netName;
    if (const NetSignal* netSignal = segment->getNetSignal()) {
      netName = *netSignal->getName();
    }
    foreach (const BI_Via* via, segment->getVias()) {
      const bool solderMaskCovered = !via->getStopMaskOffset();
      gen.throughVia(netName, via->getPosition(), via->getSize(),
                     via->getSize(), Angle::deg0(), via->getDrillDiameter(),
                     solderMaskCovered);
    }
  }

  // Footprint Pads.
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    foreach (const BI_FootprintPad* pad, device->getPads()) {
      QString netName;
      if (const NetSignal* netSignal = pad->getCompSigInstNetSignal()) {
        netName = *netSignal->getName();
      }
      const QString cmpName = *device->getComponentInstance().getName();
      QString padName;
      if (const PackagePad* pkgPad = pad->getLibPackagePad()) {
        padName = *pkgPad->getName();
      }
      const Angle rotation = pad->getMirrored()
          ? (pad->getRotation() + Angle::deg180())
          : pad->getRotation();
      if (pad->getLibPad().isTht()) {
        // THT pad. Not sure if we really need to export all holes, if there
        // are multiple. Also slots are probably not supported by IPC-D-356A.
        // I suspect it's good enough to export only a single, circular hole?
        gen.thtPad(netName, cmpName, padName, pad->getPosition(),
                   pad->getLibPad().getWidth(), pad->getLibPad().getHeight(),
                   rotation,
                   pad->getLibPad().getHoles().first()->getDiameter());
      } else {
        // SMT pad.
        const int layerNumber =
            (pad->getComponentSide() == FootprintPad::ComponentSide::Top)
            ? 1
            : (mBoard.getInnerLayerCount() + 2);
        gen.smtPad(netName, cmpName, padName, pad->getPosition(),
                   pad->getLibPad().getWidth(), pad->getLibPad().getHeight(),
                   rotation, layerNumber);
      }
    }
  }

  return gen.generate();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
