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

#ifndef LIBREPCB_EDITOR_CMDSIMPLIFYSCHEMATICSEGMENTS_H
#define LIBREPCB_EDITOR_CMDSIMPLIFYSCHEMATICSEGMENTS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../undocommandgroup.h"

#include <librepcb/core/geometry/netline.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Point;
class SI_BusJunction;
class SI_BusSegment;
class SI_NetLineAnchor;
class SI_NetSegment;

namespace editor {

/*******************************************************************************
 *  Class CmdSimplifySchematicSegments
 ******************************************************************************/

/**
 * @brief Undo command which runs ::librepcb::NetSegmentSimplifier on several
 *        ::librepcb::SI_NetSegment and ::librepcb::SI_BusSegment
 */
class CmdSimplifySchematicSegments final : public UndoCommandGroup {
public:
  // Constructors / Destructor
  CmdSimplifySchematicSegments() = delete;
  CmdSimplifySchematicSegments(const CmdSimplifySchematicSegments& other) =
      delete;
  explicit CmdSimplifySchematicSegments(
      const QSet<SI_NetSegment*>& netSegments,
      const QSet<SI_BusSegment*>& busSegments) noexcept;
  ~CmdSimplifySchematicSegments() noexcept;

  // Operator Overloadings
  CmdSimplifySchematicSegments& operator=(
      const CmdSimplifySchematicSegments& rhs) = delete;

private:  // Methods
  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  bool performExecute() override;

  void simplifySegment(SI_BusSegment& segment);
  void simplifySegment(SI_NetSegment& segment);

private:  // Data
  QSet<SI_NetSegment*> mNetSegments;
  QSet<SI_BusSegment*> mBusSegments;

  QSet<SI_NetSegment*> mTemporarilyRemovedNetSegments;
  QHash<SI_BusJunction*, SI_BusJunction*> mReplacedBusJunctions;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
