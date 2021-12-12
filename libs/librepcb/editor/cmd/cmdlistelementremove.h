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

#ifndef LIBREPCB_EDITOR_CMDLISTELEMENTREMOVE_H
#define LIBREPCB_EDITOR_CMDLISTELEMENTREMOVE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../undocommand.h"

#include <librepcb/core/serialization/serializableobjectlist.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class CmdListElementRemove
 ******************************************************************************/

/**
 * @brief The CmdListElementRemove class
 */
template <typename T, typename P, typename... OnEditedArgs>
class CmdListElementRemove final : public UndoCommand {
public:
  // Constructors / Destructor
  CmdListElementRemove() = delete;
  CmdListElementRemove(const CmdListElementRemove& other) = delete;
  CmdListElementRemove(SerializableObjectList<T, P, OnEditedArgs...>& list,
                       const T* element) noexcept
    : UndoCommand(tr("Remove %1").arg(P::tagname)),
      mList(list),
      mElement(element),
      mIndex(-1) {}
  ~CmdListElementRemove() noexcept {}

  // Operator Overloadings
  CmdListElementRemove& operator=(const CmdListElementRemove& rhs) = delete;

private:  // Methods
  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  bool performExecute() override {
    mIndex = mList.indexOf(mElement);
    Q_ASSERT(mIndex >= 0);
    performRedo();  // can throw
    return true;
  }

  /// @copydoc ::librepcb::editor::UndoCommand::performUndo()
  void performUndo() override { mList.insert(mIndex, mMemorizedElement); }

  /// @copydoc ::librepcb::editor::UndoCommand::performRedo()
  void performRedo() override {
    mMemorizedElement = mList.take(mIndex);
    Q_ASSERT(mMemorizedElement.get() == mElement);
  }

private:  // Data
  SerializableObjectList<T, P, OnEditedArgs...>& mList;
  const T* mElement;
  std::shared_ptr<T> mMemorizedElement;
  int mIndex;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
