/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
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

#ifndef LIBREPCB_WSI_APPDEFAULTMEASUREMENTUNITS_H
#define LIBREPCB_WSI_APPDEFAULTMEASUREMENTUNITS_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include "wsi_base.h"
#include <librepcb/common/units/lengthunit.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace workspace {

/*****************************************************************************************
 *  Class WSI_AppDefaultMeasurementUnits
 ****************************************************************************************/

/**
 * @brief The WSI_AppDefaultMeasurementUnits class represents the application's default
 *        measurement units (for example the application's default length unit)
 *
 * @author ubruhin
 * @date 2014-10-04
 */
class WSI_AppDefaultMeasurementUnits final : public WSI_Base
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        WSI_AppDefaultMeasurementUnits() = delete;
        WSI_AppDefaultMeasurementUnits(const WSI_AppDefaultMeasurementUnits& other) = delete;
        WSI_AppDefaultMeasurementUnits(const QString& xmlTagName, XmlDomElement* xmlElement) throw (Exception);
        ~WSI_AppDefaultMeasurementUnits() noexcept;

        // Getters
        const LengthUnit& getLengthUnit() const noexcept {return mLengthUnit;}

        // Getters: Widgets
        QString getLengthUnitLabelText() const noexcept {return tr("Default Length Unit:");}
        QComboBox* getLengthUnitComboBox() const noexcept {return mLengthUnitComboBox.data();}

        // General Methods
        void restoreDefault() noexcept override;
        void apply() noexcept override;
        void revert() noexcept override;

        /// @copydoc SerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;

        // Operator Overloadings
        WSI_AppDefaultMeasurementUnits& operator=(const WSI_AppDefaultMeasurementUnits& rhs) = delete;


    private: // Methods

        void lengthUnitComboBoxIndexChanged(int index) noexcept;
        void updateLengthUnitComboBoxIndex() noexcept;

        /// @copydoc SerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;


    private: // Data

        /**
         * @brief The application's default length unit
         *
         * Default: millimeters
         */
        LengthUnit mLengthUnit;
        LengthUnit mLengthUnitTmp;

        // Widgets
        QScopedPointer<QComboBox> mLengthUnitComboBox;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace workspace
} // namespace librepcb

#endif // LIBREPCB_WSI_APPDEFAULTMEASUREMENTUNITS_H
