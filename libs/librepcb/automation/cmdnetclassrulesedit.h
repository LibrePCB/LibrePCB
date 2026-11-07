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

#ifndef LIBREPCB_AUTOMATION_CMDNETCLASSRULESEDIT_H
#define LIBREPCB_AUTOMATION_CMDNETCLASSRULESEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/types/length.h>
#include <librepcb/editor/undocommand.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class NetClass;

namespace automation {

/*******************************************************************************
 *  Class CmdNetClassRulesEdit
 ******************************************************************************/

/**
 * @brief Edits the design rules of a ::librepcb::NetClass which
 *        ::librepcb::editor::CmdNetClassEdit does not cover
 */
class CmdNetClassRulesEdit final : public editor::UndoCommand {
public:
  // Constructors / Destructor
  CmdNetClassRulesEdit() = delete;
  CmdNetClassRulesEdit(const CmdNetClassRulesEdit& other) = delete;
  explicit CmdNetClassRulesEdit(NetClass& netclass) noexcept;
  ~CmdNetClassRulesEdit() noexcept override;

  // Setters
  void setMinCopperCopperClearance(const UnsignedLength& value) noexcept;
  void setMinCopperWidth(const UnsignedLength& value) noexcept;
  void setMinViaDrillDiameter(const UnsignedLength& value) noexcept;

  // Operator Overloadings
  CmdNetClassRulesEdit& operator=(const CmdNetClassRulesEdit& rhs) = delete;

private:
  // Private Methods

  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  bool performExecute() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performUndo()
  void performUndo() override;

  /// @copydoc ::librepcb::editor::UndoCommand::performRedo()
  void performRedo() override;

  // Private Member Variables
  NetClass& mNetClass;
  UnsignedLength mOldMinCopperCopperClearance;
  UnsignedLength mNewMinCopperCopperClearance;
  UnsignedLength mOldMinCopperWidth;
  UnsignedLength mNewMinCopperWidth;
  UnsignedLength mOldMinViaDrillDiameter;
  UnsignedLength mNewMinViaDrillDiameter;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace automation
}  // namespace librepcb

#endif
