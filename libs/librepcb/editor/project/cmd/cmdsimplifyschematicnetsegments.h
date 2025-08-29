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

#ifndef LIBREPCB_EDITOR_CMDSIMPLIFYSCHEMATICNETSEGMENTS_H
#define LIBREPCB_EDITOR_CMDSIMPLIFYSCHEMATICNETSEGMENTS_H

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
class SI_NetLineAnchor;
class SI_NetSegment;

namespace editor {

/*******************************************************************************
 *  Class CmdSimplifySchematicNetSegments
 ******************************************************************************/

/**
 * @brief Undo command which runs ::librepcb::NetSegmentSimplifier on a
 *        ::librepcb::SI_NetSegment
 */
class CmdSimplifySchematicNetSegments final : public UndoCommandGroup {
public:
  // Constructors / Destructor
  CmdSimplifySchematicNetSegments() = delete;
  CmdSimplifySchematicNetSegments(
      const CmdSimplifySchematicNetSegments& other) = delete;
  explicit CmdSimplifySchematicNetSegments(
      const QList<SI_NetSegment*>& segments) noexcept;
  ~CmdSimplifySchematicNetSegments() noexcept;

  // Operator Overloadings
  CmdSimplifySchematicNetSegments& operator=(
      const CmdSimplifySchematicNetSegments& rhs) = delete;

private:  // Methods
  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  bool performExecute() override;

  void simplifySegment(SI_NetSegment& segment);

private:  // Data
  QList<SI_NetSegment*> mSegments;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
