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

#ifndef LIBREPCB_PROJECT_BES_BASE_H
#define LIBREPCB_PROJECT_BES_BASE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../boardeditor.h"
#include "boardeditorevent.h"
#include "ui_boardeditor.h"

#include <librepcb/common/graphics/graphicsview.h>
#include <librepcb/common/units/all_length_units.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class UndoStack;

namespace workspace {
class Workspace;
}

namespace project {

class Project;
class Circuit;

namespace editor {

/*******************************************************************************
 *  Class BES_Base
 ******************************************************************************/

/**
 * @brief The BES_Base (BoardEditorState Base) class
 */
class BES_Base : public QObject {
  Q_OBJECT

public:
  /// process() return values
  enum ProcRetVal {
    ForceStayInState,   ///< event handled, stay in the current state
    ForceLeaveState,    ///< event handled, leave the current state
    PassToParentState,  ///< event unhandled, pass it to the parent
  };

  // Constructors / Destructor
  explicit BES_Base(BoardEditor& editor, Ui::BoardEditor& editorUi,
                    GraphicsView& editorGraphicsView, UndoStack& undoStack);
  virtual ~BES_Base();

  // General Methods
  virtual ProcRetVal process(BEE_Base* event) noexcept = 0;
  virtual bool       entry(BEE_Base* event) noexcept {
    Q_UNUSED(event);
    return true;
  }
  virtual bool exit(BEE_Base* event) noexcept {
    Q_UNUSED(event);
    return true;
  }

protected:
  // General Attributes which are needed by some state objects
  workspace::Workspace& mWorkspace;
  Project&              mProject;
  Circuit&              mCircuit;
  BoardEditor&          mEditor;
  Ui::BoardEditor&      mEditorUi;  ///< allows access to BoardEditor UI
  GraphicsView&
             mEditorGraphicsView;  ///< allows access to the board editor graphics view
  UndoStack& mUndoStack;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_BES_BASE_H
