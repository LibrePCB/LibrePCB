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

#ifndef LIBREPCB_GUI_EDITORTAB_H
#define LIBREPCB_GUI_EDITORTAB_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace gui {

class EditorApplication;
class EditorWindow;

/*******************************************************************************
 *  Class EditorTab
 ******************************************************************************/

/**
 * @brief A GUI tab in the main window
 */
class EditorTab : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  EditorTab() = delete;
  EditorTab(EditorApplication& application, EditorWindow& window);
  EditorTab(const EditorTab& other) noexcept = delete;
  virtual ~EditorTab() noexcept;

  // Properties
  Q_PROPERTY(QString title MEMBER mTitle NOTIFY titleChanged)

signals:
  void titleChanged(const QString& title);

private:
  EditorApplication& mApplication;
  EditorWindow& mWindow;
  QString mTitle;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace gui
}  // namespace librepcb

#endif
