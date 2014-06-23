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

#ifndef CONTROLPANEL_H
#define CONTROLPANEL_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class Workspace;

namespace Ui {
class ControlPanel;
}

/*****************************************************************************************
 *  Class ControlPanel
 ****************************************************************************************/

/**
 * @brief The ControlPanel class
 *
 * @author ubruhin
 *
 * @date 2014-06-23
 */
class ControlPanel : public QMainWindow
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit ControlPanel(Workspace* workspace);
        ~ControlPanel();

    protected:

        // Inherited Methods
        virtual void closeEvent(QCloseEvent* event);

    private:

        // make the default constructor and the copy constructor inaccessable
        ControlPanel();
        ControlPanel(const ControlPanel& other) : QMainWindow(0) {Q_UNUSED(other);}

        Ui::ControlPanel* ui;

        Workspace* mWorkspace;
};

#endif // CONTROLPANEL_H
