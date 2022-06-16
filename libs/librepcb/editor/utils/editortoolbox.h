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

#ifndef LIBREPCB_EDITOR_EDITORTOOLBOX_H
#define LIBREPCB_EDITOR_EDITORTOOLBOX_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class EditorToolbox
 ******************************************************************************/

/**
 * @brief The EditorToolbox class provides some useful general purpose methods
 *        for editors (i.e. GUI stuff)
 */
class EditorToolbox final {
  Q_DECLARE_TR_FUNCTIONS(EditorToolbox)

public:
  // Constructors / Destructor
  EditorToolbox() = delete;
  EditorToolbox(const EditorToolbox& other) = delete;
  ~EditorToolbox() = delete;

  // Operator Overloadings
  EditorToolbox& operator=(const EditorToolbox& rhs) = delete;

  // Static Methods

  /**
   * @brief Remove (hide) a whole row in a QFormLayout
   *
   * @param label   The label of the row to remove.
   */
  static void removeFormLayoutRow(QLabel& label) noexcept;

  /**
   * @brief Set the focus to the first widget of a toolbar and iterate through
   *
   * - The tab order of all widgets of the passed toolbar will be configured
   *   from left to right resp. top to bottom.
   * - After the last widget, the tab order is followed by a custom widget.
   * - The first widget of the passed toolbar will get the focus.
   *
   * Intended for the command toolbar to enter focus from the graphics view,
   * navigate though all the toolbar widgets, and then return the focus
   * back to the graphics view.
   *
   * @param toolBar               The toolbar to set the focus.
   * @param returnFocusToWidget   Widget which shall have the focus after
   *                              the last widget of the toolbar.
   * @return  True if there was at least one widget and the focus has been
   *          set. False if there was no widget and the focus was not set.
   */
  static bool startToolBarTabFocusCycle(QToolBar& toolBar,
                                        QWidget& returnFocusToWidget) noexcept;

private:
  /**
   * @brief Helper for #removeFormLayoutRow()
   *
   * @param item  The item to hide.
   */
  static void hideLayoutItem(QLayoutItem& item) noexcept;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
