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

#ifndef LIBREPCB_WSI_PROJECTAUTOSAVEINTERVAL_H
#define LIBREPCB_WSI_PROJECTAUTOSAVEINTERVAL_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include "wsi_base.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Class WSI_ProjectAutosaveInterval
 ****************************************************************************************/

/**
 * @brief The WSI_ProjectAutosaveInterval class represents the project autosave interval setting
 *
 * This setting is used by the class #project#Project for the autosave mechanism.
 * A value of zero means that the autosave mechanism is disabled! A value greater
 * than zero defines the time interval in seconds.
 *
 * @author ubruhin
 * @date 2014-10-04
 */
class WSI_ProjectAutosaveInterval final : public WSI_Base
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit WSI_ProjectAutosaveInterval(WorkspaceSettings& settings);
        ~WSI_ProjectAutosaveInterval();

        // Getters
        unsigned int getInterval() const {return mInterval;}

        // Getters: Widgets
        QString getLabelText() const {return tr("Project Autosave Interval:");}
        QWidget* getWidget() const {return mWidget;}

        // General Methods
        void restoreDefault();
        void apply();
        void revert();

    public slots:

        // Public Slots
        void spinBoxValueChanged(int value);

    private:

        // make some methods inaccessible...
        WSI_ProjectAutosaveInterval();
        WSI_ProjectAutosaveInterval(const WSI_ProjectAutosaveInterval& other);
        WSI_ProjectAutosaveInterval& operator=(const WSI_ProjectAutosaveInterval& rhs);


        // General Attributes

        /**
         * @brief the autosave interval [seconds] (0 = autosave disabled)
         *
         * Default: 600 seconds
         */
        unsigned int mInterval;
        unsigned int mIntervalTmp;

        // Widgets
        QWidget* mWidget;
        QSpinBox* mSpinBox;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_WSI_PROJECTAUTOSAVEINTERVAL_H
