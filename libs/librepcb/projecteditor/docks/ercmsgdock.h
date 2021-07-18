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

#ifndef LIBREPCB_PROJECT_ERCMSGDOCK_H
#define LIBREPCB_PROJECT_ERCMSGDOCK_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace project {

class Project;
class ErcMsg;
class ErcMsgList;

namespace editor {

namespace Ui {
class ErcMsgDock;
}

/*******************************************************************************
 *  Class ErcMsgDock
 ******************************************************************************/

/**
 * @brief The ErcMsgDock class
 */
class ErcMsgDock final : public QDockWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  ErcMsgDock() = delete;
  ErcMsgDock(const ErcMsgDock& other) = delete;
  explicit ErcMsgDock(Project& project);
  ~ErcMsgDock();

  // Operator Overloadings
  ErcMsgDock& operator=(const ErcMsgDock& rhs) = delete;

public slots:

  void ercMsgAdded(ErcMsg* ercMsg) noexcept;
  void ercMsgRemoved(ErcMsg* ercMsg) noexcept;
  void ercMsgChanged(ErcMsg* ercMsg) noexcept;

private slots:

  // GUI Actions
  void on_treeWidget_itemSelectionChanged();
  void on_btnIgnore_clicked(bool checked);

private:
  // Private Methods
  void updateTopLevelItemTexts() noexcept;

  // General
  Project& mProject;
  ErcMsgList& mErcMsgList;
  Ui::ErcMsgDock* mUi;
  QHash<int, QTreeWidgetItem*> mTopLevelItems;
  QHash<ErcMsg*, QTreeWidgetItem*> mErcMsgItems;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_ERCMSGDOCK_H
