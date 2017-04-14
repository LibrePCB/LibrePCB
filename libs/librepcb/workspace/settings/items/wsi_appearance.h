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

#ifndef LIBREPCB_WSI_APPEARANCE_H
#define LIBREPCB_WSI_APPEARANCE_H

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
 *  Class WSI_Appearance
 ****************************************************************************************/

/**
 * @brief The WSI_Appearance class
 *
 * @author ubruhin
 * @date 2015-02-08
 */
class WSI_Appearance final : public WSI_Base
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        WSI_Appearance() = delete;
        WSI_Appearance(const WSI_Appearance& other) = delete;
        WSI_Appearance(const QString& xmlTagName, XmlDomElement* xmlElement) throw (Exception);
        ~WSI_Appearance() noexcept;

        // Getters
        bool getUseOpenGl() const noexcept {return mUseOpenGlCheckBox->isChecked();}

        // Getters: Widgets
        QString getUseOpenGlLabelText() const noexcept {return tr("Rendering Method:");}
        QWidget* getUseOpenGlWidget() const noexcept {return mUseOpenGlWidget.data();}

        // General Methods
        void restoreDefault() noexcept override;
        void apply() noexcept override;
        void revert() noexcept override;

        /// @copydoc SerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;

        // Operator Overloadings
        WSI_Appearance& operator=(const WSI_Appearance& rhs) = delete;


    private: // Methods

        /// @copydoc SerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;


    private: // Data

        bool mUseOpenGl;

        // Widgets
        QScopedPointer<QWidget> mUseOpenGlWidget;
        QScopedPointer<QCheckBox> mUseOpenGlCheckBox;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace workspace
} // namespace librepcb

#endif // LIBREPCB_WSI_APPEARANCE_H
