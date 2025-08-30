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

#ifndef LIBREPCB_EDITOR_CMDSIMPLIFYBOARDNETSEGMENTS_H
#define LIBREPCB_EDITOR_CMDSIMPLIFYBOARDNETSEGMENTS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../undocommandgroup.h"

#include <librepcb/core/geometry/trace.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BI_NetLineAnchor;
class BI_NetSegment;
class Point;

namespace editor {

/*******************************************************************************
 *  Class CmdSimplifyBoardNetSegments
 ******************************************************************************/

/**
 * @brief Undo command which runs ::librepcb::NetSegmentSimplifier on a
 *        ::librepcb::BI_NetSegment
 */
class CmdSimplifyBoardNetSegments final : public UndoCommandGroup {
public:
  // Constructors / Destructor
  CmdSimplifyBoardNetSegments() = delete;
  CmdSimplifyBoardNetSegments(const CmdSimplifyBoardNetSegments& other) =
      delete;
  explicit CmdSimplifyBoardNetSegments(
      const QList<BI_NetSegment*>& segments) noexcept;
  ~CmdSimplifyBoardNetSegments() noexcept;

  // Operator Overloadings
  CmdSimplifyBoardNetSegments& operator=(
      const CmdSimplifyBoardNetSegments& rhs) = delete;

private:  // Methods
  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  bool performExecute() override;

  void simplifySegment(BI_NetSegment& segment);

private:  // Data
  QList<BI_NetSegment*> mSegments;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
