/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
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

#ifndef PROJECT_ERCMSGDOCK_H
#define PROJECT_ERCMSGDOCK_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

namespace project {
class Project;
class ErcMsg;
class ErcMsgList;
}

namespace Ui {
class ErcMsgDock;
}

/*****************************************************************************************
 *  Class ErcMsgDock
 ****************************************************************************************/

namespace project {

/**
 * @brief The ErcMsgDock class
 */
class ErcMsgDock final : public QDockWidget
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit ErcMsgDock(Project& project);
        ~ErcMsgDock();


    public slots:

        void ercMsgAdded(ErcMsg* ercMsg) noexcept;
        void ercMsgRemoved(ErcMsg* ercMsg) noexcept;
        void ercMsgChanged(ErcMsg* ercMsg) noexcept;


    private slots:

        // GUI Actions
        void on_treeWidget_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
        void on_btnIgnore_clicked(bool checked);


    private:

        // Private Methods
        void updateTopLevelItemTexts() noexcept;

        // make some methods inaccessible...
        ErcMsgDock();
        ErcMsgDock(const ErcMsgDock& other);
        ErcMsgDock& operator=(const ErcMsgDock& rhs);

        // General
        Project& mProject;
        ErcMsgList& mErcMsgList;
        Ui::ErcMsgDock* mUi;
        QHash<int, QTreeWidgetItem*> mTopLevelItems;
        QHash<ErcMsg*, QTreeWidgetItem*> mErcMsgItems;
};

} // namespace project

#endif // PROJECT_ERCMSGDOCK_H
