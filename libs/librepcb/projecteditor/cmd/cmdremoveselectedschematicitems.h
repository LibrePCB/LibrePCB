/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

#ifndef LIBREPCB_PROJECT_CMDREMOVESELECTEDSCHEMATICITEMS_H
#define LIBREPCB_PROJECT_CMDREMOVESELECTEDSCHEMATICITEMS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/undocommandgroup.h>
#include <librepcb/common/units/all_length_units.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace project {

class Schematic;
class SI_NetSegment;
class SI_NetPoint;
class SI_NetLine;
class SI_NetLabel;
class SI_Symbol;
class SI_SymbolPin;
class BI_Device;
class SI_NetLineAnchor;
class ComponentSignalInstance;

namespace editor {

/*******************************************************************************
 *  Class CmdRemoveSelectedSchematicItems
 ******************************************************************************/

/**
 * @brief The CmdRemoveSelectedSchematicItems class
 */
class CmdRemoveSelectedSchematicItems final : public UndoCommandGroup {
private:
  // Private Types
  struct NetSegmentItems {
    QSet<SI_NetPoint*> netpoints;
    QSet<SI_NetLine*>  netlines;
    QSet<SI_NetLabel*> netlabels;
  };
  typedef QHash<SI_NetSegment*, NetSegmentItems> NetSegmentItemList;

public:
  // Constructors / Destructor
  explicit CmdRemoveSelectedSchematicItems(Schematic& schematic) noexcept;
  ~CmdRemoveSelectedSchematicItems() noexcept;

private:
  // Private Methods

  /// @copydoc UndoCommand::performExecute()
  bool performExecute() override;

  void           removeNetSegment(SI_NetSegment& netsegment);
  void           splitUpNetSegment(SI_NetSegment&         netsegment,
                                   const NetSegmentItems& selectedItems);
  SI_NetSegment* createNewSubNetSegment(SI_NetSegment&         netsegment,
                                        const NetSegmentItems& items);
  void           removeNetLabel(SI_NetLabel& netlabel);
  void           removeSymbol(SI_Symbol& symbol);
  void disconnectComponentSignalInstance(ComponentSignalInstance& signal);
  QList<NetSegmentItems> getNonCohesiveNetSegmentSubSegments(
      SI_NetSegment& segment, const NetSegmentItems& removedItems) noexcept;
  void findAllConnectedNetPointsAndNetLines(
      SI_NetLineAnchor& anchor, QSet<SI_NetLineAnchor*>& processedAnchors,
      QSet<SI_NetPoint*>& netpoints, QSet<SI_NetLine*>& netlines,
      QSet<SI_NetLine*>& availableNetLines) const noexcept;
  int getNearestNetSegmentOfNetLabel(
      const SI_NetLabel& netlabel, const QList<NetSegmentItems>& segments) const
      noexcept;
  Length getDistanceBetweenNetLabelAndNetSegment(
      const SI_NetLabel& netlabel, const NetSegmentItems& netsegment) const
      noexcept;

  // Attributes from the constructor
  Schematic& mSchematic;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_CMDREMOVESELECTEDSCHEMATICITEMS_H
