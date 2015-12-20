/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
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

#ifndef PROJECT_UNPLACEDCOMPONENTSDOCK_H
#define PROJECT_UNPLACEDCOMPONENTSDOCK_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>
#include <librepcbcommon/units/all_length_units.h>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class GraphicsView;
class GraphicsScene;

namespace library {
class Device;
}

namespace project {
class Project;
class Board;
class GenCompInstance;
class ProjectEditor;
}

namespace Ui {
class UnplacedComponentsDock;
}

/*****************************************************************************************
 *  Class SchematicPagesDock
 ****************************************************************************************/

namespace project {

/**
 * @brief The UnplacedComponentsDock class
 */
class UnplacedComponentsDock final : public QDockWidget
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit UnplacedComponentsDock(project::ProjectEditor& editor);
        ~UnplacedComponentsDock();

        // Setters
        void setBoard(Board* board);


    private slots:

        void on_lstUnplacedComponents_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);
        void on_cbxSelectedComponent_currentIndexChanged(int index);
        void on_btnAdd_clicked();
        void on_pushButton_clicked();
        void on_btnAddAll_clicked();


    private:

        // make some methods inaccessible...
        UnplacedComponentsDock();
        UnplacedComponentsDock(const UnplacedComponentsDock& other);
        UnplacedComponentsDock& operator=(const UnplacedComponentsDock& rhs);

        // Private Methods
        /*void updateComponentsList() noexcept;
        void setSelectedGenCompInstance(GenCompInstance* genComp) noexcept;
        void setSelectedDevice(const library::Device* device) noexcept;
        void addComponent(GenCompInstance& genComp, const QUuid& component) noexcept;*/


        // General
        ProjectEditor& mProjectEditor;
        Project& mProject;
        Board* mBoard;
        Ui::UnplacedComponentsDock* mUi;
        GraphicsView* mFootprintPreviewGraphicsView;
        GraphicsScene* mFootprintPreviewGraphicsScene;
        GenCompInstance* mSelectedGenComp;
        const library::Device* mSelectedDevice;
        QMetaObject::Connection mCircuitConnection1;
        QMetaObject::Connection mCircuitConnection2;
        QMetaObject::Connection mBoardConnection1;
        QMetaObject::Connection mBoardConnection2;
        Point mNextPosition;
        bool mDisableListUpdate;
};

} // namespace project

#endif // PROJECT_UNPLACEDCOMPONENTSDOCK_H
