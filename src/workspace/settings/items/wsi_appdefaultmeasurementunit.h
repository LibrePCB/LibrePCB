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

#ifndef WSI_APPDEFAULTMEASUREMENTUNIT_H
#define WSI_APPDEFAULTMEASUREMENTUNIT_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include "../workspacesettingsitem.h"
#include "../../../common/units.h"

/*****************************************************************************************
 *  Class WSI_AppDefaultMeasurementUnit
 ****************************************************************************************/

/**
 * @brief The WSI_AppDefaultMeasurementUnit class represents the application's default
 *        measurement unit
 *
 * @author ubruhin
 * @date 2014-10-04
 */
class WSI_AppDefaultMeasurementUnit final : public WorkspaceSettingsItem
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit WSI_AppDefaultMeasurementUnit(WorkspaceSettings& settings);
        ~WSI_AppDefaultMeasurementUnit();

        // Getters
        Length::MeasurementUnit getMeasUnit() const {return mMeasurementUnit;}

        // Getters: Widgets
        QString getLabelText() const {return tr("Default Measurement Unit:");}
        QComboBox* getComboBox() const {return mComboBox;}

        // General Methods
        void restoreDefault();
        void apply();
        void revert();


    public slots:

        // Public Slots
        void comboBoxIndexChanged(int index);


    private:

        // make some methods inaccessible...
        WSI_AppDefaultMeasurementUnit();
        WSI_AppDefaultMeasurementUnit(const WSI_AppDefaultMeasurementUnit& other);
        WSI_AppDefaultMeasurementUnit& operator=(const WSI_AppDefaultMeasurementUnit& rhs);


        // Private Methods
        void updateComboBoxIndex();


        // General Attributes

        /**
         * @brief The application's default measurement unit
         *
         * Default: Length::millimeters
         */
        Length::MeasurementUnit mMeasurementUnit;
        Length::MeasurementUnit mMeasurementUnitTmp;

        // Widgets
        QComboBox* mComboBox;
};

#endif // WSI_APPDEFAULTMEASUREMENTUNIT_H
