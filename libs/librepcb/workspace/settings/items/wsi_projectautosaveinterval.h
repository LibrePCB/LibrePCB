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
namespace workspace {

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
        WSI_ProjectAutosaveInterval() = delete;
        WSI_ProjectAutosaveInterval(const WSI_ProjectAutosaveInterval& other) = delete;
        WSI_ProjectAutosaveInterval(const QString& xmlTagName, XmlDomElement* xmlElement) throw (Exception);
        ~WSI_ProjectAutosaveInterval() noexcept;

        // Getters
        unsigned int getInterval() const noexcept {return mInterval;}

        // Getters: Widgets
        QString getLabelText() const noexcept {return tr("Project Autosave Interval:");}
        QWidget* getWidget() const noexcept {return mWidget.data();}

        // General Methods
        void restoreDefault() noexcept override;
        void apply() noexcept override;
        void revert() noexcept override;

        /// @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;

        // Operator Overloadings
        WSI_ProjectAutosaveInterval& operator=(const WSI_ProjectAutosaveInterval& rhs) = delete;


    private: // Methods

        void spinBoxValueChanged(int value) noexcept;

        /// @copydoc IF_XmlSerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;


    private: // Data

        // General Attributes

        /**
         * @brief the autosave interval [seconds] (0 = autosave disabled)
         *
         * Default: 600 seconds
         */
        uint mInterval;
        uint mIntervalTmp;

        // Widgets
        QScopedPointer<QWidget> mWidget;
        QScopedPointer<QSpinBox> mSpinBox;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace workspace
} // namespace librepcb

#endif // LIBREPCB_WSI_PROJECTAUTOSAVEINTERVAL_H
