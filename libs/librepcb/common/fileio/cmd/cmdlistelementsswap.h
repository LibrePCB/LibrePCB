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

#ifndef LIBREPCB_CMDLISTELEMENTSSWAP_H
#define LIBREPCB_CMDLISTELEMENTSSWAP_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../undocommand.h"
#include "../serializableobjectlist.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class CmdListElementsSwap
 ******************************************************************************/

/**
 * @brief The CmdListElementsSwap class
 */
template <typename T, typename P>
class CmdListElementsSwap final : public UndoCommand {
public:
  // Constructors / Destructor
  CmdListElementsSwap()                                 = delete;
  CmdListElementsSwap(const CmdListElementsSwap& other) = delete;
  CmdListElementsSwap(SerializableObjectList<T, P>& list, int i, int j) noexcept
    : UndoCommand(QString(tr("Move %1")).arg(P::tagname)),
      mList(list),
      mI(i),
      mJ(j) {}
  ~CmdListElementsSwap() noexcept {}

  // Operator Overloadings
  CmdListElementsSwap& operator=(const CmdListElementsSwap& rhs) = delete;

private:  // Methods
  /// @copydoc UndoCommand::performExecute()
  bool performExecute() override {
    performRedo();  // can throw
    return true;
  }

  /// @copydoc UndoCommand::performUndo()
  void performUndo() override { mList.swap(mJ, mI); }

  /// @copydoc UndoCommand::performRedo()
  void performRedo() override { mList.swap(mI, mJ); }

private:  // Data
  SerializableObjectList<T, P>& mList;
  int                           mI;
  int                           mJ;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_CMDLISTELEMENTSSWAP_H
