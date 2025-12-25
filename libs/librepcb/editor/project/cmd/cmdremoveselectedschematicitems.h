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

#ifndef LIBREPCB_EDITOR_CMDREMOVESELECTEDSCHEMATICITEMS_H
#define LIBREPCB_EDITOR_CMDREMOVESELECTEDSCHEMATICITEMS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../undocommandgroup.h"

#include <librepcb/core/project/schematic/schematicnetsegmentsplitter.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class ComponentSignalInstance;
class NetSignal;
class SI_BusJunction;
class SI_BusLabel;
class SI_BusLine;
class SI_BusSegment;
class SI_NetLabel;
class SI_NetLine;
class SI_NetPoint;
class SI_NetSegment;
class SI_Symbol;

namespace editor {

class SchematicGraphicsScene;

/*******************************************************************************
 *  Class CmdRemoveSelectedSchematicItems
 ******************************************************************************/

/**
 * @brief The CmdRemoveSelectedSchematicItems class
 */
class CmdRemoveSelectedSchematicItems final : public UndoCommandGroup {
  struct Segment {
    NetSignal* net;
    SchematicNetSegmentSplitter::Segment elements;
  };

public:
  // Constructors / Destructor
  explicit CmdRemoveSelectedSchematicItems(
      SchematicGraphicsScene& scene) noexcept;
  ~CmdRemoveSelectedSchematicItems() noexcept;

  // Output
  const QSet<SI_NetSegment*>& getModifiedNetSegments() const noexcept {
    return mModifiedNetSegments;
  }
  const QSet<SI_BusSegment*>& getModifiedBusSegments() const noexcept {
    return mModifiedBusSegments;
  }

private:
  // Private Methods

  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  bool performExecute() override;

  void removeNetSegmentItems(SI_NetSegment& netsegment,
                             const QSet<SI_NetPoint*>& netpointsToRemove,
                             const QSet<SI_NetLine*>& netlinesToRemove,
                             const QSet<SI_NetLabel*>& netlabelsToRemove,
                             const QSet<SI_BusJunction*>& busJunctionsToReplace,
                             QVector<Segment>& remainingNetSegments);
  void removeBusSegmentItems(
      SI_BusSegment& busSegment, const QSet<SI_BusJunction*>& junctionsToRemove,
      const QSet<SI_BusLine*>& linesToRemove,
      const QSet<SI_BusLabel*>& labelsToRemove,
      QHash<NetLineAnchor, NetLineAnchor>& replacedBusJunctions);
  void addRemainingNetSegmentItems(
      const QVector<Segment>& remainingNetSegments,
      const QHash<NetLineAnchor, NetLineAnchor>& replacedBusJunctions);
  void removeSymbol(SI_Symbol& symbol);
  void disconnectComponentSignalInstance(ComponentSignalInstance& signal);

  // Attributes from the constructor
  SchematicGraphicsScene& mScene;

  // Output
  QSet<SI_NetSegment*> mModifiedNetSegments;
  QSet<SI_BusSegment*> mModifiedBusSegments;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
