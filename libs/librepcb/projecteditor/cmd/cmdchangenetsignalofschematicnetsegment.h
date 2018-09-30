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

#ifndef LIBREPCB_PROJECT_CMDCHANGENETSIGNALOFSCHEMATICNETSEGMENT_H
#define LIBREPCB_PROJECT_CMDCHANGENETSIGNALOFSCHEMATICNETSEGMENT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/undocommandgroup.h>
#include <librepcb/common/units/all_length_units.h>
#include <librepcb/common/uuid.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace project {

class NetSignal;
class ComponentSignalInstance;
class SI_NetSegment;

namespace editor {

/*******************************************************************************
 *  Class CmdChangeNetSignalOfSchematicNetSegment
 ******************************************************************************/

/**
 * @brief The CmdChangeNetSignalOfSchematicNetSegment class
 */
class CmdChangeNetSignalOfSchematicNetSegment final : public UndoCommandGroup {
public:
  // Constructors / Destructor
  CmdChangeNetSignalOfSchematicNetSegment() = delete;
  CmdChangeNetSignalOfSchematicNetSegment(
      const CmdChangeNetSignalOfSchematicNetSegment& other) = delete;
  CmdChangeNetSignalOfSchematicNetSegment(SI_NetSegment& seg,
                                          NetSignal&     newSig) noexcept;
  ~CmdChangeNetSignalOfSchematicNetSegment() noexcept;

private:
  // Private Methods

  /// @copydoc UndoCommand::performExecute()
  bool performExecute() override;

  void changeNetSignalOfNetSegment();
  void updateCompSigInstNetSignal(ComponentSignalInstance& cmpSig);

  // Private Member Variables

  // Attributes from the constructor
  SI_NetSegment& mNetSegment;
  NetSignal&     mNewNetSignal;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_CMDCHANGENETSIGNALOFSCHEMATICNETSEGMENT_H
