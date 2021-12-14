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

#ifndef LIBREPCB_COMMON_CMDLISTELEMENTINSERT_H
#define LIBREPCB_COMMON_CMDLISTELEMENTINSERT_H

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
 *  Class CmdListElementInsert
 ******************************************************************************/

/**
 * @brief The CmdListElementInsert class
 */
template <typename T, typename P, typename... OnEditedArgs>
class CmdListElementInsert final : public UndoCommand {
public:
  // Constructors / Destructor
  CmdListElementInsert() = delete;
  CmdListElementInsert(const CmdListElementInsert& other) = delete;
  CmdListElementInsert(SerializableObjectList<T, P, OnEditedArgs...>& list,
                       const std::shared_ptr<T>& element,
                       int index = -1) noexcept
    : UndoCommand(tr("Add %1").arg(P::tagname)),
      mList(list),
      mElement(element),
      mIndex(index) {}
  ~CmdListElementInsert() noexcept {}

  // Operator Overloadings
  CmdListElementInsert& operator=(const CmdListElementInsert& rhs) = delete;

private:  // Methods
  /// @copydoc UndoCommand::performExecute()
  bool performExecute() override {
    if (mIndex < 0) mIndex = mList.count();
    performRedo();  // can throw
    return true;
  }

  /// @copydoc UndoCommand::performUndo()
  void performUndo() override { mList.remove(mIndex); }

  /// @copydoc UndoCommand::performRedo()
  void performRedo() override { mIndex = mList.insert(mIndex, mElement); }

private:  // Data
  SerializableObjectList<T, P, OnEditedArgs...>& mList;
  std::shared_ptr<T> mElement;
  int mIndex;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
